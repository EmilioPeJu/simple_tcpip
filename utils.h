#ifndef UTILS_H
#define UTILS_H
#include <stdint.h>
#include "address.h"

#define HTONS(n) ((n>>8) + ((n & 0xff) <<8))

void apply_mask(char *prefix, char mask, char *str_prefix);

u32 convert_ip_from_str_to_int(const char *ip_addr);

void convert_ip_from_int_to_str(u32 ip_addr, char *output);

void layer2_fill_with_broadcast_mac(struct mac_addr *mac);

#define IS_MAC_BROADCAST_ADDR(mac)                             \
    ({      typeof(mac) _mac = (mac);                          \
            _mac->addr[0] == 0xff && _mac->addr[1] == 0xff     \
            && _mac->addr[2] == 0xff && _mac->addr[3] == 0xff  \
            && _mac->addr[4] == 0xff && _mac->addr[5] == 0xff; \
     })

void dump_hex(char *bytes, size_t size);

#define MIN(a,b)               \
   ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
      _a < _b ? _a : _b; })

#endif
