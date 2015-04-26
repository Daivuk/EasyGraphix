#include "eg.h"
#include "eg_batch.h"
#include "eg_error.h"
#include "eg_device.h"
#include "eg_shaders.h"

SEGDevice  *devices = NULL;
uint32_t    deviceCount = 0;
SEGDevice  *pBoundDevice = NULL;

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

    // Compile shaders
    ID3DBlob *pVSB = compileShader(g_vs, "vs_5_0");
    ID3DBlob *pPSB = compileShader(g_ps, "ps_5_0");
    ID3DBlob *pVSBPassThrough = compileShader(g_vsPassThrough, "vs_5_0");
    ID3DBlob *pPSBPassThrough = compileShader(g_psPassThrough, "ps_5_0");
    ID3DBlob *pPSBAmbient = compileShader(g_psAmbient, "ps_5_0");
    ID3DBlob *pPSBOmni = compileShader(g_psOmni, "ps_5_0");
    ID3DBlob *pPSLDR = compileShader(g_psLDR, "ps_5_0");
    ID3DBlob *pPSBlurH = compileShader(g_psBlurH, "ps_5_0");
    ID3DBlob *pPSBlurV = compileShader(g_psBlurV, "ps_5_0");
    ID3DBlob *pPSToneMap = compileShader(g_psToneMap, "ps_5_0");
    result = pBoundDevice->pDevice->lpVtbl->CreateVertexShader(pBoundDevice->pDevice, pVSB->lpVtbl->GetBufferPointer(pVSB), pVSB->lpVtbl->GetBufferSize(pVSB), NULL, &pBoundDevice->pVS);
    if (result != S_OK)
    {
        setError("Failed CreateVertexShader");
        egDestroyDevice(&ret);
        return 0;
    }
    result = pBoundDevice->pDevice->lpVtbl->CreatePixelShader(pBoundDevice->pDevice, pPSB->lpVtbl->GetBufferPointer(pPSB), pPSB->lpVtbl->GetBufferSize(pPSB), NULL, &pBoundDevice->pPS);
    if (result != S_OK)
    {
        setError("Failed CreatePixelShader");
        egDestroyDevice(&ret);
        return 0;
    }
    pBoundDevice->pActivePS = pBoundDevice->pPS;
    for (int i = 0; i < 8; ++i)
    {
        ID3DBlob *pPSBAlphaTest = compileShader(g_psAlphaTest[i], "ps_5_0");
        result = pBoundDevice->pDevice->lpVtbl->CreatePixelShader(pBoundDevice->pDevice, pPSBAlphaTest->lpVtbl->GetBufferPointer(pPSBAlphaTest), pPSBAlphaTest->lpVtbl->GetBufferSize(pPSBAlphaTest), NULL, &(pBoundDevice->pPSAlphaTest[i]));
        if (result != S_OK)
        {
            setError("Failed CreatePixelShader");
            egDestroyDevice(&ret);
            return 0;
        }
    }
    result = pBoundDevice->pDevice->lpVtbl->CreateVertexShader(pBoundDevice->pDevice, pVSBPassThrough->lpVtbl->GetBufferPointer(pVSBPassThrough), pVSBPassThrough->lpVtbl->GetBufferSize(pVSBPassThrough), NULL, &pBoundDevice->pVSPassThrough);
    if (result != S_OK)
    {
        setError("Failed CreateVertexShader PassThrough");
        egDestroyDevice(&ret);
        return 0;
    }
    result = pBoundDevice->pDevice->lpVtbl->CreatePixelShader(pBoundDevice->pDevice, pPSBPassThrough->lpVtbl->GetBufferPointer(pPSBPassThrough), pPSBPassThrough->lpVtbl->GetBufferSize(pPSBPassThrough), NULL, &pBoundDevice->pPSPassThrough);
    if (result != S_OK)
    {
        setError("Failed CreatePixelShader PassThrough");
        egDestroyDevice(&ret);
        return 0;
    }
    result = pBoundDevice->pDevice->lpVtbl->CreatePixelShader(pBoundDevice->pDevice, pPSLDR->lpVtbl->GetBufferPointer(pPSLDR), pPSLDR->lpVtbl->GetBufferSize(pPSLDR), NULL, &pBoundDevice->pPSLDR);
    if (result != S_OK)
    {
        setError("Failed CreatePixelShader LDR");
        egDestroyDevice(&ret);
        return 0;
    }
    result = pBoundDevice->pDevice->lpVtbl->CreatePixelShader(pBoundDevice->pDevice, pPSBAmbient->lpVtbl->GetBufferPointer(pPSBAmbient), pPSBAmbient->lpVtbl->GetBufferSize(pPSBAmbient), NULL, &pBoundDevice->pPSAmbient);
    if (result != S_OK)
    {
        setError("Failed CreatePixelShader Ambient");
        egDestroyDevice(&ret);
        return 0;
    }
    result = pBoundDevice->pDevice->lpVtbl->CreatePixelShader(pBoundDevice->pDevice, pPSBOmni->lpVtbl->GetBufferPointer(pPSBOmni), pPSBOmni->lpVtbl->GetBufferSize(pPSBOmni), NULL, &pBoundDevice->pPSOmni);
    if (result != S_OK)
    {
        setError("Failed CreatePixelShader Ambient");
        egDestroyDevice(&ret);
        return 0;
    }
    result = pBoundDevice->pDevice->lpVtbl->CreatePixelShader(pBoundDevice->pDevice, pPSBlurH->lpVtbl->GetBufferPointer(pPSBlurH), pPSBlurH->lpVtbl->GetBufferSize(pPSBlurH), NULL, &pBoundDevice->pPSBlurH);
    if (result != S_OK)
    {
        setError("Failed CreatePixelShader BlurH");
        egDestroyDevice(&ret);
        return 0;
    }
    result = pBoundDevice->pDevice->lpVtbl->CreatePixelShader(pBoundDevice->pDevice, pPSBlurV->lpVtbl->GetBufferPointer(pPSBlurV), pPSBlurV->lpVtbl->GetBufferSize(pPSBlurV), NULL, &pBoundDevice->pPSBlurV);
    if (result != S_OK)
    {
        setError("Failed CreatePixelShader BlurV");
        egDestroyDevice(&ret);
        return 0;
    }
    result = pBoundDevice->pDevice->lpVtbl->CreatePixelShader(pBoundDevice->pDevice, pPSToneMap->lpVtbl->GetBufferPointer(pPSToneMap), pPSToneMap->lpVtbl->GetBufferSize(pPSToneMap), NULL, &pBoundDevice->pPSToneMap);
    if (result != S_OK)
    {
        setError("Failed CreatePixelShader ToneMap");
        egDestroyDevice(&ret);
        return 0;
    }

    // Input layout
    {
        D3D11_INPUT_ELEMENT_DESC layout[6] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 56, D3D11_INPUT_PER_VERTEX_DATA, 0}
        };
        result = pBoundDevice->pDevice->lpVtbl->CreateInputLayout(pBoundDevice->pDevice, layout, 6, pVSB->lpVtbl->GetBufferPointer(pVSB), pVSB->lpVtbl->GetBufferSize(pVSB), &pBoundDevice->pInputLayout);
        if (result != S_OK)
        {
            setError("Failed CreateInputLayout");
            egDestroyDevice(&ret);
            return 0;
        }
    }
    {
        D3D11_INPUT_ELEMENT_DESC layout[3] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 56, D3D11_INPUT_PER_VERTEX_DATA, 0}
        };
        result = pBoundDevice->pDevice->lpVtbl->CreateInputLayout(pBoundDevice->pDevice, layout, 3, pVSBPassThrough->lpVtbl->GetBufferPointer(pVSBPassThrough), pVSBPassThrough->lpVtbl->GetBufferSize(pVSBPassThrough), &pBoundDevice->pInputLayoutPassThrough);
        if (result != S_OK)
        {
            setError("Failed CreateInputLayout PassThrough");
            egDestroyDevice(&ret);
            return 0;
        }
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
        float initialRef[4] = {.5f, 0, 0, 0};
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
    vertexBufferDesc.ByteWidth = MAX_VERTEX_COUNT * sizeof(SEGVertex);
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    vertexBufferDesc.MiscFlags = 0;
    vertexBufferDesc.StructureByteStride = 0;
    result = pBoundDevice->pDevice->lpVtbl->CreateBuffer(pBoundDevice->pDevice, &vertexBufferDesc, NULL, &pBoundDevice->pVertexBuffer);
    if (result != S_OK)
    {
        setError("Failed CreateBuffer VertexBuffer");
        egDestroyDevice(&ret);
        return 0;
    }
    result = pBoundDevice->pVertexBuffer->lpVtbl->QueryInterface(pBoundDevice->pVertexBuffer, &IID_ID3D11Resource, &pBoundDevice->pVertexBufferRes);
    if (result != S_OK)
    {
        setError("Failed VertexBuffer ID3D11Buffer QueryInterface -> IID_ID3D11Resource");
        egDestroyDevice(&ret);
        return 0;
    }

    // Create a white texture for default rendering
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

    // Create bloom buffers
    result = createRenderTarget(&pBoundDevice->bloomBuffer,
                                pBoundDevice->backBufferDesc.Width, pBoundDevice->backBufferDesc.Height,
                                DXGI_FORMAT_R8G8B8A8_UNORM);
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
            result = createRenderTarget(&pBoundDevice->blurBuffers[i][k],
                                        pBoundDevice->backBufferDesc.Width / divider, pBoundDevice->backBufferDesc.Height / divider,
                                        DXGI_FORMAT_R8G8B8A8_UNORM);
            if (result != S_OK)
            {
                egDestroyDevice(&ret);
                return 0;
            }
        }
    }

    resetState();
    return ret;
}

void egDestroyDevice(EGDevice *pDeviceID)
{
    SEGDevice *pDevice = devices + (*pDeviceID - 1);
    if (pDevice->bIsInBatch) return;

    for (uint32_t i = 0; i < pDevice->textureCount; ++i)
    {
        SEGTexture2D *pTexture = pDevice->textures + i;
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

    for (uint32_t i = 0; i < 3; ++i)
    {
        SEGTexture2D *pTexture = pDevice->pDefaultTextureMaps + i;
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

    for (uint32_t i = 0; i < 4; ++i)
    {
        SEGRenderTarget2D *pRenderTarget = pDevice->gBuffer + i;
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

    for (uint32_t i = 0; i < 8; ++i)
    {
        for (uint32_t k = 0; k < 2; ++k)
        {
            SEGRenderTarget2D *pRenderTarget = &pDevice->blurBuffers[i][k];
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
    }

    {
        SEGRenderTarget2D *pRenderTarget = &pDevice->bloomBuffer;
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

    if (pBoundDevice->accumulationBuffer.pRenderTargetView)
        pBoundDevice->accumulationBuffer.pRenderTargetView->lpVtbl->Release(pBoundDevice->accumulationBuffer.pRenderTargetView);
    if (pBoundDevice->accumulationBuffer.texture.pResourceView)
        pBoundDevice->accumulationBuffer.texture.pResourceView->lpVtbl->Release(pBoundDevice->accumulationBuffer.texture.pResourceView);
    if (pBoundDevice->accumulationBuffer.texture.pTexture)
        pBoundDevice->accumulationBuffer.texture.pTexture->lpVtbl->Release(pBoundDevice->accumulationBuffer.texture.pTexture);

    if (pDevice->pVertexBufferRes) pDevice->pVertexBufferRes->lpVtbl->Release(pDevice->pVertexBufferRes);
    if (pDevice->pVertexBuffer) pDevice->pVertexBuffer->lpVtbl->Release(pDevice->pVertexBuffer);

    if (pDevice->pCBModel) pDevice->pCBModel->lpVtbl->Release(pDevice->pCBModel);
    if (pDevice->pCBViewProj) pDevice->pCBViewProj->lpVtbl->Release(pDevice->pCBViewProj);
    if (pDevice->pCBInvViewProj) pDevice->pCBInvViewProj->lpVtbl->Release(pDevice->pCBInvViewProj);
    if (pDevice->pCBOmni) pDevice->pCBOmni->lpVtbl->Release(pDevice->pCBOmni);

    if (pDevice->pInputLayout) pDevice->pInputLayout->lpVtbl->Release(pDevice->pInputLayout);
    if (pDevice->pPS) pDevice->pPS->lpVtbl->Release(pDevice->pPS);
    if (pDevice->pVS) pDevice->pVS->lpVtbl->Release(pDevice->pVS);
    if (pDevice->pInputLayoutPassThrough) pDevice->pInputLayoutPassThrough->lpVtbl->Release(pDevice->pInputLayoutPassThrough);
    if (pDevice->pPSPassThrough) pDevice->pPSPassThrough->lpVtbl->Release(pDevice->pPSPassThrough);
    if (pDevice->pVSPassThrough) pDevice->pVSPassThrough->lpVtbl->Release(pDevice->pVSPassThrough);
    if (pDevice->pPSAmbient) pDevice->pPSAmbient->lpVtbl->Release(pDevice->pPSAmbient);
    if (pDevice->pPSOmni) pDevice->pPSOmni->lpVtbl->Release(pDevice->pPSOmni);

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
