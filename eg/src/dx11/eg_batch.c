#include "eg_device.h"

BOOL bMapVB = TRUE;

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
    bMapVB = TRUE;
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
            bMapVB = FALSE;
            break;
        case EG_OMNIS:
            beginOmniPass();
            bMapVB = FALSE;
            break;
        default:
            return;
    }

    pBoundDevice->bIsInBatch = TRUE;
    if (bMapVB)
    {
        D3D11_MAPPED_SUBRESOURCE mappedVertexBuffer;
        pBoundDevice->pDeviceContext->lpVtbl->Map(pBoundDevice->pDeviceContext, pBoundDevice->pVertexBufferRes, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedVertexBuffer);
        pBoundDevice->pVertex = (SEGVertex*)mappedVertexBuffer.pData;
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

    pBoundDevice->pDeviceContext->lpVtbl->Unmap(pBoundDevice->pDeviceContext, pBoundDevice->pVertexBufferRes, 0);

    // Make sure states are up to date
    updateState();

    const UINT stride = sizeof(SEGVertex);
    const UINT offset = 0;
    pBoundDevice->pDeviceContext->lpVtbl->IASetVertexBuffers(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->pVertexBuffer, &stride, &offset);
    pBoundDevice->pDeviceContext->lpVtbl->Draw(pBoundDevice->pDeviceContext, pBoundDevice->currentVertexCount, 0);

    pBoundDevice->currentVertexCount = 0;

    D3D11_MAPPED_SUBRESOURCE mappedVertexBuffer;
    pBoundDevice->pDeviceContext->lpVtbl->Map(pBoundDevice->pDeviceContext, pBoundDevice->pVertexBufferRes, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedVertexBuffer);
    pBoundDevice->pVertex = (SEGVertex*)mappedVertexBuffer.pData;
}

void egEnd()
{
    if (!pBoundDevice->bIsInBatch) return;
    if (!pBoundDevice) return;
    flush();
    pBoundDevice->bIsInBatch = FALSE;

    if (bMapVB)
    {
        pBoundDevice->pDeviceContext->lpVtbl->Unmap(pBoundDevice->pDeviceContext, pBoundDevice->pVertexBufferRes, 0);
    }
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
}

void egSpecular(float intensity, float shininess)
{
}

void egSelfIllum(float intensity)
{
}

void drawScreenQuad(float left, float top, float right, float bottom, float *pColor)
{
    D3D11_MAPPED_SUBRESOURCE mappedVertexBuffer;
    pBoundDevice->pDeviceContext->lpVtbl->Map(pBoundDevice->pDeviceContext, pBoundDevice->pVertexBufferRes, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedVertexBuffer);
    pBoundDevice->pVertex = (SEGVertex*)mappedVertexBuffer.pData;

    pBoundDevice->pVertex[0].x = left;
    pBoundDevice->pVertex[0].y = top;
    pBoundDevice->pVertex[0].u = 0;
    pBoundDevice->pVertex[0].v = 0;
    memcpy(&pBoundDevice->pVertex[0].r, pColor, 16);

    pBoundDevice->pVertex[1].x = left;
    pBoundDevice->pVertex[1].y = bottom;
    pBoundDevice->pVertex[1].u = 0;
    pBoundDevice->pVertex[1].v = 1;
    memcpy(&pBoundDevice->pVertex[1].r, pColor, 16);

    pBoundDevice->pVertex[2].x = right;
    pBoundDevice->pVertex[2].y = top;
    pBoundDevice->pVertex[2].u = 1;
    pBoundDevice->pVertex[2].v = 0;
    memcpy(&pBoundDevice->pVertex[2].r, pColor, 16);

    pBoundDevice->pVertex[3].x = right;
    pBoundDevice->pVertex[3].y = bottom;
    pBoundDevice->pVertex[3].u = 1;
    pBoundDevice->pVertex[3].v = 1;
    memcpy(&pBoundDevice->pVertex[3].r, pColor, 16);

    pBoundDevice->pDeviceContext->lpVtbl->Unmap(pBoundDevice->pDeviceContext, pBoundDevice->pVertexBufferRes, 0);

    const UINT stride = sizeof(SEGVertex);
    const UINT offset = 0;
    pBoundDevice->pDeviceContext->lpVtbl->IASetVertexBuffers(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->pVertexBuffer, &stride, &offset);
    pBoundDevice->pDeviceContext->lpVtbl->Draw(pBoundDevice->pDeviceContext, 4, 0);
}
