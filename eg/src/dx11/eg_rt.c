#include "eg_device.h"
#include "eg_error.h"

HRESULT createRenderTarget(SEGRenderTarget2D *pRenderTarget, UINT w, UINT h, DXGI_FORMAT format)
{
    memset(pRenderTarget, 0, sizeof(SEGRenderTarget2D));
    D3D11_TEXTURE2D_DESC textureDesc = {0};
    HRESULT result;
    D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = {0};
    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {0};

    if (w == 0 || h == 0) return S_OK;

    pRenderTarget->texture.w = (uint32_t)w;
    pRenderTarget->texture.h = (uint32_t)h;

    // Setup the render target texture description.
    textureDesc.Width = w;
    textureDesc.Height = h;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = format;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    // Create the render target texture.
    result = pBoundDevice->pDevice->lpVtbl->CreateTexture2D(pBoundDevice->pDevice, &textureDesc, NULL, &pRenderTarget->texture.pTexture);
    if (result != S_OK)
    {
        setError("Failed CreateTexture2D");
        return result;
    }

    // Setup the description of the render target view.
    renderTargetViewDesc.Format = textureDesc.Format;
    renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    renderTargetViewDesc.Texture2D.MipSlice = 0;

    // Create the render target view.
    ID3D11Resource *pTextureRes = NULL;
    result = pRenderTarget->texture.pTexture->lpVtbl->QueryInterface(pRenderTarget->texture.pTexture, &IID_ID3D11Resource, &pTextureRes);
    if (result != S_OK)
    {
        pRenderTarget->texture.pTexture->lpVtbl->Release(pRenderTarget->texture.pTexture);
        pRenderTarget->texture.pTexture = NULL;
        setError("Failed QueryInterface ID3D11Texture2D -> IID_ID3D11Resource");
        return result;
    }
    result = pBoundDevice->pDevice->lpVtbl->CreateRenderTargetView(pBoundDevice->pDevice, pTextureRes, &renderTargetViewDesc, &pRenderTarget->pRenderTargetView);
    if (result != S_OK)
    {
        pRenderTarget->texture.pTexture->lpVtbl->Release(pRenderTarget->texture.pTexture);
        pRenderTarget->texture.pTexture = NULL;
        pTextureRes->lpVtbl->Release(pTextureRes);
        setError("Failed CreateRenderTargetView");
        return result;
    }

    // Setup the description of the shader resource view.
    shaderResourceViewDesc.Format = textureDesc.Format;
    shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
    shaderResourceViewDesc.Texture2D.MipLevels = 1;

    // Create the shader resource view.
    result = pBoundDevice->pDevice->lpVtbl->CreateShaderResourceView(pBoundDevice->pDevice, pTextureRes, &shaderResourceViewDesc, &pRenderTarget->texture.pResourceView);
    if (result != S_OK)
    {
        pRenderTarget->texture.pTexture->lpVtbl->Release(pRenderTarget->texture.pTexture);
        pRenderTarget->texture.pTexture = NULL;
        pRenderTarget->pRenderTargetView->lpVtbl->Release(pRenderTarget->pRenderTargetView);
        pRenderTarget->pRenderTargetView = NULL;
        pTextureRes->lpVtbl->Release(pTextureRes);
        setError("Failed CreateShaderResourceView");
        return result;
    }
    pTextureRes->lpVtbl->Release(pTextureRes);

    return S_OK;
}

HRESULT createTextureRenderTarget(SEGTexture2D *pRenderTarget, UINT w, UINT h, DXGI_FORMAT format)
{
    memset(pRenderTarget, 0, sizeof(SEGTexture2D));
    D3D11_TEXTURE2D_DESC textureDesc = {0};
    HRESULT result;
    D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = {0};
    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {0};

    if (w == 0 || h == 0) return S_OK;

    pRenderTarget->w = (uint32_t)w;
    pRenderTarget->h = (uint32_t)h;

    // Setup the render target texture description.
    textureDesc.Width = w;
    textureDesc.Height = h;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = format;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    // Create the render target texture.
    result = pBoundDevice->pDevice->lpVtbl->CreateTexture2D(pBoundDevice->pDevice, &textureDesc, NULL, &pRenderTarget->pTexture);
    if (result != S_OK)
    {
        setError("Failed CreateTexture2D");
        return result;
    }

    // Setup the description of the render target view.
    renderTargetViewDesc.Format = textureDesc.Format;
    renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    renderTargetViewDesc.Texture2D.MipSlice = 0;

    // Create the render target view.
    ID3D11Resource *pTextureRes = NULL;
    result = pRenderTarget->pTexture->lpVtbl->QueryInterface(pRenderTarget->pTexture, &IID_ID3D11Resource, &pTextureRes);
    if (result != S_OK)
    {
        pRenderTarget->pTexture->lpVtbl->Release(pRenderTarget->pTexture);
        pRenderTarget->pTexture = NULL;
        setError("Failed QueryInterface ID3D11Texture2D -> IID_ID3D11Resource");
        return result;
    }
    result = pBoundDevice->pDevice->lpVtbl->CreateRenderTargetView(pBoundDevice->pDevice, pTextureRes, &renderTargetViewDesc, &pRenderTarget->pRenderTargetView);
    if (result != S_OK)
    {
        pRenderTarget->pTexture->lpVtbl->Release(pRenderTarget->pTexture);
        pRenderTarget->pTexture = NULL;
        pTextureRes->lpVtbl->Release(pTextureRes);
        setError("Failed CreateRenderTargetView");
        return result;
    }

    // Setup the description of the shader resource view.
    shaderResourceViewDesc.Format = textureDesc.Format;
    shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
    shaderResourceViewDesc.Texture2D.MipLevels = 1;

    // Create the shader resource view.
    result = pBoundDevice->pDevice->lpVtbl->CreateShaderResourceView(pBoundDevice->pDevice, pTextureRes, &shaderResourceViewDesc, &pRenderTarget->pResourceView);
    if (result != S_OK)
    {
        pRenderTarget->pTexture->lpVtbl->Release(pRenderTarget->pTexture);
        pRenderTarget->pTexture = NULL;
        pRenderTarget->pRenderTargetView->lpVtbl->Release(pRenderTarget->pRenderTargetView);
        pRenderTarget->pRenderTargetView = NULL;
        pTextureRes->lpVtbl->Release(pTextureRes);
        setError("Failed CreateShaderResourceView");
        return result;
    }
    pTextureRes->lpVtbl->Release(pTextureRes);

    return S_OK;
}
