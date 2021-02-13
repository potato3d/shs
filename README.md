# shs
Source code for the article [Solid Height-map Sets: modeling and visualization](https://dl.acm.org/doi/abs/10.1145/1364901.1364953)

Height maps are a very efficient surface representation, initially developed for terrain modeling and visualization. They are also present in other applications, such as mesostructure rendering. However, height maps are incapable of representing overhangs or self-folding surfaces, as well as several occluding objects. In this paper we propose a novel representation to overcome these limitations. A Solid Height-map Set is used to represent arbitrary solid geometry. We also describe a procedure to convert polygonal meshes into our scheme. In addition, we develop a visualization algorithm capable of efficiently rendering this novel representation and implement it using GPU programming. Results achieve an order of magnitude in memory savings as well as high performance independent of the original mesh size.

# Description

SHS construction is implemented in src/LayerGenerator.* together with the shaders directory.

SHS rendering is implemented in src/Canvas.* together with the shaders directory.

Please check the paper reference above for further implementation details.

# Results

These are some images generated using our algorithm:

![fig3](https://github.com/potato3d/shs/blob/main/imgs/fig3.png "Final results")
![fig2](https://github.com/potato3d/shs/blob/main/imgs/fig2.png "Spheres example")
![fig1](https://github.com/potato3d/shs/blob/main/imgs/fig1.png "Dragon example")

# Build

Visual Studio project files included in visualstudio directory.