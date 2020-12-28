#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

#define MAC_ADDR_SIZE (6)
#define IP_ADDR_SIZE (4)

struct ip_addr {
    union {
        u8 addr[IP_ADDR_SIZE];
        u32 iaddr;
    };
} __attribute__((packed));

struct mac_addr {
    u8 addr[MAC_ADDR_SIZE];
} __attribute__((packed));

#endif
