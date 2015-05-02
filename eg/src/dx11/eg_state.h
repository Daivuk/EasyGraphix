#pragma once

#ifndef EG_STATE_H_INCLUDED
#define EG_STATE_H_INCLUDED

#include <d3d11.h>

typedef enum
{
    DIRTY_NONE          = 0x00000000,
    DIRTY_DEPTH         = 0x00000001,
    DIRTY_RASTERIZER    = 0x00000002,
    DIRTY_BLEND         = 0x00000004,
    DIRTY_SAMPLER       = 0x00000008,
    DIRTY_ALPHA_TEST    = 0x00000010,
    DIRTY_LIGHTING      = 0x00000020,
    DIRTY_ALL           = 0xffffffff
} DIRTY_BITS;

typedef struct
{
    D3D11_DEPTH_STENCIL_DESC    desc;
    ID3D11DepthStencilState    *pState;
} SEGDepthState;

typedef struct
{
    D3D11_RASTERIZER_DESC       desc;
    ID3D11RasterizerState      *pState;
} SEGRasterizerState;

typedef struct
{
    D3D11_BLEND_DESC            desc;
    ID3D11BlendState           *pState;
} SEGBlendState;

typedef struct
{
    D3D11_SAMPLER_DESC          desc;
    ID3D11SamplerState         *pState;
} SEGSamplerState;

typedef struct
{
    EG_COMPARE                  func;
    float                       ref;
    ID3D11Buffer               *pCB;
} SEGAlphaTestState;

typedef struct
{
    float                       spread;
} SEGBlurState;

typedef struct
{
    // Current enable bits
    EGEnable                    enableBits;

    // Dirty flags
    uint32_t                    dirtyBits;

    // States
    SEGDepthState               depthState;
    SEGRasterizerState          rasterizerState;
    SEGBlendState               blendState;
    SEGSamplerState             samplerState;
    SEGAlphaTestState           alphaTestState;
    SEGBlurState                blurState;

} SEGState;

void resetState();
void updateState();
void updateBlendState(SEGState *pState);
void applyStaticState(SEGState *pState);

#endif /* EG_STATE_H_INCLUDED */
