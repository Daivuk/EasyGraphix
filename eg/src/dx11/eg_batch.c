#include "eg_device.h"
#include "eg_math.h"

void flush();

void drawAmbient()
{
    drawScreenQuad(-1, 1, 1, -1, &pBoundDevice->currentVertex.r);
}

void drawOmni()
{
    memcpy(&pBoundDevice->currentOmni.x, &pBoundDevice->currentVertex.x, 12);
    memcpy(&pBoundDevice->currentOmni.r, &pBoundDevice->currentVertex.r, 16);
    updateOmniCB();
    float color[4] = {1, 1, 1, 1};
    drawScreenQuad(-1, 1, 1, -1, color);
}

void appendVertex(SEGVertex *in_pVertex)
{
    if (pBoundDevice->currentVertexCount == MAX_VERTEX_COUNT) return;
    memcpy(pBoundDevice->pVertex + pBoundDevice->currentVertexCount, in_pVertex, sizeof(SEGVertex));
    ++pBoundDevice->currentVertexCount;
}

void drawVertex(SEGVertex *in_pVertex)
{
    if (!pBoundDevice->bIsInBatch) return;

    if (pBoundDevice->currentMode == EG_TRIANGLE_FAN)
    {
        if (pBoundDevice->currentVertexCount >= 3)
        {
            appendVertex(pBoundDevice->pVertex);
            appendVertex(pBoundDevice->pVertex + pBoundDevice->currentVertexCount - 2);
        }
    }
    else if (pBoundDevice->currentMode == EG_QUADS)
    {
        if ((pBoundDevice->currentVertexCount - 3) % 6 == 0 && pBoundDevice->currentVertexCount)
        {
            appendVertex(pBoundDevice->pVertex + pBoundDevice->currentVertexCount - 3);
            appendVertex(pBoundDevice->pVertex + pBoundDevice->currentVertexCount - 2);
        }
    }
    else if (pBoundDevice->currentMode == EG_QUAD_STRIP)
    {
        if (pBoundDevice->currentVertexCount == 3)
        {
            appendVertex(pBoundDevice->pVertex + pBoundDevice->currentVertexCount - 3);
            appendVertex(pBoundDevice->pVertex + pBoundDevice->currentVertexCount - 2);
        }
        else if (pBoundDevice->currentVertexCount >= 6)
        {
            if ((pBoundDevice->currentVertexCount - 6) % 2 == 0)
            {
                appendVertex(pBoundDevice->pVertex + pBoundDevice->currentVertexCount - 1);
                appendVertex(pBoundDevice->pVertex + pBoundDevice->currentVertexCount - 3);
            }
            else
            {
                appendVertex(pBoundDevice->pVertex + pBoundDevice->currentVertexCount - 3);
                appendVertex(pBoundDevice->pVertex + pBoundDevice->currentVertexCount - 2);
            }
        }
    }

    appendVertex(in_pVertex);
    if (pBoundDevice->currentVertexCount == MAX_VERTEX_COUNT) flush();
}

void egBegin(EG_MODE mode)
{
    if (pBoundDevice->bIsInBatch) return;
    if (!pBoundDevice) return;
    egStatePush();
    pBoundDevice->currentMode = mode;
    switch (pBoundDevice->currentMode)
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
            break;
        case EG_OMNIS:
            beginOmniPass();
            break;
        default:
            return;
    }

    pBoundDevice->pVertex = pBoundDevice->pCurrentBatchVertices;
    pBoundDevice->bIsInBatch = TRUE;
}

void computeTangentBasis(const float *P0, const float *P1, const float *P2,
                         const float *UV0, const float *UV1, const float *UV2,
                         const float *normal, float *tangent, float *binormal)
{
    //using Eric Lengyel's approach with a few modifications
    //from Mathematics for 3D Game Programmming and Computer Graphics
    // want to be able to trasform a vector in Object Space to Tangent Space
    // such that the x-axis cooresponds to the 's' direction and the
    // y-axis corresponds to the 't' direction, and the z-axis corresponds
    // to <0,0,1>, straight up out of the texture map

    //let P = v1 - v0
    float P[3] = {P1[0] - P0[0], P1[1] - P0[1], P1[2] - P0[2]};
    //let Q = v2 - v0
    float Q[3] = {P2[0] - P0[0], P2[1] - P0[1], P2[2] - P0[2]};
    float s1 = UV1[0] - UV0[0];
    float t1 = UV1[1] - UV0[1];
    float s2 = UV2[0] - UV0[0];
    float t2 = UV2[1] - UV0[1];


    //we need to solve the equation
    // P = s1*T + t1*B
    // Q = s2*T + t2*B
    // for T and B


    //this is a linear system with six unknowns and six equatinos, for TxTyTz BxByBz
    //[px,py,pz] = [s1,t1] * [Tx,Ty,Tz]
    // qx,qy,qz     s2,t2     Bx,By,Bz

    //multiplying both sides by the inverse of the s,t matrix gives
    //[Tx,Ty,Tz] = 1/(s1t2-s2t1) *  [t2,-t1] * [px,py,pz]
    // Bx,By,Bz                      -s2,s1	    qx,qy,qz  

    //solve this for the unormalized T and B to get from tangent to object space

    float tmp = 0.0f;
    if (fabsf(s1*t2 - s2*t1) <= 0.0001f)
    {
        tmp = 1.0f;
    }
    else
    {
        tmp = 1.0f / (s1*t2 - s2*t1);
    }

    tangent[0] = (t2*P[0] - t1*Q[0]);
    tangent[1] = (t2*P[1] - t1*Q[1]);
    tangent[2] = (t2*P[2] - t1*Q[2]);

    tangent[0] = tangent[0] * tmp;
    tangent[1] = tangent[1] * tmp;
    tangent[2] = tangent[2] * tmp;

    binormal[0] = (s1*Q[0] - s2*P[0]);
    binormal[1] = (s1*Q[1] - s2*P[1]);
    binormal[2] = (s1*Q[2] - s2*P[2]);

    binormal[0] = binormal[0] * tmp;
    binormal[1] = binormal[1] * tmp;
    binormal[2] = binormal[2] * tmp;
}

void generateTangentBinormal()
{
    switch (pBoundDevice->currentMode)
    {
        case EG_TRIANGLE_STRIP:
        {
            for (uint32_t i = 0; i < pBoundDevice->currentVertexCount - 1; i += 2)
            {
                SEGVertex *pVert = pBoundDevice->pVertex + i;

                computeTangentBasis(&pVert[0].x, &pVert[1].x, &pVert[2].x,
                                    &pVert[0].u, &pVert[1].u, &pVert[2].u,
                                    &pVert[0].nx, &pVert[0].tx, &pVert[0].bx);

                computeTangentBasis(&pVert[1].x, &pVert[2].x, &pVert[0].x,
                                    &pVert[1].u, &pVert[2].u, &pVert[0].u,
                                    &pVert[1].nx, &pVert[1].tx, &pVert[1].bx);
            }
            break;
        }
        case EG_TRIANGLES:
        case EG_TRIANGLE_FAN:
        case EG_QUADS:
        case EG_QUAD_STRIP:
        {
            for (uint32_t i = 0; i < pBoundDevice->currentVertexCount; i += 3)
            {
                SEGVertex *pVert = pBoundDevice->pVertex + i;

                computeTangentBasis(&pVert[0].x, &pVert[1].x, &pVert[2].x,
                                    &pVert[0].u, &pVert[1].u, &pVert[2].u,
                                    &pVert[0].nx, &pVert[0].tx, &pVert[0].bx);

                computeTangentBasis(&pVert[1].x, &pVert[2].x, &pVert[0].x,
                                    &pVert[1].u, &pVert[2].u, &pVert[0].u,
                                    &pVert[1].nx, &pVert[1].tx, &pVert[1].bx);

                computeTangentBasis(&pVert[2].x, &pVert[0].x, &pVert[1].x,
                                    &pVert[2].u, &pVert[0].u, &pVert[1].u,
                                    &pVert[2].nx, &pVert[2].tx, &pVert[2].bx);
            }
            break;
        }
        default:
            break;
    }
}

void flush()
{
    if (!pBoundDevice->bIsInBatch) return;
    if (!pBoundDevice) return;
    if (!pBoundDevice->currentVertexCount) return;

    switch (pBoundDevice->currentMode)
    {
        case EG_LINE_LOOP:
            drawVertex(pBoundDevice->pVertex);
            break;
    }

    // Generate Tangents and Binormals
    SEGState *pState = pBoundDevice->stateStack + pBoundDevice->statesStackCount;
    if (pState->enableBits & EG_GENERATE_TANGENT_BINORMAL)
    {
        generateTangentBinormal();
    }

    // Upload the data to the dynamic vertex buffer.
    // Pick the most appropriate one
    UINT vboIndex = 0;
    UINT verticesCount = pBoundDevice->currentVertexCount;
    while (verticesCount > 256)
    {
        ++vboIndex;
        verticesCount = verticesCount / 2 + (verticesCount - (verticesCount / 2 * 2));
    }
    if (vboIndex < 8)
    {
        D3D11_MAPPED_SUBRESOURCE mappedVertexBuffer;
        pBoundDevice->pDeviceContext->lpVtbl->Map(pBoundDevice->pDeviceContext, pBoundDevice->pVertexBufferResources[vboIndex], 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedVertexBuffer);
        memcpy(mappedVertexBuffer.pData, pBoundDevice->pCurrentBatchVertices, sizeof(SEGVertex) * pBoundDevice->currentVertexCount);
        pBoundDevice->pDeviceContext->lpVtbl->Unmap(pBoundDevice->pDeviceContext, pBoundDevice->pVertexBufferResources[vboIndex], 0);

        // Make sure states are up to date
        updateState();

        const UINT stride = sizeof(SEGVertex);
        const UINT offset = 0;
        pBoundDevice->pDeviceContext->lpVtbl->IASetVertexBuffers(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->pVertexBuffers[vboIndex], &stride, &offset);
        pBoundDevice->pDeviceContext->lpVtbl->Draw(pBoundDevice->pDeviceContext, pBoundDevice->currentVertexCount, 0);
    }

    pBoundDevice->currentVertexCount = 0;
    pBoundDevice->pVertex = pBoundDevice->pCurrentBatchVertices;
}

void egEnd()
{
    if (!pBoundDevice->bIsInBatch) return;
    if (!pBoundDevice) return;
    flush();
    pBoundDevice->bIsInBatch = FALSE;

    egStatePop();
}

void egColor3(float r, float g, float b)
{
    pBoundDevice->currentVertex.r = r;
    pBoundDevice->currentVertex.g = g;
    pBoundDevice->currentVertex.b = b;
    pBoundDevice->currentVertex.a = 1.f;
    if (pBoundDevice->currentMode == EG_AMBIENTS)
    {
        drawAmbient();
    }
}

void egColor3v(const float *pRGB)
{
    memcpy(&pBoundDevice->currentVertex.r, pRGB, 12);
    pBoundDevice->currentVertex.a = 1.f;
    if (pBoundDevice->currentMode == EG_AMBIENTS)
    {
        drawAmbient();
    }
}

void egColor4(float r, float g, float b, float a)
{
    pBoundDevice->currentVertex.r = r;
    pBoundDevice->currentVertex.g = g;
    pBoundDevice->currentVertex.b = b;
    pBoundDevice->currentVertex.a = a;
    if (pBoundDevice->currentMode == EG_AMBIENTS)
    {
        drawAmbient();
    }
}

void egColor4v(const float *pRGBA)
{
    memcpy(&pBoundDevice->currentVertex.r, pRGBA, 16);
    if (pBoundDevice->currentMode == EG_AMBIENTS)
    {
        drawAmbient();
    }
}

void egNormal(float nx, float ny, float nz)
{
    pBoundDevice->currentVertex.nx = nx;
    pBoundDevice->currentVertex.ny = ny;
    pBoundDevice->currentVertex.nz = nz;
}

void egNormalv(const float *pNormal)
{
    memcpy(&pBoundDevice->currentVertex.nx, pNormal, 12);
}

void egTangent(float nx, float ny, float nz)
{
    pBoundDevice->currentVertex.tx = nx;
    pBoundDevice->currentVertex.ty = ny;
    pBoundDevice->currentVertex.tz = nz;
}

void egTangentv(const float *pTangent)
{
    memcpy(&pBoundDevice->currentVertex.tx, pTangent, 12);
}

void egBinormal(float nx, float ny, float nz)
{
    pBoundDevice->currentVertex.bx = nx;
    pBoundDevice->currentVertex.by = ny;
    pBoundDevice->currentVertex.bz = nz;
}

void egBinormalv(const float *pBitnormal)
{
    memcpy(&pBoundDevice->currentVertex.bx, pBitnormal, 12);
}

void egTexCoord(float u, float v)
{
    pBoundDevice->currentVertex.u = u;
    pBoundDevice->currentVertex.v = v;
}

void egTexCoordv(const float *pTexCoord)
{
    memcpy(&pBoundDevice->currentVertex.u, pTexCoord, 8);
}

void egPosition2(float x, float y)
{
    if (!pBoundDevice->bIsInBatch) return;
    pBoundDevice->currentVertex.x = x;
    pBoundDevice->currentVertex.y = y;
    pBoundDevice->currentVertex.z = 0.f;
    if (pBoundDevice->currentMode == EG_OMNIS)
    {
        drawOmni();
    }
    else
    {
        drawVertex(&pBoundDevice->currentVertex);
    }
}

void egPosition2v(const float *pPos)
{
    if (!pBoundDevice->bIsInBatch) return;
    memcpy(&pBoundDevice->currentVertex.x, pPos, 8);
    pBoundDevice->currentVertex.z = 0.f;
    if (pBoundDevice->currentMode == EG_OMNIS)
    {
        drawOmni();
    }
    else
    {
        drawVertex(&pBoundDevice->currentVertex);
    }
}

void egPosition3(float x, float y, float z)
{
    if (!pBoundDevice->bIsInBatch) return;
    pBoundDevice->currentVertex.x = x;
    pBoundDevice->currentVertex.y = y;
    pBoundDevice->currentVertex.z = z;
    if (pBoundDevice->currentMode == EG_OMNIS)
    {
        drawOmni();
    }
    else
    {
        drawVertex(&pBoundDevice->currentVertex);
    }
}

void egPosition3v(const float *pPos)
{
    if (!pBoundDevice->bIsInBatch) return;
    memcpy(&pBoundDevice->currentVertex.x, pPos, 12);
    if (pBoundDevice->currentMode == EG_OMNIS)
    {
        drawOmni();
    }
    else
    {
        drawVertex(&pBoundDevice->currentVertex);
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
    pBoundDevice->currentOmni.radius = radius;
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
    pBoundDevice->currentOmni.multiply = multiply;
}

void egSpecular(float intensity, float shininess)
{
}

void egSelfIllum(float intensity)
{
}
