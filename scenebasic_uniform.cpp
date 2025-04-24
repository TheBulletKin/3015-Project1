#include "scenebasic_uniform.h"

#include <cstdio>
#include <cstdlib>

#include <string>

#include <sstream>

#include <vector>

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

#include "helper/glutils.h"
#include "helper/texture.h"

#include "glm/gtc/random.hpp"


SceneBasic_Uniform::SceneBasic_Uniform() : sky(100.0f)
{
	TerrainMesh = ObjMesh::load("media/Terrain.obj", true);
	RuinMesh = ObjMesh::load("media/Ruin.obj", true);
}

void SceneBasic_Uniform::initScene()
{
	window = glfwGetCurrentContext();
	glfwSetWindowUserPointer(window, this);
	glfwSetCursorPosCallback(window, SceneBasic_Uniform::mouseCallback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	lastX = width / 2.0f;
	lastY = height / 2.0f;
	firstMouse = true;

	compile();

	glEnable(GL_DEPTH_TEST);

	model = mat4(1.0f);
	model = rotate(model, radians(-35.0f), vec3(1.0f, 0.0f, 0.0f));
	view = lookAt(vec3(0.0f, 1.0f, 2.0f), vec3(0.0f, 1.0f, -4.0f), vec3(0.0f, 1.0f, 0.0f)); //First is where the eye is, second is the coordinate it is looking at, last is up vector
	projection = perspective(radians(70.0f), (float)width / height, 0.3f, 100.0f);

#pragma region Texture Loading
	//Grass texture
	glActiveTexture(GL_TEXTURE0);
	grassTexID = Texture::loadTexture("media/texture/grass_02_1k/grass_02_base_1k.png");

	//Rock texture
	glActiveTexture(GL_TEXTURE1);
	rockTexID = Texture::loadTexture("media/texture/cliff_rocks_02_1k/cliff_rocks_02_baseColor_1k.png");

	//Rock texture
	glActiveTexture(GL_TEXTURE2);
	brickTexID = Texture::loadTexture("media/texture/stone_bricks_wall_04_1k/stone_bricks_wall_04_color_1k.png");

	//Skybox texture
	glActiveTexture(GL_TEXTURE3);
	skyBoxTexID = Texture::loadCubeMap("media/texture/cubeMap/night");

	//Firefly texture
	glActiveTexture(GL_TEXTURE5);
	fireFlyTexID = Texture::loadTexture("media/texture/firefly/fireFlyTex.png");
#pragma endregion


	setupFBO();

#pragma region Post processing quad

	//Define the vertices for the full screen quad, two triangles that cover the whole screen
	GLfloat verts[] = {
		-1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,
	};

	//Texture coordinates for that full screen quad
	GLfloat tc[] = {
		0.0f, 0.0f,
		1.0f, 0.0f,

		1.0f, 1.0f,
		0.0f, 0.0f,

		1.0f, 1.0f,
		0.0f, 1.0f,
	};

	//Creates a VBOs for this quad. Vertex positions and then texture coords passed in
	unsigned int handle[2];
	glGenBuffers(2, handle);
	glBindBuffer(GL_ARRAY_BUFFER, handle[0]);
	glBufferData(GL_ARRAY_BUFFER, 6 * 3 * sizeof(float), verts, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, handle[1]);
	glBufferData(GL_ARRAY_BUFFER, 6 * 2 * sizeof(float), tc, GL_STATIC_DRAW);

	//Creates VAO for the quad. Sets up attribute pointers for vertex position and texture coord, mapped to shader
	glGenVertexArrays(1, &fsQuad);
	glBindVertexArray(fsQuad);
	glBindBuffer(GL_ARRAY_BUFFER, handle[0]);
	glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0); //Vertex position as in shader

	glBindBuffer(GL_ARRAY_BUFFER, handle[1]);
	glVertexAttribPointer((GLuint)2, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(2); //Texture coord as in shader
	glBindVertexArray(0);

#pragma endregion

#pragma region Sampler Setup

	//Set up sampler objects for linear and nearest filtering
	GLuint samplers[2];
	glGenSamplers(2, samplers);
	linearSampler = samplers[0];
	nearestSampler = samplers[1];
	GLfloat border[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	//Nearest sampler
	glSamplerParameteri(nearestSampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glSamplerParameteri(nearestSampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glSamplerParameteri(nearestSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glSamplerParameteri(nearestSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glSamplerParameterfv(nearestSampler, GL_TEXTURE_BORDER_COLOR, border);

	//Linear sampler
	glSamplerParameteri(linearSampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glSamplerParameteri(linearSampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glSamplerParameteri(linearSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glSamplerParameteri(linearSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glSamplerParameterfv(linearSampler, GL_TEXTURE_BORDER_COLOR, border);

	//Means that when sampling from each of these texture units, it uses nearest
	glBindSampler(8, nearestSampler);
	glBindSampler(9, nearestSampler);
	glBindSampler(10, nearestSampler);

#pragma endregion

#pragma region PBR Test
	PBRProg.use();
	PBRProg.setUniform("material.Rough", 0.3f);
	PBRProg.setUniform("material.Metal", 0);
	PBRProg.setUniform("material.Colour", vec3(0.4f));
	PBRProg.setUniform("Light[0].I", vec3(2.0f));
	PBRProg.setUniform("Light[0].Position", vec4(-3.5f, 3.0f, -8.0f, 1));
	PBRProg.setUniform("Light[1].I", vec3(2.0f));
	PBRProg.setUniform("Light[1].Position", vec4(-0.2f, 2.0f, -8.5f, 1));
	PBRProg.setUniform("Light[2].I", vec3(0.2f));
	PBRProg.setUniform("Light[2].Position", vec4(-1.0f, 0.5f, -6.8f, 1));

	/* Example
	PBRProg.setUniform("Light[0].L", vec3(45.0f));
	PBRProg.setUniform("Light[0].Position", view * lightPos);
	PBRProg.setUniform("Light[1].L", vec3(0.3f));
	PBRProg.setUniform("Light[1].Position", vec4(0, 0.15f, -1.0f, 0));
	PBRProg.setUniform("Light[2].L", vec3(45.0f));
	PBRProg.setUniform("Light[2].Position", view * vec4(-7, 3, 7, 1));
	*/

#pragma endregion


#pragma region Material Setup

	terrainProg.use();	
	terrainProg.setUniform("staticPointLights", numberOfStaticLights);
	terrainProg.setUniform("Material.Kd", 0.4f, 0.4f, 0.4f);
	terrainProg.setUniform("Material.Ks", 0.9f, 0.9f, 0.9f);
	terrainProg.setUniform("Material.Ka", 0.5f, 0.5f, 0.5f);
	terrainProg.setUniform("Material.Shininess", 180.0f);

	objectProg.use();	
	objectProg.setUniform("staticPointLights", numberOfStaticLights);
	objectProg.setUniform("Material.Kd", 0.4f, 0.4f, 0.4f);
	objectProg.setUniform("Material.Ks", 0.9f, 0.9f, 0.9f);
	objectProg.setUniform("Material.Ka", 0.5f, 0.5f, 0.5f);
	objectProg.setUniform("Material.Shininess", 180.0f);

#pragma endregion


#pragma region Directional Light Setup

	vec3 lightDirection = normalize(vec3(0.5f, -1.0f, 0.5f));
	vec3 lightAmbient = vec3(0.2f, 0.2f, 0.4f);
	vec3 lightDiffuse = vec3(0.3f, 0.3f, 0.5f);
	vec3 lightSpecular = vec3(1.0f, 1.0f, 1.0f);

	objectProg.use();
	objectProg.setUniform("directionalLight.Direction", lightDirection);
	objectProg.setUniform("directionalLight.La", lightAmbient * 0.8f);
	objectProg.setUniform("directionalLight.Ld", lightDiffuse * 0.8f);
	objectProg.setUniform("directionalLight.Ls", lightSpecular * 0.1f);
	objectProg.setUniform("directionalLight.Enabled", true);
	objectProg.setUniform("FogStart", 5.0f);
	objectProg.setUniform("FogEnd", 80.0f);
	objectProg.setUniform("FogColour", vec3(0.25, 0.31, 0.46));

	terrainProg.use();
	terrainProg.setUniform("directionalLight.Direction", lightDirection);
	terrainProg.setUniform("directionalLight.La", lightAmbient * 1.4f);
	terrainProg.setUniform("directionalLight.Ld", lightDiffuse * 0.8f);
	terrainProg.setUniform("directionalLight.Ls", lightSpecular * 0.1f);
	terrainProg.setUniform("directionalLight.Enabled", true);
	terrainProg.setUniform("FogStart", 5.0f);
	terrainProg.setUniform("FogEnd", 80.0f);
	terrainProg.setUniform("FogColour", vec3(0.25, 0.31, 0.46));

#pragma endregion

#pragma region Point Light Setup

	fireFlySpawnTimer = 0.0f;
	currentFireFlyCount = 0;
	maxFireFlyCount = 10;
	fireFlySpawnCooldown = 2.0f;

	for (int i = 0; i < maxFireFlyCount; i++)
	{
		string lightUniformTag = "pointLights[" + to_string(i) + "]";

		terrainProg.use();
		terrainProg.setUniform((lightUniformTag + ".Position").c_str(), vec3(0.0f, 0.0f, 0.0f));
		terrainProg.setUniform((lightUniformTag + ".Ld").c_str(), vec3(0.0f, 0.0f, 0.0f));
		terrainProg.setUniform((lightUniformTag + ".La").c_str(), vec3(0.0f, 0.0f, 0.0f));

		objectProg.use();
		objectProg.setUniform((lightUniformTag + ".Position").c_str(), vec3(0.0f, 0.0f, 0.0f));
		objectProg.setUniform((lightUniformTag + ".Ld").c_str(), vec3(0.0f, 0.0f, 0.0f));
		objectProg.setUniform((lightUniformTag + ".La").c_str(), vec3(0.0f, 0.0f, 0.0f));

	}
#pragma endregion

#pragma region Cloud Texture

	//Used for random number generation
	gen = mt19937(rd());
	uniform_real_distribution<> dis(0.0, 1.0);

	FastNoiseLite noise;
	noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	noise.SetFrequency(16.0f);
	noise.SetFractalType(FastNoiseLite::FractalType_FBm);
	noise.SetFractalOctaves(4);
	noise.SetFractalLacunarity(2.8f);
	noise.SetFractalGain(0.5f);

	int textureWidth = 2048;
	int textureHeight = 2048;

	vector<float> noiseData(textureWidth * textureHeight);

	for (int y = 0; y < textureHeight; ++y) {
		for (int x = 0; x < textureWidth; ++x) {
			float nx = float(x) / float(textureWidth);
			float ny = float(y) / float(textureHeight);
			noiseData[y * textureWidth + x] = noise.GetNoise(nx, ny);
		}
	}

	glGenTextures(1, &cloudTexID);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, cloudTexID);

	//Upload the noise data to the texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, textureWidth, textureHeight, 0, GL_RED, GL_FLOAT, &noiseData[0]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, cloudTexID);

#pragma endregion

#pragma region Firefly Sprites
	glGenVertexArrays(1, &spritesVAO);
	glGenBuffers(1, &spritesInstanceVBO);

	glBindVertexArray(spritesVAO);

	// Firefly positions (instance buffer)
	glBindBuffer(GL_ARRAY_BUFFER, spritesInstanceVBO);
	glBufferData(GL_ARRAY_BUFFER, maxFireFlyCount * sizeof(vec3), nullptr, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribDivisor(1, 1);

	glBindVertexArray(0);

	particleProg.use();
	particleProg.setUniform("Size2", 0.15f);
#pragma endregion


}

void SceneBasic_Uniform::compile()
{
	try {		
		objectProg.compileShader("shader/object.vert");
		objectProg.compileShader("shader/object.frag");
		objectProg.link();
		skyProg.compileShader("shader/skybox.vert");
		skyProg.compileShader("shader/skybox.frag");
		skyProg.link();
		screenHdrProg.compileShader("shader/screenHdr.vert");
		screenHdrProg.compileShader("shader/screenHdr.frag");
		screenHdrProg.link();
		particleProg.compileShader("shader/particle.vert");
		particleProg.compileShader("shader/particle.frag");
		particleProg.compileShader("shader/particle.geom");
		particleProg.link();
		terrainProg.compileShader("shader/terrain.vert");
		terrainProg.compileShader("shader/terrain.frag");
		terrainProg.link();
		PBRProg.compileShader("shader/PBR.vert");
		PBRProg.compileShader("shader/PBR.frag");
		PBRProg.link();

	}
	catch (GLSLProgramException& e) {
		cerr << e.what() << endl;
		exit(EXIT_FAILURE);
	}
}

void SceneBasic_Uniform::update(float t)
{
	deltaTime = t - lastFrameTime;
	lastFrameTime = t;

	processInput(window);

	/*
	if (currentFireFlyCount < maxFireFlyCount)
	{
		fireFlySpawnTimer += deltaTime;
	}
	else {
		fireFlySpawnTimer = 0.0f;
	}*/

	terrainProg.use();
	terrainProg.setUniform("time", t / 1000);

#pragma region New Firefly Spawning
	/*
	if (fireFlySpawnTimer >= fireFlySpawnCooldown && currentFireFlyCount < maxFireFlyCount)
	{
		PointLight* newLight = new PointLight(
			1.0f,
			0.8f,
			0.7f,
			ambientLightColour * 0.5f,
			fireFlyLightColour * 3.2f,
			fireFlyLightColour * 2.6f

		);

		//Random spawn location
		float ySpawnValueMin = 1.5f;
		float ySpawnValue = ySpawnValueMin + (2.0f - ySpawnValueMin) * dis(gen);

		float randomX = topLeftSpawnBound.x + (bottomRightSpawnBound.x - topLeftSpawnBound.x) * dis(gen);
		float randomY = ySpawnValue;
		float randomZ = topLeftSpawnBound.z + (bottomRightSpawnBound.z - topLeftSpawnBound.z) * dis(gen);

		vec3 spawnPosition = vec3(randomX, randomY, randomZ);

		FireFly* newFireFly = new FireFly(newLight, spawnPosition);

		fireFlies.push_back(newFireFly);
		currentFireFlyCount++;

		cout << "FireFly spawned   Number of lights: " << currentFireFlyCount << "\n";

		fireFlySpawnTimer = 0.0f;
		fireFlySpawnCooldown = linearRand(3.0f, 6.0f);
	}*/
#pragma endregion

#pragma region Firefly Update and Deletion
	/*
	for (size_t i = 0; i < fireFlies.size(); i++)
	{
		FireFly* fireFly = fireFlies[i];
		if (fireFly != NULL)
		{
			fireFly->Update(deltaTime);
			if (fireFly->ShouldDestroy())
			{
				int fireFlyLightIndex = i + numberOfStaticLights;

				string lightUniformTag = "pointLights[" + to_string(fireFlyLightIndex) + "]";

				//Set deleted pointlights to a null value equivalent to be ignored in the shader
				terrainProg.use();
				terrainProg.setUniform((lightUniformTag + ".Position").c_str(), vec3(0.0f, 0.0f, 0.0f));
				terrainProg.setUniform((lightUniformTag + ".Ld").c_str(), vec3(0.0f, 0.0f, 0.0f));
				terrainProg.setUniform((lightUniformTag + ".La").c_str(), vec3(0.0f, 0.0f, 0.0f));
				terrainProg.setUniform((lightUniformTag + ".Enabled").c_str(), false);

				objectProg.use();
				objectProg.setUniform((lightUniformTag + ".Position").c_str(), vec3(0.0f, 0.0f, 0.0f));
				objectProg.setUniform((lightUniformTag + ".Ld").c_str(), vec3(0.0f, 0.0f, 0.0f));
				objectProg.setUniform((lightUniformTag + ".La").c_str(), vec3(0.0f, 0.0f, 0.0f));
				objectProg.setUniform((lightUniformTag + ".Enabled").c_str(), false);

				delete fireFly;
				fireFlies.erase(fireFlies.begin() + i);
				currentFireFlyCount--;


				terrainProg.use();
				terrainProg.setUniform("dynamicPointLights", currentFireFlyCount);

				objectProg.use();
				objectProg.setUniform("dynamicPointLights", currentFireFlyCount);

				cout << "FireFly deleted   Number of lights: " << currentFireFlyCount << "\n";

				i--;
			}
		}
	}*/
#pragma endregion

}

void SceneBasic_Uniform::render()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
#pragma region Sky Rendering

	view = lookAt(vec3(0.0f, 0.0f, 0.0f), camera.Front, camera.Up);
	glEnable(GL_DEPTH_TEST);
	skyProg.use();
	glDepthMask(GL_FALSE);
	model = mat4(1.0f);
	setMatrices(skyProg);
	sky.render();
	glDepthMask(GL_TRUE);

#pragma endregion

	view = camera.GetViewMatrix();
	projection = perspective(radians(70.0f), (float)width / height, 0.3f, 100.0f);

#pragma region Firefly Rendering

	vector<vec3> fireFlyPositions;
	int fireFlyLightIndex = numberOfStaticLights;
	string lightUniformTag;
	for (size_t i = 0; i < fireFlies.size(); i++) {
		FireFly* fireFly = fireFlies[i];

		lightUniformTag = ("pointLights[" + to_string(fireFlyLightIndex) + "]");
		terrainProg.use();
		terrainProg.setUniform((lightUniformTag + ".Constant").c_str(), fireFly->pointLight->constant);
		terrainProg.setUniform((lightUniformTag + ".Linear").c_str(), fireFly->pointLight->linear);
		terrainProg.setUniform((lightUniformTag + ".Quadratic").c_str(), fireFly->pointLight->quadratic);

		terrainProg.setUniform((lightUniformTag + ".Position").c_str(), fireFly->GetPosition());
		terrainProg.setUniform((lightUniformTag + ".La").c_str(), fireFly->pointLight->ambient * fireFly->pointLight->brightness);
		terrainProg.setUniform((lightUniformTag + ".Ld").c_str(), fireFly->pointLight->diffuse * fireFly->pointLight->brightness);
		terrainProg.setUniform((lightUniformTag + ".Enabled").c_str(), true);

		objectProg.use();		
		objectProg.setUniform((lightUniformTag + ".Constant").c_str(), fireFly->pointLight->constant);
		objectProg.setUniform((lightUniformTag + ".Linear").c_str(), fireFly->pointLight->linear);
		objectProg.setUniform((lightUniformTag + ".Quadratic").c_str(), fireFly->pointLight->quadratic);

		objectProg.setUniform((lightUniformTag + ".Position").c_str(), fireFly->GetPosition());
		objectProg.setUniform((lightUniformTag + ".La").c_str(), fireFly->pointLight->ambient * fireFly->pointLight->brightness);		
		objectProg.setUniform((lightUniformTag + ".Ld").c_str(), fireFly->pointLight->diffuse * fireFly->pointLight->brightness);
		objectProg.setUniform((lightUniformTag + ".Enabled").c_str(), true);

		fireFlyLightIndex++;
		fireFlyPositions.push_back(fireFly->GetPosition());
	}

	glBindVertexArray(spritesVAO);
	glBindBuffer(GL_ARRAY_BUFFER, spritesInstanceVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, fireFlyPositions.size() * sizeof(vec3), fireFlyPositions.data());
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	particleProg.use();
	model = mat4(1.0f);
	setMatrices(particleProg);

	glDrawArraysInstanced(GL_POINTS, 0, 1, fireFlies.size());

#pragma endregion

#pragma region Shader Material Update

	terrainProg.use();
	terrainProg.setUniform("dynamicPointLights", fireFlyLightIndex - numberOfStaticLights);
	terrainProg.setUniform("staticPointLights", numberOfStaticLights);

	terrainProg.setUniform("Material.Kd", 0.4f, 0.4f, 0.4f);
	terrainProg.setUniform("Material.Ks", 0.9f, 0.9f, 0.9f);
	terrainProg.setUniform("Material.Ka", 0.5f, 0.5f, 0.5f);
	terrainProg.setUniform("Material.Shininess", 180.0f);

	objectProg.use();
	objectProg.setUniform("dynamicPointLights", fireFlyLightIndex - numberOfStaticLights);
	objectProg.setUniform("staticPointLights", numberOfStaticLights);

	objectProg.setUniform("Material.Kd", 0.4f, 0.4f, 0.4f);
	objectProg.setUniform("Material.Ks", 0.9f, 0.9f, 0.9f);
	objectProg.setUniform("Material.Ka", 0.5f, 0.5f, 0.5f);
	objectProg.setUniform("Material.Shininess", 180.0f);

#pragma endregion

#pragma region Object Rendering

	//Ruin Rendering
	//objectProg.use();
	PBRProg.use();

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, brickTexID);

	model = mat4(1.0f);
	model = scale(model, vec3(0.3f, 0.3f, 0.3f));
	model = translate(model, vec3(-7.0f, 4.0f, -27.0f));
	setMatrices(PBRProg);
	//setMatrices(objectProg);
	RuinMesh->render();

	//Terrain rendering
	terrainProg.use();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, grassTexID);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, rockTexID);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, cloudTexID);

	model = mat4(1.0f);
	model = rotate(model, radians(0.0f), vec3(0.0f, 1.0f, 0.0f));
	model = scale(model, vec3(0.25f, 0.25f, 0.25f));
	model = translate(model, vec3(0.0f, -3.0f, -15.0f));
	setMatrices(terrainProg);
	//TerrainMesh->render();

#pragma endregion

#pragma region HDR Pass

	//Second pass - HDR		
	/*
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST);
	//glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	//glClear(GL_COLOR_BUFFER_BIT);
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
	*/
#pragma endregion
	
}

void SceneBasic_Uniform::resize(int w, int h)
{
	width = w;
	height = h;
	glViewport(0, 0, w, h);
	projection = perspective(radians(70.0f), (float)w / h, 0.3f, 100.0f);
};

void SceneBasic_Uniform::setMatrices(GLSLProgram& program)
{
	mat4 mv;
	mv = view * model;
	program.use();
	program.setUniform("ModelViewMatrix", mv);
	program.setUniform("MVP", projection * mv);
	program.setUniform("model", model);
	program.setUniform("view", view);
	program.setUniform("viewPos", camera.Position);
	program.setUniform("projection", projection);

	mat3 normalMatrix = transpose(inverse(mat3(mv)));

	program.setUniform("NormalMatrix", normalMatrix);
	
	program.setUniform("ViewPos", camera.Position);
}

void SceneBasic_Uniform::mouseCallback(GLFWwindow* window, double xposIn, double yposIn)
{
	SceneBasic_Uniform* instance = static_cast<SceneBasic_Uniform*>(glfwGetWindowUserPointer(window));

	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (instance->firstMouse)
	{
		instance->lastX = xpos;
		instance->lastY = ypos;
		instance->firstMouse = false;
	}

	float xoffset = xpos - instance->lastX;
	float yoffset = instance->lastY - ypos; // reversed since y-coordinates go from bottom to top

	instance->lastX = xpos;
	instance->lastY = ypos;


	instance->camera.ProcessMouseMovement(xoffset, yoffset);
}

void SceneBasic_Uniform::processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);

}

void SceneBasic_Uniform::setupFBO() {
	/* EXPLANATION
	OpenGL usually renders to a default framebuffer which is the screen.
	* Can alternatively render to a texture instead.
	* That texture is used for a second pass where post processing is applied.
	*
	* 1. Create the frame buffer object like regular texture or vertex buffers
	* 2. Need to generate a texture that the rendered image will be projected onto. Same size as the window
	* 3. Assigning renderTex as the framebuffer texture means anything rendered to the FBO will be stored in renderTex
	* 4. Depth buffer is sort of like a texture, it holds data from the rendered scene, but cannot be sampled in the same way. Need to tell OpenGL to hold depth information in the buffer
	* 5. Tell openGL that when rendering to the frame buffer, write colours into colour attachment 0.
	*/

	//Create FBO for first render pass
	glGenFramebuffers(1, &hdrFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

	//Create blank texture to render to and set it as the FBO target
	glGenTextures(1, &renderTex);
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, renderTex);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, width, height);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glFramebufferTexture2D(
		GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTex, 0
	);

	//Render buffer object for depth and stencil attachments that won't be sampled by me
	glGenRenderbuffers(1, &depthRbo);
	glBindRenderbuffer(GL_RENDERBUFFER, depthRbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRbo);

	//Error checking
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}




