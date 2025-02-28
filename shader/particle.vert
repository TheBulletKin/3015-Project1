#version 460
//Set up for vertex lighting

layout(location = 1) in vec3 InstancePosition; // Light position (instance data)


out vec3 Position;
out vec3 WorldPosition;

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
    
    vec4 worldPos = model * vec4(InstancePosition, 1.0);    

    gl_Position = projection * view * worldPos;

}


