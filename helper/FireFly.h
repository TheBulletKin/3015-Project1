#pragma once

#include "PointLight.h"
#include "glm/glm.hpp"
#include "FastNoiseLite.h"

class FireFly
{
public:
	FireFly(vec3 spawnPosition);
	~FireFly();
	void Update(float deltaTime);
	bool ShouldDestroy();
	

	vec4 Position;
	vec3 Intensity;
	vec3 Ambient;
	float brightness;		
	
private:
	float lifeTimeMax;
	float currentLifeTime;	
	float easeInOutDuration;
	
	
	
	vec3 velocity;
	FastNoiseLite noiseGen;
	float noiseOffsetX, noiseOffsetY, noiseOffsetZ;

	float timeFactor;
	
};

