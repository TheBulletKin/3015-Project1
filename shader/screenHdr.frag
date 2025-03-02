#version 460

layout (location = 0) out vec4 FragColour;

uniform bool hdr;
uniform float exposure;

vec3 Colour;

in vec3 Position;
in vec2 TexCoord;

uniform mat4 view;

layout(binding = 8) uniform sampler2D RenderTex;

vec3 phongModel(int light, vec3 position, vec3 n, vec3 texColour);
void main()
{
    const float gamma = 1.0;
    vec3 hdrColor = texture(RenderTex, TexCoord).rgb;
    
    //FragColour = vec4(hdrColor, 1.0); // Output raw HDR color without tone mapping
    
    if(hdr)
    {
        // reinhard
        //vec3 result = hdrColor / (hdrColor + vec3(1.0));
        // exposure
        vec3 result = vec3(1.0) - exp(-hdrColor * exposure);
        // also gamma correct while we're at it       
        result = pow(result, vec3(1.0 / gamma));
        FragColour = vec4(result, 1.0);
    }
    else
    {
        vec3 result = pow(hdrColor, vec3(1.0 / gamma));
        FragColour = vec4(result, 1.0);
    }
} 
