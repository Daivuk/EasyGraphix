#include <assert.h>
#include <stdio.h>
#include <d3d11.h>
#include <D3Dcompiler.h>
#include "eg.h"
#include "eg_device.h"
#include "eg_shaders.h"

void egSwap()
{
    if (pBoundDevice->bIsInBatch) return;
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
    if (!pBoundDevice) return;
    if (pBoundDevice->bIsInBatch) return;
    if (clearBitFields & EG_CLEAR_COLOR)
    {
        pBoundDevice->pDeviceContext->lpVtbl->ClearRenderTargetView(pBoundDevice->pDeviceContext, pBoundDevice->accumulationBuffer.pRenderTargetView, pBoundDevice->clearColor);
    }
    if (clearBitFields & EG_CLEAR_DEPTH_STENCIL)
    {
        pBoundDevice->pDeviceContext->lpVtbl->ClearDepthStencilView(pBoundDevice->pDeviceContext, pBoundDevice->pDepthStencilView, 
                                                                    D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    }
    if (clearBitFields & (EG_CLEAR_COLOR | EG_CLEAR_G_BUFFER))
    {
        pBoundDevice->pDeviceContext->lpVtbl->ClearRenderTargetView(pBoundDevice->pDeviceContext, pBoundDevice->gBuffer[G_DIFFUSE].pRenderTargetView, pBoundDevice->clearColor);
    }
    if (clearBitFields & EG_CLEAR_G_BUFFER)
    {
        float black[4] = {0, 0, 0, 1};
        float zeroDepth[4] = {0, 0, 0, 0};
        pBoundDevice->pDeviceContext->lpVtbl->ClearRenderTargetView(pBoundDevice->pDeviceContext, pBoundDevice->gBuffer[G_NORMAL].pRenderTargetView, black);
        pBoundDevice->pDeviceContext->lpVtbl->ClearRenderTargetView(pBoundDevice->pDeviceContext, pBoundDevice->gBuffer[G_MATERIAL].pRenderTargetView, black);
        pBoundDevice->pDeviceContext->lpVtbl->ClearRenderTargetView(pBoundDevice->pDeviceContext, pBoundDevice->gBuffer[G_DEPTH].pRenderTargetView, zeroDepth);
    }
}

void egSet2DViewProj(float nearClip, float farClip)
{
    if (!pBoundDevice) return;
    if (pBoundDevice->bIsInBatch) return;

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
    if (!pBoundDevice) return;
    if (pBoundDevice->bIsInBatch) return;

    // Projection
    float aspect = (float)pBoundDevice->backBufferDesc.Width / (float)pBoundDevice->backBufferDesc.Height;
    setProjectionMatrix(&pBoundDevice->projectionMatrix, fov, aspect, nearClip, farClip);

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
    if (!pBoundDevice) return;
    if (pBoundDevice->bIsInBatch) return;

    memcpy(&pBoundDevice->viewMatrix, pView, sizeof(SEGMatrix));
    memcpy(&pBoundDevice->projectionMatrix, pProj, sizeof(SEGMatrix));

    // Multiply them
    multMatrix(&pBoundDevice->viewMatrix, &pBoundDevice->projectionMatrix, &pBoundDevice->viewProjMatrix);

    // Inverse for light pass
    inverseMatrix(&pBoundDevice->viewProjMatrix, &pBoundDevice->invViewProjMatrix);

    updateViewProjCB();
    updateInvViewProjCB();
}

void egGetView(float *pView)
{
    if (!pBoundDevice) return;
    memcpy(pView, pBoundDevice->viewMatrix.m, sizeof(float) * 16);
}

void egGetProjection(float *pProjection)
{
    if (!pBoundDevice) return;
    memcpy(pProjection, pBoundDevice->projectionMatrix.m, sizeof(float) * 16);
}

void egGetModel(float *pModel)
{
    if (!pBoundDevice) return;
    SEGMatrix *pCurrentModel = pBoundDevice->worldMatrices + pBoundDevice->worldMatricesStackCount;
    memcpy(pModel, pCurrentModel->m, sizeof(float) * 16);
}

void egViewPort(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    if (!pBoundDevice) return;
    if (pBoundDevice->bIsInBatch) return;

    pBoundDevice->viewPort[0] = x;
    pBoundDevice->viewPort[1] = y;
    pBoundDevice->viewPort[2] = width;
    pBoundDevice->viewPort[3] = height;

    D3D11_VIEWPORT d3dViewport = {(FLOAT)pBoundDevice->viewPort[0], (FLOAT)pBoundDevice->viewPort[1], (FLOAT)pBoundDevice->viewPort[2], (FLOAT)pBoundDevice->viewPort[3], D3D11_MIN_DEPTH, D3D11_MAX_DEPTH};
    pBoundDevice->pDeviceContext->lpVtbl->RSSetViewports(pBoundDevice->pDeviceContext, 1, &d3dViewport);
}

void egGetViewport(uint32_t *pViewport)
{
    if (!pBoundDevice) return;
    memcpy(pViewport, pBoundDevice->viewPort, sizeof(uint32_t) * 4);
}

void egScissor(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    if (!pBoundDevice) return;
    if (pBoundDevice->bIsInBatch) return;
}

void egModelIdentity()
{
    if (pBoundDevice->bIsInBatch) return;
    SEGMatrix *pModel = pBoundDevice->worldMatrices + pBoundDevice->worldMatricesStackCount;
    setIdentityMatrix(pModel);
    updateModelCB();
}

void egModelTranslate(float x, float y, float z)
{
    if (pBoundDevice->bIsInBatch) return;
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
    if (pBoundDevice->bIsInBatch) return;
    egModelTranslate(pAxis[0], pAxis[1], pAxis[2]);
}

void egModelMult(const float *pMatrix)
{
    if (pBoundDevice->bIsInBatch) return;
    SEGMatrix *pModel = pBoundDevice->worldMatrices + pBoundDevice->worldMatricesStackCount;
    SEGMatrix matrix;
    memcpy(&matrix, pModel, sizeof(SEGMatrix));
    multMatrix((const SEGMatrix*)pMatrix, &matrix, pModel);
    updateModelCB();
}

void egModelRotate(float angle, float x, float y, float z)
{
    if (pBoundDevice->bIsInBatch) return;
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
    if (pBoundDevice->bIsInBatch) return;
    egModelRotate(angle, pAxis[0], pAxis[1], pAxis[2]);
}

void egModelScale(float x, float y, float z)
{
    if (pBoundDevice->bIsInBatch) return;
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
    if (pBoundDevice->bIsInBatch) return;
    egModelScale(pAxis[0], pAxis[1], pAxis[2]);
}

void egModelPush()
{
    if (pBoundDevice->bIsInBatch) return;
    if (pBoundDevice->worldMatricesStackCount == MAX_STACK - 1) return;
    SEGMatrix *pPrevious = pBoundDevice->worldMatrices + pBoundDevice->worldMatricesStackCount;
    ++pBoundDevice->worldMatricesStackCount;
    SEGMatrix *pNew = pBoundDevice->worldMatrices + pBoundDevice->worldMatricesStackCount;
    memcpy(pNew, pPrevious, sizeof(SEGMatrix));
}

void egModelPop()
{
    if (pBoundDevice->bIsInBatch) return;
    if (!pBoundDevice->worldMatricesStackCount) return;
    --pBoundDevice->worldMatricesStackCount;
    updateModelCB();
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

void egPostProcess()
{
    if (pBoundDevice->bIsInBatch) return;
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
