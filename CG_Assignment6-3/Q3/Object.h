#ifndef OBJECT_H
#define OBJECT_H

#include "camera.h"
#include <cmath>  
#include <cstdio> 
typedef struct {
    float* vertexBuffer;
    int* indexBuffer;
    int numVertices;
    int numTriangles;
}ObjectData; 
#endif