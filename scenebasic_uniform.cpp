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
#include "helper/particleutils.h"
#include "helper/random.h"

#include "glm/gtc/random.hpp"


SceneBasic_Uniform::SceneBasic_Uniform() : sky(100.0f)
{
	TerrainMesh = ObjMesh::load("media/Terrain.obj", true);
	RuinMesh = ObjMesh::load("media/Ruin.obj", true);
	StandingTorch = ObjMesh::load("media/StandingTorch.obj", true);
	collectables[0].collectableMesh = ObjMesh::load("media/meat.obj", true);
	collectables[1].collectableMesh = ObjMesh::load("media/cheese.obj", true);
	collectables[2].collectableMesh = ObjMesh::load("media/mushroom.obj", true);
	campfireMesh = ObjMesh::load("media/campfire.obj", true);

}

void SceneBasic_Uniform::initScene()
{

	//------ Window and openGL setup
	window = glfwGetCurrentContext();
	glfwSetWindowUserPointer(window, this);
	glfwSetCursorPosCallback(window, SceneBasic_Uniform::mouseCallback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	lastX = width / 2.0f;
	lastY = height / 2.0f;
	firstMouse = true;

	compile();

	glEnable(GL_DEPTH_TEST);	

	timeOfDay = 0;

	//------ General initialisation
	initTextures();	
	initPostProcessing();
	initMaterials();
	initParticleStream();
	initFireflies();
	initShadows();	


	//----Console message
	cout << endl << endl << "-----WECOME------" << endl << "Welcome to this serene stylised landscape, enjoy the view and atmosphere as the sun rolls across the sky." << endl;
	cout << "There is a cooking pot in the nearby ruin, as well as 3 ingredients scattered around the map:" << endl;
	cout << "- One mushroom" << endl << "- One meat" << endl << "- One slice of cheese" << endl;
	cout << "Move around and collect these items. When near one, press E to collect it." << endl;
	cout << "Find all 3 ingredients and then return to the pot, press E again to cook something and win the game!" << endl;
	cout << "However, you only have until the next morning to find these items. Get searching!" << endl << endl;

	glDisable(GL_DEBUG_OUTPUT);

	torchNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	torchNoise.SetFrequency(0.5f);
}

void SceneBasic_Uniform::compile()
{
	try {
		//Used only for light frustum
		objectProg.compileShader("shader/object.vert");
		objectProg.compileShader("shader/object.frag");
		objectProg.link();
		skyProg.compileShader("shader/skybox.vert");
		skyProg.compileShader("shader/skybox.frag");
		skyProg.link();
		screenHdrProg.compileShader("shader/screenHdr.vert");
		screenHdrProg.compileShader("shader/screenHdr.frag");
		screenHdrProg.link();
		//Old particle shaders
		//particleProg.compileShader("shader/particle.vert");
		//particleProg.compileShader("shader/particle.frag");
		//particleProg.compileShader("shader/particle.geom");
		//particleProg.link();
		terrainProg.compileShader("shader/terrainPBR.vert");
		terrainProg.compileShader("shader/terrainPBR.frag");
		terrainProg.link();
		PBRProg.compileShader("shader/PBR.vert");
		PBRProg.compileShader("shader/PBR.frag");
		PBRProg.link();
		particleStreamProg.compileShader("shader/particleStream.vert");
		particleStreamProg.compileShader("shader/particleStream.frag");

		//Transform feedback for newParticleProg
		GLuint progHandle = particleStreamProg.getHandle();
		const char* outputNames[] = { "Position", "Velocity", "Age" };
		glTransformFeedbackVaryings(progHandle, 3, outputNames, GL_SEPARATE_ATTRIBS);

		particleStreamProg.link();
		//shadowProg.compileShader("shader/Shadow.vert");
		//shadowProg.compileShader("shader/Shadow.frag");
		//shadowProg.link();
		fireflyParticleProg.compileShader("shader/fireflyParticle.vert");
		fireflyParticleProg.compileShader("shader/fireflyParticle.frag");
		fireflyParticleProg.link();

	}
	catch (GLSLProgramException& e) {
		cerr << e.what() << endl;
		exit(EXIT_FAILURE);
	}
}

void SceneBasic_Uniform::update(float t)
{
	//---- Time passage
	deltaTime = t - lastFrameTime;
	lastFrameTime = t;

	updateDayNightCycle(deltaTime);
	
	if (t > gameEndTime && gameEnded == false) {
		cout << "game over" << endl;
		gameEnded = true;
	}

	if (currentFireFlyCount < maxFireFlyCount && timeOfDay >= 1.0)
	{
		fireFlySpawnTimer += deltaTime;
	}
	else {
		fireFlySpawnTimer = 0.0f;
	}

	terrainProg.use();
	terrainProg.setUniform("time", t / 1000);

	skyProg.use();
	skyProg.setUniform("Time", t);
	skyProg.setUniform("FullCycleDuration", secondsInFullCycle);


	//----------Input
	processInput(window);

	
	//------ Torch light and shader updates
	
	int i = 0;
	for (const TorchInfo& torch : torches)
	{
		float noiseInput = t * 2.0f;
		float noiseVal = torchNoise.GetNoise(noiseInput, 0.0f); // returns value in [-1, 1]
		float normalized = (noiseVal + 1.0f) / 2.0f; // convert to [0, 1]
		float mixed = mix(torchMinIntensity, torchMaxIntensity, normalized);

		//Update PBR and terrain shaders with updated intensities and positions
		string arrayString = "Light[" + to_string(i) + "].Intensity";
		PBRProg.use();
		PBRProg.setUniform(arrayString.c_str(), torchBrightColour * 1.0f * mixed);
		terrainProg.use();
		terrainProg.setUniform(arrayString.c_str(), torchBrightColour * 1.0f * mixed);

		arrayString = "Light[" + to_string(i) + "].Position";
		vec3 shiftedPos = torch.position + vec3(0, 0.7f, 0);
		PBRProg.use();
		PBRProg.setUniform(arrayString.c_str(), vec4(shiftedPos, 1.0f));
		terrainProg.use();
		terrainProg.setUniform(arrayString.c_str(), vec4(shiftedPos, 1.0f));
		i++;
	}

	//Campfire light
	float noiseInput = t * 2.0f;
	float noiseVal = torchNoise.GetNoise(noiseInput, 0.0f); // returns value in [-1, 1]
	float normalized = (noiseVal + 1.0f) / 2.0f; // convert to [0, 1]
	float mixed = mix(torchMinIntensity, torchMaxIntensity, normalized);

	string arrayString = "Light[" + to_string(i) + "].Intensity";
	PBRProg.use();
	PBRProg.setUniform(arrayString.c_str(), torchBrightColour * mixed * 0.5f);
	terrainProg.use();
	terrainProg.setUniform(arrayString.c_str(), torchBrightColour * mixed * 0.5f);

	arrayString = "Light[" + to_string(i) + "].Position";
	vec3 shiftedPos = campfirePosition + vec3(0, 0.2f, 0);
	PBRProg.use();
	PBRProg.setUniform(arrayString.c_str(), vec4(shiftedPos, 1.0f));
	terrainProg.use();
	terrainProg.setUniform(arrayString.c_str(), vec4(shiftedPos, 1.0f));



	//----------Firefly spawning	
	if (fireFlySpawnTimer >= fireFlySpawnCooldown && currentFireFlyCount < maxFireFlyCount)
	{
		float randomX = topLeftSpawnBound.x + (bottomRightSpawnBound.x - topLeftSpawnBound.x) * dis(gen);
		float randomY = topLeftSpawnBound.y + (bottomRightSpawnBound.y - topLeftSpawnBound.y) * dis(gen);
		float randomZ = topLeftSpawnBound.z + (bottomRightSpawnBound.z - topLeftSpawnBound.z) * dis(gen);

		vec3 spawnPosition = vec3(randomX, randomY, randomZ);

		FireFly* newFireFly = new FireFly(spawnPosition);

		fireFlies.push_back(newFireFly);
		currentFireFlyCount++;
		
		fireFlySpawnTimer = 0.0f;
		fireFlySpawnCooldown = linearRand(1.0f, 2.0f);		
	}



	//-----Firefly update
	for (size_t i = 0; i < fireFlies.size(); i++)
	{
		FireFly* fireFly = fireFlies[i];
		if (fireFly != NULL)
		{
			//Moves firefly's position
			fireFly->Update(deltaTime);

			if (fireFly->ShouldDestroy()) //Returns true when the particle age exceeds its limit
			{
				//Destroy firefly

				string lightUniformTag = "FireflyLight[" + to_string(i) + "]";

				//Set deleted pointlights to a null value equivalent to be ignored in the shader
				terrainProg.use();
				terrainProg.setUniform((lightUniformTag + ".Position").c_str(), vec4(0.0f, -10.0f, 0.0f, 1.0f));				
							
				delete fireFly;
				fireFlies.erase(fireFlies.begin() + i);
				currentFireFlyCount--;

				//cout << "FireFly deleted   Number of lights: " << currentFireFlyCount << "\n";

				i--;
			}
		}
	}

	//------Firefly shader update
	string lightUniformTag;
	//Clear fireflies list
	for (size_t i = 0; i < maxFireFlyCount; i++)
	{
		lightUniformTag = ("FireflyLight[" + to_string(i) + "]");
		terrainProg.use();
		terrainProg.setUniform((lightUniformTag + ".Position").c_str(), vec4(0.0f, 0.0f, 0.0f , 1.0f));
		terrainProg.setUniform((lightUniformTag + ".Intensity").c_str(), vec3(0.0, 0.0, 0.0));
		terrainProg.setUniform((lightUniformTag + ".Ambient").c_str(), vec3(0.0,0.0,0.0));
	}

	vector<vec3> fireFlyPositions;
	terrainProg.use();
	//Fill uniforms with updated values
	for (size_t i = 0; i < fireFlies.size(); i++) {
		FireFly* fireFly = fireFlies[i];

		lightUniformTag = ("FireflyLight[" + to_string(i) + "]");
		
		terrainProg.setUniform((lightUniformTag + ".Position").c_str(), fireFly->Position);
		terrainProg.setUniform((lightUniformTag + ".Intensity").c_str(), fireFlyLightColour * fireFly->brightness * 0.5f);
		terrainProg.setUniform((lightUniformTag + ".Ambient").c_str(), fireFlyAmbientColour * fireFly->brightness * 0.5f);

		fireFlyPositions.push_back(fireFly->Position);
	}

	terrainProg.setUniform("numberOfFireflies", static_cast<int>(fireFlies.size()));

	//Update the buffer with the new positions. Slight overhead but worth it
	glBindBuffer(GL_ARRAY_BUFFER, fireflyPosBuf);
	glBufferData(GL_ARRAY_BUFFER, fireFlyPositions.size() * sizeof(vec3), fireFlyPositions.data(), GL_DYNAMIC_DRAW);

}

void SceneBasic_Uniform::render()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	view = camera.GetViewMatrix();
	projection = perspective(radians(70.0f), (float)width / height, 0.3f, 100.0f);

	//------- Pass 1, shadow map pass
	
	model = mat4(1.0f);
	model = translate(model, lightFrustum.getOrigin()); //Move camera to light position
	view = lightFrustum.getViewMatrix();
	projection = lightFrustum.getProjectionMatrix(true);

	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, shadowMapWidth, shadowMapHeight);
	
	PBRProg.use();
	PBRProg.setUniform("Pass", 1); //First pass renders depth only to a texture
	terrainProg.use();
	terrainProg.setUniform("Pass", 1);
	
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	//Polygon offset shifts the depth values slightly so instances where the fragment sort of shadows itself
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(2.0f, 3.0f);

	//Draw the scene objects using
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, depthTex);

	//Draw scene
	drawSolidSceneObjects();	

	glCullFace(GL_BACK);
	glDisable(GL_CULL_FACE);
	glFlush();
	glDisable(GL_POLYGON_OFFSET_FILL);


	//----- Pass 2, scene render

	view = camera.GetViewMatrix();
	projection = perspective(radians(70.0f), (float)width / height, 0.01f, 100.0f);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, width, height);
	
	PBRProg.use();
	PBRProg.setUniform("Pass", 2);
	terrainProg.use();
	terrainProg.setUniform("Pass", 2);	

	//Draw scene
	drawSolidSceneObjects();
	

	//--------Light frustum render
	objectProg.use();
	model = mat4(1.0f);
	model = translate(model, lightPos);

	mat4 mv = view * lightFrustum.getInverseViewMatrix();
	objectProg.setUniform("MVP", projection * mv);
	//setMatrices(objectProg);
	//lightFrustum.render();



	//-------Skybox render
	view = lookAt(vec3(0.0f, 0.0f, 0.0f), camera.Front, camera.Up);

	skyProg.use();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, daySkyboxTexID);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, nightSkyBox);
	glDepthMask(GL_FALSE);
	model = mat4(1.0f);
	setMatrices(skyProg);
	sky.render();
	glDepthMask(GL_TRUE);

	view = camera.GetViewMatrix();

	//----Particle rendering
	renderFireParticles();
	renderFireflies();

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

void SceneBasic_Uniform::drawSolidSceneObjects() {

	//---------Ruin Rendering

	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, depthTex);
	;
	PBRProg.use();

	model = mat4(1.0f);
	model = translate(model, vec3(6.4f, -1.1f, 0.0f));
	model = rotate(model, radians(-90.0f), vec3(0.0f, 1.0f, 0.0f));
	model = scale(model, vec3(0.3f, 0.3f, 0.3f));
	setMatrices(PBRProg);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, rockTexID);

	PBRProg.setUniform("TextureScale", 20.0f);
	RuinMesh->render();

	//------ Campfire render

	PBRProg.use();

	model = mat4(1.0f);
	model = translate(model, campfirePosition);
	model = rotate(model, radians(0.0f), vec3(0.0f, 1.0f, 0.0f));
	model = scale(model, vec3(0.08f, 0.08f, 0.08f));
	setMatrices(PBRProg);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, campfireTexID);

	PBRProg.setUniform("TextureScale", 1.0f);
	campfireMesh->render();

	//------ Collectables rendering

	for (Collectable& item : collectables) {
		if (item.isActive == true)
		{
			model = mat4(1.0f);
			model = translate(model, vec3(item.location.x, item.location.y, item.location.z));
			model = scale(model, vec3(0.5f, 0.5f, 0.5f));

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, item.texID);

			setMatrices(PBRProg);
			PBRProg.setUniform("TextureScale", 1.0f);
			item.collectableMesh->render();
		}
	}

	///------- Torch rendering

	for (TorchInfo& torch : torches) 
	{
		model = mat4(1.0f);
		model = translate(model, vec3(torch.position.x, torch.position.y, torch.position.z));
		model = scale(model, vec3(0.1f, 0.1f, 0.1f));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, torchTexID);

		setMatrices(PBRProg);
		PBRProg.setUniform("TextureScale", 1.0f);
		StandingTorch->render();
	}

	//-------- Terrain rendering
	terrainProg.use();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, grassTexID);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, rockTexID);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, cloudTexID);

	terrainProg.use();
	model = mat4(1.0f);
	model = rotate(model, radians(0.0f), vec3(0.0f, 1.0f, 0.0f));
	model = scale(model, vec3(0.25f, 0.25f, 0.25f));
	model = translate(model, vec3(0.0f, -3.0f, -15.0f));
	setMatrices(terrainProg);
	terrainProg.setUniform("TextureScale", 20.0f);

	TerrainMesh->render();
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
	//LightPV is the projection view matrix for the light
	//Essentially, transforming world space to light clip space
	mat4 shadowMatrix = lightPV * model;
	program.setUniform("ShadowMatrix", shadowMatrix);

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
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		camera.ProcessKeyboard(UP, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		camera.ProcessKeyboard(DOWN, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		attemptPickup();
}

void SceneBasic_Uniform::attemptPickup() {

	float interactDistance = 3.0f;
	if (gameEnded) {
		cout << "game ended, cannot collect any more!" << endl;
		return;
	}
	for (Collectable& item : collectables)
	{		
		float distance = length(item.location - camera.Position);
		if (distance < interactDistance && item.isActive == true) {
			item.isActive = false;
			cout << "collected " + item.name << endl << endl;
			collectedItems++;
		}

	}
	float distance = length(campfirePosition - camera.Position);
	if (distance < interactDistance && hasCookedItem == false && collectedItems >= maxCollectables) {
		cout << "cooked item from ingredients" << endl << "game won!" << endl;
		hasCookedItem = true;
	}
	else if (distance < interactDistance && collectedItems < maxCollectables)
	{
		cout << "Not enough ingredients to cook item!" << endl << endl;
	}

}



void SceneBasic_Uniform::initParticleStream() {
	//For particles, it holds all the information about the position, velocity and age in buffers
	
	mat3 emitterBases[8] = {
		ParticleUtils::makeArbitraryBasis(vec3(0, 1, 0)),
		ParticleUtils::makeArbitraryBasis(vec3(0, 1, 0)),
		ParticleUtils::makeArbitraryBasis(vec3(0, 1, 0)),
		ParticleUtils::makeArbitraryBasis(vec3(0, 1, 0)),
		ParticleUtils::makeArbitraryBasis(vec3(0, 1, 0)),
		ParticleUtils::makeArbitraryBasis(vec3(0, 1, 0)),
		ParticleUtils::makeArbitraryBasis(vec3(0, 1, 0)),
		ParticleUtils::makeArbitraryBasis(vec3(0, 1, 0))		
	};

	
	glActiveTexture(GL_TEXTURE7);
	randomParticleTexID = ParticleUtils::createRandomTex1D(nParticles * 3);

	particleStreamProg.use();
	particleStreamProg.setUniform("ParticleLifetime", particleLifetime);
	particleStreamProg.setUniform("ParticleSize", 0.1f);
	particleStreamProg.setUniform("Accel", vec3(0.0f, -0.14f, 0.0f));

	//The shader holds a collection of positions particles can emit from, held as a uniform
	for (int i = 0; i < nEmitters; i++)
	{
		string arrayString = "EmitterPos[" + to_string(i) + "]";
		particleStreamProg.setUniform(arrayString.c_str(), vec3(torches[i].position));
		arrayString = "EmitterBasis[" + to_string(i) + "]";
		particleStreamProg.setUniform(arrayString.c_str(), emitterBases[i]);
	}

	//Generate bufers to hold data and swap between
	glGenBuffers(2, posBuf);
	glGenBuffers(2, velBuf);
	glGenBuffers(2, age);
	glGenBuffers(2, emitterIndexBuf);

	vector<GLint> emitterIndices(nParticles);
	for (int i = 0; i < nParticles; ++i) {
		emitterIndices[i] = i % nEmitters;
	}

	//Create buffers to fit vec3s for vel and pos, just float for age.
	int size = nParticles * 3 * sizeof(GLfloat);
	glBindBuffer(GL_ARRAY_BUFFER, posBuf[0]);
	glBufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_COPY); //Dynamic copy indicates that the buffer will be frequently changed by the CPU
	glBindBuffer(GL_ARRAY_BUFFER, posBuf[1]);
	glBufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_COPY);
	glBindBuffer(GL_ARRAY_BUFFER, velBuf[0]);
	glBufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_COPY);
	glBindBuffer(GL_ARRAY_BUFFER, velBuf[1]);
	glBufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_COPY);
	glBindBuffer(GL_ARRAY_BUFFER, age[0]);
	glBufferData(GL_ARRAY_BUFFER, nParticles * sizeof(float), 0, GL_DYNAMIC_COPY);
	glBindBuffer(GL_ARRAY_BUFFER, age[1]);
	glBufferData(GL_ARRAY_BUFFER, nParticles * sizeof(float), 0, GL_DYNAMIC_COPY);
	glBindBuffer(GL_ARRAY_BUFFER, emitterIndexBuf[0]);
	glBufferData(GL_ARRAY_BUFFER, nParticles * sizeof(GLint), emitterIndices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, emitterIndexBuf[1]);
	glBufferData(GL_ARRAY_BUFFER, nParticles * sizeof(GLint), emitterIndices.data(), GL_STATIC_DRAW);

	//Particle age container
	vector<GLfloat> tempData(nParticles);
	//Time between each particle spawn
	float rate = particleLifetime / nParticles;
	//Create ages for each particle, this time it actually is how long the particle has been alive for
	for (int i = 0; i < nParticles; i++)
	{		 
		tempData[i] = rate * i;
	}
	glBindBuffer(GL_ARRAY_BUFFER, age[0]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, nParticles * sizeof(float), tempData.data());

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenVertexArrays(2, particleArray);

	//particle array 0
	//Sets up the three buffers like i would vertex positions in a regular shader
	glBindVertexArray(particleArray[0]);
	glBindBuffer(GL_ARRAY_BUFFER, posBuf[0]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, velBuf[0]);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, age[0]);
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, emitterIndexBuf[0]);
	glVertexAttribIPointer(3, 1, GL_INT, 0, 0);
	glEnableVertexAttribArray(3);

	//particle array 1
	glBindVertexArray(particleArray[1]);
	glBindBuffer(GL_ARRAY_BUFFER, posBuf[1]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, velBuf[1]);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, age[1]);
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, emitterIndexBuf[1]);
	glVertexAttribIPointer(3, 1, GL_INT, 0, 0);
	glEnableVertexAttribArray(3);

	glBindVertexArray(0);

	/*Feedback transforms explanation
	* Normally you would supply values on the cpu side at the start of rendering.
	* Eg, applying the time or colour values of shader uniforms at the start of Render().
	* It would then work from vertex, to geometry, to fragment and whatnot.
	* Would only use the results of the previous step, so the vertex shader could only use the values passed in from the CPU.
	* But with transform feedbacks, the GPU can write back data that is processed by shaders and write it back into buffer objects.
	* This data can be read by the CPU, but doing so blocks the GPU.
	* So, instead of calculating all the particle positions on the CPU and then passing that in as a uniform or adjusting buffer data before using the GPU,
	* the GPU can instead do all the calculations and use those results to then render the updated particle positions.
	*/

	/* Here specifially, it's relating to this part of the shader
	*	layout(xfb_buffer = 0, xfb_offset = 0) out vec3 Position;
	* it says that the values assigned to the above variable in the shader will be put into the buffer defined below
	*/


	//Feedback objects
	glGenTransformFeedbacks(2, feedback);

	//transform feedback 0
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, feedback[0]);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, posBuf[0]);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, velBuf[0]);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 2, age[0]);

	//transform feedback 1
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, feedback[1]);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, posBuf[1]);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, velBuf[1]);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 2, age[1]);

	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);


}

void SceneBasic_Uniform::initFireflies()
{
	fireFlySpawnCooldown = 2.0f;	

	//Will use a buffer to hold the positions of fireflies.
	//Will update the buffer on the CPU side each frame, but this allows for instanced rendering
	glGenBuffers(1, &fireflyPosBuf);

	
	int size = maxFireFlyCount * 3 * sizeof(GLfloat);
	glBindBuffer(GL_ARRAY_BUFFER, fireflyPosBuf);

	vector<GLfloat> fireflyPositions(maxFireFlyCount * 3);
	for (int i = 0; i < maxFireFlyCount; i++)
	{
		fireflyPositions[i * 3] = 0.0f;
		fireflyPositions[i * 3 + 1] = -10.0f;
		fireflyPositions[i * 3 + 2] = 0.0f;
	}

	glBufferData(GL_ARRAY_BUFFER, size, fireflyPositions.data(), GL_DYNAMIC_DRAW); //Dynamic copy indicates that the buffer will be frequently changed by the CPU

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenVertexArrays(1, &fireflyVA);

	glBindVertexArray(fireflyVA);
	glBindBuffer(GL_ARRAY_BUFFER, fireflyPosBuf);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glVertexAttribDivisor(0, 1);
	glEnableVertexAttribArray(0);
}

void SceneBasic_Uniform::initShadows() {
	shadowMapWidth = 2056;
	shadowMapHeight = 2056;	
	
	shadowBias = mat4(vec4(0.5f, 0.0f, 0.0f, 0.0f),
		vec4(0.0f, 0.5f, 0.0f, 0.0f),
		vec4(0.0f, 0.0f, 0.5f, 0.0f),
		vec4(0.5f, 0.5f, 0.5f, 1.0f));

	//Position of the light source (frustum centre). Temporary value for explanation
	lightPos = vec3(4.0f, 5.0f, -4.5f);

	//Sets the camera at the lightPos, looking at the second argument
	//This is where the shadow is cast from
	lightFrustum.orient(lightPos, vec3(-5.0f, 0.0f, -12.0f), vec3(0.0f, 1.0f, 0.0f));

	lightFrustum.setOrtho(-20.0f, 20.0f, -20.0f, 20.0f, 2.0f, 100.0f);
	
	//Light Project View matrix
	//Shadow bias maps clip space coordinates of -1 to 1 to 0-1 texture space.
	//Therefore used to transform any world space point to the shadowmap
	//Bit like MVP going from world to view to clip space, since the shadow texture is going to be in line with the camera, essentially its own view space
	lightPV = shadowBias * lightFrustum.getProjectionMatrix(true) * lightFrustum.getViewMatrix();

	PBRProg.use();		

	//Colour used for sampling outside of the valid range
	GLfloat border2[] = { 1.0f, 0.0f, 0.0f, 0.0f };
	glGenTextures(1, &depthTex);

	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, depthTex);

	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, shadowMapWidth, shadowMapHeight);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); //Regular texture filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); //Avoid wrapping
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border2);

	//Compare mode essentially does the shadow calc for us. It enables depth comparison sampling for depth textures
	/*Without this enabled, this would have to be in the shader:
		float closestDepth = texture(shadowMap, shadowCoord.xy).r;
		float currentDepth = shadowCoord.z;
		float shadow = currentDepth > closestDepth ? 0.0 : 1.0;
	 But with it, the shader just needs this:
		float shadow = texture(shadowMap, shadowCoord.xyz);
	 It will automatically give it the xyz coords from above. Shadow will then be 0 or 1 depending on if the fragment is in shadow or not
	*/
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	
	//If the reference point in world space (fragment being rendered) is less than the texel depth of that point on the shadow map,
	// it is lit (1).
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	//So in general, render from the light and create a depth map.
	//Render from the actual camera and start working with a fragment.
	//Perform the lightPV transform on the fragment to see where it lies on the depth map created
	//Keep the fragment's depth information and compare it to that texel on the shadow map
	//If the fragment position's depth is behind the shadow map's depth, it is in shadow.

	//FBOs
	//Render the scene (from the light's perspective) to the depthTex texture.
	glGenFramebuffers(1, &shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
		GL_TEXTURE_2D, depthTex, 0);

	//Ensure it doesn't write any colour
	GLenum drawBuffers[] = { GL_NONE };
	glDrawBuffers(1, drawBuffers);

	GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (result == GL_FRAMEBUFFER_COMPLETE) {

	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneBasic_Uniform::initMaterials()
{
	PBRProg.use();
	PBRProg.setUniform("material.Rough", 0.97f);
	PBRProg.setUniform("material.Metal", 0);
	PBRProg.setUniform("material.Colour", vec3(0.4f));

	terrainProg.use();
	terrainProg.setUniform("material.Rough", 0.97f);
	terrainProg.setUniform("material.Metal", 0);
	terrainProg.setUniform("material.Colour", vec3(0.4f));
	
}

void SceneBasic_Uniform::initTextures()
{
	//-------------Main textures loading
	glActiveTexture(GL_TEXTURE0);
	grassTexID = Texture::loadTexture("media/texture/grass_02_1k/grass_02_base_1k.png");
	collectables[0].texID = Texture::loadTexture("media/meatTextures/Hunk_Of_Meat_Hunk_Of_Meat_BaseColor.png");
	collectables[1].texID = Texture::loadTexture("media/cheeseTextures/cheese_piece_colors.png");
	collectables[2].texID = Texture::loadTexture("media/mushroomTextures/Material_albedo.jpg");
	campfireTexID = Texture::loadTexture("media/campfireTextures/Material.001_albedo.jpg");
	torchTexID = Texture::loadTexture("media/texture/StandingTorchTextures/Merged_Cylinder_004_albedo.jpeg");
	rockTexID = Texture::loadTexture("media/texture/cliff_rocks_02_1k/cliff_rocks_02_baseColor_1k.png");

	//Skybox texture
	glActiveTexture(GL_TEXTURE3);
	daySkyboxTexID = Texture::loadCubeMap("media/texture/skyCubeMap/bluecloud");
	nightSkyBox = Texture::loadCubeMap("media/texture/cubeMap/night");
	
	//Particle textures
	glActiveTexture(GL_TEXTURE6);
	particleTexID = Texture::loadTexture("media/texture/particle/fire.png");
	fireFlyTexID = Texture::loadTexture("media/texture/firefly/fireFlyTex.png");

	//Texture 7 used for 1D random noise


	//---------------- Cloud texture generation
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
	
}

void SceneBasic_Uniform::initPostProcessing()
{
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

	//Creates a VBO for this quad. Vertex positions and then texture coords passed in
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
	//glBindSampler(8, nearestSampler);
	//glBindSampler(9, nearestSampler);
	//glBindSampler(10, nearestSampler);

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
	glActiveTexture(GL_TEXTURE9);
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

void SceneBasic_Uniform::renderFireflies()
{
	glBindVertexArray(fireflyVA);

	fireflyParticleProg.use();
	fireflyParticleProg.setUniform("ParticleSize", 0.2f);
	model = mat4(1.0f);
	setMatrices(fireflyParticleProg);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, fireFlyTexID);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, currentFireFlyCount);
	
	glBindVertexArray(0);

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	
}

void SceneBasic_Uniform::renderFireParticles()
{
	particleStreamProg.use();
	particleStreamProg.setUniform("Time", time);
	particleStreamProg.setUniform("DeltaT", deltaTime);

	model = mat4(1.0f);
	model = translate(model, vec3(0.0f, 0.4f, 0.0f));
	setMatrices(particleStreamProg);

	//Position update pass
	particleStreamProg.setUniform("Pass", 1);

	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_1D, randomParticleTexID); //Uses a texture to generate random values in the shader

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, particleTexID);

	glEnable(GL_RASTERIZER_DISCARD); //Tells it to not render the result to the fragment shader. Good for just doing processing.
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, feedback[drawBuf]);
	glBeginTransformFeedback(GL_POINTS);

	glBindVertexArray(particleArray[1 - drawBuf]);
	glVertexAttribDivisor(0, 0);
	glVertexAttribDivisor(1, 0);
	glVertexAttribDivisor(2, 0);
	glDrawArrays(GL_POINTS, 0, nParticles);
	glBindVertexArray(0);

	glEndTransformFeedback();
	glDisable(GL_RASTERIZER_DISCARD);

	//Render pass
	particleStreamProg.setUniform("Pass", 2);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glDepthFunc(GL_LEQUAL);

	glDepthMask(GL_FALSE);
	glBindVertexArray(particleArray[drawBuf]);
	glVertexAttribDivisor(0, 1);
	glVertexAttribDivisor(1, 1);
	glVertexAttribDivisor(2, 1);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, nParticles);
	glBindVertexArray(0);

	//glDepthFunc(GL_LESS);
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);

	drawBuf = 1 - drawBuf;

}

//Found online. Used to make the interpolation between colours better
//https://www.alanzucconi.com/2016/01/06/colour-interpolation/
vec3 SceneBasic_Uniform::rgbToHsv(vec3 c) {
	float maxVal = std::max(c.r, std::max(c.g, c.b));
	float minVal = std::min(c.r, std::min(c.g, c.b));
	float delta = maxVal - minVal;

	float h = 0.0f;
	if (delta > 0.00001f) {
		if (maxVal == c.r) {
			h = 60.0f * fmod(((c.g - c.b) / delta), 6.0f);
		}
		else if (maxVal == c.g) {
			h = 60.0f * (((c.b - c.r) / delta) + 2.0f);
		}
		else {
			h = 60.0f * (((c.r - c.g) / delta) + 4.0f);
		}
	}

	float s = (maxVal == 0.0f) ? 0.0f : delta / maxVal;
	float v = maxVal;

	if (h < 0.0f) h += 360.0f;
	return vec3(h / 360.0f, s, v);
}
vec3 SceneBasic_Uniform::hsvToRgb(vec3 hsv) {
	float h = hsv.x * 360.0f;
	float s = hsv.y;
	float v = hsv.z;

	float c = v * s;
	float x = c * (1 - fabs(fmod(h / 60.0f, 2) - 1));
	float m = v - c;

	vec3 rgb;

	if (h >= 0 && h < 60)      rgb = vec3(c, x, 0);
	else if (h < 120)          rgb = vec3(x, c, 0);
	else if (h < 180)          rgb = vec3(0, c, x);
	else if (h < 240)          rgb = vec3(0, x, c);
	else if (h < 300)          rgb = vec3(x, 0, c);
	else                       rgb = vec3(c, 0, x);

	return rgb + vec3(m);
}

vec3 SceneBasic_Uniform::mixHSV(vec3 colorA, vec3 colorB, float t) {
	vec3 hsvA = rgbToHsv(colorA);
	vec3 hsvB = rgbToHsv(colorB);

	float hueDiff = hsvB.x - hsvA.x;
	if (hueDiff > 0.5f) hsvA.x += 1.0f;
	else if (hueDiff < -0.5f) hsvB.x += 1.0f;

	vec3 mixedHSV = mix(hsvA, hsvB, t);
	mixedHSV.x = fmod(mixedHSV.x, 1.0f);

	return hsvToRgb(mixedHSV);
}

void SceneBasic_Uniform::updateDayNightShaders(TimeOfDayInfo prevState, TimeOfDayInfo currentState, float t) {
	//Interpolates between the previous state and the current state
	//Allows for smooth transition between times of day
	
	float fogStart = mix(prevState.fogInfo.fogStart, currentState.fogInfo.fogStart, t);
	float fogEnd = mix(prevState.fogInfo.fogEnd, currentState.fogInfo.fogEnd, t);
	vec3 fogColour = mixHSV(prevState.fogInfo.fogColour, currentState.fogInfo.fogColour, t);

	currentAmbientColour = mixHSV(prevState.ambientLightColour, currentState.ambientLightColour, t);
	currentSunColour = mixHSV(prevState.lightColour, currentState.lightColour, t);
	mainLightIntensity = mix(prevState.mainLightIntensity, currentState.mainLightIntensity, t);
	ambientLightIntensity = mix(prevState.ambientLightIntensity, currentState.ambientLightIntensity, t);

	//cout << "time of day: " << timeOfDay << endl;
	//cout << "light intensity: " << mainLightIntensity << endl;
	//cout << currentState.name << endl << endl;

	PBRProg.use();
	PBRProg.setUniform("fogStart", fogStart);
	PBRProg.setUniform("fogEnd", fogEnd);
	PBRProg.setUniform("fogColour", fogColour);
	PBRProg.setUniform("DirLight.Ambient", currentAmbientColour * ambientLightIntensity * 0.4f);
	PBRProg.setUniform("DirLight.Intensity", currentSunColour * mainLightIntensity);

	terrainProg.use();
	terrainProg.setUniform("fogStart", fogStart);
	terrainProg.setUniform("fogEnd", fogEnd);
	terrainProg.setUniform("fogColour", fogColour);
	terrainProg.setUniform("DirLight.Ambient", currentAmbientColour * ambientLightIntensity * 0.4f);
	terrainProg.setUniform("DirLight.Intensity", currentSunColour * mainLightIntensity);
}

void SceneBasic_Uniform::updateDayNightCycle(float deltaTime)
{
	//+= deltatime will properly increment the timer by seconds
	//Using values of 0-2 to interpolate sun position, so divide time passed by duration	
	timeOfDay += deltaTime / secondsInFullCycle;
	if (timeOfDay >= 2.0f)
	{
		timeOfDay = 0;
	}

	//----Blending for skybox
	float blend;
	if (timeOfDay < 0.15f) //When sun rising
	{
		blend = clamp(1 - ((timeOfDay) / 0.15f), 0.0f, 1.0f); //0 is day, 1 is night
	}
	else if (timeOfDay >= 1.95) { //When sun setting
		blend = clamp((1 - (timeOfDay - 1.95f) / 0.05f), 0.0f, 1.0f);
		blend = 1; //Temporarily disabled
	}
	else { //During daytime
		blend = clamp((timeOfDay - 0.9f) / 0.1f, 0.0f, 1.0f);
	}

	skyProg.use();
	skyProg.setUniform("BlendFactor", blend);

	
	//-----Time of day update
	
	// Dawn (0 - 0.25) 
	// Day (0.25 - 0.75) 
	// Dusk (0.75 - 1) 
	// Night(1 - 1.75) 
	// Dawn night (1.75 - 2)

	//Look at the next start index
	//If the current time exceeds that, swap to the next time of day

	int prevIndex = (timeOfDayIndex - 1 + 6) % 6;
	int nextIndex = (timeOfDayIndex + 1) % 6;

	float prevStartTime = timesOfDay[timeOfDayIndex].startTime;
	float nextStartTime = timesOfDay[nextIndex].startTime;

	if (nextStartTime < prevStartTime) {
		// Wrapping from something like 1.8 to 0.1
		if (timeOfDay >= nextStartTime && timeOfDay < prevStartTime) {
			prevTimeOfDay = currentTimeOfDay;
			currentTimeOfDay = nextTimeOfDay;
			timeOfDayIndex = nextIndex;
			nextTimeOfDay = timesOfDay[(timeOfDayIndex + 1) % 6];
		}
	}
	else {
		if (timeOfDay >= nextStartTime) {
			prevTimeOfDay = currentTimeOfDay;
			currentTimeOfDay = nextTimeOfDay;
			timeOfDayIndex = nextIndex;
			nextTimeOfDay = timesOfDay[(timeOfDayIndex + 1) % 6];
		}
	}

	float currentStartTime = timesOfDay[timeOfDayIndex].startTime;
	float rampUpDuration = timesOfDay[timeOfDayIndex].rampUpTime;

	//If time of day (0.25) is passed next start time (0.23)
	//Change the current day to the target
	//Change the current index to the target index
	//Target is the next index

	prevIndex = (timeOfDayIndex - 1) % 6;
	nextStartTime = timesOfDay[prevIndex].startTime;
	currentStartTime = timesOfDay[timeOfDayIndex].startTime;
	rampUpDuration = timesOfDay[timeOfDayIndex].rampUpTime;

	//Takes the ramp up duration for that time of day to smoothly blend between times
	float t;
	if (rampUpDuration == 0) //Set duration to 0 when there is no ramp up
	{
		t = 1.0f;
	}
	else {
		//If current time is 0.2 and from time is 0.1, ramp up 0.45,
		//Then that's (0.2 - 0.1) / 0.45
		// (0.1) / 0.45 = 0.2. How far along that duration it is
		float adjustedTime = timeOfDay;
		if (timeOfDay < currentStartTime) {
			adjustedTime += 2.0f; // handle wrap-around
		}

		t = (adjustedTime - currentStartTime) / rampUpDuration;
		t = clamp(t, 0.0f, 1.0f);
		//cout << t << endl;
	}

	updateDayNightShaders(prevTimeOfDay, currentTimeOfDay, t);

	//------- Sun position update

	/* Will first need an angle from the horizon to place the light
	* Since time of day is 0-2, can potentially divide by 2 to use as an interpolation value
	* With an angle, use sin and cos to determine direction vector
	*/
	float angle;
	if (timeOfDay < 1.0f) {
		angle = radians((timeOfDay / 2.0f) * 360.0f);
	}
	else {
		angle = radians(((timeOfDay - 1.0f) / 2.0f) * 360.0f);
	}
	
	vec3 sunElevationVector = normalize(vec3(0.0f, sin(angle), cos(angle)));
	sunTarget = campfirePosition;
	sunDistance = 15.0f;
	sunPos = sunTarget + sunElevationVector * sunDistance;
	sunPos.x += 10.0f;

	sunLightDirection = normalize(sunTarget - sunPos);

	vec3 viewDir = normalize(mat3(view) * -sunLightDirection);
	//Position at first argument, look at second argument
	lightFrustum.orient(sunPos, sunTarget, vec3(0.0f, 1.0f, 0.0f));
	lightPV = shadowBias * lightFrustum.getProjectionMatrix(true) * lightFrustum.getViewMatrix();

	PBRProg.use();
	PBRProg.setUniform("DirLight.Position", vec4(-sunLightDirection, 0.0f));
	terrainProg.use();
	terrainProg.setUniform("DirLight.Position", vec4(-sunLightDirection, 0.0f));
}










