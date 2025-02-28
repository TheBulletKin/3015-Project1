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





const vec3 lum = vec3(0.2126, 0.7152, 0.0722);


vec3 phongModel(int light, vec3 position, vec3 n, vec3 texColour);



vec3 Colour;

//position of fragment in view space
in vec3 Position;
in vec3 WorldPosition;
in vec3 Normal;
in vec2 TexCoord;

uniform mat4 view;



layout(binding = 8) uniform sampler2D HdrTex;



void main()
{
    const float gamma = 1.2;
    vec3 hdrColor = texture(HdrTex, TexCoord).rgb;
    
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
