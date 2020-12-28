#ifndef IP_H
#define IP_H
#include <stdbool.h>
#include <stddef.h>

struct intf;
struct sk_buff;

bool ip_input(struct intf *intf, struct sk_buff *skb);

#endif
