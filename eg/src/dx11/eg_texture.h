#pragma once

#ifndef EG_TEXTURE_H_INCLUDED
#define EG_TEXTURE_H_INCLUDED

#include <d3d11.h>
#include "eg.h"

typedef struct
{
    ID3D11Texture2D            *pTexture;
    ID3D11ShaderResourceView   *pResourceView;
    uint32_t                    w, h;
} SEGTexture2D;

EGTexture createTexture(SEGTexture2D *pTexture);
void texture2DFromData(SEGTexture2D *pOut, const uint8_t* pData, UINT w, UINT h, BOOL bGenerateMipMaps);

#endif /* EG_TEXTURE_H_INCLUDED */
