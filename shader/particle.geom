#version 460
//Set up for vertex lighting

layout ( points ) in; //Type of primitive to recieve
layout ( triangle_strip, max_vertices = 4) out; //The type of primitive to produce

uniform float Size2; //half width of the quad

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
    
   mat4 m = projection;
   //Vertex 1
   gl_Position = model * (vec4(-Size2, -Size2, 0.0, 0.0) + gl_in[0].gl_Position); //Gl in provides information

   TexCoord = vec2(0.0, 0.0);
   EmitVertex();

   //Vertex 2
   gl_Position = model * (vec4(Size2, -Size2, 0.0, 0.0) + gl_in[0].gl_Position); //Gl in provides information

   TexCoord = vec2(1.0, 0.0);
   EmitVertex();

   //Vertex 3
   gl_Position = model * (vec4(-Size2, Size2, 0.0, 0.0) + gl_in[0].gl_Position); //Gl in provides information

   TexCoord = vec2(0.0, 1.0);
   EmitVertex();

   //Vertex 4
   gl_Position = model * (vec4(Size2, Size2, 0.0, 0.0) + gl_in[0].gl_Position); //Gl in provides information

   TexCoord = vec2(1.0, 1.0);
   EmitVertex();

    EndPrimitive();
}


