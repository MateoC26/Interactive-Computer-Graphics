/* 
File Name: "fireworkvshader.glsl":
Vertex Shader
*/

#version 150 

in  vec3 vVelocity;
in  vec4 vColor;

uniform mat4 model_view;
uniform mat4 projection;
uniform float Time;

out vec4 pos;
out vec4 color;

void main()
{
	float a = -0.00000049;

	float x = 0.0 + (0.001 * vVelocity.x * Time);
	float y = 0.1 + (0.001 * vVelocity.y * Time) + (0.5 * a * Time * Time);
	float z = 0.0 + (0.001 * vVelocity.z * Time);

	pos = vec4(x, y, z, 1.0);

	gl_Position = projection * model_view * pos;

	color = vColor;
}