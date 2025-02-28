#version 460

layout (location = 0) out vec4 FragColour;






uniform int Pass;

uniform bool hdr;
uniform float exposure;
uniform bool bloom;





vec3 Colour;

//position of fragment in view space
in vec3 Position;
in vec3 WorldPosition;
in vec3 Normal;
in vec2 TexCoord;

uniform mat4 view;



layout(binding = 8) uniform sampler2D HdrTex; //Base scene texture from first draw
layout(binding = 10) uniform sampler2D BlurTex; //Texture with blurred bright areas


void main()
{

    const float gamma = 1.2;
    vec3 hdrColour = texture(HdrTex, TexCoord).rgb;
    vec3 bloomColour = texture(BlurTex, TexCoord).rgb;
    hdrColour += bloomColour;
    
   if(bloom)
        hdrColour += bloomColour; // additive blending
    // tone mapping
    vec3 result = vec3(1.0) - exp(-hdrColour * exposure);
    // also gamma correct while we're at it       
    result = pow(result, vec3(1.0 / gamma));
    FragColour = vec4(result, 1.0);
} 
