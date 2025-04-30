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
#include "helper/frustum.h"

#include <random>
#include <math.h>
#include "Camera.h"
#include <GLFW/glfw3.h>

class SceneBasic_Uniform : public Scene
{
private:
	//Textures and shaders
	GLSLProgram skyProg, screenHdrProg, particleProg, terrainProg, objectProg, PBRProg, newParticleProg, shadowProg;
	GLuint grassTexID, rockTexID, skyBoxTexID, cloudTexID, brickTexID, fireFlyTexID, particleTexID, randomParticleTexID, depthTex;

	GLFWwindow* window;

	float tPrev, lightAngle, lightRotationSpeed;


	//Shadows
	GLuint shadowFBO, pass1Index, pass2Index;
	int shadowMapWidth, shadowMapHeight;
	mat4 lightPV, shadowBias;
	Frustum lightFrustum;
	vec3 lightPos;

	//Particles
	GLuint posBuf[2], velBuf[2], age[2];
	GLuint particleArray[2];
	GLuint feedback[2];

	GLuint drawBuf;
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
	int fireFlyLightIndex;
	int maxFireFlyCount;
	vec3 fireFlyLightColour = vec3(0.53, 0.67, 0.33);
	vec3 ambientLightColour = vec3(0.1f, 0.1f, 0.3f);
	int numberOfStaticLights;
	float timeOfDay = 0;
	vec3 ambientDawnColour = vec3(0.8, 0.5, 0.3);
	vec3 ambientDayColour = vec3(0.5, 0.7, 1.0);
	vec3 ambientDuskColour = vec3(0.5, 0.2, 0.5);
	vec3 ambientNightColour = vec3(0.1, 0.1, 0.3);
	vec3 currentAmbientColour;
	vec3 sunTarget;
	float sunDistance;
	vec3 sunPos;
	vec3 sunLightDirection;
	float mainLightIntensity = 0.2;

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

	void setMatrices(GLSLProgram& program);
	void compile();
	static void mouseCallback(GLFWwindow* window, double xposIn, double yposIn);
	void processInput(GLFWwindow* window);
	void setupFBO();
	vec3 rgbToHsv(vec3 c);
	vec3 hsvToRgb(vec3 hsv);
	vec3 mixAmbientHSV(vec3 colorA, vec3 colorB, float t);
public:
	SceneBasic_Uniform();
	void initScene();
	void update(float t);
	void render();
	void drawSceneObjects();
	void initParticles();
	void initShadows();
	void initMaterials();
	void initLights();
	void initTextures();
	void initPostProcessing();
	void renderFireflies();
	void renderParticles();
	void updateDayNightCycle(float deltaTime);
	void updateShaders();
	void drawSolidSceneObjects();
	void resize(int, int);
};

#endif // SCENEBASIC_UNIFORM_H
