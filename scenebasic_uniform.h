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
	//----------Textures and shaders
	GLSLProgram skyProg, screenHdrProg, particleProg, terrainProg, objectProg, PBRProg;
	GLuint  nightSkyBox, depthTex, torchTexID, daySkyboxTexID, meatTexID, cheeseTexID, mushroomTexID, campfireTexID;
	GLFWwindow* window;


	//--------------Shadows
	GLuint shadowFBO, pass1Index, pass2Index;
	int shadowMapWidth, shadowMapHeight;
	mat4 lightPV, shadowBias;
	Frustum lightFrustum;
	vec3 lightPos;
	GLSLProgram shadowProg;

	//-----------Particles
	GLuint posBuf[2], velBuf[2], age[2];
	GLuint emitterIndexBuf[2];
	GLuint particleArray[2];
	GLuint feedback[2];	
	GLSLProgram  particleStreamProg;
	GLuint particleTexID, randomParticleTexID;
	GLuint drawBuf;
	GLuint initVel, startTime, particles;
	GLuint nParticles = 360;
	GLuint nEmitters = 8;
	float particleLifetime = 10.0f;
	float time;	
	

	//---------Fireflies
	vector<FireFly*> fireFlies;
	vec3 topLeftSpawnBound = vec3(-20, 2.2f, -20.0f);
	vec3 bottomRightSpawnBound = vec3(20, -3.4f, 20.0f);
	GLuint fireflyPosBuf;
	GLuint fireflyVA;
	GLSLProgram fireflyParticleProg;
	GLuint fireFlyTexID;
	float fireFlySpawnTimer = 0;
	int fireFlySpawnCooldown;
	int currentFireFlyCount = 0;
	int maxFireFlyCount = 20;
	vec3 fireFlyLightColour = vec3(0.53, 0.70, 0.33);
	vec3 fireFlyAmbientColour = vec3(0.1f, 0.1f, 0.3f);

	//Objects
	unique_ptr<ObjMesh> TerrainMesh;
	GLuint grassTexID, rockTexID, cloudTexID, brickTexID;
	unique_ptr<ObjMesh> RuinMesh;
	unique_ptr<ObjMesh> StandingTorch;	
	unique_ptr<ObjMesh> campfireMesh;	
	vec3 campfirePosition = vec3(6.8f, -0.8f, 0.0f);

	//Post processing
	GLuint fsQuad;
	GLuint hdrFBO;
	GLuint linearSampler, nearestSampler;
	GLuint depthRbo;
	GLuint renderTex;

	bool hdr = true;
	float exposure = 1.5f;

	//-------Other
	Camera camera;
	SkyBox sky;
	float deltaTime = 0.0f;
	float lastFrameTime = 0.0f;

	
	//------Torch lights
	float torchMaxIntensity = 0.9f;
	float torchMinIntensity = 0.32f;
	vec3 torchBrightColour = vec3(1.0f, 0.6f, 0.4f);
	vec3 torchDimColour = vec3(1.0f, 1.0f, 1.0f);
	struct TorchInfo{
		vec3 position;
		float intensity;		
	};

	TorchInfo torches[8] = {
	{ vec3(1.0f, -3.8f, 5.4f) }, //By player spawn
	{ vec3(-4, -2.7f, -11) }, //Front left valley
	{ vec3(0, -0.7, -3.3) }, //Close ridge on spawn
	{ vec3(-7, -0.15, -22)}, //Far left mound
	{ vec3(-16, -2.5, -4) }, //Bottom left of map
	{ vec3(9.55f, 3.9f, -12.7) }, //Top of mountain
	{ vec3(14, -0.8, 10) }, //Bottom right of map
	{ vec3(18, -4.0, 3)} //Close right valley
	};

	FastNoiseLite torchNoise;

	//---------Sun light and times of day
	float timeOfDay = 0;
	float gameTimer = 0;
	float gameEndTime = 240;
	bool gameEnded = false;

	vec3 currentAmbientColour;
	vec3 currentSunColour;
	vec3 sunTarget;
	float sunDistance;
	vec3 sunPos;
	vec3 sunLightDirection;
	float mainLightIntensity = 0.2;
	float ambientLightIntensity = 0.2;
	float secondsInFullCycle = 45.0f;

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
			vec3(0.6f, 0.45f, 0.3f) * 0.7f,
			8.0f,
			30.0f
		},
		0.17f, // Light intensity 
		0.12f,
		0.01f, //Start time
		0.07f //Ramp up time
	};

	TimeOfDayInfo dayInfo = {
		"day",
		vec3(0.5f, 0.5f, 0.65f), //Ambient light colour
		vec3(1.0f, 1.0f, 1.0f), //Light colour
		{
			vec3(0.5f, 0.65f, 0.9f) * 0.5f, //Fog colour
			10.0f, //Fog start
			30.0f //Fog end
		},
		0.7f, //Light intensity
		0.2f, //Ambient light intensity
		0.09f, //Start time
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
		0.01f, //Light intensity
		0.09f,
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

	//-------Collectables
	struct Collectable {
		string name;
		vec3 location;
		bool isActive;
		GLuint texID;
		unique_ptr<ObjMesh> collectableMesh;
		//model
	};

	int maxCollectables = 3;
	int collectedItems = 0;
	bool collectedAllItems = false;
	Collectable collectables[3] = {
		Collectable {
			"meat",
			vec3(9.55f, 3.6f, -12.4), //mountain
			true,
			meatTexID
		},
		Collectable {
			"cheese",
			vec3(-16, -3.0, -3), //left of map
			true,
			cheeseTexID
		},		
		Collectable {
			"mushroom",
			vec3(13.5, -0.8, 11), //right of map
			true,
			mushroomTexID
		}
	};

	vec3 cookingPotLocation = vec3(-2, 4, -4);
	bool hasCookedItem = false;

	//---------Random number gen
	random_device rd;
	mt19937 gen;
	uniform_real_distribution<float> dis;

	
	//-----Input
	float lastX;
	float lastY;
	bool firstMouse;

	void setMatrices(GLSLProgram& program);
	void compile();
	static void mouseCallback(GLFWwindow* window, double xposIn, double yposIn);
	void processInput(GLFWwindow* window);		
	void attemptPickup();
	vec3 rgbToHsv(vec3 c);
	vec3 hsvToRgb(vec3 hsv);
	vec3 mixHSV(vec3 colorA, vec3 colorB, float t);
public:

	SceneBasic_Uniform();
	void initScene();
	void update(float t);
	void render();
	void initParticleStream();
	void initFireflies();
	void initShadows();
	void initMaterials();	
	void initTextures();
	void initPostProcessing();
	void renderFireflies();
	void renderFireParticles();
	void updateDayNightCycle(float deltaTime);	
	void updateDayNightShaders(TimeOfDayInfo currentState, TimeOfDayInfo toState, float t);
	void drawSolidSceneObjects();
	void resize(int, int);
	};

#endif // SCENEBASIC_UNIFORM_H
