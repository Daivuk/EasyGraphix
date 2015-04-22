#include "eg.h"
#include "eg_math.h"

void egCube(float size)
{
    float hSize = size * .5f;

    egBegin(EG_QUADS);
    {
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
    }
    egEnd();
}

void egSphere(float radius, uint32_t slices, uint32_t stacks, float sfactor)
{
    if (slices < 3) return;
    if (stacks < 2) return;

    // Sides
    for (uint32_t j = 0; j < stacks; ++j)
    {
        float cosB = -cosf(((float)j / (float)stacks) * EG_PI);
        float cosT = -cosf(((float)(j + 1) / (float)stacks) * EG_PI);
        float aCosB = fabsf(cosB);
        float aCosT = fabsf(cosT);
        float sinB = sinf(((float)j / (float)stacks) * EG_PI);
        float sinT = sinf(((float)(j + 1) / (float)stacks) * EG_PI);
        float aSinB = fabsf(sinB);
        float aSinT = fabsf(sinT);
        for (uint32_t i = 0; i < slices; ++i)
        {
            egBegin(EG_TRIANGLE_STRIP);
            {
                for (uint32_t i = 0; i <= slices; ++i)
                {
                    float cosTheta = cosf((float)i / (float)slices * EG_PI * 2);
                    float sinTheta = sinf((float)i / (float)slices * EG_PI * 2);

                    egNormal(cosTheta * aSinT, -sinTheta * aSinT, cosT);
                    egTangent(-sinTheta, -cosTheta, 0);
                    egBinormal(cosTheta * cosT, -sinTheta * cosT, aSinT);
                    egTexCoord((float)i / (float)slices * sfactor, 1 - cosT * .5f + .5f);
                    egPosition3(cosTheta * radius * aSinT,
                                -sinTheta * radius * aSinT,
                                cosT * radius);

                    egNormal(cosTheta * aSinB, -sinTheta * aSinB, cosB);
                    egTangent(-sinTheta, -cosTheta, 0);
                    egBinormal(cosTheta * cosB, -sinTheta * cosB, aSinB);
                    egTexCoord((float)i / (float)slices * sfactor, 1 - cosB * .5f + .5f);
                    egPosition3(cosTheta * radius * aSinB,
                                -sinTheta * radius * aSinB,
                                cosB * radius);
                }
            }
            egEnd();
        }
    }
}

void egCylinder(float bottomRadius, float topRadius, float height, uint32_t slices, float sfactor)
{
    if (slices < 3) return;

    // Caps
    egBegin(EG_POLYGON);
    {
        egNormal(0, 0, 1);
        egTangent(1, 0, 0);
        egBinormal(0, -1, 0);
        for (uint32_t i = 0; i < slices; ++i)
        {
            float cosTheta = cosf((float)i / (float)slices * EG_PI * 2);
            float sinTheta = sinf((float)i / (float)slices * EG_PI * 2);
            egTexCoord(cosTheta * .5f + .5f, (sinTheta * .5f) + .5f);
            egPosition3(cosTheta * topRadius, -sinTheta * topRadius, height);
        }
    }
    egEnd();

    egBegin(EG_POLYGON);
    {
        egNormal(0, 0, -1);
        egTangent(1, 0, 0);
        egBinormal(0, 1, 0);
        for (uint32_t i = 0; i < slices; ++i)
        {
            float cosTheta = cosf((float)i / (float)slices * EG_PI * 2);
            float sinTheta = sinf((float)i / (float)slices * EG_PI * 2);
            egTexCoord(cosTheta * .5f + .5f, sinTheta * .5f + .5f);
            egPosition3(cosTheta * bottomRadius, sinTheta * bottomRadius, 0);
        }
    }
    egEnd();

    // Body
    egBegin(EG_TRIANGLE_STRIP);
    {
        for (uint32_t i = 0; i <= slices; ++i)
        {
            float cosTheta = cosf((float)i / (float)slices * EG_PI * 2);
            float sinTheta = sinf((float)i / (float)slices * EG_PI * 2);
            egNormal(cosTheta, -sinTheta, 0);
            egTangent(-sinTheta, -cosTheta, 0);
            egBinormal(0, 0, -1);
            egTexCoord((float)i / (float)slices * sfactor, 0);
            egPosition3(cosTheta * topRadius, -sinTheta * topRadius, height);
            egTexCoord((float)i / (float)slices * sfactor, 1);
            egPosition3(cosTheta * bottomRadius, -sinTheta * bottomRadius, 0);
        }
    }
    egEnd();
}

void egTube(float outterRadius, float innerRadius, float height, uint32_t slices, float sfactor)
{
    if (slices < 3) return;

    // Caps
    egBegin(EG_TRIANGLE_STRIP);
    {
        egNormal(0, 0, 1);
        egTangent(1, 0, 0);
        egBinormal(0, -1, 0);
        for (uint32_t i = 0; i <= slices; ++i)
        {
            float cosTheta = cosf((float)i / (float)slices * EG_PI * 2);
            float sinTheta = sinf((float)i / (float)slices * EG_PI * 2);

            egTexCoord(cosTheta * .5f * innerRadius / outterRadius + .5f,
                       sinTheta * .5f * innerRadius / outterRadius + .5f);
            egPosition3(cosTheta * innerRadius, -sinTheta * innerRadius, height);

            egTexCoord(cosTheta * .5f + .5f, (sinTheta * .5f) + .5f);
            egPosition3(cosTheta * outterRadius, -sinTheta * outterRadius, height);
        }
    }
    egEnd();

    egBegin(EG_TRIANGLE_STRIP);
    {
        egNormal(0, 0, -1);
        egTangent(1, 0, 0);
        egBinormal(0, 1, 0);
        for (uint32_t i = 0; i <= slices; ++i)
        {
            float cosTheta = cosf((float)i / (float)slices * EG_PI * 2);
            float sinTheta = sinf((float)i / (float)slices * EG_PI * 2);

            egTexCoord(cosTheta * .5f * innerRadius / outterRadius + .5f,
                       sinTheta * .5f * innerRadius / outterRadius + .5f);
            egPosition3(cosTheta * innerRadius, sinTheta * innerRadius, 0);

            egTexCoord(cosTheta * .5f + .5f, (sinTheta * .5f) + .5f);
            egPosition3(cosTheta * outterRadius, sinTheta * outterRadius, 0);
        }
    }
    egEnd();

    // Outter radius
    egBegin(EG_TRIANGLE_STRIP);
    {
        for (uint32_t i = 0; i <= slices; ++i)
        {
            float cosTheta = cosf((float)i / (float)slices * EG_PI * 2);
            float sinTheta = sinf((float)i / (float)slices * EG_PI * 2);
            egNormal(cosTheta, -sinTheta, 0);
            egTangent(-sinTheta, -cosTheta, 0);
            egBinormal(0, 0, -1);
            egTexCoord((float)i / (float)slices * sfactor, 0);
            egPosition3(cosTheta * outterRadius, -sinTheta * outterRadius, height);
            egTexCoord((float)i / (float)slices * sfactor, 1);
            egPosition3(cosTheta * outterRadius, -sinTheta * outterRadius, 0);
        }
    }
    egEnd();

    // Inner radius
    egBegin(EG_TRIANGLE_STRIP);
    {
        for (uint32_t i = 0; i <= slices; ++i)
        {
            float cosTheta = cosf((float)i / (float)slices * EG_PI * 2);
            float sinTheta = sinf((float)i / (float)slices * EG_PI * 2);
            egNormal(-cosTheta, -sinTheta, 0);
            egTangent(-sinTheta, cosTheta, 0);
            egBinormal(0, 0, -1);
            egTexCoord((float)i / (float)slices * sfactor, 0);
            egPosition3(cosTheta * innerRadius, sinTheta * innerRadius, height);
            egTexCoord((float)i / (float)slices * sfactor, 1);
            egPosition3(cosTheta * innerRadius, sinTheta * innerRadius, 0);
        }
    }
    egEnd();
}

void egTorus(float radius, float innerRadius, uint32_t slices, uint32_t stacks, float sfactor)
{
    if (slices < 3) return;
    if (stacks < 3) return;
}
