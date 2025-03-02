#version 460

layout (location = 0) out vec4 FragColour;

#define MAX_NUMBER_OF_LIGHTS 10

struct LightInfo{
    vec3 Position;
    vec3 La; //Ambient light intensity
    vec3 Ld; //Diffuse and spec light intensity
    //Attenuation
    float Constant;
    float Linear;
    float Quadratic; 
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

uniform float Exposure = 0.35;
uniform float FogStart;
uniform float FogEnd;
uniform vec3 FogColour;
uniform vec3 ViewPos;
uniform mat4 view;
uniform float TextureScale = 20.0;

vec3 Colour;

//position of fragment in view space
in vec3 Position;
in vec3 WorldPosition;
//position of fragment in world space
in vec3 Normal;
in vec2 TexCoord;

layout(binding = 1) uniform sampler2D BrickTex;

vec3 phongModel(int light, vec3 position, vec3 n, vec3 texColour);
vec3 directionalLightModel(vec3 n, vec3 texColour);

void main() {
    
    //Fix normals
    vec3 adjustedNormal = Normal;
    if (!gl_FrontFacing) {
        adjustedNormal = -Normal;
    }
    
    //Rotate 90 degrees clockwise
    mat2 rotationMatrix = mat2(0.0, -1.0, 1.0, 0.0);    
    vec2 rotatedTexCoord = rotationMatrix * TexCoord;

    vec3 textureColor = texture(BrickTex, rotatedTexCoord * TextureScale).rgb;
    
    vec3 colour = vec3(0.0);    
    for (int i = 0; i < MAX_NUMBER_OF_LIGHTS; i++)
    {        
        colour += phongModel(i, Position, adjustedNormal, textureColor.rgb);
    }

    if (directionalLight.Enabled) {
        colour += directionalLightModel(adjustedNormal, textureColor);
    }
    
    FragColour = vec4(colour, 1.0);    
}

vec3 phongModel(int light, vec3 position, vec3 n, vec3 texColour){
    
    //Two checks to make sure a disabled light isn't rendered
    if (!pointLights[light].Enabled) {
        return vec3(0.0f);
    }

    if (pointLights[light].Position == vec3(0.0f, 0.0f, 0.0f)) {        
        return vec3(0.0f);
    }   
    
    vec3 lightPosView = vec3(view * vec4(pointLights[light].Position, 1.0)); //Convert light position to view space
    vec3 LightDir = normalize(lightPosView - position); //Get light direction in view space
    vec3 viewPosView = vec3(view * vec4(ViewPos, 1.0)); //Turn view pos from world to view space
   
    float distance = length(lightPosView - position); //Distance as view space light post to view space fragment position

    //Attenuation formula
    float attenuation = 1.0 / (pointLights[light].Constant + pointLights[light].Linear * distance +
                                pointLights[light].Quadratic * (distance * distance));

    //Ambient
    vec3 AmbientLight = Material.Ka * pointLights[light].La * texColour * attenuation;

    //Diffuse
    float sDotN = max(dot(n, LightDir), 0.0);
    vec3 DiffuseLight = Material.Kd * sDotN * pointLights[light].Ld * texColour * attenuation;
  
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



