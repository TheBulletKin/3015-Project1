#version 460

layout (location = 0) out vec4 FragColour;
layout (location = 1) out vec4 BrightColour;

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

uniform float Exposure = 0.35;
uniform float White = 0.928;
uniform bool DoToneMap = true;
uniform float FogStart;
uniform float FogEnd;
uniform vec3 FogColour;
uniform float LumThresh;
uniform float PixOffset[10] = float[](0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0);
uniform float Weight[10];



const vec3 lum = vec3(0.2126, 0.7152, 0.0722);


vec3 phongModel(int light, vec3 position, vec3 n, vec3 texColour);



vec3 Colour;

//position of fragment in view space
in vec3 Position;
in vec3 WorldPosition;
in vec3 Normal;
in vec2 TexCoord;

uniform mat4 view;


layout(binding = 0) uniform sampler2D BrickTex;
layout(binding = 1) uniform sampler2D MossTex;
layout(binding = 6) uniform sampler2D RenderTex;
layout(binding = 7) uniform sampler2D IntermediateTex;
layout(binding = 8) uniform sampler2D HdrTex;
layout(binding = 9) uniform sampler2D BlurTex1;
layout(binding = 10) uniform sampler2D BlurTex2;


void main() {
    
    vec3 adjustedNormal = Normal;
    if (!gl_FrontFacing) {
        adjustedNormal = -Normal;
    }
    //vec3 n = normalize(adjustedNormal);
    vec3 colour = vec3(0.0);    
    for (int i = 0; i < MAX_NUMBER_OF_LIGHTS; i++)
    {        
        colour += phongModel(i, Position, adjustedNormal, texture(MossTex, TexCoord).rgb);
    }

    //First calculate frag colour. Layout specified means this is sent to colour attachment 0
    FragColour = vec4(colour, 1.0);

    //Then calculate brightness. Pixels that are bright are saved to colour attachment 1 texture, non bright are discarded
    float brightness = dot(FragColour.rgb, vec3(0.2126, 0.7152, 0.7222));
    if (brightness > 1.0){
        BrightColour = vec4(FragColour.rgb, 1.0);
    } else {
        BrightColour = vec4(0.0, 0.0, 0.0, 1.0);
    }
    
   

   
}

vec3 phongModel(int light, vec3 position, vec3 n, vec3 texColour){
    
    if (pointLights[light].Position == vec3(0.0f, 0.0f, 0.0f)) {        
        return vec3(0.0f);
    }

    vec3 AmbientLight = Material.Ka * pointLights[light].La;
    
    vec3 lightPosView = vec3(view * vec4(pointLights[light].Position, 1.0));
    vec3 LightDir = normalize(lightPosView - position);

    
    float sDotN = max(dot(n, LightDir), 0.0);
    vec3 DiffuseLight = Material.Kd * sDotN;

    //View dir is usually cameraPos - FragPos. Camera pos is 0, so this becomes - pos
    vec3 viewDir = normalize(ViewPos - position);
    vec3 reflectDir = reflect(-LightDir, n);

    //Specular
    //The harsh light you see when the reflection of the light source reaches the camera or eyes 
    //Therefore specular needs:
    //  The vector (v) from the vertex / fragment to the view position
    //  The normal (n) to the plane 
    //  The vector FROM the light source to the vertex / fragment (-s)
    //  The reflected (r) vector of that light source to the vertex / fragment position (r=-s + 2(s.n)n)
    //Equation for specular is then Ks * Ls * (r.v)^f
    //F is a power coefficient, controlling the falloff value so when you move the eye away from that reflected vector how bright is it
    float specular = pow(max(dot(viewDir, reflectDir), 0.0), Material.Shininess);

    vec3 SpecularLight = Material.Ks * pointLights[light].Ld * specular;

    return AmbientLight + DiffuseLight + SpecularLight;
}



