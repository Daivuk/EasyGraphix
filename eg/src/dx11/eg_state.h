#pragma once

#ifndef EG_STATE_H_INCLUDED
#define EG_STATE_H_INCLUDED

#include <d3d11.h>

typedef enum
{
    STATE_NONE          = 0x00000000,
    STATE_DEPTH         = 0x00000001,
    STATE_RASTERIZER    = 0x00000002,
    STATE_BLEND         = 0x00000004,
    STATE_SAMPLER       = 0x00000008,
    STATE_ALPHA_TEST    = 0x00000010,
    STATE_LIGHTING      = 0x00000020,
    STATE_VIGNETTE      = 0x00000040,
    STATE_ALL           = 0xffffffff
} STATE_BITS;

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
    float                       exponent;
} SEGVignetteState;

typedef struct
{
    // Current enable bits
    EGEnable                    enableBits;

    // Dirty flags
    uint32_t                    dirtyBits;

    // Bypass certain states
    uint32_t                    ignoreBits;

    // States
    SEGDepthState               depthState;
    SEGRasterizerState          rasterizerState;
    SEGBlendState               blendState;
    SEGSamplerState             samplerState;
    SEGAlphaTestState           alphaTestState;
    SEGBlurState                blurState;
    SEGVignetteState            vignetteState;

} SEGState;

void resetState();
void updateState();
void updateBlendState(SEGState *pState);
void applyStaticState(SEGState *pState);

#endif /* EG_STATE_H_INCLUDED */
