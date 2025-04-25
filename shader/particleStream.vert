#version 460

layout (location = 0) in vec3 VertexInitVel;
layout (location = 1) in float VertexBirthTime;

/* Vertex buffer expanded explanation
Imagine it uses uniforms instead. You'd have to do 
for i = 0 -> 8000, particle.render.
That would require you to update the uniform for each particle, then render, there is a dependency of sorts between the CPU and GPU.
Very slow, does cpu then gpu. Gpu is better with lots of data that requires a similar algorithm, this approach does one then the other.

So, in the cpu code it creates buffers for initial velocity and start time.
Buffer for initial vel is enough to fit all 8000 particles x 3 for the vec3 representation.
Start time is just float. So two buffers to hold this info on the GPU.
Runs a loop to create random initial velocities for all particles and puts that in the first buffer.
Then replaces the array used with a list of random birth times.
Then binds the vertex attribute pointers to these locations above.
Instead uses glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 8000) to draw them all
Will render 8000 instances of the vertex geometry, running the vertex shader once for each
But all the information it needs is already held on the GPU in the buffers. 
So it can perform lots of operations without waiting for data to transfer, super efficient.
*/

out float Transp;  //Transparency
out vec2 TexCoord;

uniform float Time;
uniform vec3 Gravity = vec3(0.0, -0.05, 0.0);
uniform float ParticleLifetime;
uniform float ParticleSize = 1.0;
uniform vec3 EmitterPos;

uniform mat4 ModelViewMatrix;
uniform mat4 projection;

const vec3 offsets[] = vec3[](vec3(-0.5, 0.5, 0), vec3(0.5, -0.5, 0), vec3(0.5, 0.5, 0),
                                vec3(-0.5, -0.5, 0), vec3(0.5, 0.5, 0), vec3(-0.5, 0.5, 0));

const vec2 texCoords[] = vec2[](vec2(0, 0),  vec2(1, 0), vec2(1, 1), vec2(0, 0), vec2(1,1), vec2(0, 1));

void main(){
    vec3 viewSpacePos;
    float t = Time - VertexBirthTime;
    if (t >= 0 && t < ParticleLifetime){
        vec3 pos = EmitterPos + VertexInitVel * t * Gravity * t * t;
        viewSpacePos = (ModelViewMatrix * vec4(pos, 1)).xyz + (offsets[gl_VertexID] * ParticleSize);
        Transp = mix(1, 0, t / ParticleLifetime);
    } else {
        viewSpacePos = vec3(0);
        Transp = 0.0;
    }

    TexCoord = texCoords[gl_VertexID];

    gl_Position = projection * vec4(viewSpacePos, 1);
}