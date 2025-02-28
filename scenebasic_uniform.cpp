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

SceneBasic_Uniform::SceneBasic_Uniform() : plane(10.0f, 10.0f, 100, 100), sky(100.0f)
{
	PigMesh = ObjMesh::load("media/pig_triangulated.obj", true);
	TerrainMesh = ObjMesh::load("media/Terrain.obj", true);
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
	model = glm::rotate(model, glm::radians(-35.0f), vec3(1.0f, 0.0f, 0.0f));
	view = glm::lookAt(vec3(0.0f, 1.0f, 2.0f), vec3(0.0f, 1.0f, -4.0f), vec3(0.0f, 1.0f, 0.0f)); //First is where the eye is, second is the coordinate it is looking at, last is up vector
	projection = glm::perspective(glm::radians(70.0f), (float)width / height, 0.3f, 100.0f);

	//Grass texture
	grassID = Texture::loadTexture("media/texture/grass_02_1k/grass_02_base_1k.png");
	

	//Rock texture
	rockID = Texture::loadTexture("media/texture/cliff_rocks_02_1k/cliff_rocks_02_baseColor_1k.png");
	

	skyCubeID = Texture::loadHdrCubeMap("media/texture/cube/pisa-hdr/pisa");
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyCubeID);

	setupFBO();


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

	

	

	//Gaussian blur
	/* Weights follow a gaussian distribution, falling off the further it gets from the centre
	* In the shader this is used to determine how much a neighbouring pixel contributes to blur
	*/



	


	

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

	terrainProg.use();
	terrainProg.setUniform("Kd", vec3(0.9f, 0.5f, 0.3f));
	terrainProg.setUniform("Ka", vec3(0.1f, 0.1f, 0.1f));
	terrainProg.setUniform("Ks", vec3(1.0f, 1.0f, 1.0f));
	terrainProg.setUniform("Roughness", 32.0f);
	terrainProg.setUniform("La", vec3(0.2f, 0.2f, 0.2f));
	terrainProg.setUniform("Ld", vec3(1.0f, 1.0f, 1.0f));


	glm::vec4 light1Pos = glm::vec4(-3.0f, 0.0f, -0.5f, 1.0f); //Left rim 
	glm::vec4 light2Pos = glm::vec4(0.4f, 0.8f, 1.4f, 1.0f); //Fill
	glm::vec4 light3Pos = glm::vec4(0.5f, 1.6f, -0.8f, 1.0f); //Right rim

	// Apply the view transformation to transform into view space
	terrainProg.setUniform("pointLights[0].Position", view * light1Pos);
	terrainProg.setUniform("pointLights[1].Position", view * light2Pos);
	terrainProg.setUniform("pointLights[2].Position", view * light3Pos);

	terrainProg.setUniform("pointLights[0].Ld", vec3(0.8f, 0.8f, 0.8f));
	terrainProg.setUniform("pointLights[1].Ld", vec3(0.8f, 0.8f, 0.8f));
	terrainProg.setUniform("pointLights[2].Ld", vec3(0.8f, 0.8f, 0.8f));


	numberOfStaticLights = 3;

	fireFlySpawnTimer = 0.0f;
	currentFireFlyCount = 0;
	maxFireFlyCount = 3;
	fireFlySpawnCooldown = 3.0f;

	for (int i = numberOfStaticLights; i < maxFireFlyCount; i++)
	{

		string lightUniformTag = "pointLights[" + std::to_string(i) + "]";


		terrainProg.setUniform((lightUniformTag + ".Position").c_str(), glm::vec3(0.0f, 0.0f, 0.0f));
		terrainProg.setUniform((lightUniformTag + ".Ld").c_str(), glm::vec3(0.0f, 0.0f, 0.0f));
		terrainProg.setUniform((lightUniformTag + ".La").c_str(), glm::vec3(0.0f, 0.0f, 0.0f));


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



	int textureWidth = 2048;
	int textureHeight = 2048;

	std::vector<float> noiseData(textureWidth * textureHeight);

	for (int y = 0; y < textureHeight; ++y) {
		for (int x = 0; x < textureWidth; ++x) {
			float nx = float(x) / float(textureWidth);
			float ny = float(y) / float(textureHeight);
			noiseData[y * textureWidth + x] = noise.GetNoise(nx, ny);
		}
	}

	//Cloud texture in unit 4
	
	glGenTextures(1, &cloudID);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, cloudID);

	// Upload the noise data to the texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, textureWidth, textureHeight, 0, GL_RED, GL_FLOAT, &noiseData[0]);

	// Set texture parameters for seamless tiling
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


	





	glGenVertexArrays(1, &spritesVAO);
	glGenBuffers(1, &spritesInstanceVBO);

	glBindVertexArray(spritesVAO);


	// Firefly positions (instance buffer)
	glBindBuffer(GL_ARRAY_BUFFER, spritesInstanceVBO);
	glBufferData(GL_ARRAY_BUFFER, maxFireFlyCount * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribDivisor(1, 1); // 1 instance per particle

	glBindVertexArray(0);

	GLuint spriteTex = Texture::loadTexture("media/texture/flower.png");
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, spriteTex);


	particleProg.use();	
	particleProg.setUniform("Size2", 0.15f);




}

void SceneBasic_Uniform::compile()
{
	try {
		//prog.compileShader("shader/basic_uniform.vert");
		//prog.compileShader("shader/basic_uniform.frag");
		prog.compileShader("shader/object.vert");
		prog.compileShader("shader/object.frag");
		prog.link();
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

	if (currentFireFlyCount < maxFireFlyCount)
	{
		fireFlySpawnTimer += deltaTime;
	}
	else {
		fireFlySpawnTimer = 0.0f;
	}

	terrainProg.use();
	terrainProg.setUniform("time", t / 1000);

	if (fireFlySpawnTimer >= fireFlySpawnCooldown && currentFireFlyCount < maxFireFlyCount)
	{
		PointLight* newLight = new PointLight(
			1.0f,
			0.8f,
			0.7f,
			ambientLightColour * 0.5f,
			fireFlyLightColour * 5.0f,
			fireFlyLightColour * 5.0f

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
				int fireFlyLightIndex = i + numberOfStaticLights;

				string lightUniformTag = "pointLights[" + std::to_string(fireFlyLightIndex) + "]";


				terrainProg.setUniform((lightUniformTag + ".Position").c_str(), glm::vec3(0.0f, 0.0f, 0.0f));
				terrainProg.setUniform((lightUniformTag + ".Ld").c_str(), glm::vec3(0.0f, 0.0f, 0.0f));
				terrainProg.setUniform((lightUniformTag + ".La").c_str(), glm::vec3(0.0f, 0.0f, 0.0f));
				terrainProg.setUniform((lightUniformTag + ".Enabled").c_str(), false);

				delete fireFly;
				fireFlies.erase(fireFlies.begin() + i);
				currentFireFlyCount--;




				terrainProg.setUniform("dynamicPointLights", currentFireFlyCount);




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
	glEnable(GL_DEPTH_TEST);

	view = lookAt(vec3(0.0f, 0.0f, 0.0f), camera.Front, camera.Up);

	skyProg.use();
	glDepthMask(GL_FALSE);
	model = mat4(1.0f);
	setMatrices(skyProg);
	sky.render();
	glDepthMask(GL_TRUE);


	pass1();



}

void SceneBasic_Uniform::resize(int w, int h)
{
	width = w;
	height = h;
	glViewport(0, 0, w, h);
	projection = glm::perspective(glm::radians(70.0f), (float)w / h, 0.3f, 100.0f);
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

	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(mv)));

	program.setUniform("NormalMatrix", normalMatrix);
	//prog.setUniform("NormalMatrix", glm::mat3(vec3(mv[0]), vec3(mv[1]), vec3(mv[2])));
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

	/* Refactor plan:
	* Generate the HDR frame buffer
	* Generate hdr texture (where base scene is first rendered to
	* First render to a screen texture without any editing
	* Use two shaders for this. Just to prove base functionality
	*/

	//Create FBO for first render pass
	//glGenFramebuffers(1, &hdrFBO);
	//glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

	/* Before, the shader rendering the scene only had a frag colour output
	* For bloom to work you need the base scene and then the bright colours.
	* Can add 'brightColour' as an output colour in the shader so it can pass that along in one render pass
	* This creates two textures that are 'linked' to that one frame buffer
	* Since the shader now has layout locations 0 and 1 for fragColour and brightColour,
	*  the texture assigned to colour attachment 0 will hold the fragColour,
	*  and the texture assigned to colour attachment 1 will hold the brightColour
	* (look in shader for more info)
	* Texture unit 8 holds first, 9 holds bright pixels
	*/

	/*
	glGenTextures(1, &renderTex);
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, renderTex);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glFramebufferTexture2D(
		GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTex, 0
	);
	*/

	//Attach this texture to the frame buffer
	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTex, 0);


	/*
	//Render buffer object for depth and stencil attachments that won't be sampled by me
	glGenRenderbuffers(1, &depthRbo);
	glBindRenderbuffer(GL_RENDERBUFFER, depthRbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRbo);
	*/
	//Error checking
	//if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		//cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneBasic_Uniform::pass1() {
	/* First pass undergoes the old render process, simply draws all objects to the scene as was originally required
	*
	*/

	//Bind HDR framebuffer to render to hdrTex;


	view = camera.GetViewMatrix();
	projection = glm::perspective(glm::radians(70.0f), (float)width / height, 0.3f, 100.0f);
	
	terrainProg.use();
	vector<vec3> fireFlyPositions;
	int fireFlyLightIndex = numberOfStaticLights;
	string lightUniformTag;
	for (size_t i = 0; i < fireFlies.size(); i++) {
		FireFly* fireFly = fireFlies[i];



		lightUniformTag = ("pointLights[" + to_string(fireFlyLightIndex) + "]");

		//prog.setUniform((lightUniformTag + ".position").c_str(), fireFly->pointLight->position);
	   // prog.setUniform((lightUniformTag + ".ambient").c_str(), fireFly->pointLight->ambient);
	   // prog.setUniform((lightUniformTag + ".diffuse").c_str(), fireFlyLightColour);
	   // prog.setUniform((lightUniformTag + ".specular").c_str(), fireFlyLightColour);
		terrainProg.setUniform((lightUniformTag + ".Constant").c_str(), fireFly->pointLight->constant);
		terrainProg.setUniform((lightUniformTag + ".Linear").c_str(), fireFly->pointLight->linear);
		terrainProg.setUniform((lightUniformTag + ".Quadratic").c_str(), fireFly->pointLight->quadratic);

		terrainProg.setUniform((lightUniformTag + ".Position").c_str(), fireFly->GetPosition());
		terrainProg.setUniform((lightUniformTag + ".La").c_str(), fireFly->pointLight->ambient * fireFly->pointLight->brightness);
		//prog.setUniform((lightUniformTag + ".Ld").c_str(), fireFlyLightColour);
		terrainProg.setUniform((lightUniformTag + ".Ld").c_str(), fireFly->pointLight->diffuse * fireFly->pointLight->brightness);
		terrainProg.setUniform((lightUniformTag + ".Enabled").c_str(), true);

		fireFlyLightIndex++;
		fireFlyPositions.push_back(fireFly->GetPosition());
	}

	glBindVertexArray(spritesVAO);
	glBindBuffer(GL_ARRAY_BUFFER, spritesInstanceVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, fireFlyPositions.size() * sizeof(glm::vec3), fireFlyPositions.data());
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	particleProg.use();
	model = mat4(1.0f);
	setMatrices(particleProg);

	
	glDrawArraysInstanced(GL_POINTS, 0, 1, fireFlies.size());

	
	terrainProg.use();
	terrainProg.setUniform("dynamicPointLights", fireFlyLightIndex - numberOfStaticLights);
	terrainProg.setUniform("staticPointLights", numberOfStaticLights);

	terrainProg.setUniform("Material.Kd", 0.4f, 0.4f, 0.4f);
	terrainProg.setUniform("Material.Ks", 0.9f, 0.9f, 0.9f);
	terrainProg.setUniform("Material.Ka", 0.5f, 0.5f, 0.5f);
	terrainProg.setUniform("Material.Shininess", 180.0f);

	model = mat4(1.0f);
	model = glm::rotate(model, glm::radians(90.0f), vec3(0.0f, 10.0f, 0.0f));
	setMatrices(terrainProg);
	PigMesh->render();
	//cube.render();

	//Terrain rendering
	terrainProg.use();	
	model = mat4(1.0f);
	model = glm::translate(model, vec3(0.0f, -0.45, -5.0f));
	setMatrices(terrainProg);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, grassID);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, cloudID);
	TerrainMesh->render();


	//prog.use();
	//prog.setUniform("Material.Kd", 0.1f, 0.1f, 0.1f);
	//prog.setUniform("Material.Ks", 0.9f, 0.9f, 0.9f);
	//prog.setUniform("Material.Ka", 0.1f, 0.1f, 0.1f);
	//prog.setUniform("Material.Shininess", 64.0f);

	//Plane rendering
	model = mat4(1.0f);
	model = glm::translate(model, vec3(0.0f, -0.45, 0.0f));
	setMatrices(prog);
	plane.render();

	

	//Second pass - HDR	
	/*
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	screenHdrProg.use();
	model = mat4(1.0f);
	view = mat4(1.0f);
	projection = mat4(1.0f);
	setMatrices(screenHdrProg);
	screenHdrProg.setUniform("hdr", hdr);
	screenHdrProg.setUniform("exposure", exposure);
	glBindVertexArray(fsQuad);
	glActiveTexture(GL_TEXTURE8); //Gaussian blur needs texture unit 8
	glBindTexture(GL_TEXTURE_2D, renderTex);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	*/

}



float SceneBasic_Uniform::gauss(float x, float sigma2) {
	double coeff = 1.0 / (two_pi<double>() * sigma2);
	double expon = -(x * x) / (2.0 * sigma2);
	return (float)(coeff * exp(expon));
}

void SceneBasic_Uniform::computeLogAveLuminance() {
	int size = width * height;
	vector<GLfloat> texData(size * 3);
	//glActiveTexture(GL_TEXTURE8);
	//glBindTexture(GL_TEXTURE_2D, renderTex);
	//glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, texData.data());
	float sum = 0.0f;
	for (int i = 0; i < size; i++)
	{
		float lum = dot(vec3(texData[i * 3 + 0], texData[i * 3 + 1], texData[i * 3 + 2]),
			vec3(0.2126f, 0.7152f, 0.0722f));
		sum += logf(lum + 0.00001f);
	}

	prog.setUniform("AveLum", expf(sum / size));
}
