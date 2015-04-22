#pragma once

#ifndef EG_BATCH_H_INCLUDED
#define EG_BATCH_H_INCLUDED

#define MAX_VERTEX_COUNT (300 * 2 * 3)

typedef struct
{
    float x, y, z;
    float nx, ny, nz;
    float tx, ty, tz;
    float bx, by, bz;
    float u, v;
    float r, g, b, a;
} SEGVertex;

typedef struct
{
    float x, y, z;
    float radius;
    float r, g, b, a;
} SEGOmni;

void drawScreenQuad(float left, float top, float right, float bottom, float *pColor);

#endif /* EG_BATCH_H_INCLUDED */
