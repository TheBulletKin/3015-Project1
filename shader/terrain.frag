#version 460

layout (location = 0) out vec4 FragColour;

#define MAX_NUMBER_OF_LIGHTS 6

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




//position of fragment in view space
in vec3 Position;
in vec3 WorldPosition;
in vec3 Normal;
in vec2 TexCoord;


layout(binding = 0) uniform sampler2D BaseTex;
layout(binding = 4) uniform sampler2D CloudTex;

vec3 phongModel(int light, vec3 position, vec3 n, vec3 texColour);



void main() {
     float noiseScale = 0.002f;
    float speed = 0.5f;    

    float animatedX = time * speed + sin(time * 0.1f) * 0.1f;
    float animatedY = time * speed + cos(time * 0.12f) * 0.1f;
   
    vec2 animatedCoord = WorldPosition.xz * noiseScale + vec2(animatedX, animatedY);
    float noise = texture(CloudTex, animatedCoord).r;  


    float shadow = smoothstep(0.0, 0.05f, noise); 
    shadow = mix(shadow, 1.0, 0.6);
    

    vec4 TexColour = texture(BaseTex, TexCoord);


    vec3 adjustedNormal = Normal;
    if (!gl_FrontFacing) {
        adjustedNormal = -Normal;
    }
    //vec3 n = normalize(adjustedNormal);
    vec3 colour = vec3(0.0);    
    for (int i = 0; i < MAX_NUMBER_OF_LIGHTS; i++)
    {        
        colour += phongModel(i, Position, adjustedNormal, TexColour.rgb);
    }   
    
    //FragColour = vec4(TexColour.rgb, 1.0);
    //FragColour = vec4(colour.rgb, 1.0);
    FragColour = vec4(clamp(colour * shadow, 0.0, 1.0), 1.0);   
    //FragColour = vec4(texture(CloudTex, TexCoord)); 
   
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

    vec3 SpecularLight = Material.Ks * pointLights[light].Ld * specular * attenuation;

    return AmbientLight + DiffuseLight + SpecularLight;

    
}



