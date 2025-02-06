#version 460
//Set up for vertex lighting

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;
//layout (location = 1) in vec3 VertexColor;




out vec3 Position;
out vec3 Normal;



vec3 LightDir;
vec3 ViewDir;
vec3 FragPos;

uniform mat4 ModelViewMatrix;
uniform mat4 MVP;
uniform mat3 NormalMatrix;
uniform vec3 ViewPos;



void main()
{
    //VERTEX LIGHTING 
    //Vertex lighting has been removed. But essentially calculations now done on the fragment is shader is done in the vertex shader
    //https://github.com/ruange/Gouraud-Shading-and-Phong-Shading
  
    Position = vec3(ModelViewMatrix) * VertexPosition;
    Normal = normalize(NormalMatrix * VertexNormal);
    gl_Position = MVP * vec4(VertexPosition, 1.0);

}


