#include <arpa/inet.h>
#include "arp.h"
#include "ethernet.h"
#include "ip.h"
#include "skbuff.h"
#include "utils.h"

bool ethernet_input_qualify(struct intf *intf,
                            struct ethernet_hdr *hdr)
{
    return memcmp(&hdr->dst_mac, &intf->intf_nw_prop.mac_addr,
                  sizeof(struct mac_addr)) == 0 ||
           IS_MAC_BROADCAST_ADDR(&hdr->dst_mac);
}

bool ethernet_input(struct intf *intf, struct sk_buff *skb)
{
    struct ethernet_hdr *hdr = (struct ethernet_hdr *) skb->data;
    if (skb->len < ETH_HDR_SIZE)
        return false;
    if (!ethernet_input_qualify(intf, hdr))
        return false;
    switch (hdr->type) {
    case HTONS(ETH_PROTO_ARP):
        skb_pull(skb, ETH_HDR_SIZE);
        return arp_input(intf, skb);
        break;
    case HTONS(ETH_PROTO_IP):
        skb_pull(skb, ETH_HDR_SIZE);
        return ip_input(intf, skb);
        break;
    default:
        return false;
    }
}

