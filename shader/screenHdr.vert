#version 460
//Set up for vertex lighting

layout (location = 0) in vec3 VertexPosition;
layout (location = 2) in vec2 VertexTexCoord;


out vec2 TexCoord;


uniform mat4 ModelViewMatrix;
uniform mat4 MVP;
uniform mat3 NormalMatrix;


uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;



void main()
{
    //VERTEX LIGHTING 
    //Vertex lighting has been removed. But essentially calculations now done on the fragment is shader is done in the vertex shader
    //https://github.com/ruange/Gouraud-Shading-and-Phong-Shading
  
    //Multiply by model and view to transform to view space
    

    TexCoord = VertexTexCoord;
    
    
    gl_Position = MVP * vec4(VertexPosition, 1.0);

}


