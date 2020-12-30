#include "switch.h"
#include "skbuff.h"

bool switch_input(struct sk_buff *skb)
{
    // TODO: implement L2 switching
    return false;
}

void init_mac_table(struct mac_table *table)
{
    INIT_LIST_HEAD(&table->entries);
}

void destroy_mac_table(struct mac_table *table)
{
    struct mac_table_entry *entry, *tmp;
    list_for_each_entry_safe(entry, tmp, &table->entries, list) {
        list_del(&entry->list);
        free(entry);
    }
}
