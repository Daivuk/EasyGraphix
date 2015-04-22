#include "eg_texture.h"

typedef struct
{
    SEGTexture2D                texture;
    ID3D11RenderTargetView     *pRenderTargetView;
} SEGRenderTarget2D;
