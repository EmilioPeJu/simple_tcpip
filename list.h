#ifndef LIST_H
#define LIST_H
struct list_head {
    struct list_head *next, *prev;
};

inline int list_empty(struct list_head *list)
{
    return list->next == list;
}

inline void INIT_LIST_HEAD(struct list_head *list)
{
    list->next = list->prev = list;
}

inline void list_add(struct list_head *new, struct list_head *head)
{
    new->prev = head;
    new->next = head->next;
    new->prev->next = new;
    new->next->prev = new;
}

inline void list_del(struct list_head *entry)
{
    entry->prev->next = entry->next;
    entry->next->prev = entry->prev;
    INIT_LIST_HEAD(entry);
}

#define list_for_each(pos, head) \
    for (pos = (head)->next;pos != (head); pos = pos->next)

#define container_of(ptr, type, member) \
    ((type *) ((char *) ptr - offsetof(type, member)))

#define list_entry(ptr, type, member) container_of(ptr, type, member)

#define list_for_each_entry(pos, head, member) \
     for (pos = list_entry((head)->next, typeof(*pos), member); &pos->member != (head); pos = list_entry(pos->member.next, typeof(*pos), member))

#endif
