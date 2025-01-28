#include "scenebasic_uniform.h"

#include <cstdio>
#include <cstdlib>

#include <string>
using std::string;

#include <iostream>
using std::cerr;
using std::endl;
#include <glm/gtc/matrix_transform.hpp>
using glm::vec3;
using glm::mat4;

#include "helper/glutils.h"


using glm::vec3;

SceneBasic_Uniform::SceneBasic_Uniform() : torus(0.7f, 0.3f, 30, 30){}

void SceneBasic_Uniform::initScene()
{
    compile();

    glEnable(GL_DEPTH_TEST);

    model = mat4(1.0f);
    model = glm::rotate(model, glm::radians(-35.0f), vec3(1.0f, 0.0f, 0.0f));
    view = glm::lookAt(vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    projection = glm::perspective(glm::radians(70.0f), (float)width / height, 0.3f, 100.0f);

    prog.setUniform("LightPosition", view * glm::vec4(5.0f, 5.0f, 2.0f, 1.0f));
   
    prog.setUniform("Kd", vec3(0.9f, 0.5f, 0.3f));
    prog.setUniform("Ka", vec3(0.1f, 0.1f, 0.1f));
    prog.setUniform("Ks", vec3(1.0f, 1.0f, 1.0f));    
    prog.setUniform("Roughness", 32.0f);    
    prog.setUniform("La", vec3(0.2f, 0.2f, 0.2f));
    prog.setUniform("Ld", vec3(1.0f, 1.0f, 1.0f));
}

void SceneBasic_Uniform::compile()
{
	try {
		prog.compileShader("shader/basic_uniform.vert");
		prog.compileShader("shader/basic_uniform.frag");
		prog.link();
		prog.use();
	} catch (GLSLProgramException &e) {
		cerr << e.what() << endl;
		exit(EXIT_FAILURE);
	}
}

void SceneBasic_Uniform::update( float t )
{
    
}

void SceneBasic_Uniform::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    setMatrices();
    
    torus.render();
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
    mv = model * view;

    prog.setUniform("ModelViewMatrix", mv);
    prog.setUniform("MVP", mv * projection);

    //glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(mv)));
    
    //prog.setUniform("NormalMatrix", normalMatrix);
    prog.setUniform("NormalMatrix", glm::mat3(vec3(mv[0]), vec3(mv[1]), vec3(mv[2])));
    prog.setUniform("ViewPos", vec3(0.0f, 0.0f, 2.0f));
}
