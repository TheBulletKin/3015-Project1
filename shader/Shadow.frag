#version 460

layout (location = 0) out vec4 FragColour;


in vec3 WorldPosition;

in vec2 TexCoord;

struct LightInfo {
    vec4 Position;
    vec3 Intensity;
};

uniform LightInfo light;

struct MaterialInfo {
    vec3 Ka;
    vec3 Kd;
    vec3 Ks;
    float Shininess;
};

uniform MaterialInfo material;

layout(binding = 8) uniform sampler2DShadow ShadowMap;

in vec3 Position;
in vec3 Normal;
in vec4 ShadowCoord;

vec3 phongModelDiffAndSpec(){
    vec3 n = Normal;
    vec3 s = normalize(vec3(light.Position) - Position);
    vec3 v = normalize(-Position.xyz);
    vec3 r = reflect(-s, n);
    float sDotN = max(dot(s, n), 0.0);
    vec3 diffuse = light.Intensity * material.Kd * sDotN;
    vec3 spec = vec3(0.0);
    if (sDotN > 0.0){
        spec = light.Intensity * material.Ks * pow(max(dot(r, v), 0.0), material.Shininess);
    }
    return diffuse + spec;
}

subroutine void RenderPassType();
subroutine uniform RenderPassType RenderPass;

subroutine (RenderPassType)
void shadeWithShadow(){
    vec3 ambient = light.Intensity * material.Ka;
    vec3 diffAndSpec = phongModelDiffAndSpec();

    float shadow = 1.0;
    if (ShadowCoord.z >= 0) {
        shadow = textureProj(ShadowMap, ShadowCoord);
    }

    //If in shadow, use ambient only
    FragColour = vec4(diffAndSpec * shadow + ambient, 1.0);

    //Gamma correction
    FragColour = pow(FragColour, vec4(1.0 / 2.2));
}

subroutine (RenderPassType)
void recordDepth(){
    //Noting
}

void main(){
    //Will call either shade with shadow or record recordDepth
    RenderPass();
}





