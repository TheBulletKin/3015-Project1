#version 460

layout (location = 0) out vec4 FragColour;


#define MAX_NUMBER_OF_LIGHTS 6

struct LightInfo{
    vec3 Position;
    vec3 La; //Ambient light intensity
    vec3 Ld; //Diffuse and spec light intensity
};

uniform LightInfo pointLights[MAX_NUMBER_OF_LIGHTS];

struct MaterialInfo {
    vec3 Ka; //Ambient reflectivity
    vec3 Kd; //Diffuse reflectivity
    vec3 Ks; //Specular reflectivity
    float Shininess; //Specular shininess factor
};

uniform MaterialInfo Material;


uniform float EdgeThreshold;
uniform int Pass;
uniform vec3 ViewPos;
uniform bool hdr;
uniform float exposure;
uniform bool horizontal;

uniform float AveLum;
uniform mat3 rgb2xyz = mat3(
    0.4124564, 0.2126729, 0.0193339,
    0.3575761, 0.7151522, 0.1191920,
    0.1804375, 0.0721750, 0.9503041
);

uniform mat3 xyz2rgb = mat3(
    3.2404542, -0.9692660, 0.0556434,
    -1.5371385, 1.8760108, -0.2040259,
    -0.4985314, 0.0415560, 1.0572252
);



uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

const vec3 lum = vec3(0.2126, 0.7152, 0.0722);


vec3 phongModel(int light, vec3 position, vec3 n, vec3 texColour);



vec3 Colour;

//position of fragment in view space
in vec3 Position;
in vec3 WorldPosition;
in vec3 Normal;
in vec2 TexCoord;

uniform mat4 view;



layout(binding = 8) uniform sampler2D HdrTex; //Takes in the hdr texture from last render, then the horizontally blurred texture


void main()
{
    //Solves image blurring with a guassian effect
    vec2 tex_offset = 1.0 / textureSize(HdrTex, 0); // gets size of single texel
    vec3 result = texture(HdrTex, TexCoord).rgb * weight[0]; // current fragment's contribution
    //Looks at neighbouring pixels and multiplies that value by a weight, so the current pixel is influence by those around it
    if(horizontal)
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(HdrTex, TexCoord + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result += texture(HdrTex, TexCoord - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(HdrTex, TexCoord + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
            result += texture(HdrTex, TexCoord - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
        }
    }
    FragColour = vec4(result, 1.0);
} 
