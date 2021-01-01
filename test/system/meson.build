tests = ['ping', 'udp', 'tuntap_udp']
foreach test : tests
  exe = executable('test_@0@'.format(test),
                   'test_@0@.c'.format(test),
                   dependencies : tests_deps,
                   install : true)
  #test(test, exe)
endforeach
