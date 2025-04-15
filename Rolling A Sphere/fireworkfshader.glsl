/* 
File Name: "fireworkfshader.glsl":
Fragment Shader
*/

#version 150  

in  vec4 pos;
in  vec4 color;

out vec4 fColor;

void main() 
{ 
   if(pos.y < 0.1)
	discard;

   fColor = color;
} 