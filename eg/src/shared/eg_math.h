#pragma once

#ifndef EG_MATH_H_INCLUDED
#define EG_MATH_H_INCLUDED

// Vector math
void v3normalize(float* v);
void v3cross(float* v1, float* v2, float* out);
float v3dot(float* v1, float* v2);

// Matrices
typedef struct
{
    float m[16];
} SEGMatrix;

void setIdentityMatrix(SEGMatrix *pMatrix);
void multMatrix(const SEGMatrix *pM1, const SEGMatrix *pM2, SEGMatrix *pMOut);
void swapf(float *a, float *b);
void setTranslationMatrix(SEGMatrix *pMatrix, float x, float y, float z);
void setScaleMatrix(SEGMatrix *pMatrix, float x, float y, float z);
void setRotationMatrix(SEGMatrix *pMatrix, float xDeg, float yDeg, float zDeg);
void transposeMatrix(SEGMatrix *pMatrix);
void setLookAtMatrix(SEGMatrix *pMatrix,
                     float eyex, float eyey, float eyez,
                     float centerx, float centery, float centerz,
                     float upx, float upy, float upz);
void setProjectionMatrix(SEGMatrix *pMatrix, float fov, float aspect, float nearDist, float farDist);
float detMatrix(SEGMatrix *pMatrix);
void inverseMatrix(SEGMatrix *pMatrix, SEGMatrix *pOut);

#endif /* EG_MATH_H_INCLUDED */
