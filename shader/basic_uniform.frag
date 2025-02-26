#version 460

layout (location = 0) out vec4 FragColor;

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

uniform int staticPointLights = 3;
uniform int dynamicPointLights = 0;
uniform float time;


vec3 Colour;

//position of fragment in view space
in vec3 Position;
in vec3 Normal;
in vec2 TexCoord;

layout(binding = 0) uniform sampler2D BrickTex;
layout(binding = 1) uniform sampler2D MossTex;
layout(binding = 2) uniform sampler2D BaseTex;
layout(binding = 3) uniform sampler2D AlphaTex;
layout(binding = 4) uniform sampler2D CloudTex;


uniform vec3 ViewPos;

vec3 phongModel(int light, vec3 position, vec3 n, vec3 texColour);

void main() {
    float noiseScale = 0.002f;
    float speed = 0.01f;
    float animatedX = time * speed + sin(time * 0.01f) * 0.01f;
    float animatedY = time * speed + cos(time * 0.01f) * 0.01f;


    /* Previously did this. But it meant that vertical surfaces or steep camera angles were stretched.
    Would project as if going straight down, but given how the UVs are set up this distorted it.
    Needed to use Triplanar mapping - https://gamedevelopment.tutsplus.com/use-tri-planar-texture-mapping-for-better-terrain--gamedev-13821a
    */
    //vec2 animatedCoord = Position.xz * noiseScale + vec2(animatedX, animatedY);
    //float noise = texture(CloudTex, animatedCoord).r;

    //Blending is then the normal vector without a sign. Meant to help discern weighting for the 3 axes
    vec3 blending = abs(Normal);
    blending = blending / (blending.x + blending.y + blending.z); //Normalise this blend vector

    //Since UVs are the coordinate on a texture to sample from, get the world position instead of just the texture coord relative to the mesh 
    //Do this for the 3 different axes
    vec2 uvXZ = Position.xz * noiseScale + vec2(animatedX, animatedY);
    vec2 uvXY = Position.xy * noiseScale + vec2(animatedX, animatedY);
    vec2 uvYZ = Position.yz * noiseScale + vec2(animatedX, animatedY);

    //Sample the noise texture from all three axes
    float noiseXZ = texture(CloudTex, uvXZ).r;
    float noiseXY = texture(CloudTex, uvXY).r;
    float noiseYZ = texture(CloudTex, uvYZ).r;

    //Use the blend value to determine which noise value from each projection needs to be added to the current fragment
    float noise = noiseXZ * blending.y + noiseXY * blending.z + noiseYZ * blending.x;


    float shadow = smoothstep(0.0, 0.05f, noise); 
    shadow = mix(shadow, 1.0, 0.6);
    

    vec4 BrickTexColour = texture(BrickTex, TexCoord);
    vec4 MossTexColour = texture(MossTex, TexCoord);

    //vec4 texColour = texture(BaseTex, TexCoord);
    //vec4 alphaMap = texture(AlphaTex, TexCoord);
    if (MossTexColour.a < 0.15) {
        //discard;
    }

    vec3 adjustedNormal = Normal;
    if (!gl_FrontFacing) {
        adjustedNormal = -Normal;
    }

    vec3 texColour = mix(BrickTexColour.rgb, MossTexColour.rgb, MossTexColour.a);
    //Colour = texColour.rgb;
    Colour = vec3(0);
    for (int i = 0; i < MAX_NUMBER_OF_LIGHTS; i++)
    {
        Colour += phongModel(i, Position, adjustedNormal, texColour.rgb);
    }
    FragColor = vec4(clamp(Colour * shadow, 0.0, 1.0), 1.0);
    //FragColor = vec4(vec3(shadow), 1.0f);
    
}

vec3 phongModel(int light, vec3 position, vec3 n, vec3 texColour){
    
    if (pointLights[light].Position == vec3(0.0f, 0.0f, 0.0f)) {        
        return vec3(0.0f);
    }

    //Ambient
    //Light that hides an object on all sides, or only some depending on the environment. Such as bounce light.
    //Ka is a constant that determines how much of that ambient light is reflected off of the object.
    //La is the ambient light intensity, like how bright it might be outside for instance.
    vec3 AmbientLight = Material.Ka * pointLights[light].La * texColour;
    
    //Light direction (s)
    vec3 LightDir = normalize(vec3(pointLights[light].Position) - position);

     //Diffuse
    //The majority of the light we see. Essentially what is actually lit by a light source
    //Requires two vectors - normal (perpendicular to the plane) , s (direction vector from the vertex / fragment position to the light source)
    //Equation for diffuse is to multiply the diffuse reflectivity and light diffuse value, multiply it by the dot of the light direction and normal.
    //Uses dot so that the diffuse light visible varies from 0 to 1 based on angle to the light.
    //Could also multiply s and n by cos0;
    float sDotN = max(dot(n, LightDir), 0.0);
    vec3 DiffuseLight = Material.Kd * pointLights[light].Ld * sDotN * texColour;

    //View dir is usually cameraPos - FragPos. Camera pos is 0, so this becomes - pos
    vec3 viewDir = normalize(-position);
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
