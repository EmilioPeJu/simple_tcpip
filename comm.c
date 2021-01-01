#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "comm.h"
#include "comm_udp.h"
#include "comm_tuntap.h"
#include "ethernet.h"
#include "graph.h"
#include "skbuff.h"
#include "switch.h"
#include "utils.h"
#define MAX_EVENTS (10)

static data_callback receive_callback;
static  data_callback send_callback;
static bool want_quit_recv_task;
static pthread_t recv_thread;
static bool thread_started = false;

static struct comm *fd_to_comm_map[MAX_COMMS];

bool set_fd_comm(int fd, struct comm *comm)
{
    if (fd < 0 || fd >= MAX_COMMS)
        return false;
    fd_to_comm_map[fd] = comm;
    return true;
}

struct comm *fd_to_comm(int fd)
{
    if (fd < 0 || fd >= MAX_COMMS)
        return NULL;
    return fd_to_comm_map[fd];
}

bool init_comm(struct comm *comm, enum comm_type comm_type)
{
    comm->comm_type = comm_type;
    switch (comm->comm_type) {
        case CT_UDP:
            comm->ops = &udp_comm_ops;
            break;
        case CT_TUNTAP:
            comm->ops = &tuntap_comm_ops;
            break;
        default:
            printf("Comm type not supported");
            exit(EXIT_FAILURE);
    }
    return comm->ops->init_comm(comm);
}

bool destroy_comm(struct comm *comm)
{
    return comm->ops->destroy_comm(comm);
}

bool send_pkt_out(struct intf *intf, struct sk_buff *skb)
{
    ssize_t result = send_bytes_out((char *) skb->data, skb->len, intf);
    bool ok = result >= 0;
    if (ok)
        free_skb(skb);
    return ok;
}

ssize_t send_bytes_out(char *bytes, size_t size, struct intf *intf)
{
    if (send_callback)
        send_callback(bytes, size, intf);
    return intf->comm.ops->send_bytes(&intf->comm, bytes, size);
}

bool pkt_receive(struct sk_buff *skb)
{
    if (receive_callback)
        receive_callback((char *) skb->data, skb->len, skb->intf);
    if (IS_INTF_L3_MODE(skb->intf)) {
        return ethernet_input(skb);
    } else {
        return switch_input(skb);
    }
}

bool send_pkt_flood(struct node *node, struct intf *exempted_intf,
                    struct sk_buff *skb)
{
    bool ok = send_bytes_flood(node, exempted_intf, (char *) skb->data, skb->len);
    if(ok)
        free_skb(skb);
    return ok;
}

bool send_bytes_flood(struct node *node, struct intf *exempted_intf, char *bytes,
                      size_t size)
{
    bool rc = false;
    for (size_t i=0; i < MAX_INTFS_PER_NODE; i++) {
        if (node->intfs[i] && node->intfs[i] != exempted_intf) {
            rc |= send_bytes_out(bytes, size, node->intfs[i]) < 0;
        }
    }
    return rc;
}

static void *recv_task(void *data)
{
    struct epoll_event ev, events[MAX_EVENTS];
    int epollfd = epoll_create1(0);
    if (epollfd == -1) {
        printf("epoll_create error");
        exit(EXIT_FAILURE);
    }
    for (size_t i=0; i < MAX_COMMS; i++) {
        if (fd_to_comm_map[i]) {
            ev.events = EPOLLIN;
            ev.data.fd = i;
            if (epoll_ctl(epollfd, EPOLL_CTL_ADD, i, &ev) == -1) {
                printf("epoll_ctl error");
                exit(EXIT_FAILURE);
            }
        }
    }
    while (!want_quit_recv_task) {
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, 1000);
        if (nfds < 0) {
            printf("epoll_wait error\n");
            continue;
        }
        for (size_t i=0; i < nfds; i++) {
            int fd = events[i].data.fd;
            struct comm *comm;
            if ((comm=fd_to_comm_map[fd]) != NULL) {
                struct intf *intf = COMM_INTF(comm);
                struct sk_buff *skb = alloc_skb(RECV_BUFFER_SIZE);
                skb_reserve(skb, RECV_RESERVED_HEAD_ROOM);
                skb->intf = intf;
                ssize_t nrecv = intf->comm.ops->recv_bytes(comm,
                                                           (char *) skb->data,
                                                           skb_tailroom(skb));
                if (nrecv <= 0) {
                    printf("Error %ld while receiving\n", nrecv);
                    free_skb(skb);
                    continue;
                }
                skb_put(skb, nrecv);
                if(!pkt_receive(skb)) {
                    printf("Packet dropped\n");
                    free_skb(skb);
                }
            }
        }
    }
    want_quit_recv_task = false;
    return NULL;
}

void start_recv_thread()
{
    if (thread_started) {
        return;
    }
    if (pthread_create(&recv_thread, NULL, recv_task, NULL)) {
        printf("pthread_create error\n");
        exit(EXIT_FAILURE);
    }
    thread_started = true;
}

void stop_recv_thread()
{
    if (thread_started) {
        want_quit_recv_task = true;
        if (!pthread_join(recv_thread, NULL)) {
            thread_started = false;
        }
    }
}

void set_send_callback(data_callback cb)
{
    send_callback = cb;
}

void set_receive_callback(data_callback cb)
{
    receive_callback = cb;
}

