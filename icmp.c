#include <stdio.h>
#include "address.h"
#include "graph.h"
#include "ethernet.h"
#include "icmp.h"
#include "ip.h"
#include "net.h"
#include "skbuff.h"
#include "utils.h"

static u16 calc_icmp_cksum(struct icmp_hdr *hdr, size_t size)
{
    return calc_checksum_16((char *) hdr, size);
}

static void fill_icmp_cksum(struct icmp_hdr *hdr, size_t size)
{
    hdr->cksum = HTONS(calc_icmp_cksum(hdr, size));
}

static bool check_icmp_cksum(struct icmp_hdr *hdr, size_t size)
{
   return  calc_icmp_cksum(hdr, size) == 0;
}

bool ping(struct node *node, char *ip_addr)
{
    struct ip_addr ip;
    ip.iaddr = convert_ip_from_str_to_int(ip_addr);
    return icmp_out(node, ip, ICMP_ECHO_REQUEST_TYPE, ICMP_ECHO_REQUEST_CODE,
                    NULL, 0);
}

bool icmp_out(struct node *node, struct ip_addr ip, u8 type, u8 code,
              char *payload, size_t size)
{
    struct sk_buff *skb = alloc_skb(BUFF_HEADROOM + BUFF_TAILROOM + \
        ICMP_HDR_SIZE + size);
    skb_reserve(skb, BUFF_HEADROOM);
    struct icmp_hdr *hdr = (struct icmp_hdr *) skb_put(skb, ICMP_HDR_SIZE + size);
    hdr->type = type;
    hdr->code = code;
    if (size)
        memcpy(hdr->rest, payload, size);
    fill_icmp_cksum(hdr, skb->len);
    bool result = ip_output(node, ip, IP_PROTO_ICMP, skb);
    if (!result)
        free_skb(skb);
    return result;
}

bool icmp_input(struct sk_buff *skb)
{
    struct icmp_hdr *hdr = (struct icmp_hdr *) skb->data;
    if (skb->len < ICMP_HDR_SIZE)
        return false;
    if (hdr->type == ICMP_ECHO_REQUEST_TYPE &&
            hdr->code == ICMP_ECHO_REQUEST_CODE) {
        printf("%s(%s): Received ping\n", skb->intf->node->name,
               skb->intf->name);
        icmp_out(skb->intf->node, skb->ip_hdr->src_ip, ICMP_ECHO_REPLY_TYPE,
                 ICMP_ECHO_REQUEST_CODE, (char *) hdr->rest,
                 skb->len - ICMP_HDR_SIZE);
    } else if (hdr->type == ICMP_ECHO_REPLY_TYPE &&
            hdr->code == ICMP_ECHO_REPLY_CODE) {
        printf("%s(%s): Received pong\n", skb->intf->node->name,
               skb->intf->name);
    }
    free_skb(skb);
    return true;
}
