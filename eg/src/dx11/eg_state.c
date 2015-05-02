#include "eg_device.h"

void egStatePush()
{
    if (pBoundDevice->bIsInBatch) return;
    if (pBoundDevice->statesStackCount == MAX_STACK - 1) return;
    SEGState *pPrevious = pBoundDevice->stateStack + pBoundDevice->statesStackCount;
    ++pBoundDevice->statesStackCount;
    SEGState *pNew = pBoundDevice->stateStack + pBoundDevice->statesStackCount;
    memcpy(pNew, pPrevious, sizeof(SEGState));
}

void egStatePop()
{
    if (pBoundDevice->bIsInBatch) return;
    if (!pBoundDevice->statesStackCount) return;
    // Transfer dirty bits to the previous state if they were not applied
    SEGState *pCurrent = pBoundDevice->stateStack + pBoundDevice->statesStackCount;
    SEGState *pPrevious = pCurrent - 1;
    pPrevious->dirtyBits |= pCurrent->dirtyBits;
    --pBoundDevice->statesStackCount;
    updateState();
}

void resetState()
{
    SEGState *pState = pBoundDevice->stateStack + pBoundDevice->statesStackCount;
    memset(pState, 0, sizeof(SEGState));

    egSet2DViewProj(-999, 999);
    egViewPort(0, 0, (uint32_t)pBoundDevice->backBufferDesc.Width, (uint32_t)pBoundDevice->backBufferDesc.Height);
    egModelIdentity();

    pState->enableBits = EG_DEPTH_WRITE;

    pState->depthState.desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    pState->depthState.desc.DepthFunc = D3D11_COMPARISON_LESS;
    pState->depthState.desc.StencilEnable = FALSE;
    pState->depthState.desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
    pState->depthState.desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
    pState->depthState.desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    pState->depthState.desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    pState->depthState.desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    pState->depthState.desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    pState->depthState.desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    pState->depthState.desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    pState->depthState.desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    pState->depthState.desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    pState->rasterizerState.desc.FillMode = D3D11_FILL_SOLID;
    pState->rasterizerState.desc.CullMode = D3D11_CULL_NONE;
    pState->rasterizerState.desc.FrontCounterClockwise = TRUE;

    pState->blendState.desc.IndependentBlendEnable = TRUE;
    for (int i = 0; i < 8; ++i)
    {
        pState->blendState.desc.RenderTarget[i].BlendEnable = FALSE;
        pState->blendState.desc.RenderTarget[i].SrcBlend = D3D11_BLEND_ONE;
        pState->blendState.desc.RenderTarget[i].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        pState->blendState.desc.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
        pState->blendState.desc.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
        pState->blendState.desc.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ZERO;
        pState->blendState.desc.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        pState->blendState.desc.RenderTarget[i].RenderTargetWriteMask = D3D10_COLOR_WRITE_ENABLE_ALL;
    }

    pState->samplerState.desc.Filter = D3D11_FILTER_ANISOTROPIC;
    pState->samplerState.desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    pState->samplerState.desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    pState->samplerState.desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    pState->samplerState.desc.MaxAnisotropy = 4;
    pState->samplerState.desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    pState->samplerState.desc.MaxLOD = D3D11_FLOAT32_MAX;

    pState->alphaTestState.func = EG_LEQUAL;
    pState->alphaTestState.ref = .5f;

    pState->blurState.spread = 8.f;

    pState->dirtyBits = DIRTY_ALL;

    pBoundDevice->pDeviceContext->lpVtbl->IASetInputLayout(pBoundDevice->pDeviceContext, pBoundDevice->pInputLayout);
    pBoundDevice->pDeviceContext->lpVtbl->VSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pVS, NULL, 0);
    pBoundDevice->pActivePS = pBoundDevice->pPSes[0];
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pActivePS, NULL, 0);

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
    SEGState *pState = pBoundDevice->stateStack + pBoundDevice->statesStackCount;
    if (pState->dirtyBits & DIRTY_DEPTH)
    {
        ID3D11DepthStencilState *pDs2D;
        pBoundDevice->pDevice->lpVtbl->CreateDepthStencilState(pBoundDevice->pDevice, &pState->depthState.desc, &pDs2D);
        pBoundDevice->pDeviceContext->lpVtbl->OMSetDepthStencilState(pBoundDevice->pDeviceContext, pDs2D, 1);
        pDs2D->lpVtbl->Release(pDs2D);
    }
    if (pState->dirtyBits & DIRTY_RASTERIZER)
    {
        ID3D11RasterizerState *pSr2D;
        pBoundDevice->pDevice->lpVtbl->CreateRasterizerState(pBoundDevice->pDevice, &pState->rasterizerState.desc, &pSr2D);
        pBoundDevice->pDeviceContext->lpVtbl->RSSetState(pBoundDevice->pDeviceContext, pSr2D);
        pSr2D->lpVtbl->Release(pSr2D);
    }
    if (pState->dirtyBits & DIRTY_BLEND)
    {
        ID3D11BlendState *pBs2D;
        pBoundDevice->pDevice->lpVtbl->CreateBlendState(pBoundDevice->pDevice, &pState->blendState.desc, &pBs2D);
        pBoundDevice->pDeviceContext->lpVtbl->OMSetBlendState(pBoundDevice->pDeviceContext, pBs2D, NULL, 0xffffffff);
        pBs2D->lpVtbl->Release(pBs2D);
    }
    if (pState->dirtyBits & DIRTY_SAMPLER)
    {
        ID3D11SamplerState *pSs2D;
        pBoundDevice->pDevice->lpVtbl->CreateSamplerState(pBoundDevice->pDevice, &pState->samplerState.desc, &pSs2D);
        pBoundDevice->pDeviceContext->lpVtbl->PSSetSamplers(pBoundDevice->pDeviceContext, 0, 1, &pSs2D);
        pSs2D->lpVtbl->Release(pSs2D);
    }
    if (pState->dirtyBits & DIRTY_ALPHA_TEST)
    {
        int i = 0;
        if (pState->enableBits & EG_ALPHA_TEST)
        {
            i += 1 + pState->alphaTestState.func;

            // Update the constant buffer
            D3D11_MAPPED_SUBRESOURCE map;
            ID3D11Resource *pRes = NULL;
            pBoundDevice->pCBAlphaTestRef->lpVtbl->QueryInterface(pBoundDevice->pCBAlphaTestRef, &IID_ID3D11Resource, &pRes);
            pBoundDevice->pDeviceContext->lpVtbl->Map(pBoundDevice->pDeviceContext, pRes, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
            float ref4[4] = {pState->alphaTestState.ref, 0, 0, 0};
            memcpy(map.pData, ref4, 16);
            pBoundDevice->pDeviceContext->lpVtbl->Unmap(pBoundDevice->pDeviceContext, pRes, 0);
            pRes->lpVtbl->Release(pRes);
            pBoundDevice->pDeviceContext->lpVtbl->PSSetConstantBuffers(pBoundDevice->pDeviceContext, 2, 1, &pBoundDevice->pCBAlphaTestRef);
        }
        if (!(pState->enableBits & EG_LIGHTING))
        {
            i += 9;
        }
        pBoundDevice->pActivePS = pBoundDevice->pPSes[i];
        pBoundDevice->pDeviceContext->lpVtbl->PSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pActivePS, NULL, 0);
    }
    if (pState->dirtyBits & DIRTY_LIGHTING)
    {
        int i = 0;
        if (pState->enableBits & EG_ALPHA_TEST)
        {
            i += 1 + pState->alphaTestState.func;
        }
        if (!(pState->enableBits & EG_LIGHTING))
        {
            i += 9;
        }
        pBoundDevice->pActivePS = pBoundDevice->pPSes[i];
        pBoundDevice->pDeviceContext->lpVtbl->PSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pActivePS, NULL, 0);
    }
    pState->dirtyBits = DIRTY_NONE;
}

void updateBlendState(SEGState *pState)
{
    ID3D11BlendState *pBs2D;
    pBoundDevice->pDevice->lpVtbl->CreateBlendState(pBoundDevice->pDevice, &pState->blendState.desc, &pBs2D);
    pBoundDevice->pDeviceContext->lpVtbl->OMSetBlendState(pBoundDevice->pDeviceContext, pBs2D, NULL, 0xffffffff);
    pBs2D->lpVtbl->Release(pBs2D);
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

void dirtyCurrentState(uint32_t dirtyFlags)
{
    SEGState *pState = pBoundDevice->stateStack + pBoundDevice->statesStackCount;
    pState->dirtyBits |= dirtyFlags;
    if (pBoundDevice->statesStackCount)
    {
        pState->dirtyBits |= dirtyFlags;
        --pState;
    }
}

void egBlendFunc(EG_BLEND_FACTOR src, EG_BLEND_FACTOR dst)
{
    if (!pBoundDevice) return;
    SEGState *pState = pBoundDevice->stateStack + pBoundDevice->statesStackCount;
    D3D11_BLEND dxSrc = blendFactorToDX(src);
    D3D11_BLEND dxDst = blendFactorToDX(dst);
    if (pState->blendState.desc.RenderTarget->SrcBlend != dxSrc)
    {
        pState->blendState.desc.RenderTarget->SrcBlend = dxSrc;
        dirtyCurrentState(DIRTY_BLEND);
    }
    if (pState->blendState.desc.RenderTarget->DestBlend != dxDst)
    {
        pState->blendState.desc.RenderTarget->DestBlend = dxDst;
        dirtyCurrentState(DIRTY_BLEND);
    }
}

void egFrontFace(EG_FRONT_FACE mode)
{
    if (!pBoundDevice) return;
    SEGState *pState = pBoundDevice->stateStack + pBoundDevice->statesStackCount;
    if (pState->rasterizerState.desc.FrontCounterClockwise != (BOOL)mode)
    {
        pState->rasterizerState.desc.FrontCounterClockwise = (BOOL)mode;
        dirtyCurrentState(DIRTY_RASTERIZER);
    }
}

void egFilter(EG_FILTER filter)
{
    if (!pBoundDevice) return;
    SEGState *pState = pBoundDevice->stateStack + pBoundDevice->statesStackCount;
    if ((filter & 0xff) == D3D11_FILTER_ANISOTROPIC)
    {
        UINT maxAnisotropy = (filter >> 8) & 0xff;
        if (pState->samplerState.desc.Filter != D3D11_FILTER_ANISOTROPIC ||
            pState->samplerState.desc.MaxAnisotropy != maxAnisotropy)
        {
            pState->samplerState.desc.Filter = (filter & 0xff);
            pState->samplerState.desc.MaxAnisotropy = maxAnisotropy;
            dirtyCurrentState(DIRTY_SAMPLER);
        }
    }
    else if ((filter & 0xff) != pState->samplerState.desc.Filter)
    {
        pState->samplerState.desc.Filter = (filter & 0xff);
        pState->samplerState.desc.MaxAnisotropy = 1; // We don't care, but in case
        dirtyCurrentState(DIRTY_SAMPLER);
    }
}

void egAlphaFunc(EG_COMPARE func, float ref)
{
    if (!pBoundDevice) return;
    SEGState *pState = pBoundDevice->stateStack + pBoundDevice->statesStackCount;
    if (pState->alphaTestState.func != func ||
        pState->alphaTestState.ref != ref)
    {
        ref = min(ref, 1);
        ref = max(0, ref);
        pState->alphaTestState.func = func;
        pState->alphaTestState.ref = ref;
        dirtyCurrentState(DIRTY_ALPHA_TEST);
    }
}

void egDepthFunc(EG_COMPARE func)
{
    if (!pBoundDevice) return;
    SEGState *pState = pBoundDevice->stateStack + pBoundDevice->statesStackCount;
    if (pState->depthState.desc.DepthFunc != func + 1)
    {
        pState->depthState.desc.DepthFunc = func + 1;
        dirtyCurrentState(DIRTY_DEPTH);
    }
}

void egBlur(float spread)
{
    if (!pBoundDevice) return;
    SEGState *pState = pBoundDevice->stateStack + pBoundDevice->statesStackCount;
    pState->blurState.spread = spread;
}

void egEnable(EGEnable stateBits)
{
    if (!pBoundDevice) return;
    SEGState *pState = pBoundDevice->stateStack + pBoundDevice->statesStackCount;
    if ((stateBits & EG_BLEND) && !(pState->enableBits & EG_BLEND))
    {
        pState->blendState.desc.RenderTarget->BlendEnable = TRUE;
        dirtyCurrentState(DIRTY_BLEND);
    }
    if ((stateBits & EG_DEPTH_TEST) && !(pState->enableBits & EG_DEPTH_TEST))
    {
        pState->depthState.desc.DepthEnable = TRUE;
        dirtyCurrentState(DIRTY_DEPTH);
    }
    if ((stateBits & EG_CULL) && !(pState->enableBits & EG_CULL))
    {
        pState->rasterizerState.desc.CullMode = D3D11_CULL_BACK;
        dirtyCurrentState(DIRTY_RASTERIZER);
    }
    if ((stateBits & EG_SCISSOR) && !(pState->enableBits & EG_SCISSOR))
    {
        pState->rasterizerState.desc.ScissorEnable = TRUE;
        dirtyCurrentState(DIRTY_RASTERIZER);
    }
    if ((stateBits & EG_ALPHA_TEST) && !(pState->enableBits & EG_ALPHA_TEST))
    {
        dirtyCurrentState(DIRTY_ALPHA_TEST);
    }
    if ((stateBits & EG_DEPTH_WRITE) && !(pState->enableBits & EG_DEPTH_WRITE))
    {
        pState->depthState.desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dirtyCurrentState(DIRTY_DEPTH);
    }
    if ((stateBits & EG_WIREFRAME) && !(pState->enableBits & EG_WIREFRAME))
    {
        pState->rasterizerState.desc.FillMode = D3D11_FILL_WIREFRAME;
        dirtyCurrentState(DIRTY_RASTERIZER);
    }
    if ((stateBits & EG_LIGHTING) && !(pState->enableBits & EG_LIGHTING))
    {
        dirtyCurrentState(DIRTY_LIGHTING);
    }
    pState->enableBits |= stateBits;
}

void egDisable(EGEnable stateBits)
{
    if (!pBoundDevice) return;
    SEGState *pState = pBoundDevice->stateStack + pBoundDevice->statesStackCount;
    if ((stateBits & EG_BLEND) && (pState->enableBits & EG_BLEND))
    {
        pState->blendState.desc.RenderTarget->BlendEnable = FALSE;
        dirtyCurrentState(DIRTY_BLEND);
    }
    if ((stateBits & EG_DEPTH_TEST) && (pState->enableBits & EG_DEPTH_TEST))
    {
        pState->depthState.desc.DepthEnable = FALSE;
        dirtyCurrentState(DIRTY_DEPTH);
    }
    if ((stateBits & EG_CULL) && (pState->enableBits & EG_CULL))
    {
        pState->rasterizerState.desc.CullMode = D3D11_CULL_NONE;
        dirtyCurrentState(DIRTY_RASTERIZER);
    }
    if ((stateBits & EG_SCISSOR) && (pState->enableBits & EG_SCISSOR))
    {
        pState->rasterizerState.desc.ScissorEnable = FALSE;
        dirtyCurrentState(DIRTY_RASTERIZER);
    }
    if ((stateBits & EG_ALPHA_TEST) && (pState->enableBits & EG_ALPHA_TEST))
    {
        dirtyCurrentState(DIRTY_ALPHA_TEST);
    }
    if ((stateBits & EG_DEPTH_WRITE) && (pState->enableBits & EG_DEPTH_WRITE))
    {
        pState->depthState.desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        dirtyCurrentState(DIRTY_DEPTH);
    }
    if ((stateBits & EG_WIREFRAME) && (pState->enableBits & EG_WIREFRAME))
    {
        pState->rasterizerState.desc.FillMode = D3D11_FILL_SOLID;
        dirtyCurrentState(DIRTY_RASTERIZER);
    }
    if ((stateBits & EG_LIGHTING) && (pState->enableBits & EG_LIGHTING))
    {
        dirtyCurrentState(DIRTY_LIGHTING);
    }
    pState->enableBits &= ~stateBits;
}

EGState egCreateState()
{
    if (!pBoundDevice) return 0;

    SEGState *pState = pBoundDevice->stateStack + pBoundDevice->statesStackCount;
    pBoundDevice->states = realloc(pBoundDevice->states, sizeof(SEGState) * (pBoundDevice->stateCount + 1));
    memcpy(pBoundDevice->states + pBoundDevice->stateCount, pState, sizeof(SEGState));
    pState = pBoundDevice->states + pBoundDevice->stateCount;

    // Create static objects for it
    pBoundDevice->pDevice->lpVtbl->CreateDepthStencilState(pBoundDevice->pDevice, &pState->depthState.desc, &pState->depthState.pState);
    pBoundDevice->pDevice->lpVtbl->CreateRasterizerState(pBoundDevice->pDevice, &pState->rasterizerState.desc, &pState->rasterizerState.pState);
    pBoundDevice->pDevice->lpVtbl->CreateBlendState(pBoundDevice->pDevice, &pState->blendState.desc, &pState->blendState.pState);
    pBoundDevice->pDevice->lpVtbl->CreateSamplerState(pBoundDevice->pDevice, &pState->samplerState.desc, &pState->samplerState.pState);
    {    
        D3D11_BUFFER_DESC cbDesc = {sizeof(float) * 4, D3D11_USAGE_IMMUTABLE, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0};
        float initialRef[4] = {pState->alphaTestState.ref, 0, 0, 0};
        D3D11_SUBRESOURCE_DATA initialData = {initialRef, 0, 0};
        pBoundDevice->pDevice->lpVtbl->CreateBuffer(pBoundDevice->pDevice, &cbDesc, &initialData, &pState->alphaTestState.pCB);
    }

    return ++pBoundDevice->stateCount;
}

void egDestroyState(EGState *pState)
{
    if (!pBoundDevice) return;
}

void applyStaticState(SEGState *pState)
{
    // Dirty all bits on the current / previous state
    dirtyCurrentState(DIRTY_ALL);
    SEGState *pCurrentState = pBoundDevice->stateStack + pBoundDevice->statesStackCount;
    pState->dirtyBits = DIRTY_NONE;
    memcpy(pCurrentState, pState, sizeof(SEGState));

    pBoundDevice->pDeviceContext->lpVtbl->OMSetDepthStencilState(pBoundDevice->pDeviceContext, pState->depthState.pState, 1);
    pBoundDevice->pDeviceContext->lpVtbl->RSSetState(pBoundDevice->pDeviceContext, pState->rasterizerState.pState);
    pBoundDevice->pDeviceContext->lpVtbl->OMSetBlendState(pBoundDevice->pDeviceContext, pState->blendState.pState, NULL, 0xffffffff);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetSamplers(pBoundDevice->pDeviceContext, 0, 1, &pState->samplerState.pState);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetConstantBuffers(pBoundDevice->pDeviceContext, 2, 1, &pState->alphaTestState.pCB);

    int i = 0;
    if (pState->enableBits & EG_ALPHA_TEST)
    {
        i += 1 + pState->alphaTestState.func;
    }
    if (!(pState->enableBits & EG_LIGHTING))
    {
        i += 9;
    }
    pBoundDevice->pActivePS = pBoundDevice->pPSes[i];
}

void egBindState(EGState state)
{
    if (!pBoundDevice) return;
    if (!state) return;
    if (state > pBoundDevice->stateCount) return;

    applyStaticState(&pBoundDevice->states[state - 1]);
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
