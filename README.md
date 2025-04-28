## Qubatron - Photorealistic ray traced micro-voxel fps engine

[![Qubatron](https://img.youtube.com/vi/LqytIbcjX18/0.jpg)](https://www.youtube.com/watch?v=LqytIbcjX18)

- Try it online [here](https://milgra.com/qubatron/)! ( low detail version )
- Watch the demo [here](https://youtu.be/kmjUZZyvqhA?si=56xASom5bmYTcNpD) ( high detail version, 33 million voxels, Intel Arc GPU )
- Build and run for yourself ( instructions below )

### Is ~~2024~~ 2025 the year of voxel based rendering and the end of polygons? Let's find out!

The main idea is :
- point cloud based levels
- raytraced sparse octree based visibility calculations
- raytraced shadows
- if the gpu still has enough power - raytraced reflections/transparency/refractions
- if the gpu still has enough power - path tracing with multiple bounce
- ~~distance-dependent octree detail level - tried it, doesn't really lower gpu load, maybe in bigger scenes later~~
- ~~procedural sub-detail voxel generation for close voxels - tried it, doesn't really look good, maybe later~~
- higher detail point clouds for nearby objects - for weapons
- voxel level physics for particles and skeletal animations

The method :
- upload octree structure, colors and normals to gpu
- in the fragment shader for every screen coordinate
 - create vector from camera focus point to screen coordinate
 - check intersection with octree
 - in case of intersection with leaf get it's color
 - in case of intersection with leaf check if light ray 'sees' the intersection point
 - calculate final color of screen coord using octet color, normal and light direction/visibility

Octet intersection check :
- check line intersection with all sides of base cube using x = x0 + dirX * t -> t = ( x - x0 ) / dirX
- check coordinate intervals for sides with intersection
- then go into subnodes
 - check the two bisecting planes and bring side intersections from previous step
 - figure out the touched octets and their order from the intersection points

Static octree modification :  
- zero out wanted octets in current octree structure
- octets will be added to the end of the octree array
- update zeroed out and added ranges on the gpu

Dynamic octree modifictaion / Point cloud animation :
- update all points and their normals in a compute shader based on bones
- calculate new octree paths in the compute shader
- rebuild dynamic octree array from octree paths 
- upload dynamic octree to GPU

### How to build & run

```
meson setup build
ninja -C build
```

Then generate flat model files

- Use CloudCompare, open media/abandoned_cc.bin then media/zombie_cc.bin
- Convert mesh to sample ( 90 million points for abandoned, 10 million for zombie )
- Save as PLY in binary format
- Use build/qmc to convert result .ply-s to flat vertex, normal and color files :
 - build/qmc -s 1800 -l 12 -i media/abandoned.ply
 - build/qmc -s 1800 -l 12 -i media/zombie.ply
- Copy resulting col, nrm and pnt files under /res

Then start qubatron

```
build/qubatron -v
```

webasm build

```
find src -type f -name "*.c" -not -path "./src/qubatron/shaders/*" > files.txt
source "/home/milgra/Downloads/emsdk/emsdk_env.sh"
emcc --emrun -Isrc/qubatron  -Isrc/mt_core -Isrc/mt_math -I/home/milgra/Downloads/emsdk/upstream/emscripten/system/includer/emscripten.h -DPATH_MAX=255 -DPKG_DATADIR=\"/\" -DQUBATRON_VERSION=\"1.0\" -O3 -sUSE_SDL=2 -sMAX_WEBGL_VERSION=2 $(cat files.txt) -sALLOW_MEMORY_GROWTH=1 -sMAXIMUM_MEMORY=4Gb --preload-file src/qubatron --preload-file resasm -o wasm/qubatron.html
```

### Roadmap

- Get a PC with a high-end GPU to squeeze the maximum out from the engine 
- Load time voxelization instead of pre-generated flat files, because
 - Asset sizes are too big, with load-time voxelization asset sizes can be reduced from 1.4 Gbytes to 75 Mbytes
 - WebGL version can use the same detail level as the desktop version
 - CloudCompare doesn't cover mesh perfectly since it uses random points
 - Use a modified version of [obj2voxel](https`://github.com/Eisenwave/obj2voxel)
  - it already generates voxels for abandoned house under 1 second
  - doesn't generates normals, I have to update it with normal generation

### Notes

Things learned :
- if statements, loops, arrays and variables kill shader performance badly
- it's better to calculate octree path for a new point on the GPU so the CPU only deals with indexes when building up the octree

For WebGL compatibilty a lot of workarounds had to be applied
- Insted of using shader storage buffer objects or buffer textures regular textures had to be used
- No layout qualifiers can be used in shaders
- Arrays cannot be returned in transform feedback buffers, I had to use 4 separate buffers for 12 ints and 4 floats

Todo :

- separate orind array for octree
- limit particle count
- fix floating particles
- sinus/random edge for punch hole
- webgl fencing

