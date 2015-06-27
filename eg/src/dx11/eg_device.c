#include "eg.h"
#include "eg_batch.h"
#include "eg_error.h"
#include "eg_device.h"
#include "eg_shaders.h"

SEGDevice  *devices = NULL;
uint32_t    deviceCount = 0;
SEGDevice  *pBoundDevice = NULL;

#define CREATE_VS(__src__, __ppVS__, __blob__) \
{ \
    __blob__ = compileShader(__src__, "vs_5_0"); \
    HRESULT result = pBoundDevice->pDevice->lpVtbl->CreateVertexShader(pBoundDevice->pDevice, \
                                                                       __blob__->lpVtbl->GetBufferPointer(__blob__), \
                                                                       __blob__->lpVtbl->GetBufferSize(__blob__), NULL, __ppVS__); \
    if (result != S_OK) \
    { \
        setError("Failed to compile " #__src__); \
        *__ppVS__ = NULL; \
        egDestroyDevice(&ret); \
        return 0; \
    } \
}

#define CREATE_PS(__src__, __ppPS__) \
{ \
    ID3DBlob *pCompiled = compileShader(__src__, "ps_5_0"); \
    HRESULT result = pBoundDevice->pDevice->lpVtbl->CreatePixelShader(pBoundDevice->pDevice, \
                                                                      pCompiled->lpVtbl->GetBufferPointer(pCompiled), \
                                                                      pCompiled->lpVtbl->GetBufferSize(pCompiled), NULL, __ppPS__); \
    pCompiled->lpVtbl->Release(pCompiled); \
    if (result != S_OK) \
    { \
        setError("Failed to compile " #__src__); \
        *__ppPS__ = NULL; \
        egDestroyDevice(&ret); \
        return 0; \
    } \
}

EGDevice egCreateDevice(HWND windowHandle)
{
    if (pBoundDevice)
    {
        if (pBoundDevice->bIsInBatch) return 0;
    }

    EGDevice                ret = 0;
    DXGI_SWAP_CHAIN_DESC    swapChainDesc;
    HRESULT                 result;
    ID3D11Texture2D        *pBackBuffer;
    ID3D11Resource         *pBackBufferRes;
    ID3D11Texture2D        *pDepthStencilBuffer;

    // Set as currently bound device
    devices = realloc(devices, sizeof(SEGDevice) * (deviceCount + 1));
    memset(devices + deviceCount, 0, sizeof(SEGDevice));
    pBoundDevice = devices + deviceCount;
    ++deviceCount;
    ret = deviceCount;

    // Define our swap chain
    memset(&swapChainDesc, 0, sizeof(swapChainDesc));
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = windowHandle;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;

    // Create the swap chain, device and device context
    result = D3D11CreateDeviceAndSwapChain(
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
#if _DEBUG
        D3D11_CREATE_DEVICE_DEBUG,
#else /* _DEBUG */
        0,
#endif /* _DEBUG */
        NULL, 0, D3D11_SDK_VERSION,
        &swapChainDesc, &pBoundDevice->pSwapChain,
        &pBoundDevice->pDevice, NULL, &pBoundDevice->pDeviceContext);
    if (result != S_OK)
    {
        setError("Failed D3D11CreateDeviceAndSwapChain");
        egDestroyDevice(&ret);
        return 0;
    }

    // Create render target
    result = pBoundDevice->pSwapChain->lpVtbl->GetBuffer(pBoundDevice->pSwapChain, 0, &IID_ID3D11Texture2D, (void**)&pBackBuffer);
    if (result != S_OK)
    {
        setError("Failed IDXGISwapChain GetBuffer IID_ID3D11Texture2D");
        egDestroyDevice(&ret);
        return 0;
    }
    result = pBoundDevice->pSwapChain->lpVtbl->GetBuffer(pBoundDevice->pSwapChain, 0, &IID_ID3D11Resource, (void**)&pBackBufferRes);
    if (result != S_OK)
    {
        setError("Failed IDXGISwapChain GetBuffer IID_ID3D11Resource");
        egDestroyDevice(&ret);
        return 0;
    }
    result = pBoundDevice->pDevice->lpVtbl->CreateRenderTargetView(pBoundDevice->pDevice, pBackBufferRes, NULL, &pBoundDevice->pRenderTargetView);
    if (result != S_OK)
    {
        setError("Failed CreateRenderTargetView");
        egDestroyDevice(&ret);
        return 0;
    }
    pBackBuffer->lpVtbl->GetDesc(pBackBuffer, &pBoundDevice->backBufferDesc);
    pBackBufferRes->lpVtbl->Release(pBackBufferRes);
    pBackBuffer->lpVtbl->Release(pBackBuffer);

    // Set up the description of the depth buffer.
    D3D11_TEXTURE2D_DESC depthBufferDesc = {0};
    depthBufferDesc.Width = pBoundDevice->backBufferDesc.Width;
    depthBufferDesc.Height = pBoundDevice->backBufferDesc.Height;
    depthBufferDesc.MipLevels = 1;
    depthBufferDesc.ArraySize = 1;
    depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthBufferDesc.SampleDesc.Count = 1;
    depthBufferDesc.SampleDesc.Quality = 0;
    depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthBufferDesc.CPUAccessFlags = 0;
    depthBufferDesc.MiscFlags = 0;

    // Create the texture for the depth buffer using the filled out description.
    result = pBoundDevice->pDevice->lpVtbl->CreateTexture2D(pBoundDevice->pDevice, &depthBufferDesc, NULL, &pDepthStencilBuffer);
    if (result != S_OK)
    {
        setError("Failed DepthStencil CreateTexture2D");
        egDestroyDevice(&ret);
        return 0;
    }

    // Initailze the depth stencil view.
    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {0};
    depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.Texture2D.MipSlice = 0;

    // Create the depth stencil view.
    result = pDepthStencilBuffer->lpVtbl->QueryInterface(pDepthStencilBuffer, &IID_ID3D11Resource, (void**)&pBackBufferRes);
    if (result != S_OK)
    {
        setError("Failed DepthStencil ID3D11Texture2D QueryInterface IID_ID3D11Resource");
        egDestroyDevice(&ret);
        return 0;
    }
    result = pBoundDevice->pDevice->lpVtbl->CreateDepthStencilView(pBoundDevice->pDevice, pBackBufferRes, &depthStencilViewDesc, &pBoundDevice->pDepthStencilView);
    if (result != S_OK)
    {
        setError("Failed CreateDepthStencilView");
        egDestroyDevice(&ret);
        return 0;
    }
    pBackBufferRes->lpVtbl->Release(pBackBufferRes);
    pDepthStencilBuffer->lpVtbl->Release(pDepthStencilBuffer);

    // Compile vertex shaders and related input layouts
    {
        ID3DBlob *pCompiled;
        CREATE_VS(g_vs, &pBoundDevice->pVS, pCompiled);
        D3D11_INPUT_ELEMENT_DESC layout[6] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 56, D3D11_INPUT_PER_VERTEX_DATA, 0}
        };
        result = pBoundDevice->pDevice->lpVtbl->CreateInputLayout(pBoundDevice->pDevice, layout, 6, pCompiled->lpVtbl->GetBufferPointer(pCompiled), pCompiled->lpVtbl->GetBufferSize(pCompiled), &pBoundDevice->pInputLayout);
        if (result != S_OK)
        {
            setError("Failed CreateInputLayout");
            egDestroyDevice(&ret);
            return 0;
        }
        pCompiled->lpVtbl->Release(pCompiled);
    }
    {
        ID3DBlob *pCompiled;
        CREATE_VS(g_vsPassThrough, &pBoundDevice->pVSPassThrough, pCompiled);
        D3D11_INPUT_ELEMENT_DESC layout[3] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 56, D3D11_INPUT_PER_VERTEX_DATA, 0}
        };
        result = pBoundDevice->pDevice->lpVtbl->CreateInputLayout(pBoundDevice->pDevice, layout, 3, pCompiled->lpVtbl->GetBufferPointer(pCompiled), pCompiled->lpVtbl->GetBufferSize(pCompiled), &pBoundDevice->pInputLayoutPassThrough);
        if (result != S_OK)
        {
            setError("Failed CreateInputLayout PassThrough");
            egDestroyDevice(&ret);
            return 0;
        }
        pCompiled->lpVtbl->Release(pCompiled);
    }

    // Pixel shaders
    for (int i = 0; i < 18; ++i)
    {
        CREATE_PS(g_pses[i], &(pBoundDevice->pPSes[i]));
    }
    pBoundDevice->pActivePS = pBoundDevice->pPSes[0];
    CREATE_PS(g_psPassThrough, &pBoundDevice->pPSPassThrough);
    CREATE_PS(g_psLDR, &pBoundDevice->pPSLDR);
    CREATE_PS(g_psAmbient, &pBoundDevice->pPSAmbient);
    CREATE_PS(g_psOmni, &pBoundDevice->pPSOmni);
    CREATE_PS(g_psBlurH, &pBoundDevice->pPSBlurH);
    CREATE_PS(g_psBlurV, &pBoundDevice->pPSBlurV);
    for (int i = 0; i < 2; ++i)
    {
        CREATE_PS(g_psPostProcess[i], &(pBoundDevice->pPSPostProcess[i]));
    }

    // Create uniforms
    {
        D3D11_BUFFER_DESC cbDesc = {64, D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE, 0, 0};
        result = pBoundDevice->pDevice->lpVtbl->CreateBuffer(pBoundDevice->pDevice, &cbDesc, NULL, &pBoundDevice->pCBViewProj);
        if (result != S_OK)
        {
            setError("Failed CreateBuffer CBViewProj");
            egDestroyDevice(&ret);
            return 0;
        }
        result = pBoundDevice->pDevice->lpVtbl->CreateBuffer(pBoundDevice->pDevice, &cbDesc, NULL, &pBoundDevice->pCBModel);
        if (result != S_OK)
        {
            setError("Failed CreateBuffer CBModel");
            egDestroyDevice(&ret);
            return 0;
        }
        result = pBoundDevice->pDevice->lpVtbl->CreateBuffer(pBoundDevice->pDevice, &cbDesc, NULL, &pBoundDevice->pCBInvViewProj);
        if (result != S_OK)
        {
            setError("Failed CreateBuffer CBInvViewProj");
            egDestroyDevice(&ret);
            return 0;
        }
    }
    {
        D3D11_BUFFER_DESC cbDesc = {sizeof(SEGOmni), D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE, 0, 0};
        result = pBoundDevice->pDevice->lpVtbl->CreateBuffer(pBoundDevice->pDevice, &cbDesc, NULL, &pBoundDevice->pCBOmni);
        if (result != S_OK)
        {
            setError("Failed CreateBuffer CBInvViewProj");
            egDestroyDevice(&ret);
            return 0;
        }
    }
    {
        D3D11_BUFFER_DESC cbDesc = {sizeof(float) * 4, D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE, 0, 0};
        float initialRef[4] = {.5f, 4.f, 0, 0};
        D3D11_SUBRESOURCE_DATA initialData = {initialRef, 0, 0};
        result = pBoundDevice->pDevice->lpVtbl->CreateBuffer(pBoundDevice->pDevice, &cbDesc, &initialData, &pBoundDevice->pCBAlphaTestRef);
        if (result != S_OK)
        {
            setError("Failed CreateBuffer CBAlphaTestRef");
            egDestroyDevice(&ret);
            return 0;
        }
        pBoundDevice->pDeviceContext->lpVtbl->PSSetConstantBuffers(pBoundDevice->pDeviceContext, 2, 1, &pBoundDevice->pCBAlphaTestRef);
    }
    {
        D3D11_BUFFER_DESC cbDesc = {sizeof(float) * 4, D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE, 0, 0};
        result = pBoundDevice->pDevice->lpVtbl->CreateBuffer(pBoundDevice->pDevice, &cbDesc, NULL, &pBoundDevice->pCBBlurSpread);
        if (result != S_OK)
        {
            setError("Failed CreateBuffer CBBlurSpread");
            egDestroyDevice(&ret);
            return 0;
        }
    }

    // Create our geometry batch vertex buffer that will be used to batch everything
    D3D11_BUFFER_DESC vertexBufferDesc;
    vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    vertexBufferDesc.MiscFlags = 0;
    vertexBufferDesc.StructureByteStride = 0;
    pBoundDevice->pCurrentBatchVertices = (SEGVertex *)malloc(sizeof(SEGVertex) * MAX_VERTEX_COUNT);
    for (UINT i = 0; i < VERTEX_BUFFER_COUNT; ++i)
    {
        UINT vertCount = (UINT)pow(2, 8 + (double)i);
        vertexBufferDesc.ByteWidth = vertCount * sizeof(SEGVertex);
        result = pBoundDevice->pDevice->lpVtbl->CreateBuffer(pBoundDevice->pDevice, &vertexBufferDesc, NULL, &pBoundDevice->pVertexBuffers[i]);
        if (result != S_OK)
        {
            setError("Failed CreateBuffer VertexBuffer");
            egDestroyDevice(&ret);
            return 0;
        }
        result = pBoundDevice->pVertexBuffers[i]->lpVtbl->QueryInterface(pBoundDevice->pVertexBuffers[i], &IID_ID3D11Resource, &pBoundDevice->pVertexBufferResources[i]);
        if (result != S_OK)
        {
            setError("Failed VertexBuffer ID3D11Buffer QueryInterface -> IID_ID3D11Resource");
            egDestroyDevice(&ret);
            return 0;
        }
    }

    // Create default textures
    {
        uint8_t pixel[4] = {255, 255, 255, 255};
        texture2DFromData(pBoundDevice->pDefaultTextureMaps + DIFFUSE_MAP, pixel, 1, 1, FALSE);
        if (!pBoundDevice->pDefaultTextureMaps[DIFFUSE_MAP].pTexture)
        {
            egDestroyDevice(&ret);
            return 0;
        }
    }
    {
        uint8_t pixel[4] = {128, 128, 255, 255};
        texture2DFromData(pBoundDevice->pDefaultTextureMaps + NORMAL_MAP, pixel, 1, 1, FALSE);
        if (!pBoundDevice->pDefaultTextureMaps[NORMAL_MAP].pTexture)
        {
            egDestroyDevice(&ret);
            return 0;
        }
    }
    {
        uint8_t pixel[4] = {0, 1, 0, 0};
        texture2DFromData(pBoundDevice->pDefaultTextureMaps + MATERIAL_MAP, pixel, 1, 1, FALSE);
        if (!pBoundDevice->pDefaultTextureMaps[MATERIAL_MAP].pTexture)
        {
            egDestroyDevice(&ret);
            return 0;
        }
    }
    {
        uint8_t pixel[4] = {0, 0, 0, 0};
        texture2DFromData(&pBoundDevice->transparentBlackTexture, pixel, 1, 1, FALSE);
        if (!pBoundDevice->transparentBlackTexture.pTexture)
        {
            egDestroyDevice(&ret);
            return 0;
        }
    }

    // Create our G-Buffer
    result = createRenderTarget(pBoundDevice->gBuffer + G_DIFFUSE,
                                pBoundDevice->backBufferDesc.Width, pBoundDevice->backBufferDesc.Height,
                                DXGI_FORMAT_R8G8B8A8_UNORM);
    if (result != S_OK)
    {
        egDestroyDevice(&ret);
        return 0;
    }
    result = createRenderTarget(pBoundDevice->gBuffer + G_DEPTH,
                                pBoundDevice->backBufferDesc.Width, pBoundDevice->backBufferDesc.Height,
                                DXGI_FORMAT_R32_FLOAT);
    if (result != S_OK)
    {
        egDestroyDevice(&ret);
        return 0;
    }
    result = createRenderTarget(pBoundDevice->gBuffer + G_NORMAL,
                                pBoundDevice->backBufferDesc.Width, pBoundDevice->backBufferDesc.Height,
                                DXGI_FORMAT_R8G8B8A8_UNORM);
    if (result != S_OK)
    {
        egDestroyDevice(&ret);
        return 0;
    }
    result = createRenderTarget(pBoundDevice->gBuffer + G_MATERIAL,
                                pBoundDevice->backBufferDesc.Width, pBoundDevice->backBufferDesc.Height,
                                DXGI_FORMAT_R8G8B8A8_UNORM);
    if (result != S_OK)
    {
        egDestroyDevice(&ret);
        return 0;
    }

    // Accumulation buffer. This is an HDR texture
    result = createRenderTarget(&pBoundDevice->accumulationBuffer,
                                pBoundDevice->backBufferDesc.Width, pBoundDevice->backBufferDesc.Height,
                                DXGI_FORMAT_R16G16B16A16_FLOAT); // DXGI_FORMAT_R11G11B10_FLOAT
    if (result != S_OK)
    {
        egDestroyDevice(&ret);
        return 0;
    }

    // Create blur buffers
    for (int i = 0; i < 8; ++i)
    {
        UINT divider = (UINT)pow(2, (double)i);
        for (int k = 0; k < 2; ++k)
        {
            UINT w = pBoundDevice->backBufferDesc.Width / divider;
            UINT h = pBoundDevice->backBufferDesc.Height / divider;
            result = createRenderTarget(&pBoundDevice->blurBuffers[i][k],
                                        w, h,
                                        DXGI_FORMAT_R8G8B8A8_UNORM);
            if (result != S_OK)
            {
                egDestroyDevice(&ret);
                return 0;
            }
        }
    }

    resetState();

    // Create some internal states
    {
        egDisable(EG_ALL);
        egEnable(EG_BLEND);
        egBlendFunc(EG_ONE, EG_ONE);
        pBoundDevice->passStates[EG_AMBIENT_PASS] = egCreateState();
        pBoundDevice->passStates[EG_OMNI_PASS] = egCreateState();

        SEGState *pState = pBoundDevice->states + (pBoundDevice->passStates[EG_AMBIENT_PASS] - 1);
        pState->ignoreBits = STATE_ALPHA_TEST | STATE_VIGNETTE;
        pState = pBoundDevice->states + (pBoundDevice->passStates[EG_OMNI_PASS] - 1);
        pState->ignoreBits = STATE_ALPHA_TEST | STATE_VIGNETTE;
    }
    {
        egDisable(EG_ALL);
        egFilter(EG_FILTER_MIN_MAG_LINEAR_MIP_POINT);
        SEGState *pState = pBoundDevice->stateStack + pBoundDevice->statesStackCount;
        pState->samplerState.desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        pState->samplerState.desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        pState->samplerState.desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        pState->samplerState.desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        pState->samplerState.desc.MaxLOD = D3D11_FLOAT32_MAX;
        pBoundDevice->passStates[EG_POST_PROCESS_PASS] = egCreateState();
        pState = pBoundDevice->states + (pBoundDevice->passStates[EG_POST_PROCESS_PASS] - 1);
        pState->ignoreBits = STATE_ALPHA_TEST | STATE_VIGNETTE;
    }

    resetState();

    return ret;
}

void destroyRenderTarget(SEGRenderTarget2D *pRenderTarget)
{
    if (pRenderTarget)
    {
        SEGTexture2D *pTexture = &pRenderTarget->texture;
        if (pTexture)
        {
            if (pTexture->pTexture)
            {
                pTexture->pTexture->lpVtbl->Release(pTexture->pTexture);
            }
            if (pTexture->pResourceView)
            {
                pTexture->pResourceView->lpVtbl->Release(pTexture->pResourceView);
            }
        }
        if (pRenderTarget->pRenderTargetView)
        {
            pRenderTarget->pRenderTargetView->lpVtbl->Release(pRenderTarget->pRenderTargetView);
        }
    }
}

void destroyTexture(SEGTexture2D *pTexture)
{
    if (pTexture)
    {
        if (pTexture->pTexture)
        {
            pTexture->pTexture->lpVtbl->Release(pTexture->pTexture);
        }
        if (pTexture->pResourceView)
        {
            pTexture->pResourceView->lpVtbl->Release(pTexture->pResourceView);
        }
    }
}

void destroyState(SEGState *pState)
{
    if (pState->depthState.pState) pState->depthState.pState->lpVtbl->Release(pState->depthState.pState);
    if (pState->rasterizerState.pState) pState->rasterizerState.pState->lpVtbl->Release(pState->rasterizerState.pState);
    if (pState->blendState.pState) pState->blendState.pState->lpVtbl->Release(pState->blendState.pState);
    if (pState->samplerState.pState) pState->samplerState.pState->lpVtbl->Release(pState->samplerState.pState);
    if (pState->alphaTestState.pCB) pState->alphaTestState.pCB->lpVtbl->Release(pState->alphaTestState.pCB);
}

void egDestroyDevice(EGDevice *pDeviceID)
{
    SEGDevice *pDevice = devices + (*pDeviceID - 1);
    if (pDevice->bIsInBatch) return;

    // Textures
    for (uint32_t i = 0; i < pDevice->textureCount; ++i)
    {
        SEGTexture2D *pTexture = pDevice->textures + i;
        destroyTexture(pTexture);
    }
    free(pDevice->textures);
    for (uint32_t i = 0; i < 3; ++i)
    {
        SEGTexture2D *pTexture = pDevice->pDefaultTextureMaps + i;
        destroyTexture(pTexture);
    }
    destroyTexture(&pDevice->transparentBlackTexture);

    // Render targets
    for (uint32_t i = 0; i < 4; ++i)
    {
        SEGRenderTarget2D *pRenderTarget = pDevice->gBuffer + i;
        destroyRenderTarget(pRenderTarget);
    }
    for (uint32_t i = 0; i < 8; ++i)
    {
        for (uint32_t k = 0; k < 2; ++k)
        {
            SEGRenderTarget2D *pRenderTarget = &pDevice->blurBuffers[i][k];
            destroyRenderTarget(pRenderTarget);
        }
    }
    destroyRenderTarget(&pBoundDevice->accumulationBuffer);

    // VBO
    for (int i = 0; i < 8; ++i)
    {
        if (pDevice->pVertexBufferResources[i]) pDevice->pVertexBufferResources[i]->lpVtbl->Release(pDevice->pVertexBufferResources[i]);
        if (pDevice->pVertexBuffers[i]) pDevice->pVertexBuffers[i]->lpVtbl->Release(pDevice->pVertexBuffers[i]);
    }
    if (pDevice->pCurrentBatchVertices) free(pDevice->pCurrentBatchVertices);

    // Constant buffers
    if (pDevice->pCBModel) pDevice->pCBModel->lpVtbl->Release(pDevice->pCBModel);
    if (pDevice->pCBViewProj) pDevice->pCBViewProj->lpVtbl->Release(pDevice->pCBViewProj);
    if (pDevice->pCBInvViewProj) pDevice->pCBInvViewProj->lpVtbl->Release(pDevice->pCBInvViewProj);
    if (pDevice->pCBAlphaTestRef) pDevice->pCBAlphaTestRef->lpVtbl->Release(pDevice->pCBAlphaTestRef);
    if (pDevice->pCBOmni) pDevice->pCBOmni->lpVtbl->Release(pDevice->pCBOmni);
    if (pDevice->pCBBlurSpread) pDevice->pCBBlurSpread->lpVtbl->Release(pDevice->pCBBlurSpread);

    // Input layouts
    if (pDevice->pInputLayout) pDevice->pInputLayout->lpVtbl->Release(pDevice->pInputLayout);
    if (pDevice->pInputLayoutPassThrough) pDevice->pInputLayout->lpVtbl->Release(pDevice->pInputLayoutPassThrough);

    // Shaders
    if (pDevice->pVS) pDevice->pVS->lpVtbl->Release(pDevice->pVS);
    for (int i = 0; i < 18; ++i)
    {
        if (pDevice->pPSes[i]) pDevice->pPSes[i]->lpVtbl->Release(pDevice->pPSes[i]);
    }
    if (pDevice->pPSPassThrough) pDevice->pPSPassThrough->lpVtbl->Release(pDevice->pPSPassThrough);
    if (pDevice->pVSPassThrough) pDevice->pVSPassThrough->lpVtbl->Release(pDevice->pVSPassThrough);
    if (pDevice->pPSAmbient) pDevice->pPSAmbient->lpVtbl->Release(pDevice->pPSAmbient);
    if (pDevice->pPSOmni) pDevice->pPSOmni->lpVtbl->Release(pDevice->pPSOmni);
    if (pDevice->pPSLDR) pDevice->pPSLDR->lpVtbl->Release(pDevice->pPSLDR);
    if (pDevice->pPSBlurH) pDevice->pPSBlurH->lpVtbl->Release(pDevice->pPSBlurH);
    if (pDevice->pPSBlurV) pDevice->pPSBlurV->lpVtbl->Release(pDevice->pPSBlurV);
    for (int i = 0; i < 2; ++i)
    {
        if (pDevice->pPSPostProcess[i]) pDevice->pPSPostProcess[i]->lpVtbl->Release(pDevice->pPSPostProcess[i]);
    }

    // States
    for (uint32_t i = 0; i < pDevice->stateCount; ++i)
    {
        SEGState *pState = pDevice->states + i;
        destroyState(pState);
    }
    free(pDevice->states);

    // Device
    if (pDevice->pDepthStencilView) pDevice->pDepthStencilView->lpVtbl->Release(pDevice->pDepthStencilView);
    if (pDevice->pRenderTargetView) pDevice->pRenderTargetView->lpVtbl->Release(pDevice->pRenderTargetView);
    if (pDevice->pDeviceContext) pDevice->pDeviceContext->lpVtbl->Release(pDevice->pDeviceContext);
    if (pDevice->pDevice) pDevice->pDevice->lpVtbl->Release(pDevice->pDevice);
    if (pDevice->pSwapChain) pDevice->pSwapChain->lpVtbl->Release(pDevice->pSwapChain);

    memset(pDevice, 0, sizeof(SEGDevice));
    *pDeviceID = 0;
}

void egBindDevice(EGDevice device)
{
    if (pBoundDevice->bIsInBatch) return;
    pBoundDevice = devices + (device - 1);
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
    memcpy(map.pData, &pBoundDevice->currentOmni, sizeof(SEGOmni));
    pBoundDevice->pDeviceContext->lpVtbl->Unmap(pBoundDevice->pDeviceContext, pRes, 0);
    pRes->lpVtbl->Release(pRes);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetConstantBuffers(pBoundDevice->pDeviceContext, 1, 1, &pBoundDevice->pCBOmni);
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

void egGetiv(EGGet what, int *out)
{
    if (!pBoundDevice) return;
    switch (what)
    {
        case EG_RESOLUTION:
            out[0] = (int)pBoundDevice->backBufferDesc.Width;
            out[1] = (int)pBoundDevice->backBufferDesc.Height;
            break;
    }
}

void egResize()
{
    if (!pBoundDevice) return;
    if (pBoundDevice->pRenderTargetView) pBoundDevice->pRenderTargetView->lpVtbl->Release(pBoundDevice->pRenderTargetView);

    // Delete buffers
    pBoundDevice->pSwapChain->lpVtbl->ResizeBuffers(pBoundDevice->pSwapChain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
    for (uint32_t i = 0; i < 4; ++i)
    {
        SEGRenderTarget2D *pRenderTarget = pBoundDevice->gBuffer + i;
        destroyRenderTarget(pRenderTarget);
    }
    for (uint32_t i = 0; i < 8; ++i)
    {
        for (uint32_t k = 0; k < 2; ++k)
        {
            SEGRenderTarget2D *pRenderTarget = &pBoundDevice->blurBuffers[i][k];
            destroyRenderTarget(pRenderTarget);
        }
    }
    destroyRenderTarget(&pBoundDevice->accumulationBuffer);

    ID3D11Texture2D        *pBackBuffer;
    ID3D11Resource         *pBackBufferRes;

    // Create render target
    pBoundDevice->pSwapChain->lpVtbl->GetBuffer(pBoundDevice->pSwapChain, 0, &IID_ID3D11Texture2D, (void**)&pBackBuffer);
    pBoundDevice->pSwapChain->lpVtbl->GetBuffer(pBoundDevice->pSwapChain, 0, &IID_ID3D11Resource, (void**)&pBackBufferRes);
    pBoundDevice->pDevice->lpVtbl->CreateRenderTargetView(pBoundDevice->pDevice, pBackBufferRes, NULL, &pBoundDevice->pRenderTargetView);
    pBackBuffer->lpVtbl->GetDesc(pBackBuffer, &pBoundDevice->backBufferDesc);
    pBackBufferRes->lpVtbl->Release(pBackBufferRes);
    pBackBuffer->lpVtbl->Release(pBackBuffer);

    // Recreate buffers
    // Create our G-Buffer
    createRenderTarget(pBoundDevice->gBuffer + G_DIFFUSE,
                       pBoundDevice->backBufferDesc.Width, pBoundDevice->backBufferDesc.Height,
                       DXGI_FORMAT_R8G8B8A8_UNORM);
    createRenderTarget(pBoundDevice->gBuffer + G_DEPTH,
                       pBoundDevice->backBufferDesc.Width, pBoundDevice->backBufferDesc.Height,
                       DXGI_FORMAT_R32_FLOAT);
    createRenderTarget(pBoundDevice->gBuffer + G_NORMAL,
                       pBoundDevice->backBufferDesc.Width, pBoundDevice->backBufferDesc.Height,
                       DXGI_FORMAT_R8G8B8A8_UNORM);
    createRenderTarget(pBoundDevice->gBuffer + G_MATERIAL,
                       pBoundDevice->backBufferDesc.Width, pBoundDevice->backBufferDesc.Height,
                       DXGI_FORMAT_R8G8B8A8_UNORM);

    // Accumulation buffer. This is an HDR texture
    createRenderTarget(&pBoundDevice->accumulationBuffer,
                       pBoundDevice->backBufferDesc.Width, pBoundDevice->backBufferDesc.Height,
                       DXGI_FORMAT_R16G16B16A16_FLOAT); // DXGI_FORMAT_R11G11B10_FLOAT

    // Create blur buffers
    for (int i = 0; i < 8; ++i)
    {
        UINT divider = (UINT)pow(2, (double)i);
        for (int k = 0; k < 2; ++k)
        {
            UINT w = pBoundDevice->backBufferDesc.Width / divider;
            UINT h = pBoundDevice->backBufferDesc.Height / divider;
            createRenderTarget(&pBoundDevice->blurBuffers[i][k],
                               w, h,
                               DXGI_FORMAT_R8G8B8A8_UNORM);
        }
    }
}
