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
