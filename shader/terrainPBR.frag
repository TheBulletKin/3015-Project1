#version 460

layout (location = 0) out vec4 FragColour;

const float PI = 3.14159265358979323846;

in vec3 Position;
in vec3 WorldPosition;
in vec3 Normal;
in vec2 TexCoord;
in vec3 WorldNormal;

uniform mat4 view;
uniform vec3 ViewPos;
uniform float TextureScale = 20.0;
uniform int Pass;
uniform float time;
uniform int DirLightIndex;
in vec4 ShadowCoord;

uniform vec3 fogColour;
uniform float fogStart;
uniform float fogEnd;

struct LightInfo {
    vec4 Position;
    vec3 Intensity;
    vec3 Ambient;
}; 

uniform int numberOfFireflies = 0;
uniform int numberOfTorches = 9;
uniform LightInfo FireflyLight[15];
uniform LightInfo Light[10]; //Specifically for torch lights
uniform LightInfo DirLight;

int x = 0;
int y = 0;



struct MaterialInfo{
    float Rough;
    bool Metal;
    vec3 Colour;
};

uniform MaterialInfo material;

layout(binding = 0) uniform sampler2D MainTex;
layout(binding = 1) uniform sampler2D CliffTex;
layout(binding = 3) uniform sampler2D CloudTex;
layout(binding = 8) uniform sampler2DShadow ShadowMap;

//PBR works around using roughness, metalness and colour

//alpha is the surface roughness. The function works better when using a squared value
//H is the halfway vector between the light direction and the view direction. N is normal.
//So, if nDotH is high, it means the light is being reflected into the viewer's perspective from the fragment's normal
//If surface microfacets are mostly aligned it means the surface roughness is low. GGX returns high.
//Acts as a normal distribution, creating that long falloff effect on the surface
float ggxDistribution(float nDotH){
    float alpha2 = material.Rough * material.Rough * material.Rough * material.Rough;
    float d = (nDotH * nDotH) * (alpha2 - 1) + 1;
    return alpha2 / (PI * d * d);
}

//Geometry function
//Surfaces with bump can block light from reaching parts of the surface behind it.
//This calculates that effect
float geomSmith(float dotProd){
    float k = (material.Rough + 1.0) * (material.Rough + 1.0) / 8.0;
    float denom = dotProd * (1 - k) + k;
    return 1.0 / denom;
}

//Fresnel is the 'glow' around the edges of surfaces
vec3 schlickFresnel(float lDotH) {
    vec3 f0 = vec3(0.04);
    if (material.Metal){
        f0 = material.Colour;
    }
    return f0 + (1 - f0) * pow(1.0 - lDotH, 5);
}

//Computes the final light reflection for a point on a surface
vec3 microfacetModel(int lightIdx, vec3 position, vec3 n, vec3 baseColour, int type){
    //L is light direction.
    vec3 l = vec3(0.0);
    vec3 lightI = vec3(0.0);
    vec4 lightPosition = vec4(0.0);
    if (lightIdx == -1) { //Is directional
        lightI = DirLight.Intensity;        
        lightPosition = view * DirLight.Position;
    } else if (type == 0) {
        lightI = Light[lightIdx].Intensity;
        //vec4 lightPosition = Light[lightIdx].Position;
        lightPosition = view * Light[lightIdx].Position;
    } else if (type == 1) {
        lightI = FireflyLight[lightIdx].Intensity;
        //vec4 lightPosition = FireflyLight[lightIdx].Position;
        lightPosition = view * FireflyLight[lightIdx].Position;
    }
    
    if(lightPosition.w == 0.0) {  //W = 0 for directional lights
        l = normalize(lightPosition.xyz);
    }
    else {
        //Light direction is the vector of the fragment position to the light source
        l = lightPosition.xyz - position;
        float dist = length(l);
        l = normalize(l);
        lightI /= ((dist * dist) * 0.2f);
    }

    //Position is the world space position of the fragment. V is the direction from that point to the camera.
    //Since the vertex shader uses ModelView matrix, it's in view space, camera origin is 0,0,0
    vec3 v = normalize(0 - position);
    //Halfway vector. Adding the light direction vector to the vector of the fragment to the camera, normalising gives the vector inbetween
    vec3 h = normalize(v + l);

    //Surface normal and halfway vector (direction of light from fragment and view position to fragment)
    //Essentially how close the normal is to the angle of reflection
    float nDotH = dot(n ,h);
    //Light direction to that halfway vector, for fresnel
    float lDotH = dot(l, h);
    //Normal to the light direction, to find how much light actually hits the surface
    float nDotL = max(dot(n, l), 0);
    //Essentially how aligned the camera is with the surface normal
    float nDotV = dot(n, v);

    //Specular value of surface
    vec3 specBrdf = 0.25 * ggxDistribution(nDotH) * schlickFresnel(lDotH) * geomSmith(nDotL) * geomSmith(nDotV);

    return (baseColour + PI * specBrdf) * lightI * nDotL;
}

void depthPass()
{
    //debug
    float depth = ShadowCoord.z;  
    FragColour = vec4(depth, depth, depth, 1.0);
}





vec3 determineShadow(vec3 sum){
    float pcfSum = 0;
    float shadow = 1.0;

    

    if (ShadowCoord.z >= 0){
        //Anti alias shadows
        pcfSum += textureProjOffset(ShadowMap, ShadowCoord, ivec2(-1, -1));
        pcfSum += textureProjOffset(ShadowMap, ShadowCoord, ivec2( 0, -1));
        pcfSum += textureProjOffset(ShadowMap, ShadowCoord, ivec2( 1, -1));
        pcfSum += textureProjOffset(ShadowMap, ShadowCoord, ivec2(-1,  0));
        pcfSum += textureProjOffset(ShadowMap, ShadowCoord, ivec2( 0,  0));
        pcfSum += textureProjOffset(ShadowMap, ShadowCoord, ivec2( 1,  0));
        pcfSum += textureProjOffset(ShadowMap, ShadowCoord, ivec2(-1,  1));
        pcfSum += textureProjOffset(ShadowMap, ShadowCoord, ivec2( 0,  1));
        pcfSum += textureProjOffset(ShadowMap, ShadowCoord, ivec2( 1,  1));       
        
        shadow = pcfSum / 9.0;
    }

    //FragColour = vec4(Light[3].Ambient + sum * shadow, 1.0);
    //vec3 ambient = Light[3].Ambient * 0.1;
    
    vec3 lit = sum * shadow;

    return pow(lit, vec3(1.0 / 2.2)); //Gamma correction   
}

float calculateFogFactor(float distance){   
    return clamp((distance - fogStart) / (fogEnd - fogStart), 0.0, 1.0);
}

void renderPass()
{
    vec3 sum = vec3(0.0);
    vec3 n = normalize(Normal);   

    // ----- Cloud shadows
    float noiseScale = 0.002f;
    float speed = 0.5f;    

    //Change the target coordinate to sample on the texture to move with time
    float animatedX = time * speed + sin(time * 0.1f) * 0.1f;
    float animatedY = time * speed + cos(time * 0.12f) * 0.1f;
   
    //Use the world position to determine the texture sample position to ensure uniformity with scene models
    vec2 animatedCoord = WorldPosition.xz * noiseScale + vec2(animatedX, animatedY);
    float noise = texture(CloudTex, animatedCoord).r;  

    //Smooth gradient from 0 to shadow, and overall darkening of the shadow
    float shadow = smoothstep(0.0, 0.05f, noise); 
    shadow = mix(shadow, 1.0, 0.8);     
    
    // -------- Slope texture blending
    /* The fragment's normal is essentially representing a direction by it's colour
    * Therefore the y component of worldNormal is high for flat surfaces, low for vertical ones 
    * Slopefactor is a 0 - 1 value that describes the weighting of that blend between grass and cliff
    * Determines the final texture by this blend value
    */
   
    //If WorldNormal.y < 0.9, slopeFactor is 0, between 0.9 and 1.0 it smoothly blends from 0 to 1, and when WorldNormal.y > 1.0, slopeFactor is 1
    float slopeFactor = smoothstep(0.85, 0.9, abs(WorldNormal.y));     

    //------ Base texture sampling
    vec3 grassTex = texture(MainTex, TexCoord * TextureScale).rgb;
    grassTex = pow(grassTex, vec3(1.0 / 0.4)); // gamma correction
    vec3 rockTex = texture(CliffTex, TexCoord * TextureScale).rgb;
    rockTex = pow(rockTex, vec3(1.0 / 0.6)); // gamma correction
    vec3 blendedTex = mix(rockTex, grassTex, slopeFactor);


    vec3 baseColour = vec3(0.0);
    if (!material.Metal) {
        baseColour = blendedTex;
    }

    vec3 adjustedNormal = Normal;
    if (!gl_FrontFacing) {
        //adjustedNormal = -Normal;
    }
    
    //-----Direction light and shadow
    vec3 dirLight = microfacetModel(-1, Position, n, baseColour, 0);
    vec3 dirLit = determineShadow(dirLight); // already includes shadow mapping
    //dirLit = mix(dirLit, dirLit * shadow, 0.9);

    //----Torch lights
    vec3 torchLit = vec3(0.0);
    for (int i = 0; i < numberOfTorches; i++){
       torchLit += microfacetModel(i, Position, n, baseColour, 0);
    }

    //-----Firefly lights
    vec3 flireflyLit = vec3(0.0);
    for (int i = 0; i < numberOfFireflies; i++){
        flireflyLit += microfacetModel(i, Position, n, baseColour, 1);
    }

    //Combined light output to fix issue present with old mutiight soution
    vec3 lit = dirLit + torchLit + flireflyLit;
   
    lit += DirLight.Ambient;    

    float distanceToCamera = length(WorldPosition - ViewPos);   
    float fogFactor = calculateFogFactor(distanceToCamera);

    vec3 cloudShadowedColour = mix(lit, lit * shadow, 0.95);
    vec3 finalColour = mix(cloudShadowedColour, fogColour, fogFactor);
    
    FragColour = vec4(finalColour, 1.0);
   // FragColour = vec4(lit, 1.0);
    //FragColour = pow(FragColour, vec4(1.0 / 0.4)); // gamma correction
}


void main(){   
    if (Pass == 1){
        depthPass();
    } else if (Pass == 2){
        renderPass();
    }
}






