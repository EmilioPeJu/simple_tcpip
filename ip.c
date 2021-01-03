#include <stdio.h>
#include "arp.h"
#include "ethernet.h"
#include "icmp.h"
#include "ip.h"
#include "graph.h"
#include "skbuff.h"
#include "tcp.h"
#include "udp.h"
#include "utils.h"

static bool ip_forward_enable = true;

bool ip_input(struct sk_buff *skb)
{
    struct ip_hdr *hdr = (struct ip_hdr *) skb->data;
    switch (hdr->proto)
    {
    case IP_PROTO_ICMP:
        skb_pull(skb, IP_HDR_SIZE);
        return icmp_input(skb);
    case IP_PROTO_TCP:
        skb_pull(skb, IP_HDR_SIZE);
        skb->tcp_hdr = (struct tcp_hdr *) skb->data;
        return tcp_input(skb);
    case IP_PROTO_UDP:
        skb_pull(skb, IP_HDR_SIZE);
        skb->udp_hdr = (struct udp_hdr *) skb->data;
        return udp_input(skb);
    default:
        printf("Unknown IP protocol\n");
        return false;
    }
}

bool ip_preroute(struct sk_buff *skb)
{
    struct ip_hdr *hdr = (struct ip_hdr *) skb->data;
    if (is_ip_local(skb->intf->node, hdr->dst_ip))
        return ip_input(skb);
    else
        return ip_forward(skb);
}

bool ip_forward(struct sk_buff *skb)
{
    if (!ip_forward_enable)
        return false;
    struct ip_hdr *hdr = (struct ip_hdr *) skb->data;
    if (!hdr->ttl)
        return false;
    if(!--hdr->ttl)
        return false;
    struct node *node = skb->intf->node;
    // we don't want to force an output interface
    skb->intf = NULL;
    return ip_postroute(node, skb);
}

bool ip_output(struct node *node, struct ip_addr ip, u8 proto,
               struct sk_buff *skb)
{
    struct ip_hdr *hdr = (struct ip_hdr *) skb_push(skb, IP_HDR_SIZE);
    init_ip_hdr(hdr);
    hdr->dst_ip.iaddr = ip.iaddr;
    hdr->proto = proto;
    hdr->total_len = HTONS(skb->len);
    return ip_postroute(node, skb);
}

bool ip_postroute(struct node *node, struct sk_buff *skb)
{
    struct ip_hdr *hdr = (struct ip_hdr *) skb->data;
    struct rt_entry *rt_entry = rt_lookup_lpm(NODE_RT_TABLE(node), hdr->dst_ip);
    struct ip_addr target_ip;
    if (!rt_entry) {
        printf("No matching routing table entry");
        return false;
    }
    if (skb->intf && skb->intf != rt_entry->intf) {
        // interface that upper layer  wants to use doesn't match
        // the one found in the routing table
        return false;
    }
    skb->intf = rt_entry->intf;
    hdr->src_ip.iaddr = IF_IP(rt_entry->intf)->iaddr;
    fill_ip_hcksum(hdr);
    if (is_ip_local(node, hdr->dst_ip)) {
        // it's sent to ourself
        return ip_input(skb);
    }
    struct arp_entry *arp_entry;
    if (rt_entry->next.iaddr == 0) {
        // it's going to a local network
        target_ip.iaddr = hdr->dst_ip.iaddr;
        arp_entry = arp_table_lookup(NODE_ARP_TABLE(node),
                                     hdr->dst_ip);
    } else {
        // it's going to a remote network
        target_ip.iaddr = rt_entry->next.iaddr;
    }
    arp_entry = arp_table_lookup(NODE_ARP_TABLE(node),
                                 target_ip);
    if (!arp_entry) {
        arp_entry = calloc(1, sizeof(struct arp_entry));
        init_arp_entry(arp_entry, target_ip, skb->intf);
        arp_table_entry_add(NODE_ARP_TABLE(node), arp_entry);
    }
    if (!arp_entry->resolved) {
        list_add(&skb->list, &arp_entry->pending_pkts);
        // TODO: have a timeout to remove arp_entry
        send_arp_broadcast_request(skb->intf, target_ip);
        return true;
    }
    return ethernet_output(skb->intf, arp_entry->mac, ETH_PROTO_IP, skb);
}

u16 calc_ip_hcksum(struct ip_hdr *hdr)
{
    return calc_checksum_16((char *) hdr, hdr->hlen * 4);
}

void fill_ip_hcksum(struct ip_hdr *hdr)
{
    hdr->hcksum = HTONS(calc_ip_hcksum(hdr));
}

bool check_ip_hcksum(struct ip_hdr *hdr)
{
   return  calc_ip_hcksum(hdr) == 0;
}

void init_rt_table(struct rt_table *table)
{
    INIT_LIST_HEAD(&table->routes);
}

void destroy_rt_table(struct rt_table *table)
{
    struct rt_entry *entry, *tmp;
    list_for_each_entry_safe(entry, tmp, &table->routes, list) {
        list_del(&entry->list);
        free(entry);
    }
}

void dump_rt_table(struct rt_table *table)
{
    printf("RT Table\n");
    struct rt_entry *entry;
    RT_TABLE_FOREACH(entry, table) {
        // if next is 0, it means it's a local network
        printf("\tdst: %u.%u.%u.%u/%u, next: %u.%u.%u.%u, intf: %s\n",
            entry->dst.addr[0], entry->dst.addr[1], entry->dst.addr[2],
            entry->dst.addr[3], entry->mask, entry->next.addr[0],
            entry->next.addr[1], entry->next.addr[2], entry->next.addr[3],
            entry->intf->name);
    }
}

void rt_table_add_route(struct rt_table *table, struct ip_addr dst,
                        char mask, struct ip_addr next, struct intf *intf)
{
    struct rt_entry *entry = calloc(1, sizeof(struct rt_entry));
    entry->dst.iaddr = dst.iaddr;
    entry->mask = mask;
    entry->next.iaddr = next.iaddr;
    entry->intf = intf;
    list_add(&entry->list, &table->routes);
}

void init_ip_hdr(struct ip_hdr *hdr)
{
    memset(hdr, 0, sizeof(struct ip_hdr));
    hdr->ver = 4;
    hdr->hlen = 5;
    hdr->ttl = 64;
}

struct rt_entry *rt_lookup_lpm(struct rt_table *table, struct ip_addr ip)
{
    struct rt_entry *entry, *max_entry = NULL, *default_entry = NULL;
    char max_mask = 0;
    RT_TABLE_FOREACH(entry, table) {
        if(entry->dst.iaddr == 0) {
            default_entry = entry;
        } else if (IP_NET_EQ(&ip, &entry->dst, entry->mask) &&
                   entry->mask > max_mask){
            max_mask = entry->mask;
            max_entry = entry;
        }
    }
    return max_entry ?: default_entry;
}

bool is_ip_local(struct node *node, struct ip_addr ip)
{
    for (size_t i=0; i < MAX_INTFS_PER_NODE; i++) {
        if (node->intfs[i] && IF_IP(node->intfs[i])->iaddr == ip.iaddr)
            return true;
    }
    return false;
}

void set_ip_forward(bool val)
{
    ip_forward_enable = val;
}
