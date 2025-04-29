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

uniform int Pass;

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

void shadeWithShadow(){
    vec3 ambient = light.Intensity * material.Ka;
    vec3 diffAndSpec = phongModelDiffAndSpec();

    float sum = 0;
    float shadow = 1.0;

    //Don't text points behind the light source
    if (ShadowCoord.z >= 0){
        //Anti aliase shadows
        for (int x = -1; x <= 1; ++x) {
            for (int y = -1; y <= 1; ++y) {
            sum += textureProjOffset(ShadowMap, ShadowCoord, ivec2(x, y));
            }
        }        
        shadow = sum / 9.0;
    }

    FragColour = vec4(ambient + diffAndSpec * shadow, 1.0);

    //FragColour = vec4(shadow, shadow, shadow, 1.0);

    //Gamma correction
    FragColour = pow(FragColour, vec4(1.0 / 2.2));
    //FragColour = vec4(0.0, 1.0, 0.0, 1.0); // Green - test
}

void recordDepth(){
    //Noting
    //FragColour = vec4(1.0, 0.0, 0.0, 1.0); // Red - test
    float depth = ShadowCoord.z;  
    FragColour = vec4(depth, depth, depth, 1.0); 
}

void main(){
    //Will call either shadeWithShadow or recordDepth, defined in the CPU code.
    //RenderPass();
    //recordDepth();
    //shadeWithShadow();

    if (Pass == 1){
        recordDepth();
    } else if (Pass == 2){
        shadeWithShadow();
    }
}





