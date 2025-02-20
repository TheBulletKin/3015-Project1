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
    view = glm::lookAt(vec3(0.0f, 0.0f, 2.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    projection = glm::perspective(glm::radians(70.0f), (float)width / height, 0.3f, 100.0f);

    GLuint brickID = Texture::loadTexture("media/texture/brick1.jpg");
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, brickID);

    GLuint mossID = Texture::loadTexture("media/texture/moss.png");
    glActiveTexture(GL_TEXTURE3);
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
    prog.setUniform("Lights[0].Position", view * light1Pos);
    prog.setUniform("Lights[1].Position", view * light2Pos);
    prog.setUniform("Lights[2].Position", view * light3Pos);

    prog.setUniform("Lights[0].Ld", vec3(0.8f, 0.8f, 0.8f));  
    prog.setUniform("Lights[1].Ld", vec3(0.8f, 0.8f, 0.8f));
    prog.setUniform("Lights[2].Ld", vec3(0.8f, 0.8f, 0.8f));
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

    prog.setUniform("Material.Kd", 0.4f, 0.4f, 0.4f);
    prog.setUniform("Material.Ks", 0.9f, 0.9f, 0.9f);
    prog.setUniform("Material.Ka", 0.5f, 0.5f, 0.5f);
    prog.setUniform("Material.Shininess", 180.0f);

    model = mat4(1.0f);
    model = glm::rotate(model, glm::radians(90.0f), vec3(0.0f, 10.0f, 0.0f));
    
    setMatrices();
    
    PigMesh->render();
    TerrainMesh->render();
    cube.render();

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
