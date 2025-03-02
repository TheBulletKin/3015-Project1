#version 460

layout (location = 0) out vec4 FragColour;

#define MAX_NUMBER_OF_LIGHTS 13

struct LightInfo{
    vec3 Position;
    vec3 La; //Ambient light intensity
    vec3 Ld; //Diffuse and spec light intensity
    float Constant;  // Attenuation constant
    float Linear;    // Attenuation linear factor
    float Quadratic; // Attenuation quadratic factor
    bool Enabled;
};

uniform LightInfo pointLights[MAX_NUMBER_OF_LIGHTS];

struct DirectionalLight {
    vec3 Direction; // Direction the light is shining
    vec3 La;  // Ambient light intensity
    vec3 Ld;  // Diffuse light intensity
    vec3 Ls;  // Specular light intensity
    bool Enabled;
};

uniform DirectionalLight directionalLight;

struct MaterialInfo {
    vec3 Ka; //Ambient reflectivity
    vec3 Kd; //Diffuse reflectivity
    vec3 Ks; //Specular reflectivity
    float Shininess; //Specular shininess factor
};

uniform MaterialInfo Material;

uniform int staticPointLights = 3;
uniform int dynamicPointLights = 0;
uniform float time;
uniform vec3 ViewPos;
uniform mat4 view;

uniform float FogStart;
uniform float FogEnd;
uniform vec3 FogColour;

uniform float TextureScale = 20.0;


//position of fragment in view space
in vec3 Position;
in vec3 WorldPosition;
in vec3 Normal;
in vec2 TexCoord;
in vec3 WorldNormal;


layout(binding = 0) uniform sampler2D GrassTex;
layout(binding = 1) uniform sampler2D CliffTex;
layout(binding = 4) uniform sampler2D CloudTex;

vec3 phongModel(int light, vec3 position, vec3 n, vec3 texColour);
vec3 directionalLightModel(vec3 n, vec3 texColour);



void main() {
     float noiseScale = 0.002f;
    float speed = 0.5f;    

    float animatedX = time * speed + sin(time * 0.1f) * 0.1f;
    float animatedY = time * speed + cos(time * 0.12f) * 0.1f;
   
    vec2 animatedCoord = WorldPosition.xz * noiseScale + vec2(animatedX, animatedY);
    float noise = texture(CloudTex, animatedCoord).r;  


    float shadow = smoothstep(0.0, 0.05f, noise); 
    shadow = mix(shadow, 1.0, 0.35);
    

    vec4 TexColour = texture(GrassTex, TexCoord * TextureScale);

    //Triplanar texture mapping
    /* Start by determining the blend value. Essentially the fragment's normal colour
    
    */
    vec3 blending = abs(WorldNormal);
    blending = normalize(pow(blending, vec3(1.2))); // Sharpen blending
    
    //Create texture coordinate positions based on the fragment's world position
    vec2 xzProj = WorldPosition.xz;
    vec2 xyProj = WorldPosition.xy;
    vec2 yzProj = WorldPosition.yz;   

    vec3 texXZ = texture(GrassTex, xzProj).rgb; // Top-down
    vec3 texXY = texture(CliffTex, xyProj * TextureScale).rgb; // Side projection
    vec3 texYZ = texture(CliffTex, yzProj * TextureScale).rgb; // Side projection
    vec3 triplanarTex = texXZ * blending.y + texXY * blending.z + texYZ * blending.x;

    //WorldNormal.y at 1 means vertical, 0 means horizontal. 
    //If WorldNormal.y < 0.9, slopeFactor is 0, between 0.9 and 1.0 it smoothly blends from 0 to 1, and when WorldNormal.y > 1.0, slopeFactor is 1
    float slopeFactor = smoothstep(0.9, 1.0, abs(WorldNormal.y));     

    //Scale the textures
    vec3 grassTex = texture(GrassTex, TexCoord * TextureScale).rgb;
    vec3 rockTex = texture(CliffTex, TexCoord * TextureScale).rgb;

    //Mix the textures based off the slope factor determined earlier
    vec3 blendedTex = mix(rockTex, grassTex, slopeFactor);



    vec3 adjustedNormal = Normal;
    if (!gl_FrontFacing) {
        adjustedNormal = -Normal;
    }
   // vec3 n = normalize(adjustedNormal);
    vec3 colour = vec3(0.0);    
    for (int i = 0; i < MAX_NUMBER_OF_LIGHTS; i++)
    {        
        colour += phongModel(i, Position, adjustedNormal, blendedTex.rgb);
    }   

    if (directionalLight.Enabled) {
        colour += directionalLightModel(adjustedNormal, blendedTex.rgb);
    }
    
    vec3 finalColour = mix(colour, colour * shadow, 0.9);

    float distance = length(Position - ViewPos);    
    // Linear fog factor (clamped between 0 and 1)
    float fogFactor = clamp((FogEnd - distance) / (FogEnd - FogStart), 0.0, 1.0);


    finalColour = mix(FogColour, finalColour, fogFactor);

     
    FragColour = vec4(finalColour, 1.0); 
    
   
}

vec3 phongModel(int light, vec3 position, vec3 n, vec3 texColour){
    
    if (!pointLights[light].Enabled) {
        return vec3(0.0f);
    }

    if (pointLights[light].Position == vec3(0.0f, 0.0f, 0.0f)) {        
        return vec3(0.0f);
    }

    
    
    vec3 lightPosView = vec3(view * vec4(pointLights[light].Position, 1.0));
    vec3 LightDir = normalize(lightPosView - position);
    vec3 viewPosView = vec3(view * vec4(ViewPos, 1.0));
   
    float distance = length(lightPosView - position);

    //Attenuation formula
    float attenuation = 1.0 / (pointLights[light].Constant +
                               pointLights[light].Linear * distance +
                               pointLights[light].Quadratic * (distance * distance));

    //Ambient
    vec3 AmbientLight = Material.Ka * pointLights[light].La * texColour * attenuation;

    //Diffuse
    float sDotN = max(dot(n, LightDir), 0.0);
    vec3 DiffuseLight = Material.Kd * sDotN * pointLights[light].Ld * texColour * attenuation;

    //View dir is usually cameraPos - FragPos. Camera pos is 0, so this becomes - pos
    vec3 viewDir = normalize((viewPosView) - position);
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

    vec3 SpecularLight = Material.Ks * pointLights[light].Ld * specular * attenuation;

    return AmbientLight + DiffuseLight + SpecularLight;

    
}

vec3 directionalLightModel(vec3 n, vec3 texColour) {
    
    vec3 LightDir = normalize(-vec3((view * vec4(directionalLight.Direction, 1.0)))); // Ensure it's pointing towards the surface

    // Ambient
    vec3 AmbientLight = Material.Ka * directionalLight.La * texColour;

    // Diffuse
    float sDotN = max(dot(n, LightDir), 0.0);
    vec3 DiffuseLight = Material.Kd * sDotN * directionalLight.Ld * texColour;

    vec3 viewPosView = vec3(view * vec4(ViewPos, 1.0));
    // Specular
    vec3 viewDir = normalize(viewPosView - Position);
    vec3 reflectDir = reflect(-LightDir, n);
    float specular = pow(max(dot(viewDir, reflectDir), 0.0), Material.Shininess);
    vec3 SpecularLight = Material.Ks * directionalLight.Ls * specular;

    return AmbientLight + DiffuseLight + SpecularLight;
}



