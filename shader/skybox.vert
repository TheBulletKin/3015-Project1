#version 460

layout (location = 0) in vec3 VertexPosition;

out vec3 Position;


uniform mat4 MVP;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    Position = VertexPosition;
    
    gl_Position = MVP * vec4(VertexPosition, 1.0);
}