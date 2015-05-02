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
    // Devices
    IDXGISwapChain             *pSwapChain;
    ID3D11Device               *pDevice;
    ID3D11DeviceContext        *pDeviceContext;

    // Render targets
    ID3D11RenderTargetView     *pRenderTargetView;
    ID3D11DepthStencilView     *pDepthStencilView;
    D3D11_TEXTURE2D_DESC        backBufferDesc;
    SEGRenderTarget2D           gBuffer[4];
    SEGRenderTarget2D           accumulationBuffer;
    SEGRenderTarget2D           bloomBuffer;
    SEGRenderTarget2D           blurBuffers[8][2];

    // Shaders
    ID3D11VertexShader         *pVS;
    ID3D11PixelShader          *pPSes[18];
    ID3D11PixelShader          *pActivePS;
    ID3D11InputLayout          *pInputLayout;
    ID3D11VertexShader         *pVSPassThrough;
    ID3D11PixelShader          *pPSPassThrough;
    ID3D11InputLayout          *pInputLayoutPassThrough;
    ID3D11PixelShader          *pPSAmbient;
    ID3D11PixelShader          *pPSOmni;
    ID3D11PixelShader          *pPSLDR;
    ID3D11PixelShader          *pPSBlurH;
    ID3D11PixelShader          *pPSBlurV;
    ID3D11PixelShader          *pPSToneMap;

    // Constant buffers
    ID3D11Buffer               *pCBViewProj;
    ID3D11Buffer               *pCBModel;
    ID3D11Buffer               *pCBInvViewProj;
    ID3D11Buffer               *pCBAlphaTestRef;
    ID3D11Buffer               *pCBOmni;
    ID3D11Buffer               *pCBBlurSpread;

    // Batch's dynamic vertex buffer
    ID3D11Buffer               *pVertexBuffer;
    ID3D11Resource             *pVertexBufferRes;

    // Matrices
    SEGMatrix                   projectionMatrix;
    SEGMatrix                   viewMatrix;
    SEGMatrix                   viewProjMatrix;
    SEGMatrix                   invViewProjMatrix;
    SEGMatrix                   worldMatrices[MAX_STACK];
    uint32_t                    worldMatricesStackCount;

    // Textures
    SEGTexture2D               *textures;
    uint32_t                    textureCount;
    SEGTexture2D                pDefaultTextureMaps[3];
    SEGTexture2D                transparentBlackTexture;

    // States
    uint32_t                    viewPort[4];
    SEGState                    stateStack[MAX_STACK];
    uint32_t                    statesStackCount;
    float                       clearColor[4];
    EG_PASS                     pass;
    SEGState                   *states;
    uint32_t                    stateCount;
    EGState                     passStates[EG_PASS_COUNT];
    int                         postProcessCount;

    // Batching
    BOOL                        bIsInBatch;
    EG_MODE                     currentMode;
    SEGVertex                  *pVertex;
    uint32_t                    currentVertexCount;
    SEGVertex                   currentVertex;
    SEGOmni                     currentOmni;
} SEGDevice;

extern SEGDevice *pBoundDevice;

void updateViewProjCB();
void updateInvViewProjCB();
void updateModelCB();
void updateOmniCB();

#endif /* EG_DEVICE_H_INCLUDED*/
