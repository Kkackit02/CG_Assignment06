#ifndef CAMERA_H
#define CAMERA_H

#include "camera.h"
#include <cmath>  
#include <cstdio> 
typedef struct {
    float eye[3]; // 카메라 위치(Eye Point)
    float u[3];   // 오른쪽
    float v[3];   // 위쪽
    float w[3];   // 뒤쪽(앞이지만 -)
} Camera;

#endif