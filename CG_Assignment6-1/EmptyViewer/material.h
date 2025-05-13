#ifndef MATERIAL_H
#define MATERIAL_H

#include <glm/glm.hpp>  // glm::vec3 »ç¿ë

typedef struct {
    glm::vec3 ka;  // ambient
    glm::vec3 kd;  // diffuse
    glm::vec3 ks;  // specular
    float p;       // shininess
    glm::vec3 Ia;      // ambient light intensity (scalar)
} Material;

#endif