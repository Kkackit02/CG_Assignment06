#include <cmath>  
#include <cstdio>
#include "Object.h"

#define M_PI 3.14159265358979323846

ObjectData create_scene(int width, int height) {
    ObjectData data;

    float theta, phi;
    int t;

    data.numVertices = (height - 2) * width + 2; // Áß°£ + À§/¾Æ·¡ ±ØÁ¡
    data.numTriangles = (height - 3) * width * 2 + width * 2; // Áß°£ ¶ì + À§/¾Æ·¡ ±ØÁ¡ »ï°¢Çü

    data.vertexBuffer = new float[3 * data.numVertices];
    data.indexBuffer = new int[3 * data.numTriangles];

    // Á¤Á¡ »ý¼º
    t = 0;
    for (int j = 1; j < height - 1; ++j) {
        for (int i = 0; i < width; ++i) {
            theta = (float)j / (height - 1) * M_PI;
            phi = (float)i / (width) * 2 * M_PI; // width·Î ³ª´®À¸·Î½á wrap-around °¡´É

            float x = sinf(theta) * cosf(phi);
            float y = cosf(theta);
            float z = -sinf(theta) * sinf(phi);

            data.vertexBuffer[3 * t + 0] = x;
            data.vertexBuffer[3 * t + 1] = y;
            data.vertexBuffer[3 * t + 2] = z;
            ++t;
        }
    }

    // ºÏ±Ø Á¤Á¡
    data.vertexBuffer[3 * t + 0] = 0;
    data.vertexBuffer[3 * t + 1] = 1;
    data.vertexBuffer[3 * t + 2] = 0;
    ++t;

    // ³²±Ø Á¤Á¡
    data.vertexBuffer[3 * t + 0] = 0;
    data.vertexBuffer[3 * t + 1] = -1;
    data.vertexBuffer[3 * t + 2] = 0;
    ++t;

    // »ï°¢Çü ÀÎµ¦½º: Áß°£ ¶ìµé
    t = 0;
    for (int j = 0; j < height - 3; ++j) {
        for (int i = 0; i < width; ++i) {
            int i_next = (i + 1) % width;

            int a = j * width + i;
            int b = (j + 1) * width + i;
            int c = j * width + i_next;
            int d = (j + 1) * width + i_next;

            // À§ »ï°¢Çü
            data.indexBuffer[t++] = a;
            data.indexBuffer[t++] = d;
            data.indexBuffer[t++] = c;

            // ¾Æ·¡ »ï°¢Çü
            data.indexBuffer[t++] = a;
            data.indexBuffer[t++] = b;
            data.indexBuffer[t++] = d;
        }
    }

    // ºÏ±Ø »ï°¢Çü
    int north_idx = (height - 2) * width;
    for (int i = 0; i < width; ++i) {
        int i_next = (i + 1) % width;
        data.indexBuffer[t++] = north_idx;
        data.indexBuffer[t++] = i;
        data.indexBuffer[t++] = i_next;
    }

    // ³²±Ø »ï°¢Çü
    int south_idx = north_idx + 1;
    for (int i = 0; i < width; ++i) {
        int i_next = (i + 1) % width;
        data.indexBuffer[t++] = south_idx;
        data.indexBuffer[t++] = (height - 3) * width + i_next;
        data.indexBuffer[t++] = (height - 3) * width + i;
    }

    return data;
}
