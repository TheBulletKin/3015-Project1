#version 460

layout ( points ) in;
layout ( triangle_strip, max_vertices = 4) out;

uniform float Size2; //half width of the quad

out vec2 TexCoord;
out vec3 FragWorldPos;

uniform mat3 NormalMatrix;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;


void main()
{
    
    vec4 worldPos = projection * view * model * gl_in[0].gl_Position;

     gl_Position = ((vec4(-Size2, -Size2, 0.0, 1.0) + worldPos));
    TexCoord = vec2(0.0, 0.0);
    EmitVertex();

     gl_Position = ((vec4(Size2, -Size2, 0.0, 1.0) + worldPos));
    TexCoord = vec2(1.0, 0.0);
    EmitVertex();

    gl_Position = ((vec4(-Size2, Size2, 0.0, 1.0) + worldPos));
    TexCoord = vec2(0.0, 1.0);
    EmitVertex();

    gl_Position = ((vec4(Size2, Size2, 0.0, 1.0) + worldPos));
    TexCoord = vec2(1.0, 1.0);
    EmitVertex();

    EndPrimitive();
}


