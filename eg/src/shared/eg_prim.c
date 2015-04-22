#include "eg.h"

void egCube(float size)
{
    float hSize = size * .5f;

    egBegin(EG_QUADS);

    egNormal(0, -1, 0);
    egTangent(1, 0, 0);
    egBinormal(0, 0, -1);
    egTexCoord(0, 0);
    egPosition3(-hSize, -hSize, hSize);
    egTexCoord(0, 1);
    egPosition3(-hSize, -hSize, -hSize);
    egTexCoord(1, 1);
    egPosition3(hSize, -hSize, -hSize);
    egTexCoord(1, 0);
    egPosition3(hSize, -hSize, hSize);

    egNormal(1, 0, 0);
    egTangent(0, 1, 0);
    egBinormal(0, 0, -1);
    egTexCoord(0, 0);
    egPosition3(hSize, -hSize, hSize);
    egTexCoord(0, 1);
    egPosition3(hSize, -hSize, -hSize);
    egTexCoord(1, 1);
    egPosition3(hSize, hSize, -hSize);
    egTexCoord(1, 0);
    egPosition3(hSize, hSize, hSize);

    egNormal(0, 1, 0);
    egTangent(-1, 0, 0);
    egBinormal(0, 0, -1);
    egTexCoord(0, 0);
    egPosition3(hSize, hSize, hSize);
    egTexCoord(0, 1);
    egPosition3(hSize, hSize, -hSize);
    egTexCoord(1, 1);
    egPosition3(-hSize, hSize, -hSize);
    egTexCoord(1, 0);
    egPosition3(-hSize, hSize, hSize);

    egNormal(-1, 0, 0);
    egTangent(0, -1, 0);
    egBinormal(0, 0, -1);
    egTexCoord(0, 0);
    egPosition3(-hSize, hSize, hSize);
    egTexCoord(0, 1);
    egPosition3(-hSize, hSize, -hSize);
    egTexCoord(1, 1);
    egPosition3(-hSize, -hSize, -hSize);
    egTexCoord(1, 0);
    egPosition3(-hSize, -hSize, hSize);

    egNormal(0, 0, 1);
    egTangent(1, 0, 0);
    egBinormal(0, -1, 0);
    egTexCoord(0, 0);
    egPosition3(-hSize, hSize, hSize);
    egTexCoord(0, 1);
    egPosition3(-hSize, -hSize, hSize);
    egTexCoord(1, 1);
    egPosition3(hSize, -hSize, hSize);
    egTexCoord(1, 0);
    egPosition3(hSize, hSize, hSize);

    egNormal(0, 0, -1);
    egTangent(1, 0, 0);
    egBinormal(0, 1, 0);
    egTexCoord(0, 0);
    egPosition3(-hSize, -hSize, -hSize);
    egTexCoord(0, 1);
    egPosition3(-hSize, hSize, -hSize);
    egTexCoord(1, 1);
    egPosition3(hSize, hSize, -hSize);
    egTexCoord(1, 0);
    egPosition3(hSize, -hSize, -hSize);

    egEnd();
}

void egSphere(float radius, uint32_t slices, uint32_t stacks)
{
    if (slices < 3) return;
    if (stacks < 2) return;
}

void egCylinder(float bottomRadius, float topRadius, float height, uint32_t slices)
{
    if (slices < 3) return;
}

void egTube(float outterRadius, float innerRadius, float height, uint32_t slices)
{
    if (slices < 3) return;
}

void egTorus(float radius, float innerRadius, uint32_t slices, uint32_t stacks)
{
    if (slices < 3) return;
    if (stacks < 3) return;
}
