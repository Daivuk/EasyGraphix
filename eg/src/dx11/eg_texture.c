#include <inttypes.h>
#include "eg_error.h"
#include "eg_device.h"
#include "eg_rt.h"

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

uint8_t colorFromDataFormat(EGFormat dataFormat, const void *pData, uint32_t pos)
{
    if (dataFormat & EG_U8)
    {
        return *((uint8_t *)pData + pos);
    }
    else if (dataFormat & EG_U16)
    {
        return (uint8_t)((*((uint16_t *)pData + pos) >> 8) & 0xffffffff);
    }
    else if (dataFormat & EG_U32)
    {
        return (uint8_t)((*((uint32_t *)pData + pos) >> 24) & 0xffffffff);
    }
    else if (dataFormat & EG_U64)
    {
        return (uint8_t)((*((uint64_t *)pData + pos) >> 56) & 0xffffffff);
    }
    else if (dataFormat & EG_S8)
    {
        return (uint8_t)(*((int8_t *)pData + pos)) + INT8_MAX + 1;
    }
    else if (dataFormat & EG_S16)
    {
        return (uint8_t)((((uint16_t)(*((int16_t *)pData + pos)) + INT16_MAX + 1) >> 8) & 0xffffffff);
    }
    else if (dataFormat & EG_S32)
    {
        return (uint8_t)((((uint32_t)(*((int32_t *)pData + pos)) + INT32_MAX + 1) >> 24) & 0xffffffff);
    }
    else if (dataFormat & EG_S64)
    {
        return (uint8_t)((((uint64_t)(*((int64_t *)pData + pos)) + INT64_MAX + 1) >> 56) & 0xffffffff);
    }
    else if (dataFormat & EG_F32)
    {
        return (uint8_t)((*((float *)pData + pos)) * 255.f);
    }
    else if (dataFormat & EG_F64)
    {
        return (uint8_t)((*((double *)pData + pos)) * 255.0);
    }
    return 255;
}

EGTexture egCreateTexture1D(uint32_t dimension, const void *pData, EGFormat dataFormat)
{
    return 0;
}

EGTexture egCreateTexture2D(uint32_t width, uint32_t height, const void *pData, EGFormat dataFormat, EG_TEXTURE_FLAGS flags)
{
    if (!pBoundDevice) return 0;
    if (width == 0 || height == 0) return 0;

    SEGTexture2D texture2D = {0};

    texture2D.w = width;
    texture2D.h = height;

    if (flags & EG_RENDER_TARGET)
    {
        createTextureRenderTarget(&texture2D, width, height, DXGI_FORMAT_R8G8B8A8_UNORM);
    }
    else
    {
        uint8_t *pConvertedData = NULL;
        if (dataFormat == (EG_U8 | EG_RGBA))
        {
            pConvertedData = (uint8_t *)pData;
        }
        else
        {
            uint32_t size = width * height;
            pConvertedData = (uint8_t *)malloc(size * 4);
            for (uint32_t i = 0; i < size; ++i)
            {
                if (dataFormat & EG_R)
                {
                    pConvertedData[i * 4 + 0] = colorFromDataFormat(dataFormat, pData, i);
                    pConvertedData[i * 4 + 1] = 0;
                    pConvertedData[i * 4 + 2] = 0;
                    pConvertedData[i * 4 + 3] = 255;
                }
                else if (dataFormat & EG_RG)
                {
                    pConvertedData[i * 4 + 0] = colorFromDataFormat(dataFormat, pData, i * 2 + 0);
                    pConvertedData[i * 4 + 1] = colorFromDataFormat(dataFormat, pData, i * 2 + 1);
                    pConvertedData[i * 4 + 2] = 0;
                    pConvertedData[i * 4 + 3] = 255;
                }
                else if (dataFormat & EG_RGB)
                {
                    pConvertedData[i * 4 + 0] = colorFromDataFormat(dataFormat, pData, i * 3 + 0);
                    pConvertedData[i * 4 + 1] = colorFromDataFormat(dataFormat, pData, i * 3 + 1);
                    pConvertedData[i * 4 + 2] = colorFromDataFormat(dataFormat, pData, i * 3 + 2);
                    pConvertedData[i * 4 + 3] = 255;
                }
                else if (dataFormat & EG_RGBA)
                {
                    pConvertedData[i * 4 + 0] = colorFromDataFormat(dataFormat, pData, i * 4 + 0);
                    pConvertedData[i * 4 + 1] = colorFromDataFormat(dataFormat, pData, i * 4 + 1);
                    pConvertedData[i * 4 + 2] = colorFromDataFormat(dataFormat, pData, i * 4 + 2);
                    pConvertedData[i * 4 + 3] = colorFromDataFormat(dataFormat, pData, i * 4 + 3);
                }
            }
        }

        texture2DFromData(&texture2D, pConvertedData, width, height, (flags & EG_GENERATE_MIPMAPS) ? TRUE : FALSE);

        if (dataFormat != (EG_U8 | EG_RGBA))
        {
            free(pConvertedData);
        }
    }
    if (!texture2D.pTexture) return 0;

    return createTexture(&texture2D);
}

EGTexture egCreateTexture3D(uint32_t width, uint32_t height, uint32_t depth, const void *pData, EGFormat dataFormat)
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
