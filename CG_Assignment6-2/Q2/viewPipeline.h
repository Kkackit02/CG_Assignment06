#ifndef VIEWPIPELINE_LEGACY_H
#define VIEWPIPELINE_LEGACY_H

#include "camera.h"

// �� �Լ��� ��ȯ�� ����� result[4]�� ����
void M_Model(float* result, const float* vertex, const float M[4][4]);
void M_Camera(float* result, const float* vertex, const Camera& cameraData);
void M_Orthograph(float* result, const float* vertex, float l, float r, float b, float t, float n, float f);
void M_Perspective(float* result, const float* vertex, float n, float f);
void M_Viewport(float* result, const float* vertex, int nx, int ny);

#endif // VIEWPIPELINE_LEGACY_H
