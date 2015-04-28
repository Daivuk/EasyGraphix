#include "eg_device.h"

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

#define CHAIN_DOWNSAMPLING

uint32_t blur(float spread)
{
    uint32_t blurId = 0;
    while (spread > 8 && blurId < 7)
    {
        blurId++;
        spread /= 2.0f;
    }

    float scale;
    float white[4] = {1, 1, 1, 1};
    float blurSpread[4] = {0};

    ID3D11Resource *pRes = NULL;
    pBoundDevice->pCBBlurSpread->lpVtbl->QueryInterface(pBoundDevice->pCBBlurSpread, &IID_ID3D11Resource, &pRes);

    pBoundDevice->pDeviceContext->lpVtbl->PSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pPSPassThrough, NULL, 0);
#ifdef CHAIN_DOWNSAMPLING
    for (uint32_t i = 0; i <= blurId; ++i)
#else
    for (uint32_t i = blurId; i <= blurId; ++i)
#endif
    {
        scale = 1.0f / (float)pow(2, (double)i);

        // Downscale
#ifdef CHAIN_DOWNSAMPLING
        if (i < blurId)
        {
            scale = 1.0f / (float)pow(2, (double)(i + 1));
            pBoundDevice->pDeviceContext->lpVtbl->PSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pPSPassThrough, NULL, 0);
            pBoundDevice->pDeviceContext->lpVtbl->OMSetRenderTargets(pBoundDevice->pDeviceContext, 1, &pBoundDevice->blurBuffers[i + 1][0].pRenderTargetView, NULL);
            pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->blurBuffers[i][0].texture.pResourceView);
#else
        if (i)
        {
            pBoundDevice->pDeviceContext->lpVtbl->PSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pPSPassThrough, NULL, 0);
            pBoundDevice->pDeviceContext->lpVtbl->OMSetRenderTargets(pBoundDevice->pDeviceContext, 1, &pBoundDevice->blurBuffers[i][0].pRenderTargetView, NULL);
            pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->blurBuffers[0][0].texture.pResourceView);
#endif

            // This is some magic right there! Because resolution might not always end up in base 2 when down sampled.
            // Mostly for windowed mode.
            float wOffset = 1.f;
            float hOffset = 1.f;
            if (pBoundDevice->blurBuffers[i + 1][0].texture.w * 2 < pBoundDevice->blurBuffers[i][0].texture.w)
            {
                wOffset = 1.f - 1.f / (float)pBoundDevice->blurBuffers[i + 1][0].texture.w * 
                    (float)(pBoundDevice->blurBuffers[i][0].texture.w - pBoundDevice->blurBuffers[i + 1][0].texture.w * 2);
            }
            if (pBoundDevice->blurBuffers[i + 1][0].texture.h * 2 < pBoundDevice->blurBuffers[i][0].texture.h)
            {
                hOffset = 1.f - 1.f / (float)pBoundDevice->blurBuffers[i + 1][0].texture.h *
                    (float)(pBoundDevice->blurBuffers[i][0].texture.h - pBoundDevice->blurBuffers[i + 1][0].texture.h * 2);
            }
            drawScreenQuad(-1, 1,
                           -1 + 2 * (scale * wOffset),
                           1 - 2 * (scale * hOffset),
                           white);
        }
#ifdef CHAIN_DOWNSAMPLING
        else
#endif
        {
            // Update the constant buffer
            blurSpread[0] = 1.0f / (float)pBoundDevice->blurBuffers[blurId][0].texture.w * (spread / 8.0f);
            blurSpread[1] = 1.0f / (float)pBoundDevice->blurBuffers[blurId][0].texture.h * (spread / 8.0f);
            D3D11_MAPPED_SUBRESOURCE map;
            pBoundDevice->pDeviceContext->lpVtbl->Map(pBoundDevice->pDeviceContext, pRes, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
            memcpy(map.pData, blurSpread, 16);
            pBoundDevice->pDeviceContext->lpVtbl->Unmap(pBoundDevice->pDeviceContext, pRes, 0);
            pBoundDevice->pDeviceContext->lpVtbl->PSSetConstantBuffers(pBoundDevice->pDeviceContext, 3, 1, &pBoundDevice->pCBBlurSpread);

            // Blur H
            pBoundDevice->pDeviceContext->lpVtbl->PSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pPSBlurH, NULL, 0);
            pBoundDevice->pDeviceContext->lpVtbl->OMSetRenderTargets(pBoundDevice->pDeviceContext, 1, &pBoundDevice->blurBuffers[i][1].pRenderTargetView, NULL);
            pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->blurBuffers[i][0].texture.pResourceView);
            drawScreenQuad(-1, 1, -1 + 2 * scale, 1 - 2 * scale, white);

            // Blur V
            pBoundDevice->pDeviceContext->lpVtbl->PSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pPSBlurV, NULL, 0);
            pBoundDevice->pDeviceContext->lpVtbl->OMSetRenderTargets(pBoundDevice->pDeviceContext, 1, &pBoundDevice->blurBuffers[i][0].pRenderTargetView, NULL);
            pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->blurBuffers[i][1].texture.pResourceView);
            drawScreenQuad(-1, 1, -1 + 2 * scale, 1 - 2 * scale, white);
        }
    }

    pRes->lpVtbl->Release(pRes);

    return blurId;
}

void egPostProcess()
{
    if (pBoundDevice->bIsInBatch) return;
    if (!pBoundDevice) return;
    beginPostProcessPass();
    SEGState *pState = pBoundDevice->states + pBoundDevice->statesStackCount;

    float white[4] = {1, 1, 1, 1};

    ID3D11SamplerState *pOldSs = NULL;
    pBoundDevice->pDeviceContext->lpVtbl->PSGetSamplers(pBoundDevice->pDeviceContext, 0, 1, &pOldSs);
    D3D11_SAMPLER_DESC sampler;
    memset(&sampler, 0, sizeof(D3D11_SAMPLER_DESC));
    sampler.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    sampler.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    sampler.MaxLOD = D3D11_FLOAT32_MAX;
    ID3D11SamplerState *pSs;
    pBoundDevice->pDevice->lpVtbl->CreateSamplerState(pBoundDevice->pDevice, &sampler, &pSs);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetSamplers(pBoundDevice->pDeviceContext, 0, 1, &pSs);
    pSs->lpVtbl->Release(pSs);

    if (pState->enableBits & EG_HDR)
    {
        if (pState->enableBits & EG_BLOOM)
        {
            // Render into the bloom
            pBoundDevice->pDeviceContext->lpVtbl->PSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pPSLDR, NULL, 0);
            pBoundDevice->pDeviceContext->lpVtbl->OMSetRenderTargets(pBoundDevice->pDeviceContext, 1, &pBoundDevice->blurBuffers[0][0].pRenderTargetView, NULL);
            pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->accumulationBuffer.texture.pResourceView);
            drawScreenQuad(-1, 1, 1, -1, white);

            uint32_t blurId = blur(16.f);

            pBoundDevice->pDeviceContext->lpVtbl->PSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pPSPassThrough, NULL, 0);
            pBoundDevice->pDeviceContext->lpVtbl->OMSetRenderTargets(pBoundDevice->pDeviceContext, 1, &pBoundDevice->bloomBuffer.pRenderTargetView, NULL);
            pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->blurBuffers[blurId][0].texture.pResourceView);
            drawScreenQuad(-1, 1, 1, -1, white);

            // Tonemap
            pBoundDevice->pDeviceContext->lpVtbl->PSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pPSToneMap, NULL, 0);
            if (pState->enableBits & EG_BLUR)
            {
                pBoundDevice->pDeviceContext->lpVtbl->OMSetRenderTargets(pBoundDevice->pDeviceContext, 1, &pBoundDevice->blurBuffers[0][0].pRenderTargetView, NULL);
            }
            else
            {
                pBoundDevice->pDeviceContext->lpVtbl->OMSetRenderTargets(pBoundDevice->pDeviceContext, 1, &pBoundDevice->pRenderTargetView, NULL);
            }
            pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->accumulationBuffer.texture.pResourceView);
            pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 1, 1, &pBoundDevice->bloomBuffer.texture.pResourceView);
            drawScreenQuad(-1, 1, 1, -1, white);
        }
        else
        {
            // Tonemap with transparent black bloom texture
            pBoundDevice->pDeviceContext->lpVtbl->PSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pPSToneMap, NULL, 0);
            if (pState->enableBits & EG_BLUR)
            {
                pBoundDevice->pDeviceContext->lpVtbl->OMSetRenderTargets(pBoundDevice->pDeviceContext, 1, &pBoundDevice->blurBuffers[0][0].pRenderTargetView, NULL);
            }
            else
            {
                pBoundDevice->pDeviceContext->lpVtbl->OMSetRenderTargets(pBoundDevice->pDeviceContext, 1, &pBoundDevice->pRenderTargetView, NULL);
            }
            pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->accumulationBuffer.texture.pResourceView);
            pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 1, 1, &pBoundDevice->transparentBlackTexture.pResourceView);
            drawScreenQuad(-1, 1, 1, -1, white);
        }
    }
    else
    {
        pBoundDevice->pDeviceContext->lpVtbl->PSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pPSPassThrough, NULL, 0);
        if (pState->enableBits & EG_BLUR)
        {
            pBoundDevice->pDeviceContext->lpVtbl->OMSetRenderTargets(pBoundDevice->pDeviceContext, 1, &pBoundDevice->blurBuffers[0][0].pRenderTargetView, NULL);
        }
        else
        {
            pBoundDevice->pDeviceContext->lpVtbl->OMSetRenderTargets(pBoundDevice->pDeviceContext, 1, &pBoundDevice->pRenderTargetView, NULL);
        }
        pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->accumulationBuffer.texture.pResourceView);
        drawScreenQuad(-1, 1, 1, -1, white);
    }

    if (pState->enableBits & EG_BLUR)
    {
        uint32_t blurId = blur(pState->blurSpread);

        pBoundDevice->pDeviceContext->lpVtbl->PSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pPSPassThrough, NULL, 0);
        pBoundDevice->pDeviceContext->lpVtbl->OMSetRenderTargets(pBoundDevice->pDeviceContext, 1, &pBoundDevice->pRenderTargetView, NULL);
        pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->blurBuffers[blurId][0].texture.pResourceView);
        drawScreenQuad(-1, 1, 1, -1, white);
    }

    // G-Buffer
    //pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->gBuffer[G_DIFFUSE].texture.pResourceView);
    //drawScreenQuad(-1, 1, 0, 0, white);

    //pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->gBuffer[G_MATERIAL].texture.pResourceView);
    //drawScreenQuad(0, 1, 1, 0, white);

    //pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->gBuffer[G_NORMAL].texture.pResourceView);
    //drawScreenQuad(-1, 0, 0, -1, white);

    //pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->accumulationBuffer.texture.pResourceView);
    //drawScreenQuad(0, 0, 1, -1, white);

    pBoundDevice->pDeviceContext->lpVtbl->PSSetSamplers(pBoundDevice->pDeviceContext, 0, 1, &pOldSs);
}
