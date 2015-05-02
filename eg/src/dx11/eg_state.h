#pragma once

#ifndef EG_STATE_H_INCLUDED
#define EG_STATE_H_INCLUDED

#include <d3d11.h>

typedef struct
{
    // Depth stencil
    D3D11_DEPTH_STENCIL_DESC    depth;
    BOOL                        depthDirty;

    // Rasterizer
    D3D11_RASTERIZER_DESC       rasterizer;
    BOOL                        rasterizerDirty;

    // Blending
    D3D11_BLEND_DESC            blend;
    BOOL                        blendDirty;

    // Texture sampler
    D3D11_SAMPLER_DESC          sampler;
    BOOL                        samplerDirty;

    // Alpha test
    EG_COMPARE                  alphaTestFunc;
    float                       alphaTestRef[4];
    BOOL                        alphaTestDirty;

    // Blur
    float                       blurSpread;

    // Lighting
    BOOL                        lightingDirty;

    // Current enable bits
    EGEnable                    enableBits;
} SEGState;

void resetState();
void updateState();

#endif /* EG_STATE_H_INCLUDED */
