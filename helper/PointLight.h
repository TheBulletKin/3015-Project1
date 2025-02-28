#ifndef POINTLIGHT_H
#define POINTLIGHT_H

#include <glad/glad.h> // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


#include <vector>
#include <string>

using namespace glm;
using namespace std;


class PointLight {
public:    
    float constant;      // Constant attenuation factor
    float linear;        // Linear attenuation factor
    float quadratic;     // Quadratic attenuation factor
    vec3 ambient;        // Ambient color of the light
    vec3 diffuse;        // Diffuse color of the light
    vec3 specular;  
    float brightness;
    // Specular color of the light

    // Constructor to initialize all members
    PointLight(float constant, float linear, float quadratic,
        const vec3& ambient, const vec3& diffuse, const vec3& specular) :        
        constant(constant),
        linear(linear),
        quadratic(quadratic),
        ambient(ambient),
        diffuse(diffuse),
        specular(specular)
    {
    }

    void SetIntensity(float brightness) {
        this->brightness = brightness;
    }
    
};

#endif

