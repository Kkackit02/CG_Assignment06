
#include <cmath>  
#include <cstdio>
#include"Object.h"


int     gNumVertices = 0;    // Number of 3D vertices.
int     gNumTriangles = 0;    // Number of triangles.
int* gIndexBuffer = NULL; // Vertex indices for the triangles.
#define M_PI 3.14159265358979323846


ObjectData create_scene(int width, int height) {
	ObjectData data;

	float theta, phi;
	int t;

	data.numVertices = (height - 2) * width + 2;
	data.numTriangles = (height - 2) * (width - 1) * 2 ;

	data.vertexBuffer = new float[3 * data.numVertices];
	data.normalBuffer = new float[3 * data.numVertices];
	data.indexBuffer = new int[3 * data.numTriangles];

	t = 0;
	for (int j = 1; j < height - 1; ++j)
	{
		for (int i = 0; i < width; ++i)
		{
			theta = (float)j / (height - 1) * M_PI;
			phi = (float)i / (width - 1) * 2 * M_PI;

			float x = sinf(theta) * cosf(phi);
			float y = cosf(theta);
			float z = -sinf(theta) * sinf(phi);

			data.vertexBuffer[3 * t + 0] = x;
			data.vertexBuffer[3 * t + 1] = y;
			data.vertexBuffer[3 * t + 2] = z;
			float len = sqrtf(x * x + y * y + z * z);
			if (len > 0.0f) {
				data.normalBuffer[3 * t + 0] = x / len;
				data.normalBuffer[3 * t + 1] = y / len;
				data.normalBuffer[3 * t + 2] = z / len;
			}
			else {
				data.normalBuffer[3 * t + 0] = 0.0f;
				data.normalBuffer[3 * t + 1] = 1.0f;
				data.normalBuffer[3 * t + 2] = 0.0f;
			}

			t++;
		}
	}

	data.vertexBuffer[3 * t + 0] = 0;
	data.vertexBuffer[3 * t + 1] = 1;
	data.vertexBuffer[3 * t + 2] = 0;
	t++;
	data.vertexBuffer[3 * t + 0] = 0;
	data.vertexBuffer[3 * t + 1] = -1;
	data.vertexBuffer[3 * t + 2] = 0;
	t++;

	t = 0; 
	for (int j = 0; j < height - 3; ++j)
	{
		for (int i = 0; i < width - 1; ++i)
		{
			data.indexBuffer[t++] = j * width + i;
			data.indexBuffer[t++] = (j + 1) * width + (i + 1);
			data.indexBuffer[t++] = j * width + (i + 1);

			data.indexBuffer[t++] = j * width + i;
			data.indexBuffer[t++] = (j + 1) * width + i;
			data.indexBuffer[t++] = (j + 1) * width + (i + 1);
		}
	}

	for (int i = 0; i < width - 1; ++i)
	{
		data.indexBuffer[t++] = (height - 2) * width;         
		data.indexBuffer[t++] = i;
		data.indexBuffer[t++] = i + 1;

		data.indexBuffer[t++] = (height - 2) * width + 1;    
		data.indexBuffer[t++] = (height - 3) * width + (i + 1);
		data.indexBuffer[t++] = (height - 3) * width + i;
	}


	return data;
}


