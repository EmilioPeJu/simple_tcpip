#include <stdio.h>
#include "arp.h"
#include "ethernet.h"
#include "list.h"
#include "skbuff.h"
#include "utils.h"

bool arp_input(struct intf *intf, struct sk_buff *skb)
{
    if (skb->len < ARP_HDR_SIZE)
        return false;
    struct arp_hdr *hdr = (struct arp_hdr *) skb->data;
    bool ok = false;
    switch (hdr->opcode) {
    case HTONS(ARP_OP_REQUEST):
        ok = process_arp_broadcast_request(intf, hdr);
        break;
    case HTONS(ARP_OP_REPLY):
        ok = process_arp_reply_msg(intf, hdr);
        break;
    }
    if (ok)
        free_skb(skb);
    return ok;
}

void send_arp_broadcast_request(struct intf *intf, struct ip_addr ip)
{
    // TODO: handle intf == NULL, meaning it should find the
    //       appropriate interface
    struct sk_buff *skb = alloc_skb(ETH_HDR_SIZE + ARP_HDR_SIZE);
    struct ethernet_hdr *ether_hdr = skb_put(skb, ETH_HDR_SIZE);
    struct arp_hdr *arp_hdr = skb_put(skb, ARP_HDR_SIZE);
    layer2_fill_with_broadcast_mac(&ether_hdr->dst_mac);
    memcpy(&ether_hdr->src_mac, IF_MAC(intf), MAC_ADDR_SIZE);
    ether_hdr->type = HTONS(ETH_PROTO_ARP);
    arp_hdr->hw_type = HTONS(ARP_PROTO_ETH);
    arp_hdr->proto_type = HTONS(ARP_PROTO_IP);
    arp_hdr->hw_addr_len = MAC_ADDR_SIZE;
    arp_hdr->proto_addr_len = IP_ADDR_SIZE;
    arp_hdr->opcode = HTONS(ARP_OP_REQUEST);
    memcpy(&arp_hdr->sender_hw_addr, IF_MAC(intf), MAC_ADDR_SIZE);
    memcpy(&arp_hdr->sender_proto_addr, IF_IP(intf), IP_ADDR_SIZE);
    memset(arp_hdr->target_hw_addr.addr, 0, MAC_ADDR_SIZE);
    memcpy(arp_hdr->target_proto_addr.addr, ip.addr, IP_ADDR_SIZE);
    send_pkt_out(skb, intf);
}

bool process_arp_broadcast_request(struct intf *intf, struct arp_hdr *hdr)
{
    if (IF_IP(intf)->iaddr != hdr->target_proto_addr.iaddr)
        return false;
    send_arp_reply_msg(intf, hdr);
    return true;
}

void send_arp_reply_msg(struct intf *intf, struct arp_hdr *shdr)
{
    struct sk_buff *skb = alloc_skb(ETH_HDR_SIZE + ARP_HDR_SIZE);
    struct ethernet_hdr *ether_hdr = skb_put(skb, ETH_HDR_SIZE);
    struct arp_hdr *arp_hdr = skb_put(skb, ARP_HDR_SIZE);
    memcpy(&ether_hdr->dst_mac, &shdr->sender_hw_addr, MAC_ADDR_SIZE);
    memcpy(&ether_hdr->src_mac, IF_MAC(intf), MAC_ADDR_SIZE);
    ether_hdr->type = HTONS(ETH_PROTO_ARP);
    arp_hdr->hw_type = HTONS(ARP_PROTO_ETH);
    arp_hdr->proto_type = HTONS(ARP_PROTO_IP);
    arp_hdr->hw_addr_len = MAC_ADDR_SIZE;
    arp_hdr->proto_addr_len = IP_ADDR_SIZE;
    arp_hdr->opcode = HTONS(ARP_OP_REPLY);
    memcpy(&arp_hdr->sender_hw_addr, IF_MAC(intf), MAC_ADDR_SIZE);
    memcpy(&arp_hdr->sender_proto_addr, IF_IP(intf), IP_ADDR_SIZE);
    memcpy(&arp_hdr->target_hw_addr, &shdr->sender_hw_addr, MAC_ADDR_SIZE);
    memcpy(&arp_hdr->target_proto_addr, &shdr->sender_proto_addr, IP_ADDR_SIZE);
    send_pkt_out(skb, intf);
}

bool process_arp_reply_msg(struct intf *intf, struct arp_hdr *hdr)
{
    arp_table_update_from_arp_reply(NODE_ARP_TABLE(intf->node), hdr, intf);
    return true;
}

struct arp_entry *arp_table_lookup(struct arp_table *table, struct ip_addr ip)
{
    struct arp_entry *entry;
    list_for_each_entry(entry, &table->entries, list) {
        if (entry->ip_addr.iaddr == ip.iaddr)
            return entry;
    }
    return NULL;
}

bool arp_table_entry_add(struct arp_table *table, struct arp_entry *entry)
{
    // TODO: have a maximum number of entries
    // TODO: check if the entry already exists
    list_add(&entry->list, &table->entries);
    return true;
}

void arp_table_update_from_arp_reply(struct arp_table *table,
                                     struct arp_hdr *hdr,
                                     struct intf *intf)
{
    struct arp_entry *entry = calloc(1, sizeof(struct arp_entry));
    entry->ip_addr.iaddr = hdr->sender_proto_addr.iaddr;
    memcpy(entry->mac_addr.addr, hdr->sender_hw_addr.addr, MAC_ADDR_SIZE);
    entry->intf = intf;
    arp_table_entry_add(NODE_ARP_TABLE(intf->node), entry);
}

void destroy_arp_table(struct arp_table *table)
{
    // TODO: deallocate each entry
}


void dump_arp_table(struct arp_table *table)
{
    struct arp_entry *entry;
    printf("ARP Table\n");
    list_for_each_entry(entry, &table->entries, list) {
        u8 *ip = entry->ip_addr.addr;
        u8 *mac = entry->mac_addr.addr;
        printf("\tentry ip: %u.%u.%u.%u "
               "mac: %02x:%02x:%02x:%02x:%02x:%02x intf: %s\n",
               ip[0], ip[1], ip[2], ip[3],
               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
               entry->intf->name);
    }
}

