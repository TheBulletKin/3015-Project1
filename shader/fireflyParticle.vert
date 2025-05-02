#version 460

const float PI = 3.14159265359;

layout (location = 0) in vec3 VertexPosition;


//Which pass of the shader is currently being ran
uniform int Pass;



out float Transp;  //Transparency
out vec2 TexCoord;

uniform float ParticleSize = 1.0;

uniform mat4 ModelViewMatrix;
uniform mat4 projection;


const vec3 offsets[] = vec3[](
    vec3(-0.5, -0.5, 0), // Triangle 1: Bottom-left
    vec3(0.5, -0.5, 0), // Triangle 1: Bottom-right
    vec3(0.5, 0.5, 0),  // Triangle 1: Top-right
    
    vec3(-0.5, -0.5, 0), // Triangle 2: Bottom-left
    vec3(0.5, 0.5, 0),  // Triangle 2: Top-right
    vec3(-0.5, 0.5, 0) // Triangle 2: Top-left
);

const vec2 texCoords[] = vec2[](
    vec2(0, 0),  // Bottom-left
    vec2(1, 0),  // Bottom-right
    vec2(1, 1),  // Top-right
    vec2(0, 0),  // Bottom-left
    vec2(1, 1),  // Top-right
    vec2(0, 1)   // Top-left
);

void main(){
    //Draw awways instanced with a value of 6 calls this shader 6 times
    //For an instance the instanceID remains the sample
    //But vertexID changes each time one renders
    //So by using the vertex id, the vertices can be offset
    vec3 viewSpacePos = (ModelViewMatrix * vec4(VertexPosition, 1)).xyz + offsets[gl_VertexID] * ParticleSize;
    TexCoord = texCoords[gl_VertexID];
    Transp = 1.0;  // Omit fading for now unless age is added
    gl_Position = projection * vec4(viewSpacePos, 1);
    
}