src = [
  'main.c',
  'platform/unix.c',
  'third_party/sds.c'
  ]

lib_dl   = cc.find_library('dl', required: true)
lib_pthread = cc.find_library('pthread', required: true)
lib_glfw = cc.find_library('glfw', required: true)

exe_loader = executable('loader',
                        sources: src,
                        include_directories: shared_dir,
                        dependencies: [lib_dl, lib_pthread, lib_glfw])

custom_target('copy to run',
              depends: exe_loader,
              input: exe_loader,
              output: 'fake',
              command: ['cp', '@INPUT@', join_paths(run_dir, 'loader')],
              build_by_default: true)
