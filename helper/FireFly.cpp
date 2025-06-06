#include "FireFly.h"
#include "glm/gtc/random.hpp"

using namespace std;
using namespace glm;

FireFly::FireFly(vec3 spawnPosition) {
		
	currentLifeTime = 0.0f;
	lifeTimeMax = linearRand(8.0f, 16.0f);
	Position = vec4(spawnPosition, 1.0);
	velocity = vec3(0,0,0);
	
	noiseGen.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	noiseGen.SetFrequency(0.5f);

	noiseOffsetX = linearRand(0.0f, 1000.0f);
	noiseOffsetY = linearRand(0.0f, 1000.0f);
	noiseOffsetZ = linearRand(0.0f, 1000.0f);

	timeFactor = 0.0f;
	brightness = 0.0f;
	easeInOutDuration = 3.0f;
}

FireFly::~FireFly() {

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
	vec3 lastPosition = vec3(Position);
	lastPosition += velocity * deltaTime;
	Position = vec4(lastPosition, 1.0f);
	

	
	if (currentLifeTime < easeInOutDuration) {		
		brightness = smoothstep(0.0f, 1.0f, currentLifeTime / easeInOutDuration);
	}
	else if (currentLifeTime > lifeTimeMax - easeInOutDuration) {
		brightness = smoothstep(1.0f, 0.0f, (currentLifeTime - (lifeTimeMax - easeInOutDuration)) / easeInOutDuration);
	}
	else {		
		brightness = 1.0f;
	}
	
}
