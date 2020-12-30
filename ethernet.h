#ifndef ETHERNET_H
#define ETHERNET_H
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include "graph.h"
#include "utils.h"
#define ETH_PROTO_IP (0x0800)
#define ETH_PROTO_ARP (0x0806)

struct sk_buff;

struct ethernet_hdr {
    struct mac_addr  dst_mac;
    struct mac_addr src_mac;
    uint16_t type;
    // variable size field (must be the last field to work)
    char payload[1];
} __attribute__((packed));

struct ethernet_tlr {
    uint32_t fcs;
} __attribute__((packed));

#define ETH_HDR_SIZE (sizeof(struct ethernet_hdr) - 1)
#define ETH_FCS(hdr_ptr, payload_size) (((struct ethernet_tlr *) ((char *) hdr_ptr \
    + ETH_HDR_SIZE + payload_size))->fcs)

inline struct ethernet_hdr *wrap_eth_hdr_with_payload(char *pkt, size_t size)
{
    char *p = pkt - ETH_HDR_SIZE;
    memset(p, 0, ETH_HDR_SIZE);
    return (struct ethernet_hdr *) p;
}

bool ethernet_input_qualify(struct intf *intf,
                            struct ethernet_hdr *hdr);

bool ethernet_input(struct sk_buff *skb);

bool ethernet_output(struct intf *intf, struct mac_addr mac, u16 proto,
                     struct sk_buff *skb);

#endif
