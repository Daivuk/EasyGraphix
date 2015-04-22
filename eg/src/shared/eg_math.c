#include <memory.h>
#include "eg_math.h"

// Vector math
void v3normalize(float* v)
{
    float len = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    len = sqrtf(len);
    v[0] /= len;
    v[1] /= len;
    v[2] /= len;
}

void v3cross(float* v1, float* v2, float* out)
{
    out[0] = v1[1] * v2[2] - v1[2] * v2[1];
    out[1] = v1[2] * v2[0] - v1[0] * v2[2];
    out[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

float v3dot(float* v1, float* v2)
{
    return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

// Matrices
#define at(__row__, __col__) m[__row__ * 4 + __col__]

void setIdentityMatrix(SEGMatrix *pMatrix)
{
    memset(pMatrix, 0, sizeof(SEGMatrix));
    pMatrix->m[0] = 1;
    pMatrix->m[5] = 1;
    pMatrix->m[10] = 1;
    pMatrix->m[15] = 1;
}

void multMatrix(const SEGMatrix *pM1, const SEGMatrix *pM2, SEGMatrix *pMOut)
{
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            float n = 0;
            for (int k = 0; k < 4; k++)
                n += pM2->at(i, k) * pM1->at(k, j);
            pMOut->at(i, j) = n;
        }
    }
}

void swapf(float *a, float *b)
{
    float temp;

    temp = *b;
    *b = *a;
    *a = temp;
}

void setTranslationMatrix(SEGMatrix *pMatrix, float x, float y, float z)
{
    memset(pMatrix, 0, sizeof(SEGMatrix));
    pMatrix->m[0] = 1;
    pMatrix->m[5] = 1;
    pMatrix->m[10] = 1;
    pMatrix->m[15] = 1;
    pMatrix->m[12] = x;
    pMatrix->m[13] = y;
    pMatrix->m[14] = z;
}

void setScaleMatrix(SEGMatrix *pMatrix, float x, float y, float z)
{
    memset(pMatrix, 0, sizeof(SEGMatrix));
    pMatrix->m[0] = x;
    pMatrix->m[5] = y;
    pMatrix->m[10] = z;
    pMatrix->m[15] = 1;
}

void setRotationMatrix(SEGMatrix *pMatrix, float xDeg, float yDeg, float zDeg)
{
    float xRads = (EG_TO_RAD(xDeg));
    float yRads = (EG_TO_RAD(yDeg));
    float zRads = (EG_TO_RAD(zDeg));

    SEGMatrix ma, mb, mc;
    float ac = cosf(xRads);
    float as = sinf(xRads);
    float bc = cosf(yRads);
    float bs = sinf(yRads);
    float cc = cosf(zRads);
    float cs = sinf(zRads);

    setIdentityMatrix(&ma);
    setIdentityMatrix(&mb);
    setIdentityMatrix(&mc);

    ma.at(1, 1) = ac;
    ma.at(2, 1) = as;
    ma.at(1, 2) = -as;
    ma.at(2, 2) = ac;

    mb.at(0, 0) = bc;
    mb.at(2, 0) = -bs;
    mb.at(0, 2) = bs;
    mb.at(2, 2) = bc;

    mc.at(0, 0) = cc;
    mc.at(1, 0) = cs;
    mc.at(0, 1) = -cs;
    mc.at(1, 1) = cc;

    SEGMatrix ret;
    multMatrix(&ma, &mb, &ret);
    multMatrix(&ret, &mc, pMatrix);
}

void transposeMatrix(SEGMatrix *pMatrix)
{
    swapf(&pMatrix->at(0, 1), &pMatrix->at(1, 0));
    swapf(&pMatrix->at(0, 2), &pMatrix->at(2, 0));
    swapf(&pMatrix->at(0, 3), &pMatrix->at(3, 0));

    swapf(&pMatrix->at(1, 2), &pMatrix->at(2, 1));
    swapf(&pMatrix->at(1, 3), &pMatrix->at(3, 1));

    swapf(&pMatrix->at(2, 3), &pMatrix->at(3, 2));
}

void setLookAtMatrix(SEGMatrix *pMatrix,
                     float eyex, float eyey, float eyez,
                     float centerx, float centery, float centerz,
                     float upx, float upy, float upz)
{
    float forward[3], side[3], up[3];

    forward[0] = centerx - eyex;
    forward[1] = centery - eyey;
    forward[2] = centerz - eyez;

    up[0] = upx;
    up[1] = upy;
    up[2] = upz;

    v3normalize(forward);

    /* Side = forward x up */
    v3cross(forward, up, side);
    v3normalize(side);

    /* Recompute up as: up = side x forward */
    v3cross(side, forward, up);

    SEGMatrix matrix;
    setIdentityMatrix(&matrix);
    matrix.at(0, 0) = side[0];
    matrix.at(1, 0) = side[1];
    matrix.at(2, 0) = side[2];

    matrix.at(0, 1) = up[0];
    matrix.at(1, 1) = up[1];
    matrix.at(2, 1) = up[2];

    matrix.at(0, 2) = -forward[0];
    matrix.at(1, 2) = -forward[1];
    matrix.at(2, 2) = -forward[2];

    SEGMatrix translationMatrix;
    setIdentityMatrix(&translationMatrix);
    translationMatrix.m[12] = -eyex;
    translationMatrix.m[13] = -eyey;
    translationMatrix.m[14] = -eyez;
    multMatrix(&matrix, &translationMatrix, pMatrix);
    transposeMatrix(pMatrix);
}

void setProjectionMatrix(SEGMatrix *pMatrix, float fov, float aspect, float nearDist, float farDist)
{
    float yScale = 1.0f / tanf(EG_TO_RAD(fov) / 2);
    float xScale = yScale / aspect;
    float nearmfar = nearDist - farDist;
    float m[] = {
        xScale, 0, 0, 0,
        0, yScale, 0, 0,
        0, 0, (farDist + nearDist) / nearmfar, -1,
        0, 0, 2 * farDist * nearDist / nearmfar, 0
    };
    memcpy(pMatrix->m, m, sizeof(double) * 16);
    transposeMatrix(pMatrix);
}

float detMatrix(SEGMatrix *pMatrix)
{
    return
        +pMatrix->at(3, 0) * pMatrix->at(2, 1) * pMatrix->at(1, 2) * pMatrix->at(0, 3) - pMatrix->at(2, 0) * pMatrix->at(3, 1) * pMatrix->at(1, 2) * pMatrix->at(0, 3)
        - pMatrix->at(3, 0) * pMatrix->at(1, 1) * pMatrix->at(2, 2) * pMatrix->at(0, 3) + pMatrix->at(1, 0) * pMatrix->at(3, 1) * pMatrix->at(2, 2) * pMatrix->at(0, 3)

        + pMatrix->at(2, 0) * pMatrix->at(1, 1) * pMatrix->at(3, 2) * pMatrix->at(0, 3) - pMatrix->at(1, 0) * pMatrix->at(2, 1) * pMatrix->at(3, 2) * pMatrix->at(0, 3)
        - pMatrix->at(3, 0) * pMatrix->at(2, 1) * pMatrix->at(0, 2) * pMatrix->at(1, 3) + pMatrix->at(2, 0) * pMatrix->at(3, 1) * pMatrix->at(0, 2) * pMatrix->at(1, 3)

        + pMatrix->at(3, 0) * pMatrix->at(0, 1) * pMatrix->at(2, 2) * pMatrix->at(1, 3) - pMatrix->at(0, 0) * pMatrix->at(3, 1) * pMatrix->at(2, 2) * pMatrix->at(1, 3)
        - pMatrix->at(2, 0) * pMatrix->at(0, 1) * pMatrix->at(3, 2) * pMatrix->at(1, 3) + pMatrix->at(0, 0) * pMatrix->at(2, 1) * pMatrix->at(3, 2) * pMatrix->at(1, 3)

        + pMatrix->at(3, 0) * pMatrix->at(1, 1) * pMatrix->at(0, 2) * pMatrix->at(2, 3) - pMatrix->at(1, 0) * pMatrix->at(3, 1) * pMatrix->at(0, 2) * pMatrix->at(2, 3)
        - pMatrix->at(3, 0) * pMatrix->at(0, 1) * pMatrix->at(1, 2) * pMatrix->at(2, 3) + pMatrix->at(0, 0) * pMatrix->at(3, 1) * pMatrix->at(1, 2) * pMatrix->at(2, 3)

        + pMatrix->at(1, 0) * pMatrix->at(0, 1) * pMatrix->at(3, 2) * pMatrix->at(2, 3) - pMatrix->at(0, 0) * pMatrix->at(1, 1) * pMatrix->at(3, 2) * pMatrix->at(2, 3)
        - pMatrix->at(2, 0) * pMatrix->at(1, 1) * pMatrix->at(0, 2) * pMatrix->at(3, 3) + pMatrix->at(1, 0) * pMatrix->at(2, 1) * pMatrix->at(0, 2) * pMatrix->at(3, 3)

        + pMatrix->at(2, 0) * pMatrix->at(0, 1) * pMatrix->at(1, 2) * pMatrix->at(3, 3) - pMatrix->at(0, 0) * pMatrix->at(2, 1) * pMatrix->at(1, 2) * pMatrix->at(3, 3)
        - pMatrix->at(1, 0) * pMatrix->at(0, 1) * pMatrix->at(2, 2) * pMatrix->at(3, 3) + pMatrix->at(0, 0) * pMatrix->at(1, 1) * pMatrix->at(2, 2) * pMatrix->at(3, 3);
}

void inverseMatrix(SEGMatrix *pMatrix, SEGMatrix *pOut)
{
    SEGMatrix ret;

    ret.at(0, 0) = +pMatrix->at(2, 1) *pMatrix->at(3, 2) *pMatrix->at(1, 3) - pMatrix->at(3, 1) *pMatrix->at(2, 2) *pMatrix->at(1, 3) + pMatrix->at(3, 1) *pMatrix->at(1, 2) *pMatrix->at(2, 3)
        - pMatrix->at(1, 1) *pMatrix->at(3, 2) *pMatrix->at(2, 3) - pMatrix->at(2, 1) *pMatrix->at(1, 2) *pMatrix->at(3, 3) + pMatrix->at(1, 1) *pMatrix->at(2, 2) *pMatrix->at(3, 3);

    ret.at(1, 0) = +pMatrix->at(3, 0) *pMatrix->at(2, 2) *pMatrix->at(1, 3) - pMatrix->at(2, 0) *pMatrix->at(3, 2) *pMatrix->at(1, 3) - pMatrix->at(3, 0) *pMatrix->at(1, 2) *pMatrix->at(2, 3)
        + pMatrix->at(1, 0) *pMatrix->at(3, 2) *pMatrix->at(2, 3) + pMatrix->at(2, 0) *pMatrix->at(1, 2) *pMatrix->at(3, 3) - pMatrix->at(1, 0) *pMatrix->at(2, 2) *pMatrix->at(3, 3);

    ret.at(2, 0) = +pMatrix->at(2, 0) *pMatrix->at(3, 1) *pMatrix->at(1, 3) - pMatrix->at(3, 0) *pMatrix->at(2, 1) *pMatrix->at(1, 3) + pMatrix->at(3, 0) *pMatrix->at(1, 1) *pMatrix->at(2, 3)
        - pMatrix->at(1, 0) *pMatrix->at(3, 1) *pMatrix->at(2, 3) - pMatrix->at(2, 0) *pMatrix->at(1, 1) *pMatrix->at(3, 3) + pMatrix->at(1, 0) *pMatrix->at(2, 1) *pMatrix->at(3, 3);

    ret.at(3, 0) = +pMatrix->at(3, 0) *pMatrix->at(2, 1) *pMatrix->at(1, 2) - pMatrix->at(2, 0) *pMatrix->at(3, 1) *pMatrix->at(1, 2) - pMatrix->at(3, 0) *pMatrix->at(1, 1) *pMatrix->at(2, 2)
        + pMatrix->at(1, 0) *pMatrix->at(3, 1) *pMatrix->at(2, 2) + pMatrix->at(2, 0) *pMatrix->at(1, 1) *pMatrix->at(3, 2) - pMatrix->at(1, 0) *pMatrix->at(2, 1) *pMatrix->at(3, 2);

    ret.at(0, 1) = +pMatrix->at(3, 1) *pMatrix->at(2, 2) *pMatrix->at(0, 3) - pMatrix->at(2, 1) *pMatrix->at(3, 2) *pMatrix->at(0, 3) - pMatrix->at(3, 1) *pMatrix->at(0, 2) *pMatrix->at(2, 3)
        + pMatrix->at(0, 1) *pMatrix->at(3, 2) *pMatrix->at(2, 3) + pMatrix->at(2, 1) *pMatrix->at(0, 2) *pMatrix->at(3, 3) - pMatrix->at(0, 1) *pMatrix->at(2, 2) *pMatrix->at(3, 3);

    ret.at(1, 1) = +pMatrix->at(2, 0) *pMatrix->at(3, 2) *pMatrix->at(0, 3) - pMatrix->at(3, 0) *pMatrix->at(2, 2) *pMatrix->at(0, 3) + pMatrix->at(3, 0) *pMatrix->at(0, 2) *pMatrix->at(2, 3)
        - pMatrix->at(0, 0) *pMatrix->at(3, 2) *pMatrix->at(2, 3) - pMatrix->at(2, 0) *pMatrix->at(0, 2) *pMatrix->at(3, 3) + pMatrix->at(0, 0) *pMatrix->at(2, 2) *pMatrix->at(3, 3);

    ret.at(2, 1) = +pMatrix->at(3, 0) *pMatrix->at(2, 1) *pMatrix->at(0, 3) - pMatrix->at(2, 0) *pMatrix->at(3, 1) *pMatrix->at(0, 3) - pMatrix->at(3, 0) *pMatrix->at(0, 1) *pMatrix->at(2, 3)
        + pMatrix->at(0, 0) *pMatrix->at(3, 1) *pMatrix->at(2, 3) + pMatrix->at(2, 0) *pMatrix->at(0, 1) *pMatrix->at(3, 3) - pMatrix->at(0, 0) *pMatrix->at(2, 1) *pMatrix->at(3, 3);

    ret.at(3, 1) = +pMatrix->at(2, 0) *pMatrix->at(3, 1) *pMatrix->at(0, 2) - pMatrix->at(3, 0) *pMatrix->at(2, 1) *pMatrix->at(0, 2) + pMatrix->at(3, 0) *pMatrix->at(0, 1) *pMatrix->at(2, 2)
        - pMatrix->at(0, 0) *pMatrix->at(3, 1) *pMatrix->at(2, 2) - pMatrix->at(2, 0) *pMatrix->at(0, 1) *pMatrix->at(3, 2) + pMatrix->at(0, 0) *pMatrix->at(2, 1) *pMatrix->at(3, 2);

    ret.at(0, 2) = +pMatrix->at(1, 1) *pMatrix->at(3, 2) *pMatrix->at(0, 3) - pMatrix->at(3, 1) *pMatrix->at(1, 2) *pMatrix->at(0, 3) + pMatrix->at(3, 1) *pMatrix->at(0, 2) *pMatrix->at(1, 3)
        - pMatrix->at(0, 1) *pMatrix->at(3, 2) *pMatrix->at(1, 3) - pMatrix->at(1, 1) *pMatrix->at(0, 2) *pMatrix->at(3, 3) + pMatrix->at(0, 1) *pMatrix->at(1, 2) *pMatrix->at(3, 3);

    ret.at(1, 2) = +pMatrix->at(3, 0) *pMatrix->at(1, 2) *pMatrix->at(0, 3) - pMatrix->at(1, 0) *pMatrix->at(3, 2) *pMatrix->at(0, 3) - pMatrix->at(3, 0) *pMatrix->at(0, 2) *pMatrix->at(1, 3)
        + pMatrix->at(0, 0) *pMatrix->at(3, 2) *pMatrix->at(1, 3) + pMatrix->at(1, 0) *pMatrix->at(0, 2) *pMatrix->at(3, 3) - pMatrix->at(0, 0) *pMatrix->at(1, 2) *pMatrix->at(3, 3);

    ret.at(2, 2) = +pMatrix->at(1, 0) *pMatrix->at(3, 1) *pMatrix->at(0, 3) - pMatrix->at(3, 0) *pMatrix->at(1, 1) *pMatrix->at(0, 3) + pMatrix->at(3, 0) *pMatrix->at(0, 1) *pMatrix->at(1, 3)
        - pMatrix->at(0, 0) *pMatrix->at(3, 1) *pMatrix->at(1, 3) - pMatrix->at(1, 0) *pMatrix->at(0, 1) *pMatrix->at(3, 3) + pMatrix->at(0, 0) *pMatrix->at(1, 1) *pMatrix->at(3, 3);

    ret.at(3, 2) = +pMatrix->at(3, 0) *pMatrix->at(1, 1) *pMatrix->at(0, 2) - pMatrix->at(1, 0) *pMatrix->at(3, 1) *pMatrix->at(0, 2) - pMatrix->at(3, 0) *pMatrix->at(0, 1) *pMatrix->at(1, 2)
        + pMatrix->at(0, 0) *pMatrix->at(3, 1) *pMatrix->at(1, 2) + pMatrix->at(1, 0) *pMatrix->at(0, 1) *pMatrix->at(3, 2) - pMatrix->at(0, 0) *pMatrix->at(1, 1) *pMatrix->at(3, 2);

    ret.at(0, 3) = +pMatrix->at(2, 1) *pMatrix->at(1, 2) *pMatrix->at(0, 3) - pMatrix->at(1, 1) *pMatrix->at(2, 2) *pMatrix->at(0, 3) - pMatrix->at(2, 1) *pMatrix->at(0, 2) *pMatrix->at(1, 3)
        + pMatrix->at(0, 1) *pMatrix->at(2, 2) *pMatrix->at(1, 3) + pMatrix->at(1, 1) *pMatrix->at(0, 2) *pMatrix->at(2, 3) - pMatrix->at(0, 1) *pMatrix->at(1, 2) *pMatrix->at(2, 3);

    ret.at(1, 3) = +pMatrix->at(1, 0) *pMatrix->at(2, 2) *pMatrix->at(0, 3) - pMatrix->at(2, 0) *pMatrix->at(1, 2) *pMatrix->at(0, 3) + pMatrix->at(2, 0) *pMatrix->at(0, 2) *pMatrix->at(1, 3)
        - pMatrix->at(0, 0) *pMatrix->at(2, 2) *pMatrix->at(1, 3) - pMatrix->at(1, 0) *pMatrix->at(0, 2) *pMatrix->at(2, 3) + pMatrix->at(0, 0) *pMatrix->at(1, 2) *pMatrix->at(2, 3);

    ret.at(2, 3) = +pMatrix->at(2, 0) *pMatrix->at(1, 1) *pMatrix->at(0, 3) - pMatrix->at(1, 0) *pMatrix->at(2, 1) *pMatrix->at(0, 3) - pMatrix->at(2, 0) *pMatrix->at(0, 1) *pMatrix->at(1, 3)
        + pMatrix->at(0, 0) *pMatrix->at(2, 1) *pMatrix->at(1, 3) + pMatrix->at(1, 0) *pMatrix->at(0, 1) *pMatrix->at(2, 3) - pMatrix->at(0, 0) *pMatrix->at(1, 1) *pMatrix->at(2, 3);

    ret.at(3, 3) = +pMatrix->at(1, 0) *pMatrix->at(2, 1) *pMatrix->at(0, 2) - pMatrix->at(2, 0) *pMatrix->at(1, 1) *pMatrix->at(0, 2) + pMatrix->at(2, 0) *pMatrix->at(0, 1) *pMatrix->at(1, 2)
        - pMatrix->at(0, 0) *pMatrix->at(2, 1) *pMatrix->at(1, 2) - pMatrix->at(1, 0) *pMatrix->at(0, 1) *pMatrix->at(2, 2) + pMatrix->at(0, 0) *pMatrix->at(1, 1) *pMatrix->at(2, 2);

    float det = detMatrix(pMatrix);
    for (int i = 0; i < 16; ++i)
    {
        ret.m[i] /= det;
    }
    memcpy(pOut, &ret, sizeof(SEGMatrix));
}
