#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "comm.h"
#include "comm_tuntap.h"
#include "graph.h"

struct comm_ops tuntap_comm_ops = {
    .init_comm=tuntap_init_comm,
    .destroy_comm=tuntap_destroy_comm,
    .send_bytes=tuntap_send_bytes,
    .recv_bytes=tuntap_recv_bytes
};

static int tun_alloc(const char *name)
{
    struct ifreq ifr;
    int fd, err;
    if ((fd = open("/dev/net/tun", O_RDWR)) < 0)
            return -1;
    memset(&ifr, 0, sizeof(struct ifreq));
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    if (name)
        strncpy(ifr.ifr_name, name, IFNAMSIZ);
    if ((err = ioctl(fd, TUNSETIFF, &ifr)) < 0)
        return err;
    return fd;
}

bool tuntap_init_comm(struct comm *comm)
{
    struct intf *intf = COMM_INTF(comm);
    if ((comm->fd=tun_alloc(intf->name)) < 0) {
        printf("Error allocating tap %s\n", intf->name);
        return false;
    }
    if(!set_fd_comm(comm->fd, comm)) {
        printf("set_fd_comm: error\n");
        return false;
    }
    return true;
}

bool tuntap_destroy_comm(struct comm *comm)
{
    if (comm->fd) {
        set_fd_comm(comm->fd, NULL);
        close(comm->fd);
        comm->fd = 0;
    }
    return true;
}

ssize_t tuntap_send_bytes(struct comm *comm, char *bytes, size_t size)
{
    return write(comm->fd, bytes, size);
}

ssize_t tuntap_recv_bytes(struct comm *comm, char *bytes, size_t size)
{
    return read(comm->fd, bytes, size);
}
