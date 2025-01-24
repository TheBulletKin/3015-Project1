#version 460

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexColor;

out vec3 Color;
uniform mat4 RotationMatrix;

void main()
{
    Color = VertexColor;
    vec4 OutputPos = vec4(VertexPosition, 1.0) * RotationMatrix;
    gl_Position = OutputPos;
}
