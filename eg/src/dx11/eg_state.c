#include "eg_device.h"

void egStatePush()
{
    if (pBoundDevice->bIsInBatch) return;
    if (pBoundDevice->statesStackCount == MAX_STACK - 1) return;
    SEGState *pPrevious = pBoundDevice->states + pBoundDevice->statesStackCount;
    ++pBoundDevice->statesStackCount;
    SEGState *pNew = pBoundDevice->states + pBoundDevice->statesStackCount;
    memcpy(pNew, pPrevious, sizeof(SEGState));
}

void egStatePop()
{
    if (pBoundDevice->bIsInBatch) return;
    if (!pBoundDevice->statesStackCount) return;
    --pBoundDevice->statesStackCount;
    updateState();
}

void resetState()
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
    pState->rasterizer.FrontCounterClockwise = TRUE;

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

void egFrontFace(EG_FRONT_FACE mode)
{
    if (!pBoundDevice) return;
    SEGState *pState = pBoundDevice->states + pBoundDevice->statesStackCount;
    SEGState *pPreviousState = pState;
    if (pBoundDevice->statesStackCount) --pPreviousState;
    if (pState->rasterizer.FrontCounterClockwise != (BOOL)mode)
    {
        pState->rasterizer.FrontCounterClockwise = (BOOL)mode;
        pState->rasterizerDirty = TRUE;
        pPreviousState->rasterizerDirty = TRUE;
    }
}

void egFilter(EG_FILTER filter)
{
    if (!pBoundDevice) return;
    SEGState *pState = pBoundDevice->states + pBoundDevice->statesStackCount;
    SEGState *pPreviousState = pState;
    if (pBoundDevice->statesStackCount) --pPreviousState;
    if ((filter & 0xff) == D3D11_FILTER_ANISOTROPIC)
    {
        UINT maxAnisotropy = (filter >> 8) & 0xff;
        if (pState->sampler.Filter != D3D11_FILTER_ANISOTROPIC ||
            pState->sampler.MaxAnisotropy != maxAnisotropy)
        {
            pState->sampler.Filter = (filter & 0xff);
            pState->sampler.MaxAnisotropy = maxAnisotropy;
            pState->samplerDirty = TRUE;
            pPreviousState->samplerDirty = TRUE;
        }
    }
    else if ((filter & 0xff) != pState->sampler.Filter)
    {
        pState->sampler.Filter = (filter & 0xff);
        pState->sampler.MaxAnisotropy = 1; // We don't care, but in case
        pState->samplerDirty = TRUE;
        pPreviousState->samplerDirty = TRUE;
    }
}

void egEnable(EG_ENABLE stateBits)
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
        case EG_CULL:
            if (pState->rasterizer.CullMode != D3D11_CULL_NONE) break;
            pState->rasterizer.CullMode = D3D11_CULL_BACK;
            pState->rasterizerDirty = TRUE;
            pPreviousState->rasterizerDirty = TRUE;
            break;
        case EG_SCISSOR:
            if (pState->rasterizer.ScissorEnable) break;
            pState->rasterizer.ScissorEnable = TRUE;
            pState->rasterizerDirty = TRUE;
            pPreviousState->rasterizerDirty = TRUE;
            break;
    }
}

void egDisable(EG_ENABLE stateBits)
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
        case EG_CULL:
            if (pState->rasterizer.CullMode == D3D11_CULL_NONE) break;
            pState->rasterizer.CullMode = D3D11_CULL_NONE;
            pState->rasterizerDirty = TRUE;
            pPreviousState->rasterizerDirty = TRUE;
            break;
        case EG_SCISSOR:
            if (!pState->rasterizer.ScissorEnable) break;
            pState->rasterizer.ScissorEnable = FALSE;
            pState->rasterizerDirty = TRUE;
            pPreviousState->rasterizerDirty = TRUE;
            break;
    }
}

//EG_BLEND = 0x00000001,

//EG_DEPTH_TEST = 0x00000004,
//EG_STENCIL_TEST = 0x00000008,
//EG_ALPHA_TEST = 0x00000010,
//EG_SCISSOR = 0x00000020,
//EG_GENERATE_TANGENT_BINORMAL = 0x00000040,
//EG_BLOOM = 0x00000080,
//EG_HDR = 0x00000100,
//EG_BLUR = 0x00000200,
//EG_WIREFRAME = 0x00000400,
//EG_SHADOW = 0x00000800,
//EG_DISTORTION = 0x00001000,
//EG_AMBIENT_OCCLUSION = 0x00002000
