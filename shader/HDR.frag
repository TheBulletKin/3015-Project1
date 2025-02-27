#version 460

layout (location = 0) out vec4 FragColour;
layout (location = 1) out vec3 HdrColour;

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

float luminence(vec3 colour){
    return 0.2126 * colour.r + 0.7152 * colour.g + 0.0722 * colour.b;
}


vec4 pass1(){
    //Like in the CPP file, pass 1 performs regular rendering processes. As set up there it will render this to a frame buffer, then into RenderTex which is read here

    
    
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
    
    return vec4(colour, 1.0);
}

vec4 pass2(){
    
    vec4 val = texture(HdrTex, TexCoord);
    if (luminence(val.rgb) > LumThresh){
        return val;
    } else {
        return vec4(0.0);
    }
}

vec4 pass3(){

    float dy = 1.0 / (textureSize(BlurTex1, 0)).y;
    vec4 sum = texture(BlurTex1, TexCoord) * Weight[0];
    for (int i = 1; i < 10; i++){
        sum += texture(BlurTex1, TexCoord + vec2(0.0, PixOffset[i]) * dy) * Weight[i];
        sum += texture(BlurTex1, TexCoord - vec2(0.0, PixOffset[i]) * dy) * Weight[i];
    }
    return sum;
}

vec4 pass4(){

    float dx = 1.0 / (textureSize(BlurTex2, 0)).x;
    vec4 sum = texture(BlurTex2, TexCoord) * Weight[0];
    for (int i = 1; i < 10; i++){
        sum += texture(BlurTex2, TexCoord + vec2(PixOffset[i], 0.0) * dx) * Weight[i];
        sum += texture(BlurTex2, TexCoord - vec2(PixOffset[i], 0.0) * dx) * Weight[i];
    }
    return sum;
}

vec4 pass5(){

     /* HDR and Tonemapping explanation
    * Most monitors have a limited colour range, with colours clamped from 0 - 1.
    * Tone mapping compresses the high dynamic range of a scene to a lower dynamic range of the display
    * HDR represents colours as infinitely large floating points. So a colour could be 10.0, 0.3, 3.4 and not limited by 0-1
    * 
    */

    vec4 colour = texture(HdrTex, TexCoord);

    // xyz colour space is x (red sensitivity), y (percieved brightness), z (blue sensitivity)
    vec3 xyzCol = rgb2xyz * vec3(colour);
    float xyzSum = xyzCol.x + xyzCol.y + xyzCol.z;

    //xyY colour space is x and y (chromaticity) and Y (percieved brightness)
    vec3 xyYCol = vec3(xyzCol.x / xyzSum, xyzCol.y / xyzSum, xyzCol.y);

    //Since the z part of xyY is Y, brightness, divide by average luminance to introduce auto exposure
    float L = (Exposure * xyYCol.z) / AveLum;
    
    //This now uses tone mapping to compress the extreme HDR values into ones that can be displayed
    L = (L * ( 1 + L / (White * White))) / (1 + L);

    //Convery back to xyz space from xyY
    xyzCol.x = (L * xyYCol.x) / (xyYCol.y);
    xyzCol.y = L;
    xyzCol.z = (L * ( 1 - xyYCol.x - xyYCol.y)) / xyYCol.y;

    //Then to RGB space for display
    vec4 toneMapColour = vec4(xyz2rgb * xyzCol, 1.0);
    
    //Combine with blurred texture
    vec4 blurTex = texture(BlurTex1, TexCoord);
    return toneMapColour + blurTex;

    
}




void main() {
    if (Pass == 1) FragColour = pass1();
    else if (Pass == 2) FragColour = pass2();
    else if (Pass == 3) FragColour = pass3();
    else if (Pass == 4) FragColour = pass4();
    else if (Pass == 5) FragColour = pass5();

   
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


// ---------------------------------
    //NOTES
    //LIGHT TYPES

    //Directional Light 
    //These lights have no position, simply have a direction, s.
    //For point lights you would deduce the light direction with lightPos - vertexPos.
    //For a directional light you can hide a value in the w component of the vec4 position since only x y and z are used.
    //So for instance if lightposition.w == 0, s = lightposition.xyz. If not then find it from the vertex position.
    //Not very useful for parellel calculations however. The GPU is best at taking lots of stuff and doing one thing to it.
    //A conditional means the gpu will split its work into different paths. Each path is computed separately which wastes time when switching between paths. 
    //To get everything running in parallel it's best to reduce number of if statements.
    //Instead, using s = lightPosition - (vertexPosition * lightPosition.w), it is all running in parallel. Will either subtract by zero or the vertex position.

    //Point light
    //Basic light source. Has a position and creates light around it evenly.
    //Uses the phong reflection model used above. Combining ambient, diffuse and specular.
    //For multiple lights you need to calculate each and add them together.
    //Can create a uniform array of lights with info needed. Can then set that specific light source in CPU code using array element assignment as the uniform name.

    //spotlight
    //A cone of light.
    //Has a direction vector, cut off angle and position. Cut off angle is the angle from that centre direction vector to the edge of the 'cone'
    //(Will add an example once I add it to the project)

    //Attentuation
    //The effect of a light's strength diminishing over distance
    //For diffuse light, you would divide the intensity by a distance and constant value
    //Diffuse = (Kd * Ld * (s.n)) / (r + k)
    //Where r is distance, k is a constant. This isn't full on inverse square law however.


    //Per fragment shading
    //Vertex based Gouraud shading approximated lighting based on vertices, not fragments. Can look inconsistent.
    //Will interpolate the position and normal between vertices within the vertex shader and pass it into the fragment shader,
    // can then perform the lighting calculations for each fragment.

    //Blinn-phong
    //A slight modification of phong, changing the specular highlight equation.
    //Instead of Ks * Ls * (r.v)^f, it becomes Ks * Ls * (h.n)^f
    //H is the vector halfway between r and n. Normalized(v + s).
    //Can therefore replace r, which is more expensive to calculate.

    //Cel / toon shading
    //The dot product calculated for diffuse is locked to a fixed number of values
    //Like maybe 4 thresholds at points between 0 and 1.
    //To make calulcations simple for parallel processing, take this approach:
    //levels = 4
    //scaleFactor = 1.0/levels. 
    //diffuse = Kd * floor(sDotN * levels) * scaleFactor
    //Floor will essentially round down to the nearest int. So if sDotN is 0.73 for instance, it becomes 0.73 * 4 floored. 2.92 floored is 2.
    //Multiply that by the scale factor, 0.25, 0.5. Essentially rounds to one of the equidistant bounds determined by the number of levels, without an if statement.

    //Fog simulation
    //Hold a fog max distance, min distance and colour.
    //the fog factor (0 as 100%, 1 and 0%), use f = (distMax - abs(distanceFromCamera)) / (distMax - distMin)
    //Can then mix with resulting frag colour using
    // vec3 colour = mix(fog.colour, phongColour, fogFactor);
    //Then out put that final colour
