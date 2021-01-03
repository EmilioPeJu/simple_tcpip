project('simple_tcpip', 'c')
inc = include_directories(['.'])
graph_sources = ['tcp.c',
                 'udp.c',
                 'icmp.c',
                 'ip.c',
                 'arp.c',
                 'switch.c',
                 'ethernet.c',
                 'arp.c',
                 'comm_udp.c',
                 'comm_tuntap.c',
                 'comm.c',
                 'utils.c',
                 'graph.c',
                 'net.c']
pthread_dep = dependency('threads')
graph = library('graph', graph_sources, install : true,
                dependencies : [pthread_dep])
graph_dep = declare_dependency(link_with: graph, include_directories : inc)
inproject_deps = [ graph_dep ]
subdir('test')

