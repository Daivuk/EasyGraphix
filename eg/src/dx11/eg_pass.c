#include "eg_device.h"

void beginGeometryPass()
{
    if (pBoundDevice->bIsInBatch) return;
    if (!pBoundDevice) return;

    SEGState *pState = pBoundDevice->stateStack + pBoundDevice->statesStackCount;
    if (pBoundDevice->pass == EG_GEOMETRY_PASS && !(pState->dirtyBits & (DIRTY_ALPHA_TEST | DIRTY_LIGHTING))) return;
    pBoundDevice->pass = EG_GEOMETRY_PASS;

    pBoundDevice->pDeviceContext->lpVtbl->IASetInputLayout(pBoundDevice->pDeviceContext, pBoundDevice->pInputLayout);
    pBoundDevice->pDeviceContext->lpVtbl->VSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pVS, NULL, 0);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pActivePS, NULL, 0);

    if (pState->enableBits & EG_LIGHTING)
    {
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
    else
    {
        pBoundDevice->pDeviceContext->lpVtbl->OMSetRenderTargets(pBoundDevice->pDeviceContext, 1, &pBoundDevice->accumulationBuffer.pRenderTargetView, NULL);
    }
}

void beginAmbientPass()
{
    if (pBoundDevice->bIsInBatch) return;
    if (!pBoundDevice) return;
    if (pBoundDevice->pass == EG_AMBIENT_PASS) return;
    pBoundDevice->pass = EG_AMBIENT_PASS;

    egBindState(pBoundDevice->passStates[EG_AMBIENT_PASS]);

    pBoundDevice->pDeviceContext->lpVtbl->IASetPrimitiveTopology(pBoundDevice->pDeviceContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    pBoundDevice->pDeviceContext->lpVtbl->OMSetRenderTargets(pBoundDevice->pDeviceContext, 1, &pBoundDevice->accumulationBuffer.pRenderTargetView, NULL);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->gBuffer[G_DIFFUSE].texture.pResourceView);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 3, 1, &pBoundDevice->gBuffer[G_MATERIAL].texture.pResourceView);

    pBoundDevice->pDeviceContext->lpVtbl->IASetInputLayout(pBoundDevice->pDeviceContext, pBoundDevice->pInputLayoutPassThrough);
    pBoundDevice->pDeviceContext->lpVtbl->VSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pVSPassThrough, NULL, 0);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pPSAmbient, NULL, 0);
}

void beginOmniPass()
{
    if (pBoundDevice->bIsInBatch) return;
    if (!pBoundDevice) return;
    if (pBoundDevice->pass == EG_OMNI_PASS) return;
    pBoundDevice->pass = EG_OMNI_PASS;

    egBindState(pBoundDevice->passStates[EG_OMNI_PASS]);

    pBoundDevice->pDeviceContext->lpVtbl->IASetPrimitiveTopology(pBoundDevice->pDeviceContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    pBoundDevice->pDeviceContext->lpVtbl->OMSetRenderTargets(pBoundDevice->pDeviceContext, 1, &pBoundDevice->accumulationBuffer.pRenderTargetView, NULL);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->gBuffer[G_DIFFUSE].texture.pResourceView);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 1, 1, &pBoundDevice->gBuffer[G_DEPTH].texture.pResourceView);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 2, 1, &pBoundDevice->gBuffer[G_NORMAL].texture.pResourceView);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 3, 1, &pBoundDevice->gBuffer[G_MATERIAL].texture.pResourceView);

    pBoundDevice->pDeviceContext->lpVtbl->IASetInputLayout(pBoundDevice->pDeviceContext, pBoundDevice->pInputLayoutPassThrough);
    pBoundDevice->pDeviceContext->lpVtbl->VSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pVSPassThrough, NULL, 0);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pPSOmni, NULL, 0);
}

void beginPostProcessPass()
{
    if (pBoundDevice->bIsInBatch) return;
    if (!pBoundDevice) return;

    pBoundDevice->pass = EG_POST_PROCESS_PASS;

    egBindState(pBoundDevice->passStates[EG_POST_PROCESS_PASS]);

    pBoundDevice->pDeviceContext->lpVtbl->IASetPrimitiveTopology(pBoundDevice->pDeviceContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    pBoundDevice->pDeviceContext->lpVtbl->IASetInputLayout(pBoundDevice->pDeviceContext, pBoundDevice->pInputLayoutPassThrough);
    pBoundDevice->pDeviceContext->lpVtbl->VSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pVSPassThrough, NULL, 0);

}
