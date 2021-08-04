project('game', 'c')

cc = meson.get_compiler('c')
run_dir = join_paths(meson.build_root(), 'run')

shared_dir = include_directories('src/shared')

#dep_m    = cc.find_library('m', required:false)
#dep_p    = dependency('threads')
#dep_glfw = dependency('glfw3')

#subdir('src/ctti')
#subdir('src/third_party')
subdir('src/game')
subdir('src/loader')
subdir('src/renderer')
