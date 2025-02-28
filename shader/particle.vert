#version 460
//Set up for vertex lighting

layout (location = 0) in vec3 VertexPosition;


out vec3 Position;
out vec3 WorldPosition;
out vec3 Normal;
out vec2 TexCoord;

vec3 LightDir;
vec3 ViewDir;
vec3 FragPos;

uniform mat4 ModelViewMatrix;
uniform mat4 MVP;
uniform mat3 NormalMatrix;


uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;



void main()
{
    
    gl_Position = MVP * vec4(VertexPosition, 1.0);

}


