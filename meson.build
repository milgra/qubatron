project(
  'qubatron',
  'c',
  version: '0.8',
  license: 'GPL',
  default_options: ['c_std=gnu99']
)

cc = meson.get_compiler('c')

math = cc.find_library('m')
gl = cc.find_library('GL')
glew = cc.find_library('GLEW')
glu = cc.find_library('GLU')
sdl = cc.find_library('SDL2')

qubatron_dependencies = [math,gl,glew,sdl]

qubatron_version = '"@0@"'.format(meson.project_version())
pkg_datadir = join_paths(get_option('prefix'), get_option('datadir')) / 'qubatron'
add_project_arguments('-DPKG_DATADIR="' + pkg_datadir + '"',
		      '-DQUBATRON_VERSION=@0@'.format(qubatron_version),
		      language: 'c')
add_project_arguments('-Wno-unused-but-set-variable', language: 'c')

if get_option('buildtype') == 'debug'
    add_project_arguments('-DDEBUG',language: 'c')
endif

qubatron_inc = include_directories(
	'/usr/include',
	'/usr/include/GL',
	'/usr/include/SDL2',
	'src/rply-1.1.4',
	'src/qubatron',
	'src/mt_core',
	'src/mt_math')

com_sources = ['src/rply-1.1.4/rply.c',
	       'src/qubatron/main.c',
	       'src/qubatron/model.c',
	       'src/qubatron/octree.c',
	       'src/qubatron/readfile.c',
	       'src/qubatron/skeleconn.c',
	       'src/qubatron/renderconn.c',
	       'src/qubatron/ku_gl_shader.c',
	       'src/qubatron/ku_gl_floatbuffer.c',
	       'src/mt_core/mt_log.c',
	       'src/mt_core/mt_time.c',
	       'src/mt_core/mt_memory.c',
	       'src/mt_math/mt_vector_2d.c',
	       'src/mt_math/mt_vector_3d.c',
	       'src/mt_math/mt_vector_4d.c',
	       'src/mt_math/mt_math_3d.c',
	       'src/mt_math/mt_matrix_4d.c'
	       ]

fs = import('fs')

fs.copyfile('src/qubatron/vsh.c', 'vsh.c')
fs.copyfile('src/qubatron/fsh.c', 'fsh.c')
fs.copyfile('src/qubatron/csh.c', 'csh.c')
fs.copyfile('src/qubatron/dsh.c', 'dsh.c')
fs.copyfile('src/qubatron/vsh_texquad.c', 'vsh_texquad.c')
fs.copyfile('src/qubatron/fsh_texquad.c', 'fsh_texquad.c')

executable(
  'qubatron',
  com_sources,
  include_directories: [qubatron_inc],
  dependencies: qubatron_dependencies,
  install: true,
)
