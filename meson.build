project('simple_tcpip', 'c')
inc = include_directories(['.'])
graph = library('graph', 'graph.c', install : true)
graph_dep = declare_dependency(link_with: graph, include_directories : inc)
inproject_deps = [ graph_dep ]
subdir('test/unit')
subdir('test/system')

