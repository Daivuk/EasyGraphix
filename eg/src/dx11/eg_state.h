#include <d3d11.h>

typedef struct
{
    D3D11_DEPTH_STENCIL_DESC    depth;
    BOOL                        depthDirty;
    D3D11_RASTERIZER_DESC       rasterizer;
    BOOL                        rasterizerDirty;
    D3D11_BLEND_DESC            blend;
    BOOL                        blendDirty;
    D3D11_SAMPLER_DESC          sampler;
    BOOL                        samplerDirty;
} SEGState;
