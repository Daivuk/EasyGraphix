#include <inttypes.h>
#include "eg_error.h"
#include "eg_device.h"

void texture2DFromData(SEGTexture2D *pOut, const uint8_t* pData, UINT w, UINT h, BOOL bGenerateMipMaps)
{
    HRESULT result;

    // Manually generate mip levels
    BOOL allowMipMaps = TRUE;
    UINT w2 = 1;
    UINT h2 = 1;
    while (w2 < (UINT)w) w2 *= 2;
    if (w != w2) allowMipMaps = FALSE;
    while (h2 < (UINT)h) h2 *= 2;
    if (h != h2) allowMipMaps = FALSE;
    uint8_t *pMipMaps = NULL;
    int mipLevels = 1;
    D3D11_SUBRESOURCE_DATA *mipsData = NULL;
    allowMipMaps = allowMipMaps & bGenerateMipMaps;
    if (allowMipMaps)
    {
        UINT biggest = max(w2, h2);
        UINT w2t = w2;
        UINT h2t = h2;
        UINT totalSize = w2t * h2t * 4;
        while (!(w2t == 1 && h2t == 1))
        {
            ++mipLevels;
            w2t /= 2;
            if (w2t < 1) w2t = 1;
            h2t /= 2;
            if (h2t < 1) h2t = 1;
            totalSize += w2t * h2t * 4;
        }
        pMipMaps = (uint8_t *)malloc(totalSize);
        memcpy(pMipMaps, pData, w * h * 4);

        mipsData = (D3D11_SUBRESOURCE_DATA *)malloc(sizeof(D3D11_SUBRESOURCE_DATA) * mipLevels);

        w2t = w2;
        h2t = h2;
        totalSize = 0;
        int mipTarget = mipLevels;
        mipLevels = 0;
        byte* prev;
        byte* cur;
        while (mipLevels != mipTarget)
        {
            prev = pMipMaps + totalSize;
            mipsData[mipLevels].pSysMem = prev;
            mipsData[mipLevels].SysMemPitch = w2t * 4;
            mipsData[mipLevels].SysMemSlicePitch = 0;
            totalSize += w2t * h2t * 4;
            cur = pMipMaps + totalSize;
            w2t /= 2;
            if (w2t < 1) w2t = 1;
            h2t /= 2;
            if (h2t < 1) h2t = 1;
            ++mipLevels;
            if (mipLevels == mipTarget) break;
            int accum;

            // Generate the mips
            int multX = w2 / w2t;
            int multY = h2 / h2t;
            for (UINT y = 0; y < h2t; ++y)
            {
                for (UINT x = 0; x < w2t; ++x)
                {
                    for (UINT k = 0; k < 4; ++k)
                    {
                        accum = 0;
                        accum += prev[(y * multY * w2 + x * multX) * 4 + k];
                        accum += prev[(y * multY * w2 + (x + multX / 2) * multX) * 4 + k];
                        accum += prev[((y + multY / 2) * multY * w2 + x * multX) * 4 + k];
                        accum += prev[((y + multY / 2) * multY * w2 + (x + multX / 2) * multX) * 4 + k];
                        cur[(y * w2t + x) * 4 + k] = accum / 4;
                    }
                }
            }

            w2 = w2t;
            h2 = h2t;
        }
    }

    D3D11_TEXTURE2D_DESC desc;
    desc.Width = w;
    desc.Height = h;
    desc.MipLevels = mipLevels;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA data;
    data.pSysMem = (pMipMaps) ? pMipMaps : pData;
    data.SysMemPitch = w * 4;
    data.SysMemSlicePitch = 0;

    result = pBoundDevice->pDevice->lpVtbl->CreateTexture2D(pBoundDevice->pDevice, &desc, (mipsData) ? mipsData : &data, &pOut->pTexture);
    if (result != S_OK)
    {
        setError("Failed CreateTexture2D");
        return;
    }
    ID3D11Resource *pResource = NULL;
    result = pOut->pTexture->lpVtbl->QueryInterface(pOut->pTexture, &IID_ID3D11Resource, &pResource);
    if (result != S_OK)
    {
        pOut->pTexture->lpVtbl->Release(pOut->pTexture);
        pOut->pTexture = NULL;
        setError("Failed QueryInterface ID3D11Texture2D -> IID_ID3D11Resource");
        return;
    }
    result = pBoundDevice->pDevice->lpVtbl->CreateShaderResourceView(pBoundDevice->pDevice, pResource, NULL, &pOut->pResourceView);
    if (result != S_OK)
    {
        pOut->pTexture->lpVtbl->Release(pOut->pTexture);
        pOut->pTexture = NULL;
        pResource->lpVtbl->Release(pResource);
        setError("Failed CreateShaderResourceView");
        return;
    }
    pResource->lpVtbl->Release(pResource);

    if (pMipMaps) free(pMipMaps);
    if (mipsData) free(mipsData);
}

EGTexture createTexture(SEGTexture2D *pTexture)
{
    pBoundDevice->textures = realloc(pBoundDevice->textures, sizeof(SEGTexture2D) * (pBoundDevice->textureCount + 1));
    memcpy(pBoundDevice->textures + pBoundDevice->textureCount, pTexture, sizeof(SEGTexture2D));
    return ++pBoundDevice->textureCount;
}

EGTexture egCreateTexture1D(uint32_t dimension, const void *pData, EGFormat dataFormat)
{
    return 0;
}

EGTexture egCreateTexture2D(uint32_t width, uint32_t height, const void *pData, uint32_t dataType, EG_TEXTURE_FLAGS flags)
{
    if (!pBoundDevice) return 0;
    SEGTexture2D texture2D = {0};
    uint8_t *pConvertedData = (uint8_t *)pData;
    texture2DFromData(&texture2D, pConvertedData, width, height, flags & EG_GENERATE_MIPMAPS ? TRUE : FALSE);
    if (!texture2D.pTexture) return 0;
    return createTexture(&texture2D);
}

EGTexture egCreateTexture3D(uint32_t width, uint32_t height, uint32_t depth, const void *pData, uint32_t dataType)
{
    return 0;
}

EGTexture egCreateCubeMap(uint32_t dimension, const void *pData, EGFormat dataFormat, EG_TEXTURE_FLAGS flags)
{
    return 0;
}

void egDestroyTexture(EGTexture *pTexture)
{
}

