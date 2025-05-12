#ifndef CAMERA_H
#define CAMERA_H

#include "camera.h"
#include <cmath>  
#include <cstdio> 
typedef struct {
    float eye[3]; // ī�޶� ��ġ(Eye Point)
    float u[3];   // ������
    float v[3];   // ����
    float w[3];   // ����(�������� -)
} Camera;

#endif