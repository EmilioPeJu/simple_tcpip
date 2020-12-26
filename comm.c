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
#include "graph.h"
#define MAX_COMMS (1024)
#define MAX_EVENTS (10)
#define BUFFER_SIZE (2048)
static char recv_buffer[BUFFER_SIZE];

static struct comm *sockfd_to_comm[MAX_COMMS];

static uint16_t next_udp_port = 40000;

static uint16_t get_next_udp_port()
{
    return next_udp_port++;
}

int send_pkt_out(char *pkt, size_t pkt_size, struct intf *intf)
{
    struct intf *nbr_intf = get_nbr_if(intf);
    if (!nbr_intf || !nbr_intf->comm.sock || !intf->comm.sock) {
        printf("Sockets  not available\n");
        return -1;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(nbr_intf->comm.port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ssize_t nsent = sendto(intf->comm.sock, pkt, pkt_size, MSG_DONTWAIT,
                           (struct sockaddr *) &addr, sizeof(addr));
    if (nsent < 0) {
        printf("Error while sending\n");
    }
    return 0;
}

int pkt_receive(struct intf *intf, char *pkt, size_t pkt_size)
{
    return 0;
}

int send_pkt_flood(struct node *node, struct intf *exempted_intf, char *pkt,
                   size_t pkt_size)
{
    int rc = 0;
    for (size_t i=0; i < MAX_INTFS_PER_NODE; i++) {
        if (node->intfs[i] && node->intfs[i] != exempted_intf) {
            rc |= send_pkt_out(pkt, pkt_size, node->intfs[i]);
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
    for (;;) {
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            printf("epoll_wait error");
        }
        for (size_t i=0; i < nfds; i++) {
            int fd = events[i].data.fd;
            struct sockaddr_in addr;
            socklen_t addr_len;
            if (sockfd_to_comm[fd]) {
                ssize_t nrecv = recvfrom(fd, recv_buffer, BUFFER_SIZE,
                    MSG_DONTWAIT, (struct sockaddr *) &addr, &addr_len);
                if (nrecv < 0) {
                    printf("recvfrom error: %ld\n", nrecv);
                } else {
                    struct intf *intf = COMM_INTF(sockfd_to_comm[fd]);
                    printf("Packet received from %s:%hu",
                           inet_ntoa(addr.sin_addr), htons(addr.sin_port));
                    printf(", node %s, interface %s received %lu bytes: %s\n",
                           intf->node->name, intf->name, nrecv, recv_buffer);
                    pkt_receive(intf, recv_buffer, nrecv);
                }
            }
        }
    }
}

void start_recv_thread()
{
    pthread_t thread;
    if (pthread_create(&thread, NULL, recv_task, NULL)) {
        printf("pthread_create error\n");
        exit(EXIT_FAILURE);
    }
}

void init_udp_socket(struct comm *comm)
{
    struct sockaddr_in addr;
    comm->port = get_next_udp_port();
    comm->sock = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(comm->port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(comm->sock, (struct sockaddr *) &addr, sizeof(addr))) {
        printf("Error binding to port %u", comm->port);
        comm->port = 0;
        comm->sock = 0;
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

