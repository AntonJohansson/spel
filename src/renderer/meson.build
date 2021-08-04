src = [
  'src/renderer.c'
  ]

lib_renderer = shared_library('librenderer', src,
                              include_directories: shared_dir)

custom_target('copy to run',
              depends: lib_renderer,
              input: lib_renderer,
              output: 'fake',
              command: ['cp', '@INPUT@', join_paths(run_dir, 'librenderer')],
              build_by_default: true)
