tests = ['comm', 'graph', 'net', 'utils', 'ethernet', 'arp', 'ip']
unity_dep = dependency('libunity')
tests_deps = [unity_dep, fixture_dep]
tests_deps += inproject_deps
foreach test : tests
  exe = executable('test_@0@'.format(test),
                   'test_@0@.c'.format(test),
                   dependencies : tests_deps)
  test(test, exe, args : ['-v'])
endforeach
