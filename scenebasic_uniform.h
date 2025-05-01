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
	GLuint grassTexID, rockTexID, skyBoxTexID, cloudTexID, brickTexID, fireFlyTexID, particleTexID, randomParticleTexID, depthTex, torchTexID;

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
	GLuint emitterIndexBuf[2];
	GLuint particleArray[2];
	GLuint feedback[2];

	GLuint drawBuf;
	GLuint initVel, startTime, particles, nParticles, nEmitters;
	float particleLifetime, time;

	//Objects
	unique_ptr<ObjMesh> TerrainMesh;
	unique_ptr<ObjMesh> RuinMesh;
	unique_ptr<ObjMesh> StandingTorch;	

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
	float gameTimer = 0;
	float gameEndTime = 30;
	bool gameEnded = false;

	

	float torchMaxIntensity = 1.0f;
	float torchMinIntensity = 0.2f;
	vec3 torchBrightColour = vec3(1.0f, 0.6f, 0.4f);
	vec3 torchDimColour = vec3(1.0f, 1.0f, 1.0f);
	struct TorchInfo{
		vec3 position;
		float intensity;		
	};

	TorchInfo torches[4] = {
	{ vec3(-2, 0.3, -7) },
	{ vec3(-1, 0.1f, -6) },
	{ vec3(-3, 0.3, -2) },
	{ vec3(-4, 0.3, -3.5)}
	};

	FastNoiseLite torchNoise;
	



	vec3 currentAmbientColour;
	vec3 currentSunColour;
	vec3 sunTarget;
	float sunDistance;
	vec3 sunPos;
	vec3 sunLightDirection;
	float mainLightIntensity = 0.2;
	float ambientLightIntensity = 0.2;

	struct FogInfo {
		vec3 fogColour;
		float fogStart;
		float fogEnd;
	};

	struct TimeOfDayInfo {
		string name;
		vec3 ambientLightColour;
		vec3 lightColour;
		FogInfo fogInfo;
		float mainLightIntensity; //Light intensity at the end of the ramp up time
		float ambientLightIntensity;
		float startTime;
		float rampUpTime;
	};

	TimeOfDayInfo dawnInfo = {
		"dawn",
		vec3(0.6f, 0.45f, 0.3f),
		vec3(1.0f, 0.8f, 0.6f),
		{
			vec3(0.6f, 0.45f, 0.3f) * 0.3f,
			10.0f,
			25.0f
		},
		0.17f, // Light intensity 
		0.12f,
		0.01f, //Start time
		0.02f //Ramp up time
	};

	TimeOfDayInfo dayInfo = {
		"day",
		vec3(0.5f, 0.65f, 0.9f),
		vec3(1.0f, 1.0f, 0.95f),
		{
			vec3(0.5f, 0.65f, 0.9f) * 0.3f,
			25.0f,
			60.0f
		},
		0.6f, // Light intensity
		0.12f,
		0.05f, //Start time
		0.05f //Ramp up time
	};

	TimeOfDayInfo duskInfo = {
		"dusk",
		vec3(0.5, 0.7, 1.0),
		vec3(1.0f, 0.4f, 0.2f),
		{
			vec3(0.5, 0.7, 1.0) * 0.3f,
			25.0f,
			60.0f
		},
		0.0f, // Light intensity
		0.12f,
		0.9f, //Start time
		0.08f //Ramp up time
	};

	TimeOfDayInfo moonRiseInfo = {
		"moonrise",
		vec3(0.2f, 0.2f, 0.35f),
		vec3(0.6, 0.6, 1.0),
		{
			vec3(0.2f, 0.2f, 0.35f) * 0.3f,
			10.0f,
			40.0f
		},
		0.01f, // Light intensity
		0.12f,
		1.0f,//Start time
		0.1f //Ramp up time
	};

	TimeOfDayInfo nightInfo = {
		"night",
		vec3(0.1f, 0.1f, 0.2f),
		vec3(0.4f, 0.4f, 0.8f),
		{
			vec3(0.1f, 0.1f, 0.2f) * 0.3f,
			10.0f,
			40.0f
		},
		0.04f, //Light intensity
		0.12f,
		1.08f, //Start time
		0.1f //Ramp up time
	};

	TimeOfDayInfo moonsetInfo = {
		"moonset",
		vec3(0.15f, 0.15f, 0.25f),
		vec3(0.5f, 0.5f, 0.9f),
		{
			vec3(0.15f, 0.15f, 0.25f) * 0.3f,
			10.0f,
			40.0f
		},
		0.03f, //Light intensity
		0.12f,
		1.93f, //Start time
		0.06f //Ramp up time
	};

	TimeOfDayInfo timesOfDay[6] = {
		dawnInfo,
		dayInfo,
		duskInfo,
		moonRiseInfo,
		nightInfo,
		moonsetInfo
	};

	TimeOfDayInfo prevTimeOfDay = timesOfDay[5];
	TimeOfDayInfo currentTimeOfDay = timesOfDay[0];
	TimeOfDayInfo nextTimeOfDay = timesOfDay[1];
	int timeOfDayIndex;

	struct Collectable {
		string name;
		vec3 location;
		bool isActive;
		//model (for rendering)
	};

	int maxCollectables = 4;
	int collectedItems = 0;
	bool collectedAllItems = false;
	Collectable collectables[4] = {
		Collectable {
			"meat",
			vec3(-3.0f, 4.0f, -7.0f),
			true
		},
		Collectable {
			"cheese",
			vec3(-6.0f, 3.0f, -5.0f),
			true
		},
		Collectable {
			"flower",
			vec3(-1.0f, 4.0f, -3.0f),
			true
		},
		Collectable {
			"mushroom",
			vec3(-2.0f, 4.0f, -4.0f),
			true
		}
	};

	vec3 cookingPotLocation = vec3(-2, 4, -4);
	bool hasCookedItem = false;





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
	void setArrayUniforms();
	void attemptPickup();
	vec3 rgbToHsv(vec3 c);
	vec3 hsvToRgb(vec3 hsv);
	vec3 mixHSV(vec3 colorA, vec3 colorB, float t);
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
	void updateDayNightShaders(TimeOfDayInfo currentState, TimeOfDayInfo toState, float t);
	void drawSolidSceneObjects();
	void resize(int, int);
	};

#endif // SCENEBASIC_UNIFORM_H
