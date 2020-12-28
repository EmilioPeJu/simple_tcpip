#ifndef ARP_H
#define ARP_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "address.h"
#include "list.h"
#define ARP_OP_REQUEST (1)
#define ARP_OP_REPLY (2)
#define ARP_PROTO_ETH (1)
#define ARP_PROTO_IP (0x0800)

struct intf;
struct sk_buff;

struct arp_table {
    struct list_head entries;
};

struct arp_entry {
    struct ip_addr ip_addr;
    struct mac_addr mac_addr;
    struct intf *intf;
    struct list_head list;
};

struct arp_hdr {
    u16 hw_type;
    u16 proto_type;
    u8 hw_addr_len;
    u8 proto_addr_len;
    u16 opcode;
    struct mac_addr sender_hw_addr;
    struct ip_addr sender_proto_addr;
    struct mac_addr target_hw_addr;
    struct ip_addr target_proto_addr;
} __attribute__((packed));

#define ARP_HDR_SIZE (sizeof(struct arp_hdr))

bool arp_input(struct intf *intf, struct sk_buff *skb);

void send_arp_broadcast_request(struct intf *intf, struct ip_addr ip);

bool process_arp_broadcast_request(struct intf *intf, struct arp_hdr *hdr);

void send_arp_reply_msg(struct intf *intf, struct arp_hdr *sender_hdr);

bool process_arp_reply_msg(struct intf *intf,  struct arp_hdr *hdr);

struct arp_entry *arp_table_lookup(struct arp_table *table, struct ip_addr ip);

bool arp_table_entry_add(struct arp_table *table, struct arp_entry *entry);

void arp_table_update_from_arp_reply(struct arp_table *table,
                                     struct arp_hdr *hdr,
                                     struct intf *intf);

void dump_arp_table(struct arp_table *table);

void destroy_arp_table(struct arp_table *table);

inline void init_arp_table(struct arp_table *table)
{
    INIT_LIST_HEAD(&table->entries);
}

#endif
