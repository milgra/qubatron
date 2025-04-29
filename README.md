## Qubatron - Photorealistic ray-traced micro-voxel fps engine

[![Qubatron](screenshot.png)](https://www.youtube.com/watch?v=LBzuXj21_bY)

- Try it online [here](https://milgra.com/qubatron/)! ( low detail version )
- Watch the demo [here](https://www.youtube.com/watch?v=LBzuXj21_bY) ( high detail version )
- Build and run for yourself ( instructions below )

### Support

Would you like to see a game using this engine? Please support development.
- Via Github Sponsors, use the button on the right panel
- Via [Paypal](https://paypal.me/milgra)

### Is ~~2024~~ 2025 the year of voxel based rendering and the end of polygons? Let's find out!

The main idea is :
- point cloud based levels
- ray-traced sparse octree based visibility calculations
- ray-traced shadows
- if the gpu still has enough power - ray-traced reflections/transparency/refractions
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
- update all points and their normals in a compute shader
- calculate new octree paths in the compute shader
- rebuild dynamic octree array from octree paths on the CPU
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
    - Use a modified version of [obj2voxel](https://github.com/Eisenwave/obj2voxel)
        - it already generates voxels for abandoned house under 1 second
        - normal generation must be added
- Extend level size with more basecubes/stream basecube data to GPU
    - Probably with slicing up levels into multiple basecubes automatically
- Use alpha channel for voxels ( glass, transparency effects )
- Add blender bone data import for simpler skeletal animation

### Code Internals

model.c - contains model data, position, normal and color information for every point in a flat vector array. All attributes are three component float vectors for size efficiency. Thinking about adding an alpha channel to the color attribute for transparency.

octree.c - builds up and stores octree information for a model. Each element is a 12 length int array, the first 8 contains indexes for the subnodes in the octree array, the 9th is the index of the vertex in the model object. For space efficiency it will be converted to 8 length int arrays and a separate model index array. Octets can be added by point position or by pre-calculated voxel path. 

*_glc.c - gl connectors. Shader initialization and buffer handling is their task.

#### Shaders

octree_fsh.c - the soul of the game, ray-traced octree collosion detection is happening here, shadow, final color calculations.

skeleton_vsh.c - compute shader for skeletal animation. character model point clouds are updated here based on bone positions. octree path calculation also happening here.

particle_vsh.c - blood/debris particle simulation compute shader

dust_vsh.c - dust particle simulation compute shader

texquad_vsh/fsh - render to texture shader to be able to scale output

### Notes

Things learned :
- if statements, loops, arrays and variables kill shader performance badly
- it's better to calculate octree path for a new point on the GPU so the CPU only deals with indexes when building up the octree

For WebGL compatibilty a lot of workarounds had to be applied
- Insted of using shader storage buffer objects or buffer textures regular textures had to be used
- No layout qualifiers can be used in shaders
- Arrays cannot be returned in transform feedback buffers, I had to use 4 separate buffers for 12 ints and 4 floats

Skeletal animation.
Locking points to more than two bones caused me a few sleepless nights but the result is totally satisfying.
In addition I was really surprised when I realized that I had to rotate point normals with the points locations also to make lighting work :)

Objects doesn't have volume in Qubatron, just a thin voxel layer around them for memory reasons. Creating bullet holes and especially random bullet holes are tough. Still trying to figure it out.

### Todo

- fix floating particles
- separate orind array for octree
- sinus/random edge for punch hole
- webgl fencing

