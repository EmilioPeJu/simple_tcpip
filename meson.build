project('simple_tcpip', 'c')
inc = include_directories(['.'])
graph_sources = ['utils.c',
                 'graph.c',
                 'net.c']
graph = library('graph', graph_sources, install : true)
graph_dep = declare_dependency(link_with: graph, include_directories : inc)
inproject_deps = [ graph_dep ]
subdir('test/unit')
subdir('test/system')

