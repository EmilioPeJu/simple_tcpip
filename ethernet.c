#include <arpa/inet.h>
#include "arp.h"
#include "comm.h"
#include "ethernet.h"
#include "ip.h"
#include "skbuff.h"
#include "utils.h"

bool ethernet_input_qualify(struct intf *intf,
                            struct ethernet_hdr *hdr)
{
    return memcmp(&hdr->dst_mac, IF_MAC(intf),
                  sizeof(struct mac_addr)) == 0 ||
           IS_MAC_BROADCAST_ADDR(&hdr->dst_mac);
}

bool ethernet_input(struct sk_buff *skb)
{
    struct ethernet_hdr *hdr = (struct ethernet_hdr *) skb->data;
    skb->ethernet_hdr = hdr;
    if (skb->len < ETH_HDR_SIZE)
        return false;
    if (!ethernet_input_qualify(skb->intf, hdr))
        return false;
    switch (hdr->type) {
    case HTONS(ETH_PROTO_ARP):
        skb_pull(skb, ETH_HDR_SIZE);
        return arp_input(skb);
        break;
    case HTONS(ETH_PROTO_IP):
        skb_pull(skb, ETH_HDR_SIZE);
        skb->ip_hdr = (struct ip_hdr *) skb->data;
        return ip_preroute(skb);
        break;
    default:
        return false;
    }
}

bool ethernet_output(struct intf *intf, struct mac_addr mac, u16 proto,
                     struct sk_buff *skb)
{
    struct ethernet_hdr *hdr = (struct ethernet_hdr *) skb_push(skb, ETH_HDR_SIZE);
    memcpy(hdr->dst_mac.addr, mac.addr, MAC_ADDR_SIZE);
    memcpy(hdr->src_mac.addr, IF_MAC(intf)->addr, MAC_ADDR_SIZE);
    hdr->type = HTONS(proto);
    return send_pkt_out(intf, skb);
}
