#include <stdio.h>
#include <string.h>
#include "utils.h"

void apply_mask(char *prefix, char mask, char *str_prefix)
{
    uint32_t ip = convert_ip_from_str_to_int(prefix);
    ip &= (1<<mask) - 1;
    convert_ip_from_int_to_str(ip, str_prefix);
}

uint32_t convert_ip_from_str_to_int(const char *ip_addr)
{
    char raw_ip_addr[4];
    unsigned int a, b, c, d;
    int n = sscanf(ip_addr, "%u.%u.%u.%u", &a, &b, &c, &d);
    if (n != 4 || a > 255 || b > 255 || c > 255 || d > 255)
        return 0;
    raw_ip_addr[0] = a;
    raw_ip_addr[1] = b;
    raw_ip_addr[2] = c;
    raw_ip_addr[3] = d;
    return *((uint32_t *) raw_ip_addr);
}

void convert_ip_from_int_to_str(uint32_t ip_addr, char *output)
{
    sprintf(output, "%u.%u.%u.%u",
            ip_addr & 0xff,
            ip_addr>>8 & 0xff,
            ip_addr>>16 & 0xff,
            ip_addr>>24 & 0xff);
}

void layer2_fill_with_broadcast_mac(char *mac)
{
    mac[0] = mac[1] = mac[2] = mac[3] = mac[4] = mac[5] = 255;

void dump_hex(char *bytes, size_t size)
{
    for (size_t i=0; i < size/16; i++) {
        for (size_t j=i*16; j < (i+1)*16; j++) {
            printf("%02x ", (unsigned char) bytes[j]);
        }
        printf("| ");
        for (size_t j=i*16; j < (i+1)*16; j++) {
            printf("%c", bytes[j]);
        }
        printf("\n");
    }
    for (size_t i=size/16 * 16; i < size; i++) {
        printf("%02x ", (unsigned char) bytes[i]);
    }
    if (size%16) {
        for (size_t i=0; i < 16 - size%16; i++)
            printf("   ");
    }
    printf("| ");
    for (size_t i=size/16 * 16; i < size; i++) {
        printf("%c", bytes[i]);
    }
}
