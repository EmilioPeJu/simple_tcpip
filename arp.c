#include <stdio.h>
#include "arp.h"
#include "ethernet.h"
#include "list.h"
#include "skbuff.h"
#include "utils.h"

bool arp_input(struct sk_buff *skb)
{
    if (skb->len < ARP_HDR_SIZE)
        return false;
    struct arp_hdr *hdr = (struct arp_hdr *) skb->data;
    bool ok = false;
    switch (hdr->opcode) {
    case HTONS(ARP_OP_REQUEST):
        ok = process_arp_broadcast_request(skb->intf, hdr);
        break;
    case HTONS(ARP_OP_REPLY):
        ok = process_arp_reply_msg(skb->intf, hdr);
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
    struct sk_buff *skb = alloc_skb(BUFF_HEADROOM + BUFF_TAILROOM + \
        ARP_HDR_SIZE);
    skb_reserve(skb, BUFF_HEADROOM);
    struct arp_hdr *arp_hdr = skb_push(skb, ARP_HDR_SIZE);
    struct mac_addr dst_mac;
    layer2_fill_with_broadcast_mac(&dst_mac);
    arp_hdr->hw_type = HTONS(ARP_PROTO_ETH);
    arp_hdr->proto_type = HTONS(ARP_PROTO_IP);
    arp_hdr->hw_addr_len = MAC_ADDR_SIZE;
    arp_hdr->proto_addr_len = IP_ADDR_SIZE;
    arp_hdr->opcode = HTONS(ARP_OP_REQUEST);
    memcpy(&arp_hdr->sender_hw_addr, IF_MAC(intf), MAC_ADDR_SIZE);
    memcpy(&arp_hdr->sender_proto_addr, IF_IP(intf), IP_ADDR_SIZE);
    memset(arp_hdr->target_hw_addr.addr, 0, MAC_ADDR_SIZE);
    memcpy(arp_hdr->target_proto_addr.addr, ip.addr, IP_ADDR_SIZE);
    bool ok = ethernet_output(intf, dst_mac, ETH_PROTO_ARP, skb);
    if (!ok)
        free_skb(skb);
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
    struct sk_buff *skb = alloc_skb(BUFF_HEADROOM + BUFF_TAILROOM + \
        ARP_HDR_SIZE);
    skb_reserve(skb, BUFF_HEADROOM);
    struct arp_hdr *arp_hdr = skb_push(skb, ARP_HDR_SIZE);
    arp_hdr->hw_type = HTONS(ARP_PROTO_ETH);
    arp_hdr->proto_type = HTONS(ARP_PROTO_IP);
    arp_hdr->hw_addr_len = MAC_ADDR_SIZE;
    arp_hdr->proto_addr_len = IP_ADDR_SIZE;
    arp_hdr->opcode = HTONS(ARP_OP_REPLY);
    memcpy(&arp_hdr->sender_hw_addr, IF_MAC(intf), MAC_ADDR_SIZE);
    memcpy(&arp_hdr->sender_proto_addr, IF_IP(intf), IP_ADDR_SIZE);
    memcpy(&arp_hdr->target_hw_addr, &shdr->sender_hw_addr, MAC_ADDR_SIZE);
    memcpy(&arp_hdr->target_proto_addr, &shdr->sender_proto_addr, IP_ADDR_SIZE);
    ethernet_output(intf, shdr->sender_hw_addr, ETH_PROTO_ARP, skb);
}

bool process_arp_reply_msg(struct intf *intf, struct arp_hdr *hdr)
{
    arp_table_update_from_arp_reply(NODE_ARP_TABLE(intf->node), hdr, intf);
    return true;
}

void arp_table_update_from_arp_reply(struct arp_table *table,
                                     struct arp_hdr *hdr,
                                     struct intf *intf)
{
    struct arp_entry *arp_entry = arp_table_lookup(table, hdr->sender_proto_addr);
    if (!arp_entry) {
        printf("This  ARP reply doesn't match any request you made\n");
        return;
    }
    arp_entry->resolved = true;
    memcpy(arp_entry->mac.addr, hdr->sender_hw_addr.addr, MAC_ADDR_SIZE);
    // process pending packets to send
    struct sk_buff *skb, *tmp;
    list_for_each_entry_safe(skb, tmp, &arp_entry->pending_pkts, list) {
        list_del(&skb->list);
        bool result = ethernet_output(skb->intf, arp_entry->mac, ETH_PROTO_IP,
                                      skb);
        if (!result)
            free_skb(skb);
    }
}

struct arp_entry *arp_table_lookup(struct arp_table *table, struct ip_addr ip)
{
    struct arp_entry *entry;
    list_for_each_entry(entry, &table->entries, list) {
        if (entry->ip.iaddr == ip.iaddr)
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

void init_arp_entry(struct arp_entry *entry, struct ip_addr ip,
                    struct intf *intf)
{
    entry->ip.iaddr = ip.iaddr;
    memset(entry->mac.addr, 0, MAC_ADDR_SIZE);
    entry->intf = intf;
    entry->resolved = false;
    INIT_LIST_HEAD(&entry->pending_pkts);
}

void init_arp_table(struct arp_table *table)
{
    INIT_LIST_HEAD(&table->entries);
}

void destroy_arp_table(struct arp_table *table)
{
    struct arp_entry *entry, *tmp;
    list_for_each_entry_safe(entry, tmp, &table->entries, list) {
        list_del(&entry->list);
        free(entry);
    }
}


void dump_arp_table(struct arp_table *table)
{
    struct arp_entry *entry;
    printf("ARP Table\n");
    list_for_each_entry(entry, &table->entries, list) {
        u8 *ip = entry->ip.addr;
        u8 *mac = entry->mac.addr;
        struct sk_buff *skb;
        printf("\tentry ip: %u.%u.%u.%u, "
               "mac: %02x:%02x:%02x:%02x:%02x:%02x, intf: %s\n",
               ip[0], ip[1], ip[2], ip[3],
               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
               entry->intf->name);
        size_t npacket = 0;
        list_for_each_entry(skb, &entry->pending_pkts, list) {
            printf("\t\tPending packet %lu to %s\n", npacket++,
                                                     skb->intf->name);
        }
    }
}

