#pragma once

#ifndef EG_RT_H_INCLUDED
#define EG_RT_H_INCLUDED

#include "eg_texture.h"

typedef struct
{
    SEGTexture2D                texture;
    ID3D11RenderTargetView     *pRenderTargetView;
} SEGRenderTarget2D;

HRESULT createRenderTarget(SEGRenderTarget2D *pRenderTarget, UINT w, UINT h, DXGI_FORMAT format);

#endif /* EG_RT_H_INCLUDED */
