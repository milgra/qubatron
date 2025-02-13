project(
  'qubatron',
  'c',
  version: '0.1',
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
		      '-DQMC_VERSION=@0@'.format(qubatron_version),
		      language: 'c')
add_project_arguments('-Wno-unused-but-set-variable', language: 'c')

if get_option('buildtype') == 'debug'
    add_project_arguments('-DDEBUG',language: 'c')
endif

qubatron_inc = include_directories(
	'/usr/include',
	'/usr/include/GL',
	'/usr/include/SDL2',
	'src/qubatron',
	'src/mt_core',
	'src/mt_math')

qubatron_sources = [
	       'src/qubatron/qubatron.c',
	       'src/qubatron/model.c',
	       'src/qubatron/modelutil.c',
	       'src/qubatron/octree.c',
	       'src/qubatron/skeleton_glc.c',
	       'src/qubatron/octree_glc.c',
	       'src/qubatron/shader.c',
	       'src/mt_core/mt_log.c',
	       'src/mt_core/mt_time.c',
	       'src/mt_core/mt_memory.c',
	       'src/mt_math/mt_vector_2d.c',
	       'src/mt_math/mt_vector_3d.c',
	       'src/mt_math/mt_vector_4d.c',
	       'src/mt_math/mt_quat.c',
	       'src/mt_math/mt_math_3d.c',
	       'src/mt_math/mt_matrix_4d.c'
	       ]

fs = import('fs')

fs.copyfile('src/qubatron/shaders/octree_vsh.c', 'octree_vsh.c')
fs.copyfile('src/qubatron/shaders/octree_fsh.c', 'octree_fsh.c')
fs.copyfile('src/qubatron/shaders/skeleton_vsh.c', 'skeleton_vsh.c')
fs.copyfile('src/qubatron/shaders/skeleton_fsh.c', 'skeleton_fsh.c')
fs.copyfile('src/qubatron/shaders/texquad_vsh.c', 'texquad_vsh.c')
fs.copyfile('src/qubatron/shaders/texquad_fsh.c', 'texquad_fsh.c')
fs.copyfile('src/qubatron/shaders/particle_vsh.c', 'particle_vsh.c')
fs.copyfile('src/qubatron/shaders/particle_fsh.c', 'particle_fsh.c')

fs.copyfile('res/abandoned.ply.col', 'abandoned.ply.col')
fs.copyfile('res/abandoned.ply.nrm', 'abandoned.ply.nrm')
fs.copyfile('res/abandoned.ply.pnt', 'abandoned.ply.pnt')
fs.copyfile('res/abandoned.ply.rng', 'abandoned.ply.rng')
fs.copyfile('res/zombie.ply.col', 'zombie.ply.col')
fs.copyfile('res/zombie.ply.nrm', 'zombie.ply.nrm')
fs.copyfile('res/zombie.ply.pnt', 'zombie.ply.pnt')
fs.copyfile('res/zombie.ply.rng', 'zombie.ply.rng')

executable(
  'qubatron',
  qubatron_sources,
  include_directories: [qubatron_inc],
  dependencies: qubatron_dependencies,
  install: true,
)

qmc_inc = include_directories(
	'/usr/include',
	'src/rply-1.1.4',
	'src/qubatron',
	'src/mt_core',
	'src/mt_math')

qmc_sources = ['src/rply-1.1.4/rply.c',
	       'src/qubatron/qmc.c',
	       'src/mt_core/mt_log.c',
	       'src/mt_core/mt_time.c',
	       'src/mt_core/mt_vector.c',
	       'src/mt_core/mt_string.c',
	       'src/mt_core/mt_memory.c',
	       'src/mt_math/mt_vector_2d.c',
	       'src/mt_math/mt_vector_3d.c',
	       'src/mt_math/mt_vector_4d.c',
	       'src/mt_math/mt_math_3d.c',
	       'src/mt_math/mt_matrix_4d.c'
	       ]

qmc_dependencies = [math,gl,glew,sdl]

executable(
  'qmc',
  qmc_sources,
  include_directories: [qmc_inc],
  dependencies: qmc_dependencies,
  install: true,
)
