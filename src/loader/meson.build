src = [
  'main.c',
  'platform/unix.c',
  'third_party/sds.c'
  ]

lib_dl = cc.find_library('dl', required: true)

exe_loader = executable('loader',
                        sources: src,
                        dependencies: [lib_dl])

custom_target('copy to run',
              depends: exe_loader,
              input: exe_loader,
              output: 'fake',
              command: ['cp', '@INPUT@', join_paths(run_dir, 'loader')],
              build_by_default: true)
