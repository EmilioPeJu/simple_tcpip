# simple_tcpip

## Introduction
Clone of sachinites' TCP/IP implementation with the following differences:
- Naming of structures is slightly different
- It avoids using GLThreads (implementing what's required in it)
- It uses meson as build system
- It includes unit tests
- The communication layer uses one socket per interface instead of one socket
  per node, that way there is no need to add extra info to the packet
- The receiving thread uses `epoll` instead of `select`

## Features implemented so far
- Generic graph API
- ARP request and reply handling
- Basic IP handling
- ICMP echo request/reply
