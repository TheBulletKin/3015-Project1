#ifndef FRUSTUM_H
#define FRUSTUM_H

#include <glad/glad.h>
#include "drawable.h"

#include <glm/glm.hpp>
#include <vector>

class Frustum : public Drawable
{
private:
    GLuint vao;
    glm::vec3 center, u, v, n;
    float mNear, mFar, fovy, ar;
    std::vector<GLuint> buffers;

    //For Orthographic
    float left, right, bottom, top;

public:
    Frustum();
    ~Frustum();

    void orient( const glm::vec3 &pos, const glm::vec3& a, const glm::vec3& u );
    void setPerspective( float , float , float , float  );
    void setOrtho(float left, float right, float bottom, float top, float nearDist, float farDist);
    glm::mat4 getViewMatrix() const;
    glm::mat4 getInverseViewMatrix() const;
    glm::mat4 getProjectionMatrix(bool isOrtho) const;
    glm::vec3 getOrigin() const;

    void render() const override;
    void deleteBuffers();
};

#endif // FRUSTUM_H
