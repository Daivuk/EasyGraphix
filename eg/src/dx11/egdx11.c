#include <assert.h>
#include <stdio.h>
#include <d3d11.h>
#include <D3Dcompiler.h>
#include "eg.h"
#include "eg_device.h"
#include "eg_shaders.h"

#define MAX_VERTEX_COUNT (300 * 2 * 3)
#define DIFFUSE_MAP     0
#define NORMAL_MAP      1
#define MATERIAL_MAP    2
#define MAX_STACK       256 // Used for states and matrices
#define G_DIFFUSE       0
#define G_DEPTH         1
#define G_NORMAL        2
#define G_MATERIAL      3

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
