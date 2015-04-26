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

    pState->enableBits = EG_DEPTH_WRITE;

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

    pState->alphaTestFunc = EG_LEQUAL;
    pState->alphaTestRef[0] = .5f;
    pState->alphaTestRef[1] = 0;
    pState->alphaTestRef[2] = 0;
    pState->alphaTestRef[3] = 0;

    pState->blurSpread = 8.f;

    pState->blendDirty = TRUE;
    pState->depthDirty = TRUE;
    pState->rasterizerDirty = TRUE;
    pState->samplerDirty = TRUE;
    pState->alphaTestDirty = TRUE;

    pBoundDevice->pDeviceContext->lpVtbl->IASetInputLayout(pBoundDevice->pDeviceContext, pBoundDevice->pInputLayout);
    pBoundDevice->pDeviceContext->lpVtbl->VSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pVS, NULL, 0);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pPS, NULL, 0);
    pBoundDevice->pActivePS = pBoundDevice->pPS;

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
    if (pState->alphaTestDirty)
    {
        if (pState->enableBits & EG_ALPHA_TEST)
        {
            pBoundDevice->pActivePS = pBoundDevice->pPSAlphaTest[pState->alphaTestFunc];

            // Update the constant buffer
            D3D11_MAPPED_SUBRESOURCE map;
            ID3D11Resource *pRes = NULL;
            pBoundDevice->pCBAlphaTestRef->lpVtbl->QueryInterface(pBoundDevice->pCBAlphaTestRef, &IID_ID3D11Resource, &pRes);
            pBoundDevice->pDeviceContext->lpVtbl->Map(pBoundDevice->pDeviceContext, pRes, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
            memcpy(map.pData, pState->alphaTestRef, 16);
            pBoundDevice->pDeviceContext->lpVtbl->Unmap(pBoundDevice->pDeviceContext, pRes, 0);
            pRes->lpVtbl->Release(pRes);
            pBoundDevice->pDeviceContext->lpVtbl->PSSetConstantBuffers(pBoundDevice->pDeviceContext, 2, 1, &pBoundDevice->pCBAlphaTestRef);
        }
        else
        {
            pBoundDevice->pActivePS = pBoundDevice->pPS;
        }
        pBoundDevice->pDeviceContext->lpVtbl->PSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pActivePS, NULL, 0);
        pState->alphaTestDirty = FALSE;
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

void egAlphaFunc(EG_COMPARE func, float ref)
{
    if (!pBoundDevice) return;
    SEGState *pState = pBoundDevice->states + pBoundDevice->statesStackCount;
    SEGState *pPreviousState = pState;
    if (pBoundDevice->statesStackCount) --pPreviousState;
    if (pState->alphaTestFunc != func ||
        pState->alphaTestRef[0] != ref)
    {
        ref = min(ref, 1);
        ref = max(0, ref);
        pState->alphaTestFunc = func;
        pState->alphaTestRef[0] = ref;
        pState->alphaTestDirty = TRUE;
        pPreviousState->alphaTestDirty = TRUE;
    }
}

void egDepthFunc(EG_COMPARE func)
{
    if (!pBoundDevice) return;
    SEGState *pState = pBoundDevice->states + pBoundDevice->statesStackCount;
    SEGState *pPreviousState = pState;
    if (pBoundDevice->statesStackCount) --pPreviousState;
    if (pState->depth.DepthFunc != func + 1)
    {
        pState->depth.DepthFunc = func + 1;
        pState->depthDirty = TRUE;
        pPreviousState->depthDirty = TRUE;
    }
}

void egBlur(float spread)
{
    if (!pBoundDevice) return;
    SEGState *pState = pBoundDevice->states + pBoundDevice->statesStackCount;
    pState->blurSpread = spread;
}

void egEnable(EGEnable stateBits)
{
    if (!pBoundDevice) return;
    SEGState *pState = pBoundDevice->states + pBoundDevice->statesStackCount;
    SEGState *pPreviousState = pState;
    if (pBoundDevice->statesStackCount) --pPreviousState;
    if ((stateBits & EG_BLEND) && !(pState->enableBits & EG_BLEND))
    {
        pState->blend.RenderTarget->BlendEnable = TRUE;
        pState->blendDirty = TRUE;
        pPreviousState->blendDirty = TRUE;
    }
    if ((stateBits & EG_DEPTH_TEST) && !(pState->enableBits & EG_DEPTH_TEST))
    {
        pState->depth.DepthEnable = TRUE;
        pState->depthDirty = TRUE;
        pPreviousState->depthDirty = TRUE;
    }
    if ((stateBits & EG_CULL) && !(pState->enableBits & EG_CULL))
    {
        pState->rasterizer.CullMode = D3D11_CULL_BACK;
        pState->rasterizerDirty = TRUE;
        pPreviousState->rasterizerDirty = TRUE;
    }
    if ((stateBits & EG_SCISSOR) && !(pState->enableBits & EG_SCISSOR))
    {
        pState->rasterizer.ScissorEnable = TRUE;
        pState->rasterizerDirty = TRUE;
        pPreviousState->rasterizerDirty = TRUE;
    }
    if ((stateBits & EG_ALPHA_TEST) && !(pState->enableBits & EG_ALPHA_TEST))
    {
        pState->alphaTestDirty = TRUE;
        pPreviousState->alphaTestDirty = TRUE;
    }
    if ((stateBits & EG_DEPTH_WRITE) && !(pState->enableBits & EG_DEPTH_WRITE))
    {
        pState->depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        pState->depthDirty = TRUE;
        pPreviousState->depthDirty = TRUE;
    }
    if ((stateBits & EG_WIREFRAME) && !(pState->enableBits & EG_WIREFRAME))
    {
        pState->rasterizer.FillMode = D3D11_FILL_WIREFRAME;
        pState->rasterizerDirty = TRUE;
        pPreviousState->rasterizerDirty = TRUE;
    }
    pState->enableBits |= stateBits;
}

void egDisable(EGEnable stateBits)
{
    if (!pBoundDevice) return;
    SEGState *pState = pBoundDevice->states + pBoundDevice->statesStackCount;
    SEGState *pPreviousState = pState;
    if (pBoundDevice->statesStackCount) --pPreviousState;
    if ((stateBits & EG_BLEND) && (pState->enableBits & EG_BLEND))
    {
        pState->blend.RenderTarget->BlendEnable = FALSE;
        pState->blendDirty = TRUE;
        pPreviousState->blendDirty = TRUE;
    }
    if ((stateBits & EG_DEPTH_TEST) && (pState->enableBits & EG_DEPTH_TEST))
    {
        pState->depth.DepthEnable = FALSE;
        pState->depthDirty = TRUE;
        pPreviousState->depthDirty = TRUE;
    }
    if ((stateBits & EG_CULL) && (pState->enableBits & EG_CULL))
    {
        pState->rasterizer.CullMode = D3D11_CULL_NONE;
        pState->rasterizerDirty = TRUE;
        pPreviousState->rasterizerDirty = TRUE;
    }
    if ((stateBits & EG_SCISSOR) && (pState->enableBits & EG_SCISSOR))
    {
        pState->rasterizer.ScissorEnable = FALSE;
        pState->rasterizerDirty = TRUE;
        pPreviousState->rasterizerDirty = TRUE;
    }
    if ((stateBits & EG_ALPHA_TEST) && (pState->enableBits & EG_ALPHA_TEST))
    {
        pState->alphaTestDirty = TRUE;
        pPreviousState->alphaTestDirty = TRUE;
    }
    if ((stateBits & EG_DEPTH_WRITE) && (pState->enableBits & EG_DEPTH_WRITE))
    {
        pState->depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        pState->depthDirty = TRUE;
        pPreviousState->depthDirty = TRUE;
    }
    if ((stateBits & EG_WIREFRAME) && (pState->enableBits & EG_WIREFRAME))
    {
        pState->rasterizer.FillMode = D3D11_FILL_SOLID;
        pState->rasterizerDirty = TRUE;
        pPreviousState->rasterizerDirty = TRUE;
    }
    pState->enableBits &= ~stateBits;
}

//--- New features
//EG_SHADOW = 0x00000800,

//--- Post process
//EG_BLOOM = 0x00000080,
//EG_HDR = 0x00000100,
//EG_BLUR = 0x00000200,
//EG_DISTORTION = 0x00001000,
//EG_AMBIENT_OCCLUSION = 0x00002000

//--- Potential ditch
//EG_STENCIL_TEST = 0x00000008,
//EG_BLEND = 0x00000001, // Alpha channel op. Useful?
