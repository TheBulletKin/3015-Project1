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
    //GLuint vaoHandle;
    GLSLProgram skyProg, screenHdrProg, screenBlur, particleProg, terrainProg, objectProg;
    
    GLFWwindow* window;
    //float angle;
    //Torus torus;
    Plane plane;
    std::unique_ptr<ObjMesh> PigMesh;
    std::unique_ptr<ObjMesh> TerrainMesh;
    std::unique_ptr<ObjMesh> RuinMesh;
    Cube cube;
    GLuint fsQuad;
    GLuint hdrFBO;   
   
    GLuint linearSampler, nearestSampler;
    GLuint depthRbo; 
    GLuint renderTex;
    GLuint spritesVAO, spritesQuadVBO, spritesInstanceVBO;

    GLuint lightPositionsBuffer;

    GLuint grassID, rockID, skyCubeID, cloudID, brickID;
    
 

    int bloomBufWidth, bloomBufHeight;
    bool hdr = true;
    float exposure = 1.3f;
    bool bloom = false;

    GLuint sprites;
    int numSprites;
    float* locations;

    Camera camera;
    
    SkyBox sky; 

    int numberOfStaticLights;

    std::vector<FireFly*> fireFlies;
    float fireFlySpawnTimer;
    int fireFlySpawnCooldown;
    int currentFireFlyCount;
    int maxFireFlyCount;
    vec3 fireFlyLightColour = vec3(0.53, 0.67, 0.33);
    vec3 ambientLightColour = vec3(0.1f, 0.1f, 0.3f);

    struct Point {
        float x, y, z;
    };
    
    Point topLeftSpawnBound = { -7.0f, 23.0f, -14.0f }; //-X means left  -Z means forward
    Point bottomRightSpawnBound = { 2.0f, 17.0f, -3.0 };

    std::random_device rd;  
    std::mt19937 gen;
    std::uniform_real_distribution<float> dis;

    float deltaTime = 0.0f;
    float lastFrameTime = 0.0f;
    

    void setMatrices(GLSLProgram &program);
    void compile();
    static void mouseCallback(GLFWwindow* window, double xposIn, double yposIn);
    float lastX;
    float lastY;
    bool firstMouse;
    void processInput(GLFWwindow* window);
    
    void pass1();
    void pass2();
    void pass3();
    void pass4();
    void pass5();
    void setupFBO();
    float gauss(float, float);
    void computeLogAveLuminance();
    void drawScene();
public:
    SceneBasic_Uniform();

    void initScene();
    void update(float t );
    void render();
    void resize(int, int);
    
};

#endif // SCENEBASIC_UNIFORM_H
