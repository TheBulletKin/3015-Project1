#version 460
layout(location = 1) in vec3 InstancePosition; // Light position (instance data)

out vec3 Position;

uniform mat4 ModelViewMatrix;
uniform mat4 MVP;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{    
    vec4 worldPos = model * vec4(InstancePosition, 1.0);  
   
    gl_Position = vec4(InstancePosition, 1.0);
}


