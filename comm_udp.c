#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "address.h"
#include "comm.h"
#include "comm_udp.h"
#include "graph.h"
#include "utils.h"

struct comm_ops udp_comm_ops = {
    .init_comm=udp_init_comm,
    .destroy_comm=udp_destroy_comm,
    .send_bytes=udp_send_bytes,
    .recv_bytes=udp_recv_bytes
};

static uint16_t next_udp_port = 40000;

static uint16_t get_next_udp_port()
{
    return next_udp_port++;
}

bool udp_init_comm(struct comm *comm)
{
    struct sockaddr_in addr;
    comm->fd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    while (true) {
        comm->port = get_next_udp_port();
        addr.sin_port = htons(comm->port);
        if (!bind(comm->fd, (struct sockaddr *) &addr, sizeof(addr)))
            break;
    }
    if(!set_fd_comm(comm->fd, comm)) {
        printf("set_fd_comm: error\n");
        return false;
    }
    return true;
}

bool udp_destroy_comm(struct comm *comm)
{
    if (comm->fd) {
        set_fd_comm(comm->fd, NULL);
        close(comm->fd);
        comm->fd = 0;
        comm->port = 0;
    }
    return true;
}

ssize_t udp_send_bytes(struct comm *comm, char *bytes, size_t size)
{
    struct intf *intf = COMM_INTF(comm);
    struct intf *nbr_intf = get_nbr_if(intf);
    if (!nbr_intf || !nbr_intf->comm.port || !intf->comm.fd) {
        printf("Socket  not available\n");
        return false;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(nbr_intf->comm.port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ssize_t nsent = sendto(intf->comm.fd, bytes, size, MSG_DONTWAIT,
                           (struct sockaddr *) &addr, sizeof(addr));
    if (nsent < 0) {
        printf("Error while sending\n");
        return false;
    }
    return true;
}

ssize_t udp_recv_bytes(struct comm *comm, char *buff, size_t size)
{
    struct sockaddr_in addr;
    socklen_t addr_len;
    ssize_t nrecv = recvfrom(comm->fd, buff, size, MSG_DONTWAIT,
                             (struct sockaddr *) &addr, &addr_len);
    if (nrecv >= 0) {
        struct intf *intf = COMM_INTF(comm);
        printf("Packet received from %s:%hu",
               inet_ntoa(addr.sin_addr), htons(addr.sin_port));
        printf(", node %s, interface %s received %lu bytes: \n",
               intf->node->name, intf->name, nrecv);
        dump_hex(buff , nrecv);
        printf("\n");
    }
    return nrecv;
}
