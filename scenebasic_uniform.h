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
    GLSLProgram prog, skyProg;
    
    GLFWwindow* window;
    //float angle;
    //Torus torus;
    Plane plane;
    std::unique_ptr<ObjMesh> PigMesh;
    std::unique_ptr<ObjMesh> TerrainMesh;
    Cube cube;
    GLuint fsQuad;
    GLuint renderFBO, intermediateFBO, hdrFBO, blurFBO;
    GLuint renderTex, intermediateTex;
    GLuint hdrTex, avgTex, tex1, tex2;
    GLuint linearSampler, nearestSampler;

    int bloomBufWidth, bloomBufHeight;


    Camera camera;
    
    SkyBox sky; 

    int numberOfStaticLights;

    std::vector<FireFly*> fireFlies;
    float fireFlySpawnTimer;
    int fireFlySpawnCooldown;
    int currentFireFlyCount;
    int maxFireFlyCount;
    vec3 fireFlyLightColour = vec3(0.3f, 0.3f, 0.3f);
    vec3 ambientLightColour = vec3(0.3f, 0.6f, 0.2f);

    struct Point {
        float x, y, z;
    };
    
    Point topLeftSpawnBound = { -1.0f, -0.4f, -0.2f }; //-X means left  -Z means forward
    Point bottomRightSpawnBound = { 1.0f, -0.4f, -0.2f };

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
