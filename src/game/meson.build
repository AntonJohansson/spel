src = [
  'src/game.c'
  ]

lib_game = shared_library('libgame', src,
                          include_directories: shared_dir)

custom_target('copy to run',
              depends: lib_game,
              input: lib_game,
              output: 'fake',
              command: ['cp', '@INPUT@', join_paths(run_dir, 'libgame')],
              build_by_default: true)
