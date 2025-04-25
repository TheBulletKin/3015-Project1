#ifndef SCENEBASIC_UNIFORM_H
#define SCENEBASIC_UNIFORM_H

#include "helper/scene.h"

#include <glad/glad.h>
#include "helper/glslprogram.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "helper/torus.h"
#include "helper/plane.h"
#include "helper/objmesh.h"
#include "helper/cube.h"
#include "helper/FireFly.h"
#include "helper/skybox.h"

#include <random>
#include <math.h>
#include "Camera.h"
#include <GLFW/glfw3.h>

class SceneBasic_Uniform : public Scene
{
private:
    //Textures and shaders
    GLSLProgram skyProg, screenHdrProg, particleProg, terrainProg, objectProg, PBRProg, newParticleProg;
    GLuint grassTexID, rockTexID, skyBoxTexID, cloudTexID, brickTexID, fireFlyTexID, particleTexID;
    
    GLFWwindow* window;     

    float tPrev, lightAngle, lightRotationSpeed;

    GLuint initVel, startTime, particles, nParticles;
    vec3 emitterPos, emitterDir;
    float particleLifetime, time;
   
    //Objects
    unique_ptr<ObjMesh> TerrainMesh;
    unique_ptr<ObjMesh> RuinMesh;
   
    //Post processing
    GLuint fsQuad;
    GLuint hdrFBO;  
    GLuint linearSampler, nearestSampler;
    GLuint depthRbo; 
    GLuint renderTex;

    bool hdr = true;
    float exposure = 1.5f;

    //Particle effects
    GLuint spritesVAO, spritesQuadVBO, spritesInstanceVBO;
    GLuint lightPositionsBuffer;

    Camera camera;
    
    SkyBox sky; 

    //Fireflies
    vector<FireFly*> fireFlies;
    float fireFlySpawnTimer;
    int fireFlySpawnCooldown;
    int currentFireFlyCount;
    int maxFireFlyCount;
    vec3 fireFlyLightColour = vec3(0.53, 0.67, 0.33);
    vec3 ambientLightColour = vec3(0.1f, 0.1f, 0.3f);
    int numberOfStaticLights;

    struct Point {
        float x, y, z;
    };
    
    Point topLeftSpawnBound = { -7.0f, 23.0f, -14.0f }; //-X means left  -Z means forward
    Point bottomRightSpawnBound = { 2.0f, 17.0f, -3.0 };

    //Random number gen
    random_device rd;  
    mt19937 gen;
    uniform_real_distribution<float> dis;

    float deltaTime = 0.0f;
    float lastFrameTime = 0.0f;
    
    //Input
    float lastX;
    float lastY;
    bool firstMouse;

    void setMatrices(GLSLProgram &program);
    void compile();
    static void mouseCallback(GLFWwindow* window, double xposIn, double yposIn);    
    void processInput(GLFWwindow* window);   
    void setupFBO();    
public:
    SceneBasic_Uniform();
    void initScene();
    void update(float t );
    void render();
    void resize(int, int);    
};

#endif // SCENEBASIC_UNIFORM_H
