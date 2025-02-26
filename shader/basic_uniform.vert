#version 460
//Set up for vertex lighting

layout (location = 0) in vec3 VertexPosition;


out vec3 Position;
out vec3 Normal;
out vec2 TexCoord;
out vec3 Vec;

vec3 LightDir;
vec3 ViewDir;
vec3 FragPos;

uniform mat4 MVP;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    Vec = VertexPosition;
    gl_Position = MVP * vec4(VertexPosition, 1.0);

}


