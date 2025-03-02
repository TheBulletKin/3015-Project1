#pragma once

#include "PointLight.h"
#include "glm/glm.hpp"
#include "FastNoiseLite.h"

class FireFly
{
public:
	FireFly(PointLight* pointLight, vec3 spawnPosition);
	~FireFly();
	void Update(float deltaTime);
	bool ShouldDestroy();
	glm::vec3 GetPosition() const {
		return currentPosition;
	}	
	
	PointLight* pointLight;
private:
	float lifeTimeMax;
	float currentLifeTime;	
	float easeInOutDuration;
	float brightness;
	
	vec3 currentPosition;
	vec3 velocity;
	FastNoiseLite noiseGen;
	float noiseOffsetX, noiseOffsetY, noiseOffsetZ;

	float timeFactor;
	
};

