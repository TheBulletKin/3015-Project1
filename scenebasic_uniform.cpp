#include "scenebasic_uniform.h"

#include <cstdio>
#include <cstdlib>

#include <string>
using std::string;

#include<sstream>

#include<vector>

#include <iostream>
using std::cerr;
using std::endl;
#include <glm/gtc/matrix_transform.hpp>
using glm::vec3;
using glm::mat4;

#include "helper/glutils.h"
#include "helper/texture.h"

#include "glm/gtc/random.hpp"


using glm::vec3;

SceneBasic_Uniform::SceneBasic_Uniform() : plane(10.0f, 10.0f, 100, 100)
{
	PigMesh = ObjMesh::load("media/pig_triangulated.obj", true);
	TerrainMesh = ObjMesh::load("media/Terrain.obj", true);
}

void SceneBasic_Uniform::initScene()
{





	compile();

	glEnable(GL_DEPTH_TEST);

	model = mat4(1.0f);
	model = glm::rotate(model, glm::radians(-35.0f), vec3(1.0f, 0.0f, 0.0f));
	view = glm::lookAt(vec3(0.0f, 10.0f, 2.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f)); //First is where the eye is, second is the coordinate it is looking at, last is up vector
	projection = glm::perspective(glm::radians(70.0f), (float)width / height, 0.3f, 100.0f);

	GLuint brickID = Texture::loadTexture("media/texture/brick1.jpg");
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, brickID);

	GLuint mossID = Texture::loadTexture("media/texture/moss.png");
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, mossID);

	prog.setUniform("Kd", vec3(0.9f, 0.5f, 0.3f));
	prog.setUniform("Ka", vec3(0.1f, 0.1f, 0.1f));
	prog.setUniform("Ks", vec3(1.0f, 1.0f, 1.0f));
	prog.setUniform("Roughness", 32.0f);
	prog.setUniform("La", vec3(0.2f, 0.2f, 0.2f));
	prog.setUniform("Ld", vec3(1.0f, 1.0f, 1.0f));


	glm::vec4 light1Pos = glm::vec4(-3.0f, 0.0f, -0.5f, 1.0f); //Left rim 
	glm::vec4 light2Pos = glm::vec4(0.4f, 0.8f, 1.4f, 1.0f); //Fill
	glm::vec4 light3Pos = glm::vec4(0.5f, 1.6f, -0.8f, 1.0f); //Right rim

	// Apply the view transformation to transform into view space
	prog.setUniform("pointLights[0].Position", view * light1Pos);
	prog.setUniform("pointLights[1].Position", view * light2Pos);
	prog.setUniform("pointLights[2].Position", view * light3Pos);

	prog.setUniform("pointLights[0].Ld", vec3(0.8f, 0.8f, 0.8f));
	prog.setUniform("pointLights[1].Ld", vec3(0.8f, 0.8f, 0.8f));
	prog.setUniform("pointLights[2].Ld", vec3(0.8f, 0.8f, 0.8f));


	numberOfStaticLights = 3;

	fireFlySpawnTimer = 0.0f;
	currentFireFlyCount = 0;
	maxFireFlyCount = 2;
	fireFlySpawnCooldown = 3.0f;

	for (int i = numberOfStaticLights; i < maxFireFlyCount; i++)
	{

		string lightUniformTag = "pointLights[" + std::to_string(i) + "]";


		prog.setUniform((lightUniformTag + ".Position").c_str(), glm::vec3(0.0f, 0.0f, 0.0f));
		prog.setUniform((lightUniformTag + ".Ld").c_str(), glm::vec3(0.0f, 0.0f, 0.0f));


	}

	gen = mt19937(rd());
	uniform_real_distribution<> dis(0.0, 1.0);

	FastNoiseLite noise;
	noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	noise.SetFrequency(16.0f);
	noise.SetFractalType(FastNoiseLite::FractalType_FBm);
	noise.SetFractalOctaves(4);
	noise.SetFractalLacunarity(2.8f);
	noise.SetFractalGain(0.5f);
	
	
	
	int textureWidth = 1024;
	int textureHeight = 1024;
	
	std::vector<float> noiseData(textureWidth * textureHeight);
	
	for (int y = 0; y < textureHeight; ++y) {
		for (int x = 0; x < textureWidth; ++x) {
			float nx = float(x) / float(textureWidth);
			float ny = float(y) / float(textureHeight);
			noiseData[y * textureWidth + x] = noise.GetNoise(nx, ny);
		}
	}

	GLuint noiseTexture;
	glGenTextures(1, &noiseTexture);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);

	// Upload the noise data to the texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, textureWidth, textureHeight, 0, GL_RED, GL_FLOAT, &noiseData[0]);

	// Set texture parameters for seamless tiling
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	
	prog.setUniform("CloudTex", 4);
	
	




}

void SceneBasic_Uniform::compile()
{
	try {
		prog.compileShader("shader/basic_uniform.vert");
		prog.compileShader("shader/basic_uniform.frag");
		prog.link();
		prog.use();
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

	if (currentFireFlyCount < maxFireFlyCount)
	{
		fireFlySpawnTimer += deltaTime;
	}
	else {
		fireFlySpawnTimer = 0.0f;
	}
	
	prog.setUniform("time", t * 0.3f);

	if (fireFlySpawnTimer >= fireFlySpawnCooldown && currentFireFlyCount < maxFireFlyCount)
	{
		PointLight* newLight = new PointLight(
			1.0f,
			0.09f,
			0.032f,
			ambientLightColour,
			fireFlyLightColour,
			fireFlyLightColour

		);




		float ySpawnValueMin = 1.5f;
		float ySpawnValue = ySpawnValueMin + (2.0f - ySpawnValueMin) * dis(gen);

		float randomX = topLeftSpawnBound.x + (bottomRightSpawnBound.x - topLeftSpawnBound.x) * dis(gen);
		float randomY = ySpawnValue;
		float randomZ = topLeftSpawnBound.z + (bottomRightSpawnBound.z - topLeftSpawnBound.z) * dis(gen);

		vec3 spawnPosition = vec3(randomX, randomY, randomZ);

		FireFly* newFireFly = new FireFly(newLight, spawnPosition, fireFlies.size());

		fireFlies.push_back(newFireFly);
		currentFireFlyCount++;

		std::cout << "FireFly spawned   Number of lights: " << currentFireFlyCount << "\n";

		fireFlySpawnTimer = 0.0f;

		fireFlySpawnCooldown = linearRand(3.0f, 6.0f);
	}

	for (size_t i = 0; i < fireFlies.size(); i++)
	{
		FireFly* fireFly = fireFlies[i];
		if (fireFly != NULL)
		{
			fireFly->Update(deltaTime);
			if (fireFly->ShouldDestroy())
			{
				int fireFlyLightIndex = i;

				string lightUniformTag = "pointLights[" + std::to_string(fireFlyLightIndex) + "]";


				prog.setUniform((lightUniformTag + ".Position").c_str(), glm::vec3(0.0f, 0.0f, 0.0f));
				prog.setUniform((lightUniformTag + ".Ld").c_str(), glm::vec3(0.0f, 0.0f, 0.0f));


				fireFlies.erase(fireFlies.begin() + i);
				currentFireFlyCount--;




				prog.setUniform("dynamicPointLights", currentFireFlyCount);

				delete fireFly;

				std::cout << "FireFly deleted   Number of lights: " << currentFireFlyCount << "\n";

				i--;
			}
		}
	}


	/* Make a collection for firelies
	*  Timer that counts down
	*  Max number of fireflies
	*  Create new firefly within defined bounds
	* Update all fireflies in this method
	* Moves along random path
	*/
}

void SceneBasic_Uniform::render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	int fireFlyLightIndex = numberOfStaticLights;
	string lightUniformTag;
	for (size_t i = 0; i < fireFlies.size(); i++) {
		FireFly* fireFly = fireFlies[i];
		


		lightUniformTag = ("pointLights[" + to_string(fireFlyLightIndex) + "]");

		//prog.setUniform((lightUniformTag + ".position").c_str(), fireFly->pointLight->position);
	   // prog.setUniform((lightUniformTag + ".ambient").c_str(), fireFly->pointLight->ambient);
	   // prog.setUniform((lightUniformTag + ".diffuse").c_str(), fireFlyLightColour);
	   // prog.setUniform((lightUniformTag + ".specular").c_str(), fireFlyLightColour);
		//prog.setUniform((lightUniformTag + ".constant").c_str(), fireFly->pointLight->constant);
		//prog.setUniform((lightUniformTag + ".linear").c_str(), fireFly->pointLight->linear);
		//prog.setUniform((lightUniformTag + ".quadratic").c_str(), fireFly->pointLight->quadratic);

		prog.setUniform((lightUniformTag + ".Position").c_str(), fireFly->GetPosition());
		//prog.setUniform((lightUniformTag + ".La").c_str(), fireFly->pointLight->ambient);
		//prog.setUniform((lightUniformTag + ".Ld").c_str(), fireFlyLightColour);
		prog.setUniform((lightUniformTag + ".Ld").c_str(), vec3(0.8f, 0.8f, 0.8f));


		fireFlyLightIndex++;
	}

	prog.setUniform("dynamicPointLights", fireFlyLightIndex - numberOfStaticLights);
	prog.setUniform("staticPointLights", numberOfStaticLights);

	prog.setUniform("Material.Kd", 0.4f, 0.4f, 0.4f);
	prog.setUniform("Material.Ks", 0.9f, 0.9f, 0.9f);
	prog.setUniform("Material.Ka", 0.5f, 0.5f, 0.5f);
	prog.setUniform("Material.Shininess", 180.0f);

	model = mat4(1.0f);
	model = glm::rotate(model, glm::radians(90.0f), vec3(0.0f, 10.0f, 0.0f));

	setMatrices();

	PigMesh->render();
	//cube.render();

	model = mat4(1.0f);
	model = glm::translate(model, vec3(0.0f, -0.45, -5.0f));
	setMatrices();
	TerrainMesh->render();



	prog.setUniform("Material.Kd", 0.1f, 0.1f, 0.1f);
	prog.setUniform("Material.Ks", 0.9f, 0.9f, 0.9f);
	prog.setUniform("Material.Ka", 0.1f, 0.1f, 0.1f);
	prog.setUniform("Material.Shininess", 180.0f);

	model = mat4(1.0f);
	model = glm::translate(model, vec3(0.0f, -0.45, 0.0f));

	setMatrices();

	plane.render();

}

void SceneBasic_Uniform::resize(int w, int h)
{
	width = w;
	height = h;
	glViewport(0, 0, w, h);
	projection = glm::perspective(glm::radians(70.0f), (float)w / h, 0.3f, 100.0f);
};

void SceneBasic_Uniform::setMatrices()
{
	mat4 mv;
	mv = view * model;

	prog.setUniform("ModelViewMatrix", mv);
	prog.setUniform("MVP", projection * mv);

	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(mv)));

	prog.setUniform("NormalMatrix", normalMatrix);
	//prog.setUniform("NormalMatrix", glm::mat3(vec3(mv[0]), vec3(mv[1]), vec3(mv[2])));
	prog.setUniform("ViewPos", vec3(0.0f, 0.0f, 0.0f));
}
