#pragma once

#include "PointLight.h"
#include "glm/glm.hpp"
#include "FastNoiseLite.h"

class FireFly
{
public:
	FireFly(PointLight* pointLight, vec3 spawnPosition, int index);
	~FireFly();
	void Update(float deltaTime);
	bool ShouldDestroy();
	glm::vec3 GetPosition() const {
		return currentPosition;
	}	
	int GetIndex() const {
		return index;
	}
	PointLight* pointLight;
private:
	int index;
	float lifeTimeMax;
	float currentLifeTime;
	float easeTime;
	vec3 currentPosition;
	vec3 velocity;
	FastNoiseLite noiseGen;
	float noiseOffsetX, noiseOffsetY, noiseOffsetZ;

	float timeFactor;

	
};

