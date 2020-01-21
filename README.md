# NPR_skecth 
Basic C++ OpenGL and python script for sketch generation from mesh.    
  
It's a naive Non-Photorealistic Rendering algorithm which combines the edges of depth map and normal map. The procedure is described in the outline generation section at p.69 of https://www.mrl.nyu.edu/publications/npr-course1999/npr99.pdf. This project can be used as a beginner tutorial of OpenGL. It contains three shaders that output normal map, depth map and wireframe mode of the rendered mesh. It has complete functions for entire rendering process that one can easily obtain a sketch from any angle. Off and Obj mesh files are supported. Multi-process processing script for Shapenet will be published. 
  
Edge(<img src="https://github.com/WordBearerYI/NPR_skecth/blob/master/images/normal.gif" width="140" height="140" />) U
Edge(<img src="https://github.com/WordBearerYI/NPR_skecth/blob/master/images/depth.gif" width="140" height="140" />) =
(<img src="https://github.com/WordBearerYI/NPR_skecth/blob/master/images/sketch.gif" width="140" height="140" />) 
  
Execute run.sh in root directory of this repo. 
`./run.sh PATH_TO_DATA MESH_NAME`
