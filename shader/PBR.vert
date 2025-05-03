#version 460
//Set up for vertex lighting

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;
layout (location = 2) in vec2 VertexTexCoord;

out vec3 Position;
out vec3 Normal;
out vec3 WorldPosition;
out vec2 TexCoord;
out vec4 ShadowCoord;

uniform mat4 ModelViewMatrix;
uniform mat4 MVP;
uniform mat3 NormalMatrix;
uniform mat4 ShadowMatrix;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    //Multiply by model and view to transform to view space
    Position = vec3(ModelViewMatrix * vec4(VertexPosition, 1.0));    
    //Position = vec3(model * vec4(VertexPosition, 1.0)); 
    WorldPosition = vec3(model * vec4(VertexPosition, 1.0));
   
    TexCoord = VertexTexCoord;

    ShadowCoord = ShadowMatrix * vec4(VertexPosition, 1.0);
    ShadowCoord /= ShadowCoord.w;

    //Normal Matrix is glm::transpose(glm::inverse(glm::mat3(mv))); in code.
    //Model view to become view space
    Normal = normalize(NormalMatrix * VertexNormal);
    gl_Position = MVP * vec4(VertexPosition, 1.0);
}


