# qubatron
Photorealistic path-traced micro-voxel fps engine prototype

[![Qubatron](https://img.youtube.com/vi/LqytIbcjX18/0.jpg)](https://www.youtube.com/watch?v=LqytIbcjX18)

Is 2024 the year of voxel-based rendering and the end of polygons? Let's find out!

The main idea is :
- point cloud based levels
- path traced octree-based visibility calculations
- if path tracing is already there use it for lighting calculations
- and later - if the gpu still has enough power - for reflections/transparency/everything
- ~~distance-dependent octree detail level - tried it, doesn't really lower gpu load~~
- ~~procedural sub-detail voxel generation for close voxels - tried it, doesn't really look good~~
- instead of the above use super-detailed voxel clouds in case of nearby octrees

The method :
- upload octree structure to gpu
- in the fragment shader for every screen coordinate
 - create vector from camera focus point to screen coordinate
 - check intersection with octree
 - in case of intersection with leaf use it's color
 - in case of intersection with leaf check if intersection point 'sees' the light source
 - calculate final color for screen point

Octet intersection check :
- check line intersection with all sides of octet using x = x0 + dirX * t -> t = ( x - x0 ) / dirX
- check coordinate intervals for sides with intersection
- then go into subnodes
 - first I used brute force and checked all sides of all octets, it was slow
 - now I just check the 6 sides of the parent octet and the two bisecting planes
 - and figure out the touched octets and their order from the intersection points

Next steps :
- animated actor
- shotgun that blows holes in walls and in actor

Things learned :
- if statements and loops kill shader performance badly

Performance :
Measured on my ASUS Zenbook UX3405 14 OLED / Intel(R) Core(TM) Ultra 5 125H / Intel Arc Graphics / 1200x800 points resolution
- 4502804 cubes / no lights : 50 fps
- 4502804 cubes / 1 light : 16 fps

How to build :
```
meson setup build
ninja -C build
build/qubatron -v
```

How to create point cloud :

- Download mesh : ![https://sketchfab.com/3d-models/abandoned-house-interior-c2130293962244d0b8e325919b4cf99d]
- Use CloudCompare
- Convert mesh to sample ( 40 million points )
- Edit - Octree - Compute
- Subdivison level : 10
- Problem : CloudCompare doesn't cover mesh perfectly since it uses random points
- Alternative solution : Convert ![obj2voxel](https://github.com/Eisenwave/obj2voxel) to export surface normals

Videos :

- commit 344c25a / 4502804 cubes with 1 light : ![https://www.youtube.com/watch?v=LqytIbcjX18]