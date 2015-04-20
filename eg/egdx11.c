#include <assert.h>
#include <stdio.h>
#include <d3d11.h>
#include <D3Dcompiler.h>
#include <math.h>
#include "eg.h"

#define PI 3.1415926535897932384626433832795f
#define TO_RAD(__deg__) (__deg__ * PI / 180.f)
#define TO_DEG(__rad__) (__rad__ * 180.f / PI)

char lastError[256] = {0};

// Shaders
const char *g_vs =
"\
cbuffer ViewProjCB:register(b0)\n\
{\n\
    matrix viewProj;\n\
}\n\
cbuffer ModelVB:register(b1)\n\
{\n\
    matrix model;\n\
}\n\
struct sInput\n\
{\n\
    float3 position:POSITION;\n\
    float3 normal:NORMAL;\n\
    float3 tangent:TANGENT;\n\
    float3 binormal:BINORMAL;\n\
    float2 texCoord:TEXCOORD;\n\
    float4 color:COLOR;\n\
};\n\
struct sOutput\n\
{\n\
    float4 position:SV_POSITION;\n\
    float3 normal:NORMAL;\n\
    float3 tangent:TANGENT;\n\
    float3 binormal:BINORMAL;\n\
    float2 texCoord:TEXCOORD;\n\
    float4 color:COLOR;\n\
    float2 depth:TEXCOORD1;\n\
};\n\
sOutput main(sInput input)\n\
{\n\
    sOutput output;\n\
    float4 worldPosition = mul(float4(input.position, 1), model);\n\
    output.position = mul(worldPosition, viewProj);\n\
    output.normal = normalize(mul(float4(input.normal, 0), model).xyz);\n\
    output.tangent = normalize(mul(float4(input.tangent, 0), model).xyz);\n\
    output.binormal = normalize(mul(float4(input.binormal, 0), model).xyz);\n\
    output.texCoord = input.texCoord;\n\
    output.color = input.color;\n\
    output.depth.xy = output.position.zw;\n\
    return output;\n\
}";

const char *g_ps =
"\
Texture2D xDiffuse:register(t0);\n\
Texture2D xNormal:register(t1);\n\
Texture2D xMaterial:register(t2);\n\
SamplerState sSampler:register(s0);\n\
struct sInput\n\
{\n\
    float4 position:SV_POSITION;\n\
    float3 normal:NORMAL;\n\
    float3 tangent:TANGENT;\n\
    float3 binormal:BINORMAL;\n\
    float2 texCoord:TEXCOORD;\n\
    float4 color:COLOR;\n\
    float2 depth:TEXCOORD1;\n\
};\n\
struct sOutput\n\
{\n\
    float4 diffuse:SV_Target0;\n\
    float4 depth:SV_Target1;\n\
    float4 normal:SV_Target2;\n\
    float4 material:SV_Target3;\n\
};\n\
sOutput main(sInput input)\n\
{\n\
    float4 xdiffuse = xDiffuse.Sample(sSampler, input.texCoord);\n\
    float4 xnormal = xNormal.Sample(sSampler, input.texCoord);\n\
    float4 xmaterial = xMaterial.Sample(sSampler, input.texCoord);\n\
    sOutput output;\n\
    output.diffuse = xdiffuse * input.color;\n\
    output.depth = input.depth.x / input.depth.y;\n\
    xnormal.xyz = xnormal.xyz * 2 - 1;\n\
    output.normal.xyz = normalize(xnormal.x * input.tangent + xnormal.y * input.binormal + xnormal.z * input.normal);\n\
    output.normal.xyz = output.normal.xyz * .5 + .5;\n\
    output.normal.a = 0;\n\
    output.material = xmaterial;\n\
    return output;\n\
}";

const char *g_vsPassThrough =
"\
struct sInput\n\
{\n\
    float2 position:POSITION;\n\
    float2 texCoord:TEXCOORD;\n\
    float4 color:COLOR;\n\
};\n\
struct sOutput\n\
{\n\
    float4 position:SV_POSITION;\n\
    float2 texCoord:TEXCOORD;\n\
    float4 color:COLOR;\n\
};\n\
sOutput main(sInput input)\n\
{\n\
    sOutput output;\n\
    output.position = float4(input.position, 0, 1);\n\
    output.texCoord = input.texCoord;\n\
    output.color = input.color;\n\
    return output;\n\
}";

const char *g_psPassThrough =
"\
Texture2D xDiffuse:register(t0);\n\
SamplerState sSampler:register(s0);\n\
struct sInput\n\
{\n\
    float4 position:SV_POSITION;\n\
    float2 texCoord:TEXCOORD;\n\
    float4 color:COLOR;\n\
};\n\
float4 main(sInput input):SV_TARGET\n\
{\n\
    float4 xdiffuse = xDiffuse.Sample(sSampler, input.texCoord);\n\
    return xdiffuse * input.color;\n\
}";

const char *g_psAmbient =
"\
Texture2D xDiffuse:register(t0);\n\
Texture2D xMaterial:register(t3);\n\
SamplerState sSampler:register(s0);\n\
struct sInput\n\
{\n\
    float4 position:SV_POSITION;\n\
    float2 texCoord:TEXCOORD;\n\
    float4 color:COLOR;\n\
};\n\
float4 main(sInput input):SV_TARGET\n\
{\n\
    float4 xdiffuse = xDiffuse.Sample(sSampler, input.texCoord);\n\
    float4 xmaterial = xMaterial.Sample(sSampler, input.texCoord);\n\
    return xdiffuse * input.color + xdiffuse * xmaterial.b;\n\
}";

const char *g_psOmni =
"\
cbuffer InvViewProjCB:register(b0)\n\
{\n\
    matrix invViewProjection;\n\
}\n\
cbuffer OmniCB:register(b1)\n\
{\n\
    float3 lPos;\n\
    float lRadius;\n\
    float4 lColor;\n\
}\n\
Texture2D xDiffuse:register(t0);\n\
Texture2D xDepth:register(t1);\n\
Texture2D xNormal:register(t2);\n\
Texture2D xMaterial:register(t3);\n\
SamplerState sSampler:register(s0);\n\
struct sInput\n\
{\n\
    float4 position:SV_POSITION;\n\
    float2 texCoord:TEXCOORD;\n\
    float4 color:COLOR;\n\
};\n\
float4 main(sInput input):SV_TARGET\n\
{\n\
    float4 xdiffuse = xDiffuse.Sample(sSampler, input.texCoord);\n\
    float xdepth = xDepth.Sample(sSampler, input.texCoord).r;\n\
    float3 xnormal = xNormal.Sample(sSampler, input.texCoord).xyz;\n\
    float4 xmaterial = xMaterial.Sample(sSampler, input.texCoord);\n\
    \n\
    float4 finalColor = float4(0, 0, 0, 0);\n\
    \n\
    // Position\n\
    float4 position;\n\
    position.xy = float2(input.texCoord.x * 2 - 1, -(input.texCoord.y * 2 - 1));\n\
    position.z = xdepth;\n\
    position.w = 1.0f;\n\
    position = mul(position, invViewProjection);\n\
    position /= position.w;\n\
    \n\
    // Normal\n\
    float3 normal = normalize(xnormal * 2 - 1);\n\
    float3 lightDir = lPos - position.xyz;\n\
    \n\
    // Attenuation stuff\n\
    float dis = length(lightDir);\n\
    lightDir /= dis;\n\
    dis /= lRadius;\n\
    dis = saturate(1 - dis);\n\
    dis = pow(dis, 1);\n\
    float dotNormal = dot(normal, lightDir);\n\
    //dotNormal = 1 - (1 - dotNormal) * (1 - dotNormal);\n\
    dotNormal = saturate(dotNormal);\n\
    float intensity = dis * dotNormal;\n\
    finalColor += xdiffuse * lColor * intensity;\n\
    \n\
    // Calculate specular\n\
    float3 v = normalize(float3(-5, -5, 5));\n\
    float3 h = normalize(lightDir + v);\n\
    finalColor += xmaterial.r * lColor * intensity * (pow(saturate(dot(normal, h)), xmaterial.g * 100)/*, 0*/);\n\
    \n\
    return finalColor;\n\
}";

#define MAX_VERTEX_COUNT (300 * 2 * 3)
#define DIFFUSE_MAP     0
#define NORMAL_MAP      1
#define MATERIAL_MAP    2
#define MAX_STACK       256 // Used for states and matrices
#define G_DIFFUSE       0
#define G_DEPTH         1
#define G_NORMAL        2
#define G_MATERIAL      3

typedef enum
{
    EG_GEOMETRY_PASS        = 0,
    EG_AMBIENT_PASS         = 1,
    EG_OMNI_PASS            = 2,
    EG_SPOT_PASS            = 3,
    EG_POST_PROCESS_PASS    = 4
} EG_PASS;

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
typedef struct
{
    float m[16];
} SEGMatrix;
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
    float xRads = (TO_RAD(xDeg));
    float yRads = (TO_RAD(yDeg));
    float zRads = (TO_RAD(zDeg));

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
    float yScale = 1.0f / tanf(fov / 2);
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
        + pMatrix->at(3, 0) * pMatrix->at(2, 1) * pMatrix->at(1, 2) * pMatrix->at(0, 3) - pMatrix->at(2, 0) * pMatrix->at(3, 1) * pMatrix->at(1, 2) * pMatrix->at(0, 3)
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

// Batch stuff
typedef struct
{
    float x, y, z;
    float nx, ny, nz;
    float tx, ty, tz;
    float bx, by, bz;
    float u, v;
    float r, g, b, a;
} SEGVertex;
SEGVertex *pVertex = NULL;
uint32_t currentVertexCount = 0;
BOOL bIsInBatch = FALSE;
SEGVertex currentVertex = {0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1};
EG_TOPOLOGY currentTopology;
typedef struct
{
    float x, y, z;
    float radius;
    float r, g, b, a;
} SEGOmni;
SEGOmni currentOmni;

// Textures
typedef struct
{
    ID3D11Texture2D            *pTexture;
    ID3D11ShaderResourceView   *pResourceView;
} SEGTexture2D;

// Render target
typedef struct
{
    SEGTexture2D                texture;
    ID3D11RenderTargetView     *pRenderTargetView;
} SEGRenderTarget2D;

// States
typedef struct
{
    D3D11_DEPTH_STENCIL_DESC    depth;
    BOOL                        depthDirty;
    D3D11_RASTERIZER_DESC       rasterizer;
    BOOL                        rasterizerDirty;
    D3D11_BLEND_DESC            blend;
    BOOL                        blendDirty;
    D3D11_SAMPLER_DESC          sampler;
    BOOL                        samplerDirty;
} SEGState;

// Devices
typedef struct
{
    IDXGISwapChain             *pSwapChain;
    ID3D11Device               *pDevice;
    ID3D11DeviceContext        *pDeviceContext;
    ID3D11RenderTargetView     *pRenderTargetView;
    ID3D11DepthStencilView     *pDepthStencilView;
    D3D11_TEXTURE2D_DESC        backBufferDesc;
    ID3D11VertexShader         *pVS;
    ID3D11PixelShader          *pPS;
    ID3D11InputLayout          *pInputLayout;
    ID3D11VertexShader         *pVSPassThrough;
    ID3D11PixelShader          *pPSPassThrough;
    ID3D11InputLayout          *pInputLayoutPassThrough;
    ID3D11PixelShader          *pPSAmbient;
    ID3D11PixelShader          *pPSOmni;
    ID3D11Buffer               *pCBViewProj;
    ID3D11Buffer               *pCBModel;
    ID3D11Buffer               *pCBInvViewProj;
    ID3D11Buffer               *pVertexBuffer;
    ID3D11Resource             *pVertexBufferRes;
    SEGTexture2D               *textures;
    uint32_t                    textureCount;
    SEGTexture2D                pDefaultTextureMaps[3];
    uint32_t                    viewPort[4];
    SEGMatrix                   worldMatrices[MAX_STACK];
    uint32_t                    worldMatricesStackCount;
    SEGMatrix                  *pBoundWorldMatrix;
    SEGMatrix                   projectionMatrix;
    SEGMatrix                   viewMatrix;
    SEGMatrix                   viewProjMatrix;
    SEGMatrix                   invViewProjMatrix;
    SEGState                    states[MAX_STACK];
    uint32_t                    statesStackCount;
    float                       clearColor[4];
    SEGRenderTarget2D           gBuffer[4];
    SEGRenderTarget2D           accumulationBuffer;
    EG_PASS                     pass;
    ID3D11Buffer               *pCBOmni;
} SEGDevice;
SEGDevice  *devices = NULL;
uint32_t    deviceCount = 0;
SEGDevice  *pBoundDevice = NULL;

// Clear states

void resetStates()
{
    SEGState *pState = pBoundDevice->states + pBoundDevice->statesStackCount;
    memset(pState, 0, sizeof(SEGState));

    egSet2DViewProj(-999, 999);
    egViewPort(0, 0, (uint32_t)pBoundDevice->backBufferDesc.Width, (uint32_t)pBoundDevice->backBufferDesc.Height);
    egModelIdentity();

    pState->depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    pState->depth.DepthFunc = D3D11_COMPARISON_LESS;
    pState->depth.StencilEnable = FALSE;
    pState->depth.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
    pState->depth.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
    pState->depth.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    pState->depth.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    pState->depth.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    pState->depth.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    pState->depth.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    pState->depth.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    pState->depth.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    pState->depth.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    pState->rasterizer.FillMode = D3D11_FILL_SOLID;
    pState->rasterizer.CullMode = D3D11_CULL_NONE;
    for (int i = 0; i < 8; ++i)
    {
        pState->blend.RenderTarget[i].BlendEnable = FALSE;
        pState->blend.RenderTarget[i].SrcBlend = D3D11_BLEND_ONE;
        pState->blend.RenderTarget[i].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        pState->blend.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
        pState->blend.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
        pState->blend.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ONE;
        pState->blend.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        pState->blend.RenderTarget[i].RenderTargetWriteMask = D3D10_COLOR_WRITE_ENABLE_ALL;
    }
    pState->sampler.Filter = D3D11_FILTER_ANISOTROPIC;
    pState->sampler.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    pState->sampler.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    pState->sampler.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    pState->sampler.MaxAnisotropy = 4;
    pState->sampler.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    pState->sampler.MaxLOD = D3D11_FLOAT32_MAX;
    pState->blendDirty = TRUE;
    pState->depthDirty = TRUE;
    pState->rasterizerDirty = TRUE;
    pState->samplerDirty = TRUE;

    pBoundDevice->pDeviceContext->lpVtbl->IASetInputLayout(pBoundDevice->pDeviceContext, pBoundDevice->pInputLayout);
    pBoundDevice->pDeviceContext->lpVtbl->VSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pVS, NULL, 0);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pPS, NULL, 0);

    ID3D11RenderTargetView *gBuffer[4] = {
        pBoundDevice->gBuffer[G_DIFFUSE].pRenderTargetView,
        pBoundDevice->gBuffer[G_DEPTH].pRenderTargetView,
        pBoundDevice->gBuffer[G_NORMAL].pRenderTargetView,
        pBoundDevice->gBuffer[G_MATERIAL].pRenderTargetView,
    };
    pBoundDevice->pDeviceContext->lpVtbl->OMSetRenderTargets(pBoundDevice->pDeviceContext, 4, gBuffer, pBoundDevice->pDepthStencilView);

    pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->pDefaultTextureMaps[DIFFUSE_MAP].pResourceView);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 1, 1, &pBoundDevice->pDefaultTextureMaps[NORMAL_MAP].pResourceView);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 2, 1, &pBoundDevice->pDefaultTextureMaps[MATERIAL_MAP].pResourceView);
}

ID3DBlob* compileShader(const char *szSource, const char *szProfile)
{
    ID3DBlob *shaderBlob = NULL;
#ifdef _DEBUG
    ID3DBlob *errorBlob = NULL;
#endif

    HRESULT result = D3DCompile(szSource, (SIZE_T)strlen(szSource), NULL, NULL, NULL, "main", szProfile,
                            D3DCOMPILE_ENABLE_STRICTNESS 
#ifdef _DEBUG
                            | D3DCOMPILE_DEBUG
#endif
                            , 0, &shaderBlob, &errorBlob);

#ifdef _DEBUG
    if (errorBlob)
    {
        char *pError = (char*)errorBlob->lpVtbl->GetBufferPointer(errorBlob);
        assert(FALSE);
    }
#endif

    return shaderBlob;
}

void texture2DFromData(SEGTexture2D *pOut, const uint8_t* pData, UINT w, UINT h, BOOL bGenerateMipMaps)
{
    HRESULT result;

    // Manually generate mip levels
    BOOL allowMipMaps = TRUE;
    UINT w2 = 1;
    UINT h2 = 1;
    while (w2 < (UINT)w) w2 *= 2;
    if (w != w2) allowMipMaps = FALSE;
    while (h2 < (UINT)h) h2 *= 2;
    if (h != h2) allowMipMaps = FALSE;
    uint8_t *pMipMaps = NULL;
    int mipLevels = 1;
    D3D11_SUBRESOURCE_DATA *mipsData = NULL;
    allowMipMaps = allowMipMaps & bGenerateMipMaps;
    if (allowMipMaps)
    {
        UINT biggest = max(w2, h2);
        UINT w2t = w2;
        UINT h2t = h2;
        UINT totalSize = w2t * h2t * 4;
        while (!(w2t == 1 && h2t == 1))
        {
            ++mipLevels;
            w2t /= 2;
            if (w2t < 1) w2t = 1;
            h2t /= 2;
            if (h2t < 1) h2t = 1;
            totalSize += w2t * h2t * 4;
        }
        pMipMaps = (uint8_t *)malloc(totalSize);
        memcpy(pMipMaps, pData, w * h * 4);

        mipsData = (D3D11_SUBRESOURCE_DATA *)malloc(sizeof(D3D11_SUBRESOURCE_DATA) * mipLevels);

        w2t = w2;
        h2t = h2;
        totalSize = 0;
        int mipTarget = mipLevels;
        mipLevels = 0;
        byte* prev;
        byte* cur;
        while (mipLevels != mipTarget)
        {
            prev = pMipMaps + totalSize;
            mipsData[mipLevels].pSysMem = prev;
            mipsData[mipLevels].SysMemPitch = w2t * 4;
            mipsData[mipLevels].SysMemSlicePitch = 0;
            totalSize += w2t * h2t * 4;
            cur = pMipMaps + totalSize;
            w2t /= 2;
            if (w2t < 1) w2t = 1;
            h2t /= 2;
            if (h2t < 1) h2t = 1;
            ++mipLevels;
            if (mipLevels == mipTarget) break;
            int accum;

            // Generate the mips
            int multX = w2 / w2t;
            int multY = h2 / h2t;
            for (UINT y = 0; y < h2t; ++y)
            {
                for (UINT x = 0; x < w2t; ++x)
                {
                    for (UINT k = 0; k < 4; ++k)
                    {
                        accum = 0;
                        accum += prev[(y * multY * w2 + x * multX) * 4 + k];
                        accum += prev[(y * multY * w2 + (x + multX / 2) * multX) * 4 + k];
                        accum += prev[((y + multY / 2) * multY * w2 + x * multX) * 4 + k];
                        accum += prev[((y + multY / 2) * multY * w2 + (x + multX / 2) * multX) * 4 + k];
                        cur[(y * w2t + x) * 4 + k] = accum / 4;
                    }
                }
            }

            w2 = w2t;
            h2 = h2t;
        }
    }

    D3D11_TEXTURE2D_DESC desc;
    desc.Width = w;
    desc.Height = h;
    desc.MipLevels = mipLevels;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA data;
    data.pSysMem = (pMipMaps) ? pMipMaps : pData;
    data.SysMemPitch = w * 4;
    data.SysMemSlicePitch = 0;

    result = pBoundDevice->pDevice->lpVtbl->CreateTexture2D(pBoundDevice->pDevice, &desc, (mipsData) ? mipsData : &data, &pOut->pTexture);
    if (result != S_OK)
    {
        sprintf_s(lastError, 256, "Failed CreateTexture2D");
        return;
    }
    ID3D11Resource *pResource = NULL;
    result = pOut->pTexture->lpVtbl->QueryInterface(pOut->pTexture, &IID_ID3D11Resource, &pResource);
    if (result != S_OK)
    {
        pOut->pTexture->lpVtbl->Release(pOut->pTexture);
        pOut->pTexture = NULL;
        sprintf_s(lastError, 256, "Failed QueryInterface ID3D11Texture2D -> IID_ID3D11Resource");
        return;
    }
    result = pBoundDevice->pDevice->lpVtbl->CreateShaderResourceView(pBoundDevice->pDevice, pResource, NULL, &pOut->pResourceView);
    if (result != S_OK)
    {
        pOut->pTexture->lpVtbl->Release(pOut->pTexture);
        pOut->pTexture = NULL;
        pResource->lpVtbl->Release(pResource);
        sprintf_s(lastError, 256, "Failed CreateShaderResourceView");
        return;
    }
    pResource->lpVtbl->Release(pResource);

    if (pMipMaps) free(pMipMaps);
    if (mipsData) free(mipsData);
}

HRESULT createRenderTarget(SEGRenderTarget2D *pRenderTarget, UINT w, UINT h, DXGI_FORMAT format)
{
    memset(pRenderTarget, 0, sizeof(SEGRenderTarget2D));
    D3D11_TEXTURE2D_DESC textureDesc = {0};
    HRESULT result;
    D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = {0};
    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {0};

    // Setup the render target texture description.
    textureDesc.Width = w;
    textureDesc.Height = h;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = format;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    // Create the render target texture.
    result = pBoundDevice->pDevice->lpVtbl->CreateTexture2D(pBoundDevice->pDevice, &textureDesc, NULL, &pRenderTarget->texture.pTexture);
    if (result != S_OK)
    {
        sprintf_s(lastError, 256, "Failed CreateTexture2D");
        return result;
    }

    // Setup the description of the render target view.
    renderTargetViewDesc.Format = textureDesc.Format;
    renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    renderTargetViewDesc.Texture2D.MipSlice = 0;

    // Create the render target view.
    ID3D11Resource *pTextureRes = NULL;
    result = pRenderTarget->texture.pTexture->lpVtbl->QueryInterface(pRenderTarget->texture.pTexture, &IID_ID3D11Resource, &pTextureRes);
    if (result != S_OK)
    {
        pRenderTarget->texture.pTexture->lpVtbl->Release(pRenderTarget->texture.pTexture);
        pRenderTarget->texture.pTexture = NULL;
        sprintf_s(lastError, 256, "Failed QueryInterface ID3D11Texture2D -> IID_ID3D11Resource");
        return result;
    }
    result = pBoundDevice->pDevice->lpVtbl->CreateRenderTargetView(pBoundDevice->pDevice, pTextureRes, &renderTargetViewDesc, &pRenderTarget->pRenderTargetView);
    if (result != S_OK)
    {
        pRenderTarget->texture.pTexture->lpVtbl->Release(pRenderTarget->texture.pTexture);
        pRenderTarget->texture.pTexture = NULL;
        pTextureRes->lpVtbl->Release(pTextureRes);
        sprintf_s(lastError, 256, "Failed CreateRenderTargetView");
        return result;
    }

    // Setup the description of the shader resource view.
    shaderResourceViewDesc.Format = textureDesc.Format;
    shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
    shaderResourceViewDesc.Texture2D.MipLevels = 1;

    // Create the shader resource view.
    result = pBoundDevice->pDevice->lpVtbl->CreateShaderResourceView(pBoundDevice->pDevice, pTextureRes, &shaderResourceViewDesc, &pRenderTarget->texture.pResourceView);
    if (result != S_OK)
    {
        pRenderTarget->texture.pTexture->lpVtbl->Release(pRenderTarget->texture.pTexture);
        pRenderTarget->texture.pTexture = NULL;
        pRenderTarget->pRenderTargetView->lpVtbl->Release(pRenderTarget->pRenderTargetView);
        pRenderTarget->pRenderTargetView = NULL;
        pTextureRes->lpVtbl->Release(pTextureRes);
        sprintf_s(lastError, 256, "Failed CreateShaderResourceView");
        return result;
    }
    pTextureRes->lpVtbl->Release(pTextureRes);

    return S_OK;
}

EGDevice egCreateDevice(HWND windowHandle)
{
    if (bIsInBatch) return 0;
    EGDevice                ret = 0;
    DXGI_SWAP_CHAIN_DESC    swapChainDesc;
    HRESULT                 result;
    ID3D11Texture2D        *pBackBuffer;
    ID3D11Resource         *pBackBufferRes;
    ID3D11Texture2D        *pDepthStencilBuffer;

    // Set as currently bound device
    devices = realloc(devices, sizeof(SEGDevice) * (deviceCount + 1));
    memset(devices + deviceCount, 0, sizeof(SEGDevice));
    pBoundDevice = devices + deviceCount;
    ++deviceCount;
    ret = deviceCount;

    // Define our swap chain
    memset(&swapChainDesc, 0, sizeof(swapChainDesc));
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = windowHandle;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;

    // Create the swap chain, device and device context
    result = D3D11CreateDeviceAndSwapChain(
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
#if _DEBUG
        D3D11_CREATE_DEVICE_DEBUG,
#else /* _DEBUG */
        0,
#endif /* _DEBUG */
        NULL, 0, D3D11_SDK_VERSION,
        &swapChainDesc, &pBoundDevice->pSwapChain,
        &pBoundDevice->pDevice, NULL, &pBoundDevice->pDeviceContext);
    if (result != S_OK)
    {
        sprintf_s(lastError, 256, "Failed D3D11CreateDeviceAndSwapChain");
        egDestroyDevice(&ret);
        return 0;
    }

    // Create render target
    result = pBoundDevice->pSwapChain->lpVtbl->GetBuffer(pBoundDevice->pSwapChain, 0, &IID_ID3D11Texture2D, (void**)&pBackBuffer);
    if (result != S_OK)
    {
        sprintf_s(lastError, 256, "Failed IDXGISwapChain GetBuffer IID_ID3D11Texture2D");
        egDestroyDevice(&ret);
        return 0;
    }
    result = pBoundDevice->pSwapChain->lpVtbl->GetBuffer(pBoundDevice->pSwapChain, 0, &IID_ID3D11Resource, (void**)&pBackBufferRes);
    if (result != S_OK)
    {
        sprintf_s(lastError, 256, "Failed IDXGISwapChain GetBuffer IID_ID3D11Resource");
        egDestroyDevice(&ret);
        return 0;
    }
    result = pBoundDevice->pDevice->lpVtbl->CreateRenderTargetView(pBoundDevice->pDevice, pBackBufferRes, NULL, &pBoundDevice->pRenderTargetView);
    if (result != S_OK)
    {
        sprintf_s(lastError, 256, "Failed CreateRenderTargetView");
        egDestroyDevice(&ret);
        return 0;
    }
    pBackBuffer->lpVtbl->GetDesc(pBackBuffer, &pBoundDevice->backBufferDesc);
    pBackBufferRes->lpVtbl->Release(pBackBufferRes);
    pBackBuffer->lpVtbl->Release(pBackBuffer);

    // Set up the description of the depth buffer.
    D3D11_TEXTURE2D_DESC depthBufferDesc = {0};
    depthBufferDesc.Width = pBoundDevice->backBufferDesc.Width;
    depthBufferDesc.Height = pBoundDevice->backBufferDesc.Height;
    depthBufferDesc.MipLevels = 1;
    depthBufferDesc.ArraySize = 1;
    depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthBufferDesc.SampleDesc.Count = 1;
    depthBufferDesc.SampleDesc.Quality = 0;
    depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthBufferDesc.CPUAccessFlags = 0;
    depthBufferDesc.MiscFlags = 0;

    // Create the texture for the depth buffer using the filled out description.
    result = pBoundDevice->pDevice->lpVtbl->CreateTexture2D(pBoundDevice->pDevice, &depthBufferDesc, NULL, &pDepthStencilBuffer);
    if (result != S_OK)
    {
        sprintf_s(lastError, 256, "Failed DepthStencil CreateTexture2D");
        egDestroyDevice(&ret);
        return 0;
    }

    // Initailze the depth stencil view.
    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {0};
    depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.Texture2D.MipSlice = 0;

    // Create the depth stencil view.
    result = pDepthStencilBuffer->lpVtbl->QueryInterface(pDepthStencilBuffer, &IID_ID3D11Resource, (void**)&pBackBufferRes);
    if (result != S_OK)
    {
        sprintf_s(lastError, 256, "Failed DepthStencil ID3D11Texture2D QueryInterface IID_ID3D11Resource");
        egDestroyDevice(&ret);
        return 0;
    }
    result = pBoundDevice->pDevice->lpVtbl->CreateDepthStencilView(pBoundDevice->pDevice, pBackBufferRes, &depthStencilViewDesc, &pBoundDevice->pDepthStencilView);
    if (result != S_OK)
    {
        sprintf_s(lastError, 256, "Failed CreateDepthStencilView");
        egDestroyDevice(&ret);
        return 0;
    }
    pBackBufferRes->lpVtbl->Release(pBackBufferRes);
    pDepthStencilBuffer->lpVtbl->Release(pDepthStencilBuffer);

    // Compile shaders
    ID3DBlob *pVSB = compileShader(g_vs, "vs_5_0");
    ID3DBlob *pPSB = compileShader(g_ps, "ps_5_0");
    ID3DBlob *pVSBPassThrough = compileShader(g_vsPassThrough, "vs_5_0");
    ID3DBlob *pPSBPassThrough = compileShader(g_psPassThrough, "ps_5_0");
    ID3DBlob *pPSBAmbient = compileShader(g_psAmbient, "ps_5_0");
    ID3DBlob *pPSBOmni = compileShader(g_psOmni, "ps_5_0");
    result = pBoundDevice->pDevice->lpVtbl->CreateVertexShader(pBoundDevice->pDevice, pVSB->lpVtbl->GetBufferPointer(pVSB), pVSB->lpVtbl->GetBufferSize(pVSB), NULL, &pBoundDevice->pVS);
    if (result != S_OK)
    {
        sprintf_s(lastError, 256, "Failed CreateVertexShader");
        egDestroyDevice(&ret);
        return 0;
    }
    result = pBoundDevice->pDevice->lpVtbl->CreatePixelShader(pBoundDevice->pDevice, pPSB->lpVtbl->GetBufferPointer(pPSB), pPSB->lpVtbl->GetBufferSize(pPSB), NULL, &pBoundDevice->pPS);
    if (result != S_OK)
    {
        sprintf_s(lastError, 256, "Failed CreatePixelShader");
        egDestroyDevice(&ret);
        return 0;
    }
    result = pBoundDevice->pDevice->lpVtbl->CreateVertexShader(pBoundDevice->pDevice, pVSBPassThrough->lpVtbl->GetBufferPointer(pVSBPassThrough), pVSBPassThrough->lpVtbl->GetBufferSize(pVSBPassThrough), NULL, &pBoundDevice->pVSPassThrough);
    if (result != S_OK)
    {
        sprintf_s(lastError, 256, "Failed CreateVertexShader PassThrough");
        egDestroyDevice(&ret);
        return 0;
    }
    result = pBoundDevice->pDevice->lpVtbl->CreatePixelShader(pBoundDevice->pDevice, pPSBPassThrough->lpVtbl->GetBufferPointer(pPSBPassThrough), pPSBPassThrough->lpVtbl->GetBufferSize(pPSBPassThrough), NULL, &pBoundDevice->pPSPassThrough);
    if (result != S_OK)
    {
        sprintf_s(lastError, 256, "Failed CreatePixelShader PassThrough");
        egDestroyDevice(&ret);
        return 0;
    }
    result = pBoundDevice->pDevice->lpVtbl->CreatePixelShader(pBoundDevice->pDevice, pPSBAmbient->lpVtbl->GetBufferPointer(pPSBAmbient), pPSBAmbient->lpVtbl->GetBufferSize(pPSBAmbient), NULL, &pBoundDevice->pPSAmbient);
    if (result != S_OK)
    {
        sprintf_s(lastError, 256, "Failed CreatePixelShader Ambient");
        egDestroyDevice(&ret);
        return 0;
    }
    result = pBoundDevice->pDevice->lpVtbl->CreatePixelShader(pBoundDevice->pDevice, pPSBOmni->lpVtbl->GetBufferPointer(pPSBOmni), pPSBOmni->lpVtbl->GetBufferSize(pPSBOmni), NULL, &pBoundDevice->pPSOmni);
    if (result != S_OK)
    {
        sprintf_s(lastError, 256, "Failed CreatePixelShader Ambient");
        egDestroyDevice(&ret);
        return 0;
    }

    // Input layout
    {
        D3D11_INPUT_ELEMENT_DESC layout[6] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 56, D3D11_INPUT_PER_VERTEX_DATA, 0}
        };
        result = pBoundDevice->pDevice->lpVtbl->CreateInputLayout(pBoundDevice->pDevice, layout, 6, pVSB->lpVtbl->GetBufferPointer(pVSB), pVSB->lpVtbl->GetBufferSize(pVSB), &pBoundDevice->pInputLayout);
        if (result != S_OK)
        {
            sprintf_s(lastError, 256, "Failed CreateInputLayout");
            egDestroyDevice(&ret);
            return 0;
        }
    }
    {
        D3D11_INPUT_ELEMENT_DESC layout[3] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 56, D3D11_INPUT_PER_VERTEX_DATA, 0}
        };
        result = pBoundDevice->pDevice->lpVtbl->CreateInputLayout(pBoundDevice->pDevice, layout, 3, pVSBPassThrough->lpVtbl->GetBufferPointer(pVSBPassThrough), pVSBPassThrough->lpVtbl->GetBufferSize(pVSBPassThrough), &pBoundDevice->pInputLayoutPassThrough);
        if (result != S_OK)
        {
            sprintf_s(lastError, 256, "Failed CreateInputLayout PassThrough");
            egDestroyDevice(&ret);
            return 0;
        }
    }

    // Create uniforms
    {
        D3D11_BUFFER_DESC cbDesc = {64, D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE, 0, 0};
        result = pBoundDevice->pDevice->lpVtbl->CreateBuffer(pBoundDevice->pDevice, &cbDesc, NULL, &pBoundDevice->pCBViewProj);
        if (result != S_OK)
        {
            sprintf_s(lastError, 256, "Failed CreateBuffer CBViewProj");
            egDestroyDevice(&ret);
            return 0;
        }
        result = pBoundDevice->pDevice->lpVtbl->CreateBuffer(pBoundDevice->pDevice, &cbDesc, NULL, &pBoundDevice->pCBModel);
        if (result != S_OK)
        {
            sprintf_s(lastError, 256, "Failed CreateBuffer CBModel");
            egDestroyDevice(&ret);
            return 0;
        }
        result = pBoundDevice->pDevice->lpVtbl->CreateBuffer(pBoundDevice->pDevice, &cbDesc, NULL, &pBoundDevice->pCBInvViewProj);
        if (result != S_OK)
        {
            sprintf_s(lastError, 256, "Failed CreateBuffer CBInvViewProj");
            egDestroyDevice(&ret);
            return 0;
        }
    }
    {
        D3D11_BUFFER_DESC cbDesc = {sizeof(SEGOmni), D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE, 0, 0};
        result = pBoundDevice->pDevice->lpVtbl->CreateBuffer(pBoundDevice->pDevice, &cbDesc, NULL, &pBoundDevice->pCBOmni);
        if (result != S_OK)
        {
            sprintf_s(lastError, 256, "Failed CreateBuffer CBInvViewProj");
            egDestroyDevice(&ret);
            return 0;
        }
    }

    // Create our geometry batch vertex buffer that will be used to batch everything
    D3D11_BUFFER_DESC vertexBufferDesc;
    vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    vertexBufferDesc.ByteWidth = MAX_VERTEX_COUNT * sizeof(SEGVertex);
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    vertexBufferDesc.MiscFlags = 0;
    vertexBufferDesc.StructureByteStride = 0;
    result = pBoundDevice->pDevice->lpVtbl->CreateBuffer(pBoundDevice->pDevice, &vertexBufferDesc, NULL, &pBoundDevice->pVertexBuffer);
    if (result != S_OK)
    {
        sprintf_s(lastError, 256, "Failed CreateBuffer VertexBuffer");
        egDestroyDevice(&ret);
        return 0;
    }
    result = pBoundDevice->pVertexBuffer->lpVtbl->QueryInterface(pBoundDevice->pVertexBuffer, &IID_ID3D11Resource, &pBoundDevice->pVertexBufferRes);
    if (result != S_OK)
    {
        sprintf_s(lastError, 256, "Failed VertexBuffer ID3D11Buffer QueryInterface -> IID_ID3D11Resource");
        egDestroyDevice(&ret);
        return 0;
    }

    // Create a white texture for default rendering
    {
        uint8_t pixel[4] = {255, 255, 255, 255};
        texture2DFromData(pBoundDevice->pDefaultTextureMaps + DIFFUSE_MAP, pixel, 1, 1, FALSE);
        if (!pBoundDevice->pDefaultTextureMaps[DIFFUSE_MAP].pTexture)
        {
            egDestroyDevice(&ret);
            return 0;
        }
    }
    {
        uint8_t pixel[4] = {128, 128, 255, 255};
        texture2DFromData(pBoundDevice->pDefaultTextureMaps + NORMAL_MAP, pixel, 1, 1, FALSE);
        if (!pBoundDevice->pDefaultTextureMaps[NORMAL_MAP].pTexture)
        {
            egDestroyDevice(&ret);
            return 0;
        }
    }
    {
        uint8_t pixel[4] = {0, 0, 0, 0};
        texture2DFromData(pBoundDevice->pDefaultTextureMaps + MATERIAL_MAP, pixel, 1, 1, FALSE);
        if (!pBoundDevice->pDefaultTextureMaps[MATERIAL_MAP].pTexture)
        {
            egDestroyDevice(&ret);
            return 0;
        }
    }

    // Create our G-Buffer
    result = createRenderTarget(pBoundDevice->gBuffer + G_DIFFUSE,
                       pBoundDevice->backBufferDesc.Width, pBoundDevice->backBufferDesc.Height,
                       DXGI_FORMAT_R8G8B8A8_UNORM);
    if (result != S_OK)
    {
        egDestroyDevice(&ret);
        return 0;
    }
    result = createRenderTarget(pBoundDevice->gBuffer + G_DEPTH,
                       pBoundDevice->backBufferDesc.Width, pBoundDevice->backBufferDesc.Height,
                       DXGI_FORMAT_R32_FLOAT);
    if (result != S_OK)
    {
        egDestroyDevice(&ret);
        return 0;
    }
    result = createRenderTarget(pBoundDevice->gBuffer + G_NORMAL,
                       pBoundDevice->backBufferDesc.Width, pBoundDevice->backBufferDesc.Height,
                       DXGI_FORMAT_R8G8B8A8_UNORM);
    if (result != S_OK)
    {
        egDestroyDevice(&ret);
        return 0;
    }
    result = createRenderTarget(pBoundDevice->gBuffer + G_MATERIAL,
                       pBoundDevice->backBufferDesc.Width, pBoundDevice->backBufferDesc.Height,
                       DXGI_FORMAT_R8G8B8A8_UNORM);
    if (result != S_OK)
    {
        egDestroyDevice(&ret);
        return 0;
    }

    // Accumulation buffer. This is an HDR texture
    result = createRenderTarget(&pBoundDevice->accumulationBuffer,
                       pBoundDevice->backBufferDesc.Width, pBoundDevice->backBufferDesc.Height,
                       DXGI_FORMAT_R16G16B16A16_FLOAT);
    if (result != S_OK)
    {
        egDestroyDevice(&ret);
        return 0;
    }

    resetStates();
    return ret;
}

void updateState()
{
    SEGState *pState = pBoundDevice->states + pBoundDevice->statesStackCount;
    if (pState->depthDirty)
    {
        ID3D11DepthStencilState *pDs2D;
        pBoundDevice->pDevice->lpVtbl->CreateDepthStencilState(pBoundDevice->pDevice, &pState->depth, &pDs2D);
        pBoundDevice->pDeviceContext->lpVtbl->OMSetDepthStencilState(pBoundDevice->pDeviceContext, pDs2D, 1);
        pDs2D->lpVtbl->Release(pDs2D);
        pState->depthDirty = FALSE;
    }
    if (pState->rasterizerDirty)
    {
        ID3D11RasterizerState *pSr2D;
        pBoundDevice->pDevice->lpVtbl->CreateRasterizerState(pBoundDevice->pDevice, &pState->rasterizer, &pSr2D);
        pBoundDevice->pDeviceContext->lpVtbl->RSSetState(pBoundDevice->pDeviceContext, pSr2D);
        pSr2D->lpVtbl->Release(pSr2D);
        pState->rasterizerDirty = FALSE;
    }
    if (pState->blendDirty)
    {
        ID3D11BlendState *pBs2D;
        pBoundDevice->pDevice->lpVtbl->CreateBlendState(pBoundDevice->pDevice, &pState->blend, &pBs2D);
        pBoundDevice->pDeviceContext->lpVtbl->OMSetBlendState(pBoundDevice->pDeviceContext, pBs2D, NULL, 0xffffffff);
        pBs2D->lpVtbl->Release(pBs2D);
        pState->blendDirty = FALSE;
    }
    if (pState->samplerDirty)
    {
        ID3D11SamplerState *pSs2D;
        pBoundDevice->pDevice->lpVtbl->CreateSamplerState(pBoundDevice->pDevice, &pState->sampler, &pSs2D);
        pBoundDevice->pDeviceContext->lpVtbl->PSSetSamplers(pBoundDevice->pDeviceContext, 0, 1, &pSs2D);
        pSs2D->lpVtbl->Release(pSs2D);
        pState->samplerDirty = FALSE;
    }
}

void egDestroyDevice(EGDevice *pDeviceID)
{
    if (bIsInBatch) return;
    SEGDevice *pDevice = devices + (*pDeviceID - 1);

    for (uint32_t i = 0; i < pDevice->textureCount; ++i)
    {
        SEGTexture2D *pTexture = pDevice->textures + i;
        if (pTexture)
        {
            if (pTexture->pTexture)
            {
                pTexture->pTexture->lpVtbl->Release(pTexture->pTexture);
            }
            if (pTexture->pResourceView)
            {
                pTexture->pResourceView->lpVtbl->Release(pTexture->pResourceView);
            }
        }
    }

    for (uint32_t i = 0; i < 3; ++i)
    {
        SEGTexture2D *pTexture = pDevice->pDefaultTextureMaps + i;
        if (pTexture)
        {
            if (pTexture->pTexture)
            {
                pTexture->pTexture->lpVtbl->Release(pTexture->pTexture);
            }
            if (pTexture->pResourceView)
            {
                pTexture->pResourceView->lpVtbl->Release(pTexture->pResourceView);
            }
        }
    }

    for (uint32_t i = 0; i < 4; ++i)
    {
        SEGRenderTarget2D *pRenderTarget = pDevice->gBuffer + i;
        if (pRenderTarget)
        {
            SEGTexture2D *pTexture = &pRenderTarget->texture;
            if (pTexture)
            {
                if (pTexture->pTexture)
                {
                    pTexture->pTexture->lpVtbl->Release(pTexture->pTexture);
                }
                if (pTexture->pResourceView)
                {
                    pTexture->pResourceView->lpVtbl->Release(pTexture->pResourceView);
                }
            }
            if (pRenderTarget->pRenderTargetView)
            {
                pRenderTarget->pRenderTargetView->lpVtbl->Release(pRenderTarget->pRenderTargetView);
            }
        }
    }

    if (pBoundDevice->accumulationBuffer.pRenderTargetView)
        pBoundDevice->accumulationBuffer.pRenderTargetView->lpVtbl->Release(pBoundDevice->accumulationBuffer.pRenderTargetView);
    if (pBoundDevice->accumulationBuffer.texture.pResourceView)
        pBoundDevice->accumulationBuffer.texture.pResourceView->lpVtbl->Release(pBoundDevice->accumulationBuffer.texture.pResourceView);
    if (pBoundDevice->accumulationBuffer.texture.pTexture)
        pBoundDevice->accumulationBuffer.texture.pTexture->lpVtbl->Release(pBoundDevice->accumulationBuffer.texture.pTexture);

    if (pDevice->pVertexBufferRes) pDevice->pVertexBufferRes->lpVtbl->Release(pDevice->pVertexBufferRes);
    if (pDevice->pVertexBuffer) pDevice->pVertexBuffer->lpVtbl->Release(pDevice->pVertexBuffer);

    if (pDevice->pCBModel) pDevice->pCBModel->lpVtbl->Release(pDevice->pCBModel);
    if (pDevice->pCBViewProj) pDevice->pCBViewProj->lpVtbl->Release(pDevice->pCBViewProj);
    if (pDevice->pCBInvViewProj) pDevice->pCBInvViewProj->lpVtbl->Release(pDevice->pCBInvViewProj);
    if (pDevice->pCBOmni) pDevice->pCBOmni->lpVtbl->Release(pDevice->pCBOmni);

    if (pDevice->pInputLayout) pDevice->pInputLayout->lpVtbl->Release(pDevice->pInputLayout);
    if (pDevice->pPS) pDevice->pPS->lpVtbl->Release(pDevice->pPS);
    if (pDevice->pVS) pDevice->pVS->lpVtbl->Release(pDevice->pVS);
    if (pDevice->pInputLayoutPassThrough) pDevice->pInputLayoutPassThrough->lpVtbl->Release(pDevice->pInputLayoutPassThrough);
    if (pDevice->pPSPassThrough) pDevice->pPSPassThrough->lpVtbl->Release(pDevice->pPSPassThrough);
    if (pDevice->pVSPassThrough) pDevice->pVSPassThrough->lpVtbl->Release(pDevice->pVSPassThrough);
    if (pDevice->pPSAmbient) pDevice->pPSAmbient->lpVtbl->Release(pDevice->pPSAmbient);
    if (pDevice->pPSOmni) pDevice->pPSOmni->lpVtbl->Release(pDevice->pPSOmni);

    if (pDevice->pDepthStencilView) pDevice->pDepthStencilView->lpVtbl->Release(pDevice->pDepthStencilView);
    if (pDevice->pRenderTargetView) pDevice->pRenderTargetView->lpVtbl->Release(pDevice->pRenderTargetView);
    if (pDevice->pDeviceContext) pDevice->pDeviceContext->lpVtbl->Release(pDevice->pDeviceContext);
    if (pDevice->pDevice) pDevice->pDevice->lpVtbl->Release(pDevice->pDevice);
    if (pDevice->pSwapChain) pDevice->pSwapChain->lpVtbl->Release(pDevice->pSwapChain);

    memset(pDevice, 0, sizeof(SEGDevice));
    *pDeviceID = 0;
}

void egBindDevice(EGDevice device)
{
    if (bIsInBatch) return;
    pBoundDevice = devices + (device - 1);
}

void egSwap()
{
    if (bIsInBatch) return;
    if (!pBoundDevice) return;
    pBoundDevice->pSwapChain->lpVtbl->Present(pBoundDevice->pSwapChain, 1, 0);
    pBoundDevice->worldMatricesStackCount = 0;
    pBoundDevice->statesStackCount = 0;
}

void egClearColor(float r, float g, float b, float a)
{
    if (!pBoundDevice) return;

    pBoundDevice->clearColor[0] = r;
    pBoundDevice->clearColor[1] = g;
    pBoundDevice->clearColor[2] = b;
    pBoundDevice->clearColor[3] = a;
}

void egClear(uint32_t clearBitFields)
{
    if (bIsInBatch) return;
    if (!pBoundDevice) return;
    if (clearBitFields & EG_CLEAR_COLOR)
    {
        pBoundDevice->pDeviceContext->lpVtbl->ClearRenderTargetView(pBoundDevice->pDeviceContext, pBoundDevice->accumulationBuffer.pRenderTargetView, pBoundDevice->clearColor);
        pBoundDevice->pDeviceContext->lpVtbl->ClearRenderTargetView(pBoundDevice->pDeviceContext, pBoundDevice->gBuffer[G_DIFFUSE].pRenderTargetView, pBoundDevice->clearColor);
        float black[4] = {0, 0, 0, 1};
        pBoundDevice->pDeviceContext->lpVtbl->ClearRenderTargetView(pBoundDevice->pDeviceContext, pBoundDevice->gBuffer[G_NORMAL].pRenderTargetView, black);
        pBoundDevice->pDeviceContext->lpVtbl->ClearRenderTargetView(pBoundDevice->pDeviceContext, pBoundDevice->gBuffer[G_MATERIAL].pRenderTargetView, black);
    }
    if (clearBitFields & EG_CLEAR_DEPTH_STENCIL)
    {
        pBoundDevice->pDeviceContext->lpVtbl->ClearDepthStencilView(pBoundDevice->pDeviceContext, pBoundDevice->pDepthStencilView, 
                                                                    D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        float zeroDepth[4] = {0, 0, 0, 0};
        pBoundDevice->pDeviceContext->lpVtbl->ClearRenderTargetView(pBoundDevice->pDeviceContext, pBoundDevice->gBuffer[G_DEPTH].pRenderTargetView, zeroDepth);
    }
}

void updateViewProjCB()
{
    D3D11_MAPPED_SUBRESOURCE map;
    ID3D11Resource *pRes = NULL;
    pBoundDevice->pCBViewProj->lpVtbl->QueryInterface(pBoundDevice->pCBViewProj, &IID_ID3D11Resource, &pRes);
    pBoundDevice->pDeviceContext->lpVtbl->Map(pBoundDevice->pDeviceContext, pRes, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
    memcpy(map.pData, pBoundDevice->viewProjMatrix.m, 64);
    pBoundDevice->pDeviceContext->lpVtbl->Unmap(pBoundDevice->pDeviceContext, pRes, 0);
    pRes->lpVtbl->Release(pRes);
    pBoundDevice->pDeviceContext->lpVtbl->VSSetConstantBuffers(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->pCBViewProj);
}

void updateInvViewProjCB()
{
    D3D11_MAPPED_SUBRESOURCE map;
    ID3D11Resource *pRes = NULL;
    pBoundDevice->pCBInvViewProj->lpVtbl->QueryInterface(pBoundDevice->pCBInvViewProj, &IID_ID3D11Resource, &pRes);
    pBoundDevice->pDeviceContext->lpVtbl->Map(pBoundDevice->pDeviceContext, pRes, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
    memcpy(map.pData, pBoundDevice->invViewProjMatrix.m, 64);
    pBoundDevice->pDeviceContext->lpVtbl->Unmap(pBoundDevice->pDeviceContext, pRes, 0);
    pRes->lpVtbl->Release(pRes);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetConstantBuffers(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->pCBInvViewProj);
}

void updateOmniCB()
{
    D3D11_MAPPED_SUBRESOURCE map;
    ID3D11Resource *pRes = NULL;
    pBoundDevice->pCBOmni->lpVtbl->QueryInterface(pBoundDevice->pCBOmni, &IID_ID3D11Resource, &pRes);
    pBoundDevice->pDeviceContext->lpVtbl->Map(pBoundDevice->pDeviceContext, pRes, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
    memcpy(map.pData, &currentOmni, sizeof(SEGOmni));
    pBoundDevice->pDeviceContext->lpVtbl->Unmap(pBoundDevice->pDeviceContext, pRes, 0);
    pRes->lpVtbl->Release(pRes);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetConstantBuffers(pBoundDevice->pDeviceContext, 1, 1, &pBoundDevice->pCBOmni);
}

void egSet2DViewProj(float nearClip, float farClip)
{
    if (bIsInBatch) return;
    if (!pBoundDevice) return;

    // viewProj2D matrix
    pBoundDevice->projectionMatrix.m[0] = 2.f / (float)pBoundDevice->viewPort[2];
    pBoundDevice->projectionMatrix.m[1] = 0.f;
    pBoundDevice->projectionMatrix.m[2] = 0.f;
    pBoundDevice->projectionMatrix.m[3] = -1.f;

    pBoundDevice->projectionMatrix.m[4] = 0.f;
    pBoundDevice->projectionMatrix.m[5] = -2.f / (float)pBoundDevice->viewPort[3];
    pBoundDevice->projectionMatrix.m[6] = 0.f;
    pBoundDevice->projectionMatrix.m[7] = 1.f;

    pBoundDevice->projectionMatrix.m[8] = 0.f;
    pBoundDevice->projectionMatrix.m[9] = 0.f;
    pBoundDevice->projectionMatrix.m[10] = -2.f / (farClip - nearClip);
    pBoundDevice->projectionMatrix.m[11] = -(farClip + nearClip) / (farClip - nearClip);
    
    pBoundDevice->projectionMatrix.m[12] = 0.f;
    pBoundDevice->projectionMatrix.m[13] = 0.f;
    pBoundDevice->projectionMatrix.m[14] = 0.f;
    pBoundDevice->projectionMatrix.m[15] = 1.f;

    // Identity view
    setIdentityMatrix(&pBoundDevice->viewMatrix);

    // Multiply them
    multMatrix(&pBoundDevice->viewMatrix, &pBoundDevice->projectionMatrix, &pBoundDevice->viewProjMatrix);

    // Inverse for light pass
    inverseMatrix(&pBoundDevice->viewProjMatrix, &pBoundDevice->invViewProjMatrix);

    updateViewProjCB();
    updateInvViewProjCB();
}

void egSet3DViewProj(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ, float fov, float nearClip, float farClip)
{
    if (bIsInBatch) return;
    if (!pBoundDevice) return;

    // Projection
    float aspect = (float)pBoundDevice->backBufferDesc.Width / (float)pBoundDevice->backBufferDesc.Height;
    setProjectionMatrix(&pBoundDevice->projectionMatrix, TO_RAD(fov), aspect, nearClip, farClip);

    // View
    setLookAtMatrix(&pBoundDevice->viewMatrix, eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ);

    // Multiply them
    multMatrix(&pBoundDevice->viewMatrix, &pBoundDevice->projectionMatrix, &pBoundDevice->viewProjMatrix);

    // Inverse for light pass
    inverseMatrix(&pBoundDevice->viewProjMatrix, &pBoundDevice->invViewProjMatrix);

    updateViewProjCB();
    updateInvViewProjCB();
}

void egSetViewProj(const float *pView, const float *pProj)
{
    if (bIsInBatch) return;
    if (!pBoundDevice) return;

    memcpy(&pBoundDevice->viewMatrix, pView, sizeof(SEGMatrix));
    memcpy(&pBoundDevice->projectionMatrix, pProj, sizeof(SEGMatrix));

    // Multiply them
    multMatrix(&pBoundDevice->viewMatrix, &pBoundDevice->projectionMatrix, &pBoundDevice->viewProjMatrix);

    // Inverse for light pass
    inverseMatrix(&pBoundDevice->viewProjMatrix, &pBoundDevice->invViewProjMatrix);

    updateViewProjCB();
    updateInvViewProjCB();
}

void egViewPort(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    if (bIsInBatch) return;
    if (!pBoundDevice) return;

    pBoundDevice->viewPort[0] = x;
    pBoundDevice->viewPort[1] = y;
    pBoundDevice->viewPort[2] = width;
    pBoundDevice->viewPort[3] = height;

    D3D11_VIEWPORT d3dViewport = {(FLOAT)pBoundDevice->viewPort[0], (FLOAT)pBoundDevice->viewPort[1], (FLOAT)pBoundDevice->viewPort[2], (FLOAT)pBoundDevice->viewPort[3], D3D11_MIN_DEPTH, D3D11_MAX_DEPTH};
    pBoundDevice->pDeviceContext->lpVtbl->RSSetViewports(pBoundDevice->pDeviceContext, 1, &d3dViewport);
}

void egScissor(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    if (bIsInBatch) return;
}

void updateModelCB()
{
    SEGMatrix *pModel = pBoundDevice->worldMatrices + pBoundDevice->worldMatricesStackCount;
    SEGMatrix model;
    memcpy(&model, pModel, sizeof(SEGMatrix));
    transposeMatrix(&model);

    D3D11_MAPPED_SUBRESOURCE map;
    ID3D11Resource *pRes = NULL;
    pBoundDevice->pCBModel->lpVtbl->QueryInterface(pBoundDevice->pCBModel, &IID_ID3D11Resource, &pRes);
    pBoundDevice->pDeviceContext->lpVtbl->Map(pBoundDevice->pDeviceContext, pRes, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
    memcpy(map.pData, model.m, 64);
    pBoundDevice->pDeviceContext->lpVtbl->Unmap(pBoundDevice->pDeviceContext, pRes, 0);
    pRes->lpVtbl->Release(pRes);
    pBoundDevice->pDeviceContext->lpVtbl->VSSetConstantBuffers(pBoundDevice->pDeviceContext, 1, 1, &pBoundDevice->pCBModel);
}

void egModelIdentity()
{
    if (bIsInBatch) return;
    SEGMatrix *pModel = pBoundDevice->worldMatrices + pBoundDevice->worldMatricesStackCount;
    setIdentityMatrix(pModel);
    updateModelCB();
}

void egModelTranslate(float x, float y, float z)
{
    if (bIsInBatch) return;
    SEGMatrix *pModel = pBoundDevice->worldMatrices + pBoundDevice->worldMatricesStackCount;
    SEGMatrix translationMatrix;
    setTranslationMatrix(&translationMatrix, x, y, z);
    SEGMatrix matrix;
    memcpy(&matrix, pModel, sizeof(SEGMatrix));
    multMatrix(&translationMatrix, &matrix, pModel);
    updateModelCB();
}

void egModelTranslatev(const float *pAxis)
{
    if (bIsInBatch) return;
    egModelTranslate(pAxis[0], pAxis[1], pAxis[2]);
}

void egModelMult(const float *pMatrix)
{
    if (bIsInBatch) return;
    SEGMatrix *pModel = pBoundDevice->worldMatrices + pBoundDevice->worldMatricesStackCount;
    SEGMatrix matrix;
    memcpy(&matrix, pModel, sizeof(SEGMatrix));
    multMatrix((const SEGMatrix*)pMatrix, &matrix, pModel);
    updateModelCB();
}

void egModelRotate(float angle, float x, float y, float z)
{
    if (bIsInBatch) return;
    SEGMatrix *pModel = pBoundDevice->worldMatrices + pBoundDevice->worldMatricesStackCount;
    SEGMatrix rotationMatrix;
    setRotationMatrix(&rotationMatrix, angle * x, angle * y, angle * z);
    SEGMatrix matrix;
    memcpy(&matrix, pModel, sizeof(SEGMatrix));
    multMatrix(&rotationMatrix, &matrix, pModel);
    updateModelCB();
}

void egModelRotatev(float angle, const float *pAxis)
{
    if (bIsInBatch) return;
    egModelRotate(angle, pAxis[0], pAxis[1], pAxis[2]);
}

void egModelScale(float x, float y, float z)
{
    if (bIsInBatch) return;
    SEGMatrix *pModel = pBoundDevice->worldMatrices + pBoundDevice->worldMatricesStackCount;
    SEGMatrix scaleMatrix;
    setScaleMatrix(&scaleMatrix, x, y, z);
    SEGMatrix matrix;
    memcpy(&matrix, pModel, sizeof(SEGMatrix));
    multMatrix(&scaleMatrix, &matrix, pModel);
    updateModelCB();
}

void egModelScalev(const float *pAxis)
{
    if (bIsInBatch) return;
    egModelScale(pAxis[0], pAxis[1], pAxis[2]);
}

void egModelPush()
{
    if (bIsInBatch) return;
    if (pBoundDevice->worldMatricesStackCount == MAX_STACK - 1) return;
    SEGMatrix *pPrevious = pBoundDevice->worldMatrices + pBoundDevice->worldMatricesStackCount;
    ++pBoundDevice->worldMatricesStackCount;
    SEGMatrix *pNew = pBoundDevice->worldMatrices + pBoundDevice->worldMatricesStackCount;
    memcpy(pNew, pPrevious, sizeof(SEGMatrix));
}

void egModelPop()
{
    if (bIsInBatch) return;
    if (!pBoundDevice->worldMatricesStackCount) return;
    --pBoundDevice->worldMatricesStackCount;
    updateModelCB();
}

void flush();

void appendVertex(SEGVertex *in_pVertex)
{
    if (currentVertexCount == MAX_VERTEX_COUNT) return;
    memcpy(pVertex + currentVertexCount, in_pVertex, sizeof(SEGVertex));
    ++currentVertexCount;
}

void drawVertex(SEGVertex *in_pVertex)
{
    if (!bIsInBatch) return;

    if (currentTopology == EG_TRIANGLE_FAN)
    {
        if (currentVertexCount >= 3)
        {
            appendVertex(pVertex);
            appendVertex(pVertex + currentVertexCount - 2);
        }
    }
    else if (currentTopology == EG_QUADS)
    {
        if ((currentVertexCount - 3) % 6 == 0 && currentVertexCount)
        {
            appendVertex(pVertex + currentVertexCount - 3);
            appendVertex(pVertex + currentVertexCount - 2);
        }
    }
    else if (currentTopology == EG_QUAD_STRIP)
    {
        if (currentVertexCount == 3)
        {
            appendVertex(pVertex + currentVertexCount - 3);
            appendVertex(pVertex + currentVertexCount - 2);
        }
        else if (currentVertexCount >= 6)
        {
            if ((currentVertexCount - 6) % 2 == 0)
            {
                appendVertex(pVertex + currentVertexCount - 1);
                appendVertex(pVertex + currentVertexCount - 3);
            }
            else
            {
                appendVertex(pVertex + currentVertexCount - 3);
                appendVertex(pVertex + currentVertexCount - 2);
            }
        }
    }

    appendVertex(in_pVertex);
    if (currentVertexCount == MAX_VERTEX_COUNT) flush();
}

void beginGeometryPass()
{
    if (bIsInBatch) return;
    if (!pBoundDevice) return;
    if (pBoundDevice->pass == EG_GEOMETRY_PASS) return;
    pBoundDevice->pass = EG_GEOMETRY_PASS;

    pBoundDevice->pDeviceContext->lpVtbl->IASetInputLayout(pBoundDevice->pDeviceContext, pBoundDevice->pInputLayout);
    pBoundDevice->pDeviceContext->lpVtbl->VSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pVS, NULL, 0);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pPS, NULL, 0);

    // Unbind if it's still bound
    ID3D11ShaderResourceView *res = NULL;
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 3, 1, &res);
    pBoundDevice->pDeviceContext->lpVtbl->PSGetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &res);
    if (res == pBoundDevice->gBuffer[G_DIFFUSE].texture.pResourceView)
    {
        pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->pDefaultTextureMaps[DIFFUSE_MAP].pResourceView);
    }
    pBoundDevice->pDeviceContext->lpVtbl->PSGetShaderResources(pBoundDevice->pDeviceContext, 1, 1, &res);
    if (res == pBoundDevice->gBuffer[G_DEPTH].texture.pResourceView)
    {
        pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 1, 1, &pBoundDevice->pDefaultTextureMaps[NORMAL_MAP].pResourceView);
    }
    pBoundDevice->pDeviceContext->lpVtbl->PSGetShaderResources(pBoundDevice->pDeviceContext, 2, 1, &res);
    if (res == pBoundDevice->gBuffer[G_NORMAL].texture.pResourceView)
    {
        pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 2, 1, &pBoundDevice->pDefaultTextureMaps[MATERIAL_MAP].pResourceView);
    }

    // Bind G-Buffer
    ID3D11RenderTargetView *gBuffer[4] = {
        pBoundDevice->gBuffer[G_DIFFUSE].pRenderTargetView,
        pBoundDevice->gBuffer[G_DEPTH].pRenderTargetView,
        pBoundDevice->gBuffer[G_NORMAL].pRenderTargetView,
        pBoundDevice->gBuffer[G_MATERIAL].pRenderTargetView,
    };
    pBoundDevice->pDeviceContext->lpVtbl->OMSetRenderTargets(pBoundDevice->pDeviceContext, 4, gBuffer, pBoundDevice->pDepthStencilView);
}

void beginAmbientPass()
{
    if (bIsInBatch) return;
    if (!pBoundDevice) return;
    if (pBoundDevice->pass == EG_AMBIENT_PASS) return;
    pBoundDevice->pass = EG_AMBIENT_PASS;

    pBoundDevice->pDeviceContext->lpVtbl->IASetPrimitiveTopology(pBoundDevice->pDeviceContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    pBoundDevice->pDeviceContext->lpVtbl->OMSetRenderTargets(pBoundDevice->pDeviceContext, 1, &pBoundDevice->accumulationBuffer.pRenderTargetView, NULL);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->gBuffer[G_DIFFUSE].texture.pResourceView);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 3, 1, &pBoundDevice->gBuffer[G_MATERIAL].texture.pResourceView);

    pBoundDevice->pDeviceContext->lpVtbl->IASetInputLayout(pBoundDevice->pDeviceContext, pBoundDevice->pInputLayoutPassThrough);
    pBoundDevice->pDeviceContext->lpVtbl->VSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pVSPassThrough, NULL, 0);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pPSAmbient, NULL, 0);

    egEnable(EG_BLEND);
    egBlendFunc(EG_ONE, EG_ONE);
    updateState();
}

void beginOmniPass()
{
    if (bIsInBatch) return;
    if (!pBoundDevice) return;
    if (pBoundDevice->pass == EG_OMNI_PASS) return;
    pBoundDevice->pass = EG_OMNI_PASS;

    pBoundDevice->pDeviceContext->lpVtbl->IASetPrimitiveTopology(pBoundDevice->pDeviceContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    pBoundDevice->pDeviceContext->lpVtbl->OMSetRenderTargets(pBoundDevice->pDeviceContext, 1, &pBoundDevice->accumulationBuffer.pRenderTargetView, NULL);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->gBuffer[G_DIFFUSE].texture.pResourceView);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 1, 1, &pBoundDevice->gBuffer[G_DEPTH].texture.pResourceView);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 2, 1, &pBoundDevice->gBuffer[G_NORMAL].texture.pResourceView);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 3, 1, &pBoundDevice->gBuffer[G_MATERIAL].texture.pResourceView);

    pBoundDevice->pDeviceContext->lpVtbl->IASetInputLayout(pBoundDevice->pDeviceContext, pBoundDevice->pInputLayoutPassThrough);
    pBoundDevice->pDeviceContext->lpVtbl->VSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pVSPassThrough, NULL, 0);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pPSOmni, NULL, 0);

    egEnable(EG_BLEND);
    egBlendFunc(EG_ONE, EG_ONE);
    updateState();
}

void beginPostProcessPass()
{
    if (bIsInBatch) return;
    if (!pBoundDevice) return;

    if (pBoundDevice->pass == EG_POST_PROCESS_PASS) return;
    pBoundDevice->pass = EG_POST_PROCESS_PASS;

    pBoundDevice->pDeviceContext->lpVtbl->OMSetRenderTargets(pBoundDevice->pDeviceContext, 1, &pBoundDevice->pRenderTargetView, NULL);
    pBoundDevice->pDeviceContext->lpVtbl->IASetPrimitiveTopology(pBoundDevice->pDeviceContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    pBoundDevice->pDeviceContext->lpVtbl->IASetInputLayout(pBoundDevice->pDeviceContext, pBoundDevice->pInputLayoutPassThrough);
    pBoundDevice->pDeviceContext->lpVtbl->VSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pVSPassThrough, NULL, 0);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pPSPassThrough, NULL, 0);

    egDisable(EG_BLEND);
    updateState();
}

BOOL bMapVB = TRUE;

void egBegin(EG_TOPOLOGY topology)
{
    if (bIsInBatch) return;
    if (!pBoundDevice) return;
    bMapVB = TRUE;
    currentTopology = topology;
    switch (currentTopology)
    {
        case EG_POINTS:
            beginGeometryPass();
            pBoundDevice->pDeviceContext->lpVtbl->IASetPrimitiveTopology(pBoundDevice->pDeviceContext, D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
            break;
        case EG_LINES:
            beginGeometryPass();
            pBoundDevice->pDeviceContext->lpVtbl->IASetPrimitiveTopology(pBoundDevice->pDeviceContext, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
            break;
        case EG_LINE_STRIP:
        case EG_LINE_LOOP:
            beginGeometryPass();
            pBoundDevice->pDeviceContext->lpVtbl->IASetPrimitiveTopology(pBoundDevice->pDeviceContext, D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
            break;
        case EG_TRIANGLES:
            beginGeometryPass();
            pBoundDevice->pDeviceContext->lpVtbl->IASetPrimitiveTopology(pBoundDevice->pDeviceContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            break;
        case EG_TRIANGLE_STRIP:
            beginGeometryPass();
            pBoundDevice->pDeviceContext->lpVtbl->IASetPrimitiveTopology(pBoundDevice->pDeviceContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            break;
        case EG_TRIANGLE_FAN:
            beginGeometryPass();
            pBoundDevice->pDeviceContext->lpVtbl->IASetPrimitiveTopology(pBoundDevice->pDeviceContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            break;
        case EG_QUADS:
            beginGeometryPass();
            pBoundDevice->pDeviceContext->lpVtbl->IASetPrimitiveTopology(pBoundDevice->pDeviceContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            break;
        case EG_QUAD_STRIP:
            beginGeometryPass();
            pBoundDevice->pDeviceContext->lpVtbl->IASetPrimitiveTopology(pBoundDevice->pDeviceContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            break;
        case EG_AMBIENTS:
            beginAmbientPass();
            bMapVB = FALSE;
            break;
        case EG_OMNIS:
            beginOmniPass();
            bMapVB = FALSE;
            break;
        default:
            return;
    }

    bIsInBatch = TRUE;
    if (bMapVB)
    {
        D3D11_MAPPED_SUBRESOURCE mappedVertexBuffer;
        pBoundDevice->pDeviceContext->lpVtbl->Map(pBoundDevice->pDeviceContext, pBoundDevice->pVertexBufferRes, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedVertexBuffer);
        pVertex = (SEGVertex*)mappedVertexBuffer.pData;
    }
}

void flush()
{
    if (!bIsInBatch) return;
    if (!pBoundDevice) return;
    if (!currentVertexCount) return;

    switch (currentTopology)
    {
        case EG_LINE_LOOP:
            drawVertex(pVertex);
            break;
    }

    pBoundDevice->pDeviceContext->lpVtbl->Unmap(pBoundDevice->pDeviceContext, pBoundDevice->pVertexBufferRes, 0);

    // Make sure states are up to date
    updateState();

    const UINT stride = sizeof(SEGVertex);
    const UINT offset = 0;
    pBoundDevice->pDeviceContext->lpVtbl->IASetVertexBuffers(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->pVertexBuffer, &stride, &offset);
    pBoundDevice->pDeviceContext->lpVtbl->Draw(pBoundDevice->pDeviceContext, currentVertexCount, 0);

    currentVertexCount = 0;

    D3D11_MAPPED_SUBRESOURCE mappedVertexBuffer;
    pBoundDevice->pDeviceContext->lpVtbl->Map(pBoundDevice->pDeviceContext, pBoundDevice->pVertexBufferRes, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedVertexBuffer);
    pVertex = (SEGVertex*)mappedVertexBuffer.pData;
}

void egEnd()
{
    if (!bIsInBatch) return;
    if (!pBoundDevice) return;
    flush();
    bIsInBatch = FALSE;
    
    if (bMapVB)
    {
        pBoundDevice->pDeviceContext->lpVtbl->Unmap(pBoundDevice->pDeviceContext, pBoundDevice->pVertexBufferRes, 0);
    }
}

void drawScreenQuad(float left, float top, float right, float bottom, float *pColor)
{
    D3D11_MAPPED_SUBRESOURCE mappedVertexBuffer;
    pBoundDevice->pDeviceContext->lpVtbl->Map(pBoundDevice->pDeviceContext, pBoundDevice->pVertexBufferRes, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedVertexBuffer);
    pVertex = (SEGVertex*)mappedVertexBuffer.pData;

    pVertex[0].x = left;
    pVertex[0].y = top;
    pVertex[0].u = 0;
    pVertex[0].v = 0;
    memcpy(&pVertex[0].r, pColor, 16);

    pVertex[1].x = left;
    pVertex[1].y = bottom;
    pVertex[1].u = 0;
    pVertex[1].v = 1;
    memcpy(&pVertex[1].r, pColor, 16);

    pVertex[2].x = right;
    pVertex[2].y = top;
    pVertex[2].u = 1;
    pVertex[2].v = 0;
    memcpy(&pVertex[2].r, pColor, 16);

    pVertex[3].x = right;
    pVertex[3].y = bottom;
    pVertex[3].u = 1;
    pVertex[3].v = 1;
    memcpy(&pVertex[3].r, pColor, 16);

    pBoundDevice->pDeviceContext->lpVtbl->Unmap(pBoundDevice->pDeviceContext, pBoundDevice->pVertexBufferRes, 0);

    const UINT stride = sizeof(SEGVertex);
    const UINT offset = 0;
    pBoundDevice->pDeviceContext->lpVtbl->IASetVertexBuffers(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->pVertexBuffer, &stride, &offset);
    pBoundDevice->pDeviceContext->lpVtbl->Draw(pBoundDevice->pDeviceContext, 4, 0);
}

void drawAmbient()
{
    drawScreenQuad(-1, 1, 1, -1, &currentVertex.r);
}

void drawOmni()
{
    memcpy(&currentOmni.x, &currentVertex.x, 12);
    memcpy(&currentOmni.r, &currentVertex.r, 16);
    updateOmniCB();
    float color[4] = {1, 1, 1, 1};
    drawScreenQuad(-1, 1, 1, -1, color);
}

void egColor3(float r, float g, float b)
{
    currentVertex.r = r;
    currentVertex.g = g;
    currentVertex.b = b;
    currentVertex.a = 1.f;
    if (currentTopology == EG_AMBIENTS)
    {
        drawAmbient();
    }
}

void egColor3v(const float *pRGB)
{
    memcpy(&currentVertex.r, pRGB, 12);
    currentVertex.a = 1.f;
    if (currentTopology == EG_AMBIENTS)
    {
        drawAmbient();
    }
}

void egColor4(float r, float g, float b, float a)
{
    currentVertex.r = r;
    currentVertex.g = g;
    currentVertex.b = b;
    currentVertex.a = a;
    if (currentTopology == EG_AMBIENTS)
    {
        drawAmbient();
    }
}

void egColor4v(const float *pRGBA)
{
    memcpy(&currentVertex.r, pRGBA, 16);
    if (currentTopology == EG_AMBIENTS)
    {
        drawAmbient();
    }
}

void egNormal(float nx, float ny, float nz)
{
    currentVertex.nx = nx;
    currentVertex.ny = ny;
    currentVertex.nz = nz;
}

void egNormalv(const float *pNormal)
{
    memcpy(&currentVertex.nx, pNormal, 12);
}

void egTangent(float nx, float ny, float nz)
{
    currentVertex.tx = nx;
    currentVertex.ty = ny;
    currentVertex.tz = nz;
}

void egTangentv(const float *pTangent)
{
    memcpy(&currentVertex.tx, pTangent, 12);
}

void egBinormal(float nx, float ny, float nz)
{
    currentVertex.bx = nx;
    currentVertex.by = ny;
    currentVertex.bz = nz;
}

void egBinormalv(const float *pBitnormal)
{
    memcpy(&currentVertex.bx, pBitnormal, 12);
}

void egTexCoord(float u, float v)
{
    currentVertex.u = u;
    currentVertex.v = v;
}

void egTexCoordv(const float *pTexCoord)
{
    memcpy(&currentVertex.u, pTexCoord, 8);
}

void egPosition2(float x, float y)
{
    if (!bIsInBatch) return;
    currentVertex.x = x;
    currentVertex.y = y;
    currentVertex.z = 0.f;
    if (currentTopology == EG_OMNIS)
    {
        drawOmni();
    }
    else
    {
        drawVertex(&currentVertex);
    }
}

void egPosition2v(const float *pPos)
{
    if (!bIsInBatch) return;
    memcpy(&currentVertex.x, pPos, 8);
    currentVertex.z = 0.f;
    if (currentTopology == EG_OMNIS)
    {
        drawOmni();
    }
    else
    {
        drawVertex(&currentVertex);
    }
}

void egPosition3(float x, float y, float z)
{
    if (!bIsInBatch) return;
    currentVertex.x = x;
    currentVertex.y = y;
    currentVertex.z = z;
    if (currentTopology == EG_OMNIS)
    {
        drawOmni();
    }
    else
    {
        drawVertex(&currentVertex);
    }
}

void egPosition3v(const float *pPos)
{
    if (!bIsInBatch) return;
    memcpy(&currentVertex.x, pPos, 12);
    if (currentTopology == EG_OMNIS)
    {
        drawOmni();
    }
    else
    {
        drawVertex(&currentVertex);
    }
}

void egTarget2(float x, float y)
{
}

void egTarget2v(const float *pPos)
{
}

void egTarget3(float x, float y, float z)
{
}

void egTarget3v(const float *pPos)
{
}

void egRadius(float radius)
{
    currentOmni.radius = radius;
}

void egRadius2(float innerRadius, float outterRadius)
{
}

void egRadius2v(const float *pRadius)
{
}

void egFalloffExponent(float exponent)
{
}

void egMultiply(float multiply)
{
}

void egSpecular(float intensity, float shininess)
{
}

void egSelfIllum(float intensity)
{
}

EGTexture createTexture(SEGTexture2D *pTexture)
{
    pBoundDevice->textures = realloc(pBoundDevice->textures, sizeof(SEGTexture2D) * (pBoundDevice->textureCount + 1));
    memcpy(pBoundDevice->textures + pBoundDevice->textureCount, pTexture, sizeof(SEGTexture2D));
    return ++pBoundDevice->textureCount;
}

EGTexture egCreateTexture1D(uint32_t dimension, const void *pData, EG_FORMAT dataFormat)
{
    return 0;
}

EGTexture egCreateTexture2D(uint32_t width, uint32_t height, const void *pData, uint32_t dataType, EG_TEXTURE_FLAGS flags)
{
    if (!pBoundDevice) return 0;
    SEGTexture2D texture2D = {0};
    uint8_t *pConvertedData = (uint8_t *)pData;
    texture2DFromData(&texture2D, pConvertedData, width, height, flags & EG_GENERATE_MIPMAPS ? TRUE : FALSE);
    if (!texture2D.pTexture) return 0;
    return createTexture(&texture2D);
}

EGTexture egCreateTexture3D(uint32_t width, uint32_t height, uint32_t depth, const void *pData, uint32_t dataType)
{
    return 0;
}

EGTexture egCreateCubeMap(uint32_t dimension, const void *pData, EG_FORMAT dataFormat, EG_TEXTURE_FLAGS flags)
{
    return 0;
}

void egDestroyTexture(EGTexture *pTexture)
{
}

void egBindDiffuse(EGTexture texture)
{
    if (!pBoundDevice) return;
    if (!texture)
    {
        pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->pDefaultTextureMaps[DIFFUSE_MAP].pResourceView);
        return;
    }
    if (texture > pBoundDevice->textureCount) return;
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->textures[texture - 1].pResourceView);
}

void egBindNormal(EGTexture texture)
{
    if (!pBoundDevice) return;
    if (!texture)
    {
        pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 1, 1, &pBoundDevice->pDefaultTextureMaps[NORMAL_MAP].pResourceView);
        return;
    }
    if (texture > pBoundDevice->textureCount) return;
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 1, 1, &pBoundDevice->textures[texture - 1].pResourceView);
}

void egBindMaterial(EGTexture texture)
{
    if (!pBoundDevice) return;
    if (!texture)
    {
        pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 2, 1, &pBoundDevice->pDefaultTextureMaps[MATERIAL_MAP].pResourceView);
        return;
    }
    if (texture > pBoundDevice->textureCount) return;
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 2, 1, &pBoundDevice->textures[texture - 1].pResourceView);
}

void egEnable(EG_ENABLE_BITS stateBits)
{
    if (!pBoundDevice) return;
    SEGState *pState = pBoundDevice->states + pBoundDevice->statesStackCount;
    SEGState *pPreviousState = pState;
    if (pBoundDevice->statesStackCount) --pPreviousState;
    switch (stateBits)
    {
        case EG_BLEND:
            if (pState->blend.RenderTarget->BlendEnable) break;
            pState->blend.RenderTarget->BlendEnable = TRUE;
            pState->blendDirty = TRUE;
            pPreviousState->blendDirty = TRUE;
            break;
        case EG_DEPTH_TEST:
            if (pState->depth.DepthEnable) break;
            pState->depth.DepthEnable = TRUE;
            pState->depthDirty = TRUE;
            pPreviousState->depthDirty = TRUE;
            break;
    }
}

void egDisable(EG_ENABLE_BITS stateBits)
{
    if (!pBoundDevice) return;
    SEGState *pState = pBoundDevice->states + pBoundDevice->statesStackCount;
    SEGState *pPreviousState = pState;
    if (pBoundDevice->statesStackCount) --pPreviousState;
    switch (stateBits)
    {
        case EG_BLEND:
            if (!pState->blend.RenderTarget->BlendEnable) break;
            pState->blend.RenderTarget->BlendEnable = FALSE;
            pState->blendDirty = TRUE;
            pPreviousState->blendDirty = TRUE;
            break;
        case EG_DEPTH_TEST:
            if (!pState->depth.DepthEnable) break;
            pState->depth.DepthEnable = FALSE;
            pState->depthDirty = TRUE;
            pPreviousState->depthDirty = TRUE;
            break;
    }
}

void egStatePush()
{
    if (bIsInBatch) return;
    if (pBoundDevice->statesStackCount == MAX_STACK - 1) return;
    SEGState *pPrevious = pBoundDevice->states + pBoundDevice->statesStackCount;
    ++pBoundDevice->statesStackCount;
    SEGState *pNew = pBoundDevice->states + pBoundDevice->statesStackCount;
    memcpy(pNew, pPrevious, sizeof(SEGState));
}

void egStatePop()
{
    if (bIsInBatch) return;
    if (!pBoundDevice->statesStackCount) return;
    --pBoundDevice->statesStackCount;
    updateState();
}

D3D11_BLEND blendFactorToDX(EG_BLEND_FACTOR factor)
{
    switch (factor)
    {
        case EG_ZERO:                   return D3D11_BLEND_ZERO;
        case EG_ONE:                    return D3D11_BLEND_ONE;
        case EG_SRC_COLOR:              return D3D11_BLEND_SRC_COLOR;
        case EG_ONE_MINUS_SRC_COLOR:    return D3D11_BLEND_INV_SRC_COLOR;
        case EG_DST_COLOR:              return D3D11_BLEND_DEST_COLOR;
        case EG_ONE_MINUS_DST_COLOR:    return D3D11_BLEND_INV_DEST_COLOR;
        case EG_SRC_ALPHA:              return D3D11_BLEND_SRC_ALPHA;
        case EG_ONE_MINUS_SRC_ALPHA:    return D3D11_BLEND_INV_SRC_ALPHA;
        case EG_DST_ALPHA:              return D3D11_BLEND_DEST_ALPHA;
        case EG_ONE_MINUS_DST_ALPHA:    return D3D11_BLEND_INV_DEST_ALPHA;
        case EG_SRC_ALPHA_SATURATE:     return D3D11_BLEND_SRC_ALPHA_SAT;
    }
    return D3D11_BLEND_ZERO;
}

void egBlendFunc(EG_BLEND_FACTOR src, EG_BLEND_FACTOR dst)
{
    if (!pBoundDevice) return;
    SEGState *pState = pBoundDevice->states + pBoundDevice->statesStackCount;
    SEGState *pPreviousState = pState;
    if (pBoundDevice->statesStackCount) --pPreviousState;
    D3D11_BLEND dxSrc = blendFactorToDX(src);
    D3D11_BLEND dxDst = blendFactorToDX(dst);
    if (pState->blend.RenderTarget->SrcBlend != dxSrc)
    {
        pState->blend.RenderTarget->SrcBlend = dxSrc;
        pState->blendDirty = TRUE;
        pPreviousState->blendDirty = TRUE;
    }
    if (pState->blend.RenderTarget->DestBlend != dxDst)
    {
        pState->blend.RenderTarget->DestBlend = dxDst;
        pState->blendDirty = TRUE;
        pPreviousState->blendDirty = TRUE;
    }
}

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

void egPostProcess()
{
    if (bIsInBatch) return;
    if (!pBoundDevice) return;
    beginPostProcessPass();

    float white[4] = {1, 1, 1, 1};
    //pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->gBuffer[G_DIFFUSE].texture.pResourceView);
    //drawScreenQuad(-1, 1, 0, 0, white);

    //pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->gBuffer[G_MATERIAL].texture.pResourceView);
    //drawScreenQuad(0, 1, 1, 0, white);

    //pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->gBuffer[G_NORMAL].texture.pResourceView);
    //drawScreenQuad(-1, 0, 0, -1, white);

    //pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->accumulationBuffer.texture.pResourceView);
    //drawScreenQuad(0, 0, 1, -1, white);

    pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->accumulationBuffer.texture.pResourceView);
    drawScreenQuad(-1, 1, 1, -1, white);
}
