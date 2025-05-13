#ifndef OBJECT_H
#define OBJECT_H

#include "camera.h"
#include <cmath>  
#include <cstdio> 
typedef struct {
    float* vertexBuffer;
    float* normalBuffer;
    int* indexBuffer;
    int numVertices;
    int numTriangles;
}ObjectData; 
#endif