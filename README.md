## Qubatron - Photorealistic path-traced micro-voxel fps engine prototype

[![Qubatron](https://img.youtube.com/vi/LqytIbcjX18/0.jpg)](https://www.youtube.com/watch?v=LqytIbcjX18)

- Try it online [here](https://milgra.com/qubatron/)! ( low detail version, 17 million voxels, 300Mbyte download, WASD + mouse + numbers for resolution )  
- Watch the demo [here](https://youtu.be/kmjUZZyvqhA?si=56xASom5bmYTcNpD) ( high detail version, 33 million voxels, Intel Arc GPU )
- Build and run for yourself ( instructions below )

### Is 2024 the year of voxel-based rendering and the end of polygons? Let's find out!

The main idea is :
- point cloud based levels
- path traced octree-based visibility calculations
- if path tracing is already there use it for lighting calculations ( voxel level shadow )
- and later - if the gpu still has enough power - for reflections/transparency/everything
- ~~distance-dependent octree detail level - tried it, doesn't really lower gpu load~~
- ~~procedural sub-detail voxel generation for close voxels - tried it, doesn't really look good~~
- instead of the above use super-detailed voxel clouds in case of nearby octrees
- voxel level physics for particles and skeletal animations

The method :
- upload octree structure to gpu
- in the fragment shader for every screen coordinate
 - create vector from camera focus point to screen coordinate
 - check intersection with octree
 - in case of intersection with leaf use it's color
 - in case of intersection with leaf check if light ray 'sees' the intersection point
 - calculate final color for screen point using octet normal

Octet intersection check :
- check line intersection with all sides of base cube using x = x0 + dirX * t -> t = ( x - x0 ) / dirX
- check coordinate intervals for sides with intersection
- then go into subnodes
 - first I used brute force and checked all sides of all octets, it was slow
 - now I just check the two bisecting planes and bring side intersections from previous step
 - figure out the touched octets and their order from the intersection points

Static octree modification :  
- zero out wanted octets in current octree structure
- octets will be added to the end of the octree array
- update zeroed out and added ranges on the gpu

Dynamic octree modifictaion :
- modify all points in a compute shader based on bones
- recreate octree structure
- upload as dynamic octree

Things learned :
- if statements, loops, arrays and variables kill shader performance badly

Performance :
Measured on my ASUS Zenbook UX3405 14 OLED / Intel(R) Core(TM) Ultra 5 125H / Intel Arc Graphics / 1200x800 points resolution
- 3318142 voxels / no lights : 50 fps
- 3318142 voxels / 1 light : 16 fps
- 16969878 voxels / 1 light : 10 fps 

How to build :
```
meson setup build
ninja -C build
build/qubatron -v
```

How to create model files

- Use CloudCompare, open abandoned_cc.bin then zombie_cc.bin
- Convert mesh to sample ( 90 million points for abandoned, 10 million for zombie )
- Export as ply with color and normal data
- Use build/qmc to convert result .ply-s to flat vertex, normal and color files :
 - build/qmc -s 1800 -l 12 -i media/abandoned.ply
 - build/qmc -s 1800 -l 12 -i media/zombie.ply
- Problem : CloudCompare doesn't cover mesh perfectly since it uses random points
- Alternative solution : Convert [obj2voxel](https`://github.com/Eisenwave/obj2voxel) to export surface normals

Videos :

- zombie added - high detail - 13488259 voxels with 1 light - commit 206478b : [https://youtu.be/u8h5nE1rz5Y]
- high detail - 16969878 voxels with 1 light - commit eb3f1a4 : [https://youtu.be/OZ8vxzFvEM8]
- normal detail - 3318142 voxels with no light, sub-detail randomization enabled - commit 7ab1f55 : [https://youtu.be/giQ5RIZmgMQ]
- normal detail - 3318142 voxels with 1 light - commit 344c25a : [https://www.youtube.com/watch?v=LqytIbcjX18]

WebAssembly :

find src -type f -name "*.c" > files.txt

source "/home/milgra/Downloads/emsdk/emsdk_env.sh"

emcc -Isrc/qubatron -Isrc/mt_core -Isrc/mt_math -Isrc/rply-1.1.4 -I/home/milgra/Downloads/emsdk/upstream/emscripten/system/includer/emscripten.h -DPATH_MAX=255 -DPKG_DATADIR=\"/\" -DQUBATRON_VERSION=\"1.0\" -O3 -sUSE_SDL=2 -sMAX_WEBGL_VERSION=2 $(cat files.txt) -sALLOW_MEMORY_GROWTH=1 -sMAXIMUM_MEMORY=4Gb --preload-file src/qubatron --preload-file res -o wasm/qubatron.html

Todo :

- walking fix
- legzeses fel-le mozgas kameranak
- gun light sphere
- blood spill
- dust cloud ( zero out nearby voxels in dust shader )
- animated standup
- bigger building
- separate orind array for octree
- webgl fencing
