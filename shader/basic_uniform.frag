#version 460

layout (location = 0) out vec4 FragColor;

#define MAX_NUMBER_OF_LIGHTS 3

struct LightInfo{
    vec4 Position;
    vec3 La; //Ambient light intensity
    vec3 Ld; //Diffuse and spec light intensity
};

uniform LightInfo Lights[MAX_NUMBER_OF_LIGHTS];

struct MaterialInfo {
    vec3 Ka; //Ambient reflectivity
    vec3 Kd; //Diffuse reflectivity
    vec3 Ks; //Specular reflectivity
    float Shininess; //Specular shininess factor
};

uniform MaterialInfo Material;

vec3 Colour;

//position of fragment in view space
in vec3 Position;
in vec3 Normal;
in vec2 TexCoord;

layout(binding = 0) uniform sampler2D BrickTex;
layout(binding = 1) uniform sampler2D MossTex;

uniform vec3 ViewPos;

vec3 phongModel(int light, vec3 position, vec3 n);

void main() {
    
    vec4 BrickTexColour = texture(BrickTex, TexCoord);
    vec4 MossTexColour = texture(MossTex, TexCoord);
    Colour = mix(BrickTexColour.rgb, MossTexColour.rgb, MossTexColour.a);
    for (int i = 0; i < MAX_NUMBER_OF_LIGHTS; i++)
    {
        Colour += phongModel(i, Position, Normal);
    }
    FragColor = vec4(Colour, 1.0f);
    
}

vec3 phongModel(int light, vec3 position, vec3 n){
    
    //Ambient
    //Light that hides an object on all sides, or only some depending on the environment. Such as bounce light.
    //Ka is a constant that determines how much of that ambient light is reflected off of the object.
    //La is the ambient light intensity, like how bright it might be outside for instance.
    vec3 AmbientLight = Material.Ka * Lights[light].La;
    
    //Light direction (s)
    vec3 LightDir = normalize(vec3(Lights[light].Position) - position);

     //Diffuse
    //The majority of the light we see. Essentially what is actually lit by a light source
    //Requires two vectors - normal (perpendicular to the plane) , s (direction vector from the vertex / fragment position to the light source)
    //Equation for diffuse is to multiply the diffuse reflectivity and light diffuse value, multiply it by the dot of the light direction and normal.
    //Uses dot so that the diffuse light visible varies from 0 to 1 based on angle to the light.
    //Could also multiply s and n by cos0;
    float sDotN = max(dot(n, LightDir), 0.0);
    vec3 DiffuseLight = Material.Kd * Lights[light].Ld * sDotN;

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

    vec3 SpecularLight = Material.Ks * Lights[light].Ld * specular;

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
