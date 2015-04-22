#include <d3d11.h>
#include "eg.h"

typedef struct
{
    ID3D11Texture2D            *pTexture;
    ID3D11ShaderResourceView   *pResourceView;
} SEGTexture2D;

EGTexture createTexture(SEGTexture2D *pTexture);
