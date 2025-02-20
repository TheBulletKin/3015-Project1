#ifndef SCENEBASIC_UNIFORM_H
#define SCENEBASIC_UNIFORM_H

#include "helper/scene.h"

#include <glad/glad.h>
#include "helper/glslprogram.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "helper/torus.h"
#include "helper/plane.h"
#include "helper/objmesh.h"

class SceneBasic_Uniform : public Scene
{
private:
    //GLuint vaoHandle;
    GLSLProgram prog;
    //float angle;
    //Torus torus;
    Plane plane;
    std::unique_ptr<ObjMesh> PigMesh;
    std::unique_ptr<ObjMesh> TerrainMesh;

    void setMatrices();
    void compile();

public:
    SceneBasic_Uniform();

    void initScene();
    void update( float t );
    void render();
    void resize(int, int);
};

#endif // SCENEBASIC_UNIFORM_H
