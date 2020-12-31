#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "comm.h"
#include "ethernet.h"
#include "graph.h"
#include "skbuff.h"
#include "switch.h"
#include "utils.h"
#define MAX_COMMS (1024)
#define MAX_EVENTS (10)

static data_callback receive_callback;
static  data_callback send_callback;
static bool want_quit_recv_task;
static pthread_t recv_thread;
static bool thread_started = false;

static struct comm *sockfd_to_comm[MAX_COMMS];

static uint16_t next_udp_port = 40000;

static uint16_t get_next_udp_port()
{
    return next_udp_port++;
}

bool send_pkt_out(struct intf *intf, struct sk_buff *skb)
{
    bool result = send_bytes_out((char *) skb->data, skb->len, intf);
    if (result)
        free_skb(skb);
    return result;
}

bool send_bytes_out(char *bytes, size_t size, struct intf *intf)
{
    if (send_callback)
        send_callback(bytes, size, intf);
    struct intf *nbr_intf = get_nbr_if(intf);
    if (!nbr_intf || !nbr_intf->comm.port || !intf->comm.sock) {
        printf("Socket  not available\n");
        return false;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(nbr_intf->comm.port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ssize_t nsent = sendto(intf->comm.sock, bytes, size, MSG_DONTWAIT,
                           (struct sockaddr *) &addr, sizeof(addr));
    if (nsent < 0) {
        printf("Error while sending\n");
        return false;
    }
    return true;
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
    return send_bytes_flood(node, exempted_intf, (char *) skb->data, skb->len);
}

bool send_bytes_flood(struct node *node, struct intf *exempted_intf, char *bytes,
                      size_t size)
{
    bool rc = false;
    for (size_t i=0; i < MAX_INTFS_PER_NODE; i++) {
        if (node->intfs[i] && node->intfs[i] != exempted_intf) {
            rc |= send_bytes_out(bytes, size, node->intfs[i]);
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
        if (sockfd_to_comm[i]) {
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
        }
        for (size_t i=0; i < nfds; i++) {
            int fd = events[i].data.fd;
            struct sockaddr_in addr;
            socklen_t addr_len;
            if (sockfd_to_comm[fd]) {
                struct sk_buff *skb = alloc_skb(RECV_BUFFER_SIZE);
                skb_reserve(skb, RECV_RESERVED_HEAD_ROOM);
                ssize_t nrecv = recvfrom(fd, skb->data, skb_tailroom(skb),
                    MSG_DONTWAIT, (struct sockaddr *) &addr, &addr_len);
                if (nrecv < 0) {
                    printf("recvfrom error: %ld\n", nrecv);
                    free_skb(skb);
                } else {
                    skb_put(skb, nrecv);
                    struct intf *intf = COMM_INTF(sockfd_to_comm[fd]);
                    skb->intf = intf;
                    printf("Packet received from %s:%hu",
                           inet_ntoa(addr.sin_addr), htons(addr.sin_port));
                    printf(", node %s, interface %s received %lu bytes: \n",
                           intf->node->name, intf->name, skb->len);
                    dump_hex((char *) skb->data, skb->len);
                    printf("\n");
                    if(!pkt_receive(skb)) {
                        printf("Packet dropped\n");
                        free(skb);
                    }
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

void init_udp_socket(struct comm *comm)
{
    struct sockaddr_in addr;
    comm->sock = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    while (true) {
        comm->port = get_next_udp_port();
        addr.sin_port = htons(comm->port);
        if (!bind(comm->sock, (struct sockaddr *) &addr, sizeof(addr)))
            break;
    }
    sockfd_to_comm[comm->sock] = comm;
}

void destroy_udp_socket(struct comm *comm)
{
    if (comm->sock) {
        close(comm->sock);
        sockfd_to_comm[comm->sock] = 0;
        comm-> port = 0;
        comm->sock = 0;
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

