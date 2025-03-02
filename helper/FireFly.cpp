#include "FireFly.h"
#include "glm/gtc/random.hpp"

using namespace std;
using namespace glm;

FireFly::FireFly(PointLight* pointLight, vec3 spawnPosition) {
	this->pointLight = pointLight;	
	currentLifeTime = 0.0f;
	lifeTimeMax = linearRand(12.0f, 32.0f);
	currentPosition = spawnPosition;
	
	noiseGen.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	noiseGen.SetFrequency(0.5f);

	noiseOffsetX = linearRand(0.0f, 1000.0f);
	noiseOffsetY = linearRand(0.0f, 1000.0f);
	noiseOffsetZ = linearRand(0.0f, 1000.0f);

	timeFactor = 0.0f;
	brightness = 0.0f;
	easeInOutDuration = 2.0f;
}

FireFly::~FireFly() {
	delete pointLight;
}

bool FireFly::ShouldDestroy() {

	if (currentLifeTime >= lifeTimeMax)
	{
		return true;
	}
	return false;
}

void FireFly::Update(float deltaTime) {
	currentLifeTime += deltaTime;
	timeFactor += deltaTime * 0.5f;
	
	float noiseX = noiseGen.GetNoise(noiseOffsetX, timeFactor);
	float noiseY = noiseGen.GetNoise(noiseOffsetY, timeFactor);
	float noiseZ = noiseGen.GetNoise(noiseOffsetZ, timeFactor);

	vec3 noiseOffset = vec3(noiseX, noiseY, noiseZ) * 0.2f;
	velocity = mix(velocity, noiseOffset, 0.1f);
	currentPosition += velocity * deltaTime;

	
	if (currentLifeTime < easeInOutDuration) {		
		brightness = smoothstep(0.0f, 1.0f, currentLifeTime / easeInOutDuration);
	}
	else if (currentLifeTime > lifeTimeMax - easeInOutDuration) {
		brightness = smoothstep(1.0f, 0.0f, (currentLifeTime - (lifeTimeMax - easeInOutDuration)) / easeInOutDuration);
	}
	else {		
		brightness = 1.0f;
	}
	
	if (pointLight) {
		pointLight->SetIntensity(brightness);
	}
}
