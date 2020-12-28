#ifndef UTILS_H
#define UTILS_H
#include <stdint.h>

void apply_mask(char *prefix, char mask, char *str_prefix);

uint32_t convert_ip_from_str_to_int(const char *ip_addr);

void convert_ip_from_int_to_str(uint32_t ip_addr, char *output);

void dump_hex(char *bytes, size_t size);
void layer2_fill_with_broadcast_mac(char *mac);

#define IS_MAC_BROADCAST_ADDR(mac) (mac[0] == -1 && mac[1] == -1 && \
    mac[2] == -1 && mac[3] == -1 && mac[4] == -1 && mac[5] == -1)

#endif
