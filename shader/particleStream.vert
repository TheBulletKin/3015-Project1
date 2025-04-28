#version 460

const float PI = 3.14159265359;

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexVelocity;
layout (location = 2) in float VertexAge;

//Which pass of the shader is currently being ran
uniform int Pass;

//Output to transform feedback buffers update pass
layout(xfb_buffer = 0, xfb_offset = 0) out vec3 Position;
layout(xfb_buffer = 1, xfb_offset = 0) out vec3 Velocity;
layout(xfb_buffer = 2, xfb_offset = 0) out float Age;

out float Transp;  //Transparency
out vec2 TexCoord;

uniform float Time;  //Time since program start
uniform float DeltaT; //Deltatime between frames
uniform vec3 Accel; //Particle acceleration ( gravity)
uniform float ParticleLifetime;
uniform float ParticleSize = 1.0;
uniform vec3 EmitterPos;
uniform mat3 EmitterBasis;

uniform mat4 ModelViewMatrix;
uniform mat4 projection;

layout(binding = 7) uniform sampler1D RandomTex;

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

//Random texture is a 1D texture of random values, acts sort of like a seed.
vec3 randomInitialVelocity(){
    float theta = mix(0.0, PI / 8.0, texelFetch(RandomTex, 3 * gl_VertexID, 0).r);
    float phi = mix(0.0, 2.0 * PI, texelFetch(RandomTex, 3 * gl_VertexID + 1, 0).r);
    float velocity = mix(1.25, 1.5, texelFetch(RandomTex, 3 * gl_VertexID + 2, 0).r);
    vec3 v = vec3(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));
    //Emitter basis is the provided direction. This whole method gives it some variance
    return normalize(EmitterBasis * v) * velocity;
}

void update(){
    if (VertexAge < 0 || VertexAge > ParticleLifetime){
        //Hasn't spawned yet or is dead
        Position = EmitterPos;
        Velocity = randomInitialVelocity();
        if (VertexAge < 0)
        {
            Age = VertexAge + DeltaT; //Effective spawns it
        } else
        {
            Age = -0.1f;
        }
    } else
    {
        //Particle is alive
        Position = VertexPosition + VertexVelocity * DeltaT; //Use velocity to alter position
        Velocity = VertexVelocity + Accel * DeltaT; //Use accel to alter velocity
        Age = VertexAge + DeltaT; //Increment age over time
    }
}

void render(){
    Transp = 0.0;
    vec3 viewSpacePos = vec3(0.0);
    if (VertexAge >= 0.0){
        viewSpacePos = (ModelViewMatrix * vec4(VertexPosition,1)).xyz + offsets[gl_VertexID] * ParticleSize;
        Transp = clamp(1.0 - VertexAge / ParticleLifetime, 0, 1);
    }
    TexCoord = texCoords[gl_VertexID];

    gl_Position = projection * vec4(viewSpacePos, 1);
}



void main(){
    if (Pass == 1)
    {
        update();
    } else {
        render();
    }
}