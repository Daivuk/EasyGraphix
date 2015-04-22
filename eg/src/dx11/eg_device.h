#pragma once

#ifndef EG_DEVICE_H_INCLUDED
#define EG_DEVICE_H_INCLUDED

#include <d3d11.h>
#include <inttypes.h>
#include "eg_batch.h"
#include "eg_math.h"
#include "eg_pass.h"
#include "eg_rt.h"
#include "eg_state.h"

#define MAX_STACK       256

#define DIFFUSE_MAP     0
#define NORMAL_MAP      1
#define MATERIAL_MAP    2

typedef struct
{
    IDXGISwapChain             *pSwapChain;
    ID3D11Device               *pDevice;
    ID3D11DeviceContext        *pDeviceContext;
    ID3D11RenderTargetView     *pRenderTargetView;
    ID3D11DepthStencilView     *pDepthStencilView;
    D3D11_TEXTURE2D_DESC        backBufferDesc;
    ID3D11VertexShader         *pVS;
    ID3D11PixelShader          *pPS;
    ID3D11InputLayout          *pInputLayout;
    ID3D11VertexShader         *pVSPassThrough;
    ID3D11PixelShader          *pPSPassThrough;
    ID3D11InputLayout          *pInputLayoutPassThrough;
    ID3D11PixelShader          *pPSAmbient;
    ID3D11PixelShader          *pPSOmni;
    ID3D11Buffer               *pCBViewProj;
    ID3D11Buffer               *pCBModel;
    ID3D11Buffer               *pCBInvViewProj;
    ID3D11Buffer               *pVertexBuffer;
    ID3D11Resource             *pVertexBufferRes;
    SEGTexture2D               *textures;
    uint32_t                    textureCount;
    SEGTexture2D                pDefaultTextureMaps[3];
    uint32_t                    viewPort[4];
    SEGMatrix                   worldMatrices[MAX_STACK];
    uint32_t                    worldMatricesStackCount;
    SEGMatrix                  *pBoundWorldMatrix;
    SEGMatrix                   projectionMatrix;
    SEGMatrix                   viewMatrix;
    SEGMatrix                   viewProjMatrix;
    SEGMatrix                   invViewProjMatrix;
    SEGState                    states[MAX_STACK];
    uint32_t                    statesStackCount;
    float                       clearColor[4];
    SEGRenderTarget2D           gBuffer[4];
    SEGRenderTarget2D           accumulationBuffer;
    EG_PASS                     pass;
    ID3D11Buffer               *pCBOmni;
    BOOL                        bIsInBatch;
    EG_TOPOLOGY                 currentTopology;
    SEGVertex                  *pVertex;
    uint32_t                    currentVertexCount;
    SEGVertex                   currentVertex;
    SEGOmni                     currentOmni;
} SEGDevice;

extern SEGDevice *pBoundDevice;

void updateViewProjCB();
void updateInvViewProjCB();
void updateOmniCB();

#endif /* EG_DEVICE_H_INCLUDED*/
