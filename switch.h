#ifndef SWITCH_H
#define SWITCH_H
#include <stdbool.h>
#include <stddef.h>
#include "address.h"
#include "list.h"
struct intf;
struct sk_buff;

struct mac_table {
    struct list_head entries;
};

struct mac_table_entry {
    struct mac_addr mac;
    struct intf *intf;
    struct list_head list;
};

bool switch_input(struct intf *intf, struct sk_buff *skb);

inline void init_mac_table(struct mac_table *table)
{
    INIT_LIST_HEAD(&table->entries);
}

#endif
