# 3015 Coursework 1 - Breath of the Mild 
By Oliver Warry

Demo video [here](https://youtu.be/KHnyvOIJeQM)

## Overview
Inspired by Tears of the Kingdom, I wanted to recreate the terrain and visual style of a place in the game with this old ruin perched on a hill. 
![Inspiration](Documentation_images/inspo.jpg)

And this is my project outcome.
![Demo image](Documentation_images/DEMO.png)

## Technical
Developed with Visual Studio 2022 and OpenGL 4.4.
### External Dependencies
[GLAD](https://github.com/Dav1dde/glad) | [GLFW](https://www.glfw.org/) | [GLM](https://github.com/g-truc/glm) | [FastNoiseLite](https://github.com/Auburn/FastNoiseLite) | [OpenGL](https://www.opengl.org/)

### Asset declaration
Some textures were not directly created by me, being sources from other sites. All citations and sources are held in [TextureSources.txt](media/texture/TextureSources.txt)

The ruin model present in the scene as well as the terrain were both modelled by me. Used textures are held in the textures folder.

# Main features
## Phong Lighting
As visible in the scene, I use fragment based phong lighting. The fragment shader [Terrain.frag](shader/terrain.frag) is a good demonstration. The [vertex shader](shader/terrain.vert) sets gl-Position to the projected vertex position, but also passes in the world position and view space position for later use. 

The phongModel method takes in the necessary arguments and returns a colour vector which is added to the existing colour so the output is the culmination of multiple lights.  This method works in view space, but the light position is passed in as world space, so it is first converted into view space, alongside the view position. 

This lighting model also features attenuation, using the Constant, Linear and Quadratic values to determine light falloff range and intensity, which has been tuned in code to fit the scene. Said model considers ambient, diffuse and specular, which uses the view space positions calculated prior to alter the colour of the current fragment.

## Basic Texture Sampling
Objects (the ruin in this case) has a simple texture sampling approach. UVs have been defined in Blender after unwrapping, so in [object.frag](shader/object.frag) the texture colour for the fragment is sampled with a slight alteration to the texCoord it will be sampling from. The texture was improperly rotated when I first imported it, and ended up using a rotation matrix to rotate it 90 degrees to get the proper rotation while also scaling it to the correct size. The phongModel then uses this texture colour in it's lighting calculations.

## Lighting subtechniques
Two lighting subtechiques were used for this scene - fog and multilight. 

### Fog
Fog is a fairly simple implementation and can be seen in [Terrain.frag](shader/terrain.frag). I get the position of the fragment in world space and the position of the view in world space and calculate the distance between them. Fogfactor is a 0 - 1 value that linearly interpolates between the fogStart value and fogEnd value, where the fog starts to become visible and when the fog is strongest, respectively. It then just mixes what would be the final colour of the fragment with this fog colour.

### Multilight
To achieve multiple lights I have a structure for individual lights held in the shader defining position, light intensity and attenuation. I've hard coded a size of 13 for the maximum number of point lights that can affect a surface, and created an array of pointlights of that size.
```GLSL
#define MAX_NUMBER_OF_LIGHTS 13

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
```
Then instead of just calling phongModel once, it iterates for every point light and adds the newly calculated light to the fragment's output colour
```GLSL
vec3 colour = vec3(0.0);    
    for (int i = 0; i < MAX_NUMBER_OF_LIGHTS; i++)
    {        
        colour += phongModel(i, Position, adjustedNormal, blendedTex.rgb);
    }   
```

Then I just update that array of pointLights in code like as shown below (found in [scenebasic_uniform.cpp](scenebasic_uniform.cpp)) by setting the uniforms.

```GLSL
lightUniformTag = ("pointLights[" + to_string(fireFlyLightIndex) + "]");
terrainProg.use();		
terrainProg.setUniform((lightUniformTag + ".Constant").c_str(), fireFly->pointLight->constant);
terrainProg.setUniform((lightUniformTag + ".Linear").c_str(), fireFly->pointLight->linear);
terrainProg.setUniform((lightUniformTag + ".Quadratic").c_str(), fireFly->pointLight->quadratic);

terrainProg.setUniform((lightUniformTag + ".Position").c_str(), fireFly->GetPosition());
terrainProg.setUniform((lightUniformTag + ".La").c_str(), fireFly->pointLight->ambient * fireFly->pointLight->brightness);		
terrainProg.setUniform((lightUniformTag + ".Ld").c_str(), fireFly->pointLight->diffuse * fireFly->pointLight->brightness);
terrainProg.setUniform((lightUniformTag + ".Enabled").c_str(), true);
```

## Texturing subtechniques
Two texturing techniques were used here alongside basic texture sampling - Texture mixing and alpha discard
### Texture mixing - Cloud shadows
I have two instances of texture mixing in the scene. The first is a rolling cloud shadow effect on the terrain. While the scene is set at night, it's meant to be an open countryside setting, so the sky is clear and clouds can obstruct the moonlight. This is the effect i'm aiming to replicate.

 In [scenebasic_uniform.cpp](scenebasic_uniform.cpp) 's initScene method, I generate a noise texture for clouds using fastNoiseLite, generating a 2048x2048 texture used in [Terrain.frag](shader/terrain.frag). To create the moving cloud effect I have altered the texture coordinate to sample from based on time, with some variation added to give it some more variance in movement. Time is passed in as a uniform at the start of the update method in [scenebasic_uniform.cpp](scenebasic_uniform.cpp), so that it holds the time in seconds since the application started. By getting the world position of the fragment (so multiple objects look to have the same cloud coverage independant of position or size) and adding on this animation coordinate offset, a noise value is generated after sampling the texture. Since it is a black and white image, only the red channel needs to be used. Then decreases the intensity of the shadow with an initial mix function. After the base lighting has been calculated it will mix the final output with this shadow to reduce the brightness of the fragment, simulating cloud shadows while not making said shadows pitch black.

```GLSL
void main() {
    float noiseScale = 0.002f;
    float speed = 0.5f;    

    float animatedX = time * speed + sin(time * 0.1f) * 0.1f;
    float animatedY = time * speed + cos(time * 0.12f) * 0.1f;
   
    vec2 animatedCoord = WorldPosition.xz * noiseScale + vec2(animatedX, animatedY);
    float noise = texture(CloudTex, animatedCoord).r;  


    float shadow = smoothstep(0.0, 0.05f, noise); 
    shadow = mix(shadow, 1.0, 0.35);
    ...
    vec3 colour = vec3(0.0);    
    for (int i = 0; i < MAX_NUMBER_OF_LIGHTS; i++)
    {        
        colour += phongModel(i, Position, adjustedNormal, blendedTex.rgb);
    }  
    
    vec3 finalColour = mix(colour, colour * shadow, 0.9);
```

### Texture mixing - Terrain surface
I wanted to look into a way to give the terrain some better variation and detail without just using random splotches of noise, and after doing some research found [this video](https://www.youtube.com/watch?v=rNuDkDhadfU)  on various texture blending methods. Triplanar mapping was one that caught my eye, so I chose to implement it for my terrain in the scene. Previous commits show moments where I added this and reinvented them to better suit my approach, but in the end I found a much simpler method of achieving similar results.

Triplanar mapping works by sampling 3 textures by projecting them onto the fragment from the 3 different axes, choosing the weighting for each projected texture based on the normal of the fragment. Since it is based on world position, and uses 3 planes, it means really vertical surfaces don't have that stretched look that sharp terrain can produce.
```GLSL
    vec3 blending = normalize(abs(WorldNormal));
    
    
    //Create texture coordinate positions based on the fragment's world position
    vec2 xzProj = WorldPosition.xz;
    vec2 xyProj = WorldPosition.xy;
    vec2 yzProj = WorldPosition.yz;   

    vec3 texXZ = texture(GrassTex, xzProj).rgb; // Top-down
    vec3 texXY = texture(CliffTex, xyProj * TextureScale).rgb; // Side projection
    vec3 texYZ = texture(CliffTex, yzProj * TextureScale).rgb; // Side projection
    vec3 triplanarTex = texXZ * blending.y + texXY * blending.z + texYZ * blending.x;
```
This solution worked, but I eventually found a much simpler way to achieve this. 

The idea behind this new approach is that you look at a fragment's normal value in world space. The colour represents direction, so it's y component wil be high if it's a flatter surface. So creating a slopeFactor blend value based on the intensity of that vertical component can be used to mix between rock and grass, where it is grassy when that slopeFactor is high (plane is horizontal). This achieves that same texture blending look, but without the sideways projection that can help avoid texture stretching. However since none of my terrain is particularly vertical, I can get by with this simpler method and it looks just as good.

```GLSL    
    //If WorldNormal.y < 0.9, slopeFactor is 0, between 0.9 and 1.0 it smoothly blends from 0 to 1, and when WorldNormal.y > 1.0, slopeFactor is 1
    float slopeFactor = smoothstep(0.9, 1.0, abs(WorldNormal.y));     

    //Scale the textures
    vec3 grassTex = texture(GrassTex, TexCoord * TextureScale).rgb;
    vec3 rockTex = texture(CliffTex, TexCoord * TextureScale).rgb;
  
    vec3 blendedTex = mix(rockTex, grassTex, slopeFactor);
```

### Alpha Discard
In my [particle.frag](shader/particle.frag) shader, the shader responsible for rendering the firefly particle effects, I make use of alpha discard. The image has an alpha channel so that only the fly itself is seen, and it will simply discard fragment pixels if the alpha is low enough.

```GLSL
void main() {
    
    vec4 texColor = texture(spriteTex, TexCoord);    
    
    if (texColor.a < 0.1) {
        discard;
    }
    FragColour = texColor;
}
```

## Skybox
The skybox follows the camera and rotates too, making it look like an infinitely expanding backdrop, rendered with [skybox.frag](shader/skybox.frag)

## Light Animation
My fireflies spawn in dynamically, move around and then despawn. In the update method of [scenebasic_uniform.cpp](scenebasic_uniform.cpp) a timer will count up when there are fewer than the max number of fireflies present, which will spawn a new one once it reaches a spawnCooldown value. It chooses a random location with a set region, creates a new fireFly and pointLight object and adds it to a container holding all active fireflies. The same update method then also iterates through all active firflies and checks `shouldDestroy`, destroying the light and disabling it in the shader. Render will then iterate through active fireFlies and update the shader light array so it matches their current state.

## HDR
Implementing HDR has been rather fussy. I did manage to get it working, with tonemapping, exposure and gamma correction by rendering to a framebuffer and then a texture, but doing so for some reason had my skybox fail to render. The project as it is currently has HDR disabled so that I can properly display the skybox. 

However, the back end setup and implementation for HDR does still exist. Inside of setupFBO I create the hdrFBO, create and assign it a texture target to render to, create the depth buffer and bind it. It's just that inside of render it calls `glBindFramebuffer(GL_FRAMEBUFFER, 0);` to allow for the skybox to render as intended. If I uncomment the code after the second pass marker in the render method as follows and change the framebuffer target to the hdrFBO in the render method too, it shows the implementation of HDR in action.
```GLSL
	...
    TerrainMesh->render();

	//Second pass - HDR		
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST);	
	screenHdrProg.use();
	model = mat4(1.0f);
	view = mat4(1.0f);
	projection = mat4(1.0f);
	setMatrices(screenHdrProg);
	screenHdrProg.setUniform("hdr", hdr);
	screenHdrProg.setUniform("exposure", exposure);
	glBindVertexArray(fsQuad);
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, renderTex);
	glDrawArrays(GL_TRIANGLES, 0, 6);	

}
```
HDR on with exposure set to 2.2
![HDR on with exposure set to 2.2](Documentation_images/HDR_on_EXP_2.2.png)
HDR on with exposure set to 0.5
![HDR on with exposure set to 0.5](Documentation_images/HDR_on_EXP_0.5.png)

This renders to a texture and then renders onto a full screen quad as is required for HDR, handled in the [screenHdr.frag](shader/screenHdr.frag) shader. The functionality is there, just disabled as I figure out how to fix the skybox.

## Keyboard and mouse controls
Present in the scene are mouse and keyboard controls, with a camera object that holds a position which is manipulated in response to key presses or mouse movement. 

# Unique Features
## Firefly particles - geometry shaders
While the dynamic lighting has already been discussed, I also made use of geometry shaders to create point sprites for my firefly particles. To achieve this I set up a new VAO for sprites which has one attribute for instanced vertex positions.

```GLSL
glGenVertexArrays(1, &spritesVAO);
glGenBuffers(1, &spritesInstanceVBO);

glBindVertexArray(spritesVAO);


//Firefly positions (instance buffer)
glBindBuffer(GL_ARRAY_BUFFER, spritesInstanceVBO);
glBufferData(GL_ARRAY_BUFFER, maxFireFlyCount * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
glEnableVertexAttribArray(1);
glVertexAttribDivisor(1, 1);

glBindVertexArray(0);
```

Then after all the fireFly objects have been updated and the shader uniforms passed, it rebinds the VAO and alters the data in this first attribute location on the VAO to a new array of light positions.

```GLSL
glBindVertexArray(spritesVAO);
glBindBuffer(GL_ARRAY_BUFFER, spritesInstanceVBO);
glBufferSubData(GL_ARRAY_BUFFER, 0, fireFlyPositions.size() * sizeof(glm::vec3), fireFlyPositions.data());
glBindBuffer(GL_ARRAY_BUFFER, 0);
particleProg.use();
model = mat4(1.0f);
setMatrices(particleProg);


glDrawArraysInstanced(GL_POINTS, 0, 1, fireFlies.size());
```

When drawn it uses [particle.vert](shader/particle.vert), [particle.geom](shader/particle.geom) and [particle.frag](shader/particle.frag) to render the firefly sprites. The geometry shader generates a quad around the position of the firefly, which then has the new texture coordinates passed to the fragment shader to render the texture on this new geometry.