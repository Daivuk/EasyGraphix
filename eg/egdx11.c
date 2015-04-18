#include <assert.h>
#include <d3d11.h>
#include <D3Dcompiler.h>
#include <math.h>
#include "eg.h"

#define PI 3.1415926535897932384626433832795f
#define TO_RAD(__deg__) (__deg__ * PI / 180.f)
#define TO_DEG(__rad__) (__rad__ * 180.f / PI)

// Shaders
const char *g_vs =
"\
cbuffer ViewProjCB:register(b0)\n\
{\n\
    matrix viewProj;\n\
}\n\
cbuffer ModelVB:register(b1)\n\
{\n\
    matrix model;\n\
}\n\
struct sInput\n\
{\n\
    float3 position:POSITION;\n\
    float3 normal:NORMAL;\n\
    float2 texCoord:TEXCOORD;\n\
    float4 color:COLOR;\n\
};\n\
struct sOutput\n\
{\n\
    float4 position:SV_POSITION;\n\
    float3 normal:NORMAL;\n\
    float2 texCoord:TEXCOORD;\n\
    float4 color:COLOR;\n\
};\n\
sOutput main(sInput input)\n\
{\n\
    sOutput output;\n\
    float4 worldPosition = mul(float4(input.position, 1), model);\n\
    output.position = mul(worldPosition, viewProj);\n\
    output.normal = mul(float4(input.normal, 0), model).xyz;\n\
    output.texCoord = input.texCoord;\n\
    output.color = input.color;\n\
    return output;\n\
}";

const char *g_ps =
"\
Texture2D xDiffuse:register(t0);\n\
Texture2D xNormal:register(t1);\n\
Texture2D xMaterial:register(t2);\n\
SamplerState sSampler:register(s0);\n\
struct sInput\n\
{\n\
    float4 position:SV_POSITION;\n\
    float3 normal:NORMAL;\n\
    float2 texCoord:TEXCOORD;\n\
    float4 color:COLOR;\n\
};\n\
float4 main(sInput input):SV_TARGET\n\
{\n\
    float4 diffuse = xDiffuse.Sample(sSampler, input.texCoord);\n\
    return diffuse * input.color;\n\
}";

#define MAX_VERTEX_COUNT (300 * 2 * 3)
#define DIFFUSE_MAP     0
#define NORMAL_MAP      1
#define MATERIAL_MAP    2

// Batch stuff
typedef struct
{
    float x, y, z;
    float nx, ny, nz;
    float u, v;
    float r, g, b, a;
} SEGVertex;
SEGVertex *pVertex = NULL;
uint32_t currentVertexCount = 0;
BOOL bIsInBatch = FALSE;
SEGVertex currentVertex = {0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1};
EG_TOPOLOGY currentTopology;

// Textures
typedef struct
{
    ID3D11Texture2D            *pTexture;
    ID3D11ShaderResourceView   *pResourceView;
} SEGTexture2D;

// Devices
typedef struct
{
    IDXGISwapChain             *pSwapChain;
    ID3D11Device               *pDevice;
    ID3D11DeviceContext        *pDeviceContext;
    ID3D11RenderTargetView     *pRenderTargetView;
    D3D11_TEXTURE2D_DESC        backBufferDesc;
    ID3D11VertexShader         *pVS;
    ID3D11PixelShader          *pPS;
    ID3D11InputLayout          *pInputLayout;
    ID3D11Buffer               *pCBViewProj;
    ID3D11Buffer               *pCBModel;
    ID3D11Buffer               *pVertexBuffer;
    ID3D11Resource             *pVertexBufferRes;
    SEGTexture2D               *textures;
    uint32_t                    textureCount;
    SEGTexture2D                pDefaultTextureMaps[3];
} SEGDevice;
SEGDevice  *devices = NULL;
uint32_t    deviceCount = 0;
SEGDevice  *pBoundDevice = NULL;

// Viewport
uint32_t    viewPort[4];

// Clear states
float clearColor[4] = {0};

// Vector math
void v3normalize(float* v)
{
    float len = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    len = sqrtf(len);
    v[0] /= len;
    v[1] /= len;
    v[2] /= len;
}
void v3cross(float* v1, float* v2, float* out)
{
    out[0] = v1[1] * v2[2] - v1[2] * v2[1];
    out[1] = v1[2] * v2[0] - v1[0] * v2[2];
    out[2] = v1[0] * v2[1] - v1[1] * v2[0];
}
float v3dot(float* v1, float* v2)
{
    return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

// Matrices
typedef struct
{
    float m[16];
} SEGMatrix;
#define MAX_WORLD_STACK 1024
SEGMatrix  *worldMatrices = NULL;
uint32_t    worldMatricesStackCount = 0;
SEGMatrix  *pBoundWorldMatrix = NULL;
SEGMatrix   projectionMatrix;
SEGMatrix   viewMatrix;
SEGMatrix   viewProjMatrix;
void setIdentityMatrix(SEGMatrix *pMatrix)
{
    memset(pMatrix, 0, sizeof(SEGMatrix));
    pMatrix->m[0] = 1;
    pMatrix->m[5] = 1;
    pMatrix->m[10] = 1;
    pMatrix->m[15] = 1;
}
#define at(__row__, __col__) m[__row__ * 4 + __col__]
void multMatrix(SEGMatrix *pM1, SEGMatrix *pM2, SEGMatrix *pMOut)
{
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            float n = 0;
            for (int k = 0; k < 4; k++)
                n += pM2->at(i, k) * pM1->at(k, j);
            pMOut->at(i, j) = n;
        }
    }
}
void swapf(float *a, float *b)
{
    float temp;

    temp = *b;
    *b = *a;
    *a = temp;
}
void transposeMatrix(SEGMatrix *pMatrix)
{
    swapf(&pMatrix->at(0, 1), &pMatrix->at(1, 0));
    swapf(&pMatrix->at(0, 2), &pMatrix->at(2, 0));
    swapf(&pMatrix->at(0, 3), &pMatrix->at(3, 0));

    swapf(&pMatrix->at(1, 2), &pMatrix->at(2, 1));
    swapf(&pMatrix->at(1, 3), &pMatrix->at(3, 1));

    swapf(&pMatrix->at(2, 3), &pMatrix->at(3, 2));
}
void setLookAtMatrix(SEGMatrix *pMatrix,
                     float eyex, float eyey, float eyez,
                     float centerx, float centery, float centerz,
                     float upx, float upy, float upz)
{
    float forward[3], side[3], up[3];

    forward[0] = centerx - eyex;
    forward[1] = centery - eyey;
    forward[2] = centerz - eyez;

    up[0] = upx;
    up[1] = upy;
    up[2] = upz;

    v3normalize(forward);

    /* Side = forward x up */
    v3cross(forward, up, side);
    v3normalize(side);

    /* Recompute up as: up = side x forward */
    v3cross(side, forward, up);

    SEGMatrix matrix;
    setIdentityMatrix(&matrix);
    matrix.at(0, 0) = side[0];
    matrix.at(1, 0) = side[1];
    matrix.at(2, 0) = side[2];

    matrix.at(0, 1) = up[0];
    matrix.at(1, 1) = up[1];
    matrix.at(2, 1) = up[2];

    matrix.at(0, 2) = -forward[0];
    matrix.at(1, 2) = -forward[1];
    matrix.at(2, 2) = -forward[2];

    SEGMatrix translationMatrix;
    setIdentityMatrix(&translationMatrix);
    translationMatrix.m[12] = -eyex;
    translationMatrix.m[13] = -eyey;
    translationMatrix.m[14] = -eyez;
    multMatrix(&matrix, &translationMatrix, pMatrix);
    transposeMatrix(pMatrix);
}
void setProjectionMatrix(SEGMatrix *pMatrix, float fov, float aspect, float nearDist, float farDist)
{
    float yScale = 1.0f / tanf(fov / 2);
    float xScale = yScale / aspect;
    float nearmfar = nearDist - farDist;
    float m[] = {
        xScale, 0, 0, 0,
        0, yScale, 0, 0,
        0, 0, (farDist + nearDist) / nearmfar, -1,
        0, 0, 2 * farDist * nearDist / nearmfar, 0
    };
    memcpy(pMatrix->m, m, sizeof(double) * 16);
    transposeMatrix(pMatrix);
}

void resetStates()
{
    worldMatricesStackCount = 0;

    egSet2DViewProj(-999, 999);
    egViewPort(0, 0, (uint32_t)pBoundDevice->backBufferDesc.Width, (uint32_t)pBoundDevice->backBufferDesc.Height);
    egModelIdentity();

    pBoundDevice->pDeviceContext->lpVtbl->IASetInputLayout(pBoundDevice->pDeviceContext, pBoundDevice->pInputLayout);
    pBoundDevice->pDeviceContext->lpVtbl->VSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pVS, NULL, 0);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShader(pBoundDevice->pDeviceContext, pBoundDevice->pPS, NULL, 0);
    pBoundDevice->pDeviceContext->lpVtbl->OMSetRenderTargets(pBoundDevice->pDeviceContext, 1, &pBoundDevice->pRenderTargetView, NULL);

    pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->pDefaultTextureMaps[DIFFUSE_MAP].pResourceView);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 1, 1, &pBoundDevice->pDefaultTextureMaps[NORMAL_MAP].pResourceView);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 2, 1, &pBoundDevice->pDefaultTextureMaps[MATERIAL_MAP].pResourceView);
}

ID3DBlob* compileShader(const char *szSource, const char *szProfile)
{
    ID3DBlob *shaderBlob = NULL;
#ifdef _DEBUG
    ID3DBlob *errorBlob = NULL;
#endif

    HRESULT result = D3DCompile(szSource, (SIZE_T)strlen(szSource), NULL, NULL, NULL, "main", szProfile,
                            D3DCOMPILE_ENABLE_STRICTNESS 
#ifdef _DEBUG
                            | D3DCOMPILE_DEBUG
#endif
                            , 0, &shaderBlob, &errorBlob);

#ifdef _DEBUG
    if (errorBlob)
    {
        char *pError = (char*)errorBlob->lpVtbl->GetBufferPointer(errorBlob);
        assert(FALSE);
    }
#endif

    return shaderBlob;
}

void texture2DFromData(SEGTexture2D *pOut, const uint8_t* pData, UINT w, UINT h, BOOL bGenerateMipMaps)
{
    HRESULT result;

    // Manually generate mip levels
    BOOL allowMipMaps = TRUE;
    UINT w2 = 1;
    UINT h2 = 1;
    while (w2 < (UINT)w) w2 *= 2;
    if (w != w2) allowMipMaps = FALSE;
    while (h2 < (UINT)h) h2 *= 2;
    if (h != h2) allowMipMaps = FALSE;
    uint8_t *pMipMaps = NULL;
    int mipLevels = 1;
    D3D11_SUBRESOURCE_DATA *mipsData = NULL;
    allowMipMaps = allowMipMaps & bGenerateMipMaps;
    if (allowMipMaps)
    {
        UINT biggest = max(w2, h2);
        UINT w2t = w2;
        UINT h2t = h2;
        UINT totalSize = w2t * h2t * 4;
        while (!(w2t == 1 && h2t == 1))
        {
            ++mipLevels;
            w2t /= 2;
            if (w2t < 1) w2t = 1;
            h2t /= 2;
            if (h2t < 1) h2t = 1;
            totalSize += w2t * h2t * 4;
        }
        pMipMaps = (uint8_t *)malloc(totalSize);
        memcpy(pMipMaps, pData, w * h * 4);

        mipsData = (D3D11_SUBRESOURCE_DATA *)malloc(sizeof(D3D11_SUBRESOURCE_DATA) * mipLevels);

        w2t = w2;
        h2t = h2;
        totalSize = 0;
        int mipTarget = mipLevels;
        mipLevels = 0;
        byte* prev;
        byte* cur;
        while (mipLevels != mipTarget)
        {
            prev = pMipMaps + totalSize;
            mipsData[mipLevels].pSysMem = prev;
            mipsData[mipLevels].SysMemPitch = w2t * 4;
            mipsData[mipLevels].SysMemSlicePitch = 0;
            totalSize += w2t * h2t * 4;
            cur = pMipMaps + totalSize;
            w2t /= 2;
            if (w2t < 1) w2t = 1;
            h2t /= 2;
            if (h2t < 1) h2t = 1;
            ++mipLevels;
            if (mipLevels == mipTarget) break;
            int accum;

            // Generate the mips
            int multX = w2 / w2t;
            int multY = h2 / h2t;
            for (UINT y = 0; y < h2t; ++y)
            {
                for (UINT x = 0; x < w2t; ++x)
                {
                    for (UINT k = 0; k < 4; ++k)
                    {
                        accum = 0;
                        accum += prev[(y * multY * w2 + x * multX) * 4 + k];
                        accum += prev[(y * multY * w2 + (x + multX / 2) * multX) * 4 + k];
                        accum += prev[((y + multY / 2) * multY * w2 + x * multX) * 4 + k];
                        accum += prev[((y + multY / 2) * multY * w2 + (x + multX / 2) * multX) * 4 + k];
                        cur[(y * w2t + x) * 4 + k] = accum / 4;
                    }
                }
            }

            w2 = w2t;
            h2 = h2t;
        }
    }

    D3D11_TEXTURE2D_DESC desc;
    desc.Width = w;
    desc.Height = h;
    desc.MipLevels = mipLevels;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA data;
    data.pSysMem = (pMipMaps) ? pMipMaps : pData;
    data.SysMemPitch = w * 4;
    data.SysMemSlicePitch = 0;

    result = pBoundDevice->pDevice->lpVtbl->CreateTexture2D(pBoundDevice->pDevice, &desc, (mipsData) ? mipsData : &data, &pOut->pTexture);
    if (result != S_OK)
    {
        return;
    }
    ID3D11Resource *pResource = NULL;
    result = pOut->pTexture->lpVtbl->QueryInterface(pOut->pTexture, &IID_ID3D11Resource, &pResource);
    if (result != S_OK)
    {
        pOut->pTexture->lpVtbl->Release(pOut->pTexture);
        pOut->pTexture = NULL;
        return;
    }
    result = pBoundDevice->pDevice->lpVtbl->CreateShaderResourceView(pBoundDevice->pDevice, pResource, NULL, &pOut->pResourceView);
    if (result != S_OK)
    {
        pOut->pTexture->lpVtbl->Release(pOut->pTexture);
        pOut->pTexture = NULL;
        return;
    }
    pResource->lpVtbl->Release(pResource);

    if (pMipMaps) free(pMipMaps);
    if (mipsData) free(mipsData);
}

EGDevice egCreateDevice(HWND windowHandle)
{
    if (bIsInBatch) return 0;
    EGDevice                ret = 0;
    DXGI_SWAP_CHAIN_DESC    swapChainDesc;
    SEGDevice               device = {0};
    HRESULT                 result;
    ID3D11Texture2D        *pBackBuffer;
    ID3D11Resource         *pBackBufferRes;

    if (!worldMatrices)
    {
        worldMatrices = (SEGMatrix*)malloc(sizeof(SEGMatrix) * MAX_WORLD_STACK);
    }

    // Define our swap chain
    memset(&swapChainDesc, 0, sizeof(swapChainDesc));
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = windowHandle;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;

    // Create the swap chain, device and device context
    result = D3D11CreateDeviceAndSwapChain(
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
#if _DEBUG
        D3D11_CREATE_DEVICE_DEBUG,
#else /* _DEBUG */
        0,
#endif /* _DEBUG */
        NULL, 0, D3D11_SDK_VERSION,
        &swapChainDesc, &device.pSwapChain,
        &device.pDevice, NULL, &device.pDeviceContext);
    if (result != S_OK) return 0;

    // Create render target
    result = device.pSwapChain->lpVtbl->GetBuffer(device.pSwapChain, 0, &IID_ID3D11Texture2D, (void**)&pBackBuffer);
    if (result != S_OK) return 0;
    result = device.pSwapChain->lpVtbl->GetBuffer(device.pSwapChain, 0, &IID_ID3D11Resource, (void**)&pBackBufferRes);
    if (result != S_OK) return 0;
    result = device.pDevice->lpVtbl->CreateRenderTargetView(device.pDevice, pBackBufferRes, NULL, &device.pRenderTargetView);
    if (result != S_OK) return 0;
    pBackBuffer->lpVtbl->GetDesc(pBackBuffer, &device.backBufferDesc);
    pBackBufferRes->lpVtbl->Release(pBackBufferRes);
    pBackBuffer->lpVtbl->Release(pBackBuffer);

    devices = realloc(devices, sizeof(SEGDevice) * (deviceCount + 1));
    memcpy(devices + deviceCount, &device, sizeof(SEGDevice));
    pBoundDevice = devices + deviceCount;

    // Compile shaders
    ID3DBlob *pVSB = compileShader(g_vs, "vs_5_0");
    ID3DBlob *pPSB = compileShader(g_ps, "ps_5_0");
    result = device.pDevice->lpVtbl->CreateVertexShader(device.pDevice, pVSB->lpVtbl->GetBufferPointer(pVSB), pVSB->lpVtbl->GetBufferSize(pVSB), NULL, &pBoundDevice->pVS);
    if (result != S_OK) return 0;
    result = device.pDevice->lpVtbl->CreatePixelShader(device.pDevice, pPSB->lpVtbl->GetBufferPointer(pPSB), pPSB->lpVtbl->GetBufferSize(pPSB), NULL, &pBoundDevice->pPS);
    if (result != S_OK) return 0;

    // Input layout
    D3D11_INPUT_ELEMENT_DESC layout[4] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
    device.pDevice->lpVtbl->CreateInputLayout(device.pDevice, layout, 4, pVSB->lpVtbl->GetBufferPointer(pVSB), pVSB->lpVtbl->GetBufferSize(pVSB), &pBoundDevice->pInputLayout);

    // Create uniforms
    D3D11_BUFFER_DESC cbDesc = {64, D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE, 0, 0};
    device.pDevice->lpVtbl->CreateBuffer(device.pDevice, &cbDesc, NULL, &pBoundDevice->pCBViewProj);
    device.pDevice->lpVtbl->CreateBuffer(device.pDevice, &cbDesc, NULL, &pBoundDevice->pCBModel);

    // ------------- TEMP ------------
    ID3D11DepthStencilState*    pDs2D;
    ID3D11RasterizerState*      pSr2D;
    ID3D11BlendState*           pBs2D;
    ID3D11SamplerState*         pSs2D;
    {
        D3D11_DEPTH_STENCIL_DESC depthDesc = {0};
        device.pDevice->lpVtbl->CreateDepthStencilState(device.pDevice, &depthDesc, &pDs2D);
    }
    {
        D3D11_RASTERIZER_DESC rasterizerDesc = {0};
        rasterizerDesc.FillMode = D3D11_FILL_SOLID;
        rasterizerDesc.CullMode = D3D11_CULL_NONE;
        device.pDevice->lpVtbl->CreateRasterizerState(device.pDevice, &rasterizerDesc, &pSr2D);
    }
    {
        D3D11_BLEND_DESC blendDesc = {0};
        blendDesc.RenderTarget->BlendEnable = TRUE;
        blendDesc.RenderTarget->SrcBlend = D3D11_BLEND_ONE;
        blendDesc.RenderTarget->DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget->BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget->SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget->DestBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget->BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget->RenderTargetWriteMask = D3D10_COLOR_WRITE_ENABLE_ALL;
        device.pDevice->lpVtbl->CreateBlendState(device.pDevice, &blendDesc, &pBs2D);
    }
    {
        D3D11_SAMPLER_DESC samplerDesc = {0};
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.MaxAnisotropy = 1;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        device.pDevice->lpVtbl->CreateSamplerState(device.pDevice, &samplerDesc, &pSs2D);
    }
    device.pDeviceContext->lpVtbl->OMSetDepthStencilState(device.pDeviceContext, pDs2D, 1);
    device.pDeviceContext->lpVtbl->RSSetState(device.pDeviceContext, pSr2D);
    device.pDeviceContext->lpVtbl->OMSetBlendState(device.pDeviceContext, pBs2D, NULL, 0xffffffff);
    device.pDeviceContext->lpVtbl->PSSetSamplers(device.pDeviceContext, 0, 1, &pSs2D);
    // ------------- END TEMP ------------

    // Create our geometry batch vertex buffer that will be used to batch everything
    D3D11_BUFFER_DESC vertexBufferDesc;
    vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    vertexBufferDesc.ByteWidth = MAX_VERTEX_COUNT * sizeof(SEGVertex);
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    vertexBufferDesc.MiscFlags = 0;
    vertexBufferDesc.StructureByteStride = 0;
    device.pDevice->lpVtbl->CreateBuffer(device.pDevice, &vertexBufferDesc, NULL, &pBoundDevice->pVertexBuffer);
    pBoundDevice->pVertexBuffer->lpVtbl->QueryInterface(pBoundDevice->pVertexBuffer, &IID_ID3D11Resource, &pBoundDevice->pVertexBufferRes);

    // Create a white texture for default rendering
    {
        uint8_t pixel[4] = {255, 255, 255, 255};
        texture2DFromData(pBoundDevice->pDefaultTextureMaps + DIFFUSE_MAP, pixel, 1, 1, FALSE);
    }
    {
        uint8_t pixel[4] = {128, 128, 255, 255};
        texture2DFromData(pBoundDevice->pDefaultTextureMaps + NORMAL_MAP, pixel, 1, 1, FALSE);
    }
    {
        uint8_t pixel[4] = {0, 0, 0, 0};
        texture2DFromData(pBoundDevice->pDefaultTextureMaps + MATERIAL_MAP, pixel, 1, 1, FALSE);
    }

    ++deviceCount;
    if (deviceCount == 1) resetStates();
    return deviceCount;
}

void egDestroyDevice(EGDevice *pDeviceID)
{
    if (bIsInBatch) return;
    SEGDevice *pDevice = devices + (*pDeviceID - 1);

    pDevice->pRenderTargetView->lpVtbl->Release(pDevice->pRenderTargetView);
    pDevice->pDeviceContext->lpVtbl->Release(pDevice->pDeviceContext);
    pDevice->pDevice->lpVtbl->Release(pDevice->pDevice);
    pDevice->pSwapChain->lpVtbl->Release(pDevice->pSwapChain);

    if (*pDeviceID < deviceCount - 1)
    {
        memcpy(pDevice, pDevice + 1, sizeof(SEGDevice) * (deviceCount - *pDeviceID - 1));
    }
    --deviceCount;
    *pDeviceID = 0;
}

void egBindDevice(EGDevice device)
{
    if (bIsInBatch) return;
    pBoundDevice = devices + (device - 1);
}

void egSwap()
{
    if (bIsInBatch) return;
    if (!pBoundDevice) return;
    pBoundDevice->pSwapChain->lpVtbl->Present(pBoundDevice->pSwapChain, 1, 0);

    resetStates();
}

void egClearColor(float r, float g, float b, float a)
{
    clearColor[0] = r;
    clearColor[1] = g;
    clearColor[2] = b;
    clearColor[3] = a;
}

void egClear(EG_CLEAR_BITFIELD clearBitFields)
{
    if (bIsInBatch) return;
    if (!pBoundDevice) return;
    if (clearBitFields & EG_CLEAR_COLOR)
    {
        pBoundDevice->pDeviceContext->lpVtbl->ClearRenderTargetView(pBoundDevice->pDeviceContext, pBoundDevice->pRenderTargetView, clearColor);
    }
}

void updateViewProjCB()
{
    D3D11_MAPPED_SUBRESOURCE map;
    ID3D11Resource *pRes = NULL;
    pBoundDevice->pCBViewProj->lpVtbl->QueryInterface(pBoundDevice->pCBViewProj, &IID_ID3D11Resource, &pRes);
    pBoundDevice->pDeviceContext->lpVtbl->Map(pBoundDevice->pDeviceContext, pRes, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
    memcpy(map.pData, viewProjMatrix.m, 64);
    pBoundDevice->pDeviceContext->lpVtbl->Unmap(pBoundDevice->pDeviceContext, pRes, 0);
    pRes->lpVtbl->Release(pRes);
    pBoundDevice->pDeviceContext->lpVtbl->VSSetConstantBuffers(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->pCBViewProj);
}

void egSet2DViewProj(float nearClip, float farClip)
{
    if (bIsInBatch) return;
    if (!pBoundDevice) return;

    // viewProj2D matrix
    projectionMatrix.m[0] = 2.f / (float)viewPort[2];
    projectionMatrix.m[1] = 0.f;
    projectionMatrix.m[2] = 0.f;
    projectionMatrix.m[3] = -1.f;

    projectionMatrix.m[4] = 0.f;
    projectionMatrix.m[5] = -2.f / (float)viewPort[3];
    projectionMatrix.m[6] = 0.f;
    projectionMatrix.m[7] = 1.f;

    projectionMatrix.m[8] = 0.f;
    projectionMatrix.m[9] = 0.f;
    projectionMatrix.m[10] = -2.f / (farClip - nearClip);
    projectionMatrix.m[11] = -(farClip + nearClip) / (farClip - nearClip);
    
    projectionMatrix.m[12] = 0.f;
    projectionMatrix.m[13] = 0.f;
    projectionMatrix.m[14] = 0.f;
    projectionMatrix.m[15] = 1.f;

    // Identity view
    setIdentityMatrix(&viewMatrix);

    // Multiply them
    multMatrix(&viewMatrix, &projectionMatrix, &viewProjMatrix);

    updateViewProjCB();
}

void egSet3DViewProj(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ, float fov, float nearClip, float farClip)
{
    if (bIsInBatch) return;
    if (!pBoundDevice) return;

    // Projection
    float aspect = (float)pBoundDevice->backBufferDesc.Width / (float)pBoundDevice->backBufferDesc.Height;
    setProjectionMatrix(&projectionMatrix, TO_RAD(fov), aspect, nearClip, farClip);

    // View
    setLookAtMatrix(&viewMatrix, eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ);

    // Multiply them
    multMatrix(&viewMatrix, &projectionMatrix, &viewProjMatrix);

    updateViewProjCB();
}

void egViewPort(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    if (bIsInBatch) return;
    if (!pBoundDevice) return;

    viewPort[0] = x;
    viewPort[1] = y;
    viewPort[2] = width;
    viewPort[3] = height;

    D3D11_VIEWPORT d3dViewport = {(FLOAT)viewPort[0], (FLOAT)viewPort[1], (FLOAT)viewPort[2], (FLOAT)viewPort[3], D3D11_MIN_DEPTH, D3D11_MAX_DEPTH};
    pBoundDevice->pDeviceContext->lpVtbl->RSSetViewports(pBoundDevice->pDeviceContext, 1, &d3dViewport);
}

void egScissor(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    if (bIsInBatch) return;
}

void updateModelCB()
{
    SEGMatrix *pModel = worldMatrices + worldMatricesStackCount;

    D3D11_MAPPED_SUBRESOURCE map;
    ID3D11Resource *pRes = NULL;
    pBoundDevice->pCBModel->lpVtbl->QueryInterface(pBoundDevice->pCBModel, &IID_ID3D11Resource, &pRes);
    pBoundDevice->pDeviceContext->lpVtbl->Map(pBoundDevice->pDeviceContext, pRes, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
    memcpy(map.pData, pModel->m, 64);
    pBoundDevice->pDeviceContext->lpVtbl->Unmap(pBoundDevice->pDeviceContext, pRes, 0);
    pRes->lpVtbl->Release(pRes);
    pBoundDevice->pDeviceContext->lpVtbl->VSSetConstantBuffers(pBoundDevice->pDeviceContext, 1, 1, &pBoundDevice->pCBModel);
}

void egModelIdentity()
{
    if (bIsInBatch) return;
    SEGMatrix *pModel = worldMatrices + worldMatricesStackCount;
    setIdentityMatrix(pModel);
    updateModelCB();
}

void egModelTranslate(float x, float y, float z)
{
    if (bIsInBatch) return;
}

void egModelTranslatev(const float *pAxis)
{
    if (bIsInBatch) return;
}

void egModelMult(const float *pMatrix)
{
    if (bIsInBatch) return;
}

void egModelRotate(float angle, float x, float y, float z)
{
    if (bIsInBatch) return;
}

void egModelRotatev(float angle, const float *pAxis)
{
    if (bIsInBatch) return;
}

void egModelScale(float x, float y, float z)
{
    if (bIsInBatch) return;
}

void egModelScalev(const float *pAxis)
{
    if (bIsInBatch) return;
}

void egModelPush()
{
    if (bIsInBatch) return;
    if (worldMatricesStackCount == MAX_WORLD_STACK - 1) return;
    SEGMatrix *pPrevious = worldMatrices + worldMatricesStackCount;
    ++worldMatricesStackCount;
    SEGMatrix *pNew = worldMatrices + worldMatricesStackCount;
    memcpy(pNew, pPrevious, sizeof(SEGMatrix));
}

void egModelPop()
{
    if (bIsInBatch) return;
    if (!worldMatricesStackCount) return;
    --worldMatricesStackCount;
    updateModelCB();
}

void flush();

void appendVertex(SEGVertex *in_pVertex)
{
    if (currentVertexCount == MAX_VERTEX_COUNT) return;
    memcpy(pVertex + currentVertexCount, in_pVertex, sizeof(SEGVertex));
    ++currentVertexCount;
}

void drawVertex(SEGVertex *in_pVertex)
{
    if (!bIsInBatch) return;

    if (currentTopology == EG_TRIANGLE_FAN)
    {
        if (currentVertexCount >= 3)
        {
            appendVertex(pVertex);
            appendVertex(pVertex + currentVertexCount - 2);
        }
    }
    else if (currentTopology == EG_QUADS)
    {
        if ((currentVertexCount - 3) % 6 == 0 && currentVertexCount)
        {
            appendVertex(pVertex + currentVertexCount - 3);
            appendVertex(pVertex + currentVertexCount - 2);
        }
    }
    else if (currentTopology == EG_QUAD_STRIP)
    {
        if (currentVertexCount == 3)
        {
            appendVertex(pVertex + currentVertexCount - 3);
            appendVertex(pVertex + currentVertexCount - 2);
        }
        else if (currentVertexCount >= 6)
        {
            if ((currentVertexCount - 6) % 2 == 0)
            {
                appendVertex(pVertex + currentVertexCount - 1);
                appendVertex(pVertex + currentVertexCount - 3);
            }
            else
            {
                appendVertex(pVertex + currentVertexCount - 3);
                appendVertex(pVertex + currentVertexCount - 2);
            }
        }
    }

    appendVertex(in_pVertex);
    if (currentVertexCount == MAX_VERTEX_COUNT) flush();
}

void egBegin(EG_TOPOLOGY topology)
{
    if (bIsInBatch) return;
    if (!pBoundDevice) return;

    currentTopology = topology;
    switch (currentTopology)
    {
        case EG_POINTS:
            pBoundDevice->pDeviceContext->lpVtbl->IASetPrimitiveTopology(pBoundDevice->pDeviceContext, D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
            break;
        case EG_LINES:
            pBoundDevice->pDeviceContext->lpVtbl->IASetPrimitiveTopology(pBoundDevice->pDeviceContext, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
            break;
        case EG_LINE_STRIP:
        case EG_LINE_LOOP:
            pBoundDevice->pDeviceContext->lpVtbl->IASetPrimitiveTopology(pBoundDevice->pDeviceContext, D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
            break;
        case EG_TRIANGLES:
            pBoundDevice->pDeviceContext->lpVtbl->IASetPrimitiveTopology(pBoundDevice->pDeviceContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            break;
        case EG_TRIANGLE_STRIP:
            pBoundDevice->pDeviceContext->lpVtbl->IASetPrimitiveTopology(pBoundDevice->pDeviceContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            break;
        case EG_TRIANGLE_FAN:
            pBoundDevice->pDeviceContext->lpVtbl->IASetPrimitiveTopology(pBoundDevice->pDeviceContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            break;
        case EG_QUADS:
            pBoundDevice->pDeviceContext->lpVtbl->IASetPrimitiveTopology(pBoundDevice->pDeviceContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            break;
        case EG_QUAD_STRIP:
            pBoundDevice->pDeviceContext->lpVtbl->IASetPrimitiveTopology(pBoundDevice->pDeviceContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            break;
        default:
            return;
    }

    bIsInBatch = TRUE;
    D3D11_MAPPED_SUBRESOURCE mappedVertexBuffer;
    pBoundDevice->pDeviceContext->lpVtbl->Map(pBoundDevice->pDeviceContext, pBoundDevice->pVertexBufferRes, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedVertexBuffer);
    pVertex = (SEGVertex*)mappedVertexBuffer.pData;
}

void flush()
{
    if (!bIsInBatch) return;
    if (!pBoundDevice) return;
    if (!currentVertexCount) return;

    switch (currentTopology)
    {
        case EG_LINE_LOOP:
            drawVertex(pVertex);
            break;
    }

    pBoundDevice->pDeviceContext->lpVtbl->Unmap(pBoundDevice->pDeviceContext, pBoundDevice->pVertexBufferRes, 0);

    const UINT stride = sizeof(SEGVertex);
    const UINT offset = 0;
    pBoundDevice->pDeviceContext->lpVtbl->IASetVertexBuffers(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->pVertexBuffer, &stride, &offset);
    pBoundDevice->pDeviceContext->lpVtbl->Draw(pBoundDevice->pDeviceContext, currentVertexCount, 0);

    currentVertexCount = 0;

    D3D11_MAPPED_SUBRESOURCE mappedVertexBuffer;
    pBoundDevice->pDeviceContext->lpVtbl->Map(pBoundDevice->pDeviceContext, pBoundDevice->pVertexBufferRes, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedVertexBuffer);
    pVertex = (SEGVertex*)mappedVertexBuffer.pData;
}

void egEnd()
{
    if (!bIsInBatch) return;
    if (!pBoundDevice) return;
    flush();
    bIsInBatch = FALSE;

    pBoundDevice->pDeviceContext->lpVtbl->Unmap(pBoundDevice->pDeviceContext, pBoundDevice->pVertexBufferRes, 0);
}

void egColor3(float r, float g, float b)
{
    currentVertex.r = r;
    currentVertex.g = g;
    currentVertex.b = b;
    currentVertex.a = 1.f;
}

void egColor3v(const float *pRGB)
{
    memcpy(&currentVertex.r, pRGB, 12);
    currentVertex.a = 1.f;
}

void egColor4(float r, float g, float b, float a)
{
    currentVertex.r = r;
    currentVertex.g = g;
    currentVertex.b = b;
    currentVertex.a = a;
}

void egColor4v(const float *pRGBA)
{
    memcpy(&currentVertex.r, pRGBA, 16);
}

void egNormal(float nx, float ny, float nz)
{
    currentVertex.nx = nx;
    currentVertex.ny = ny;
    currentVertex.nz = nz;
}

void egNormalv(const float *pNormal)
{
    memcpy(&currentVertex.nx, pNormal, 12);
}

void egTexCoord(float u, float v)
{
    currentVertex.u = u;
    currentVertex.v = v;
}

void egTexCoordv(const float *pTexCoord)
{
    memcpy(&currentVertex.u, pTexCoord, 8);
}

void egPosition2(float x, float y)
{
    if (!bIsInBatch) return;
    currentVertex.x = x;
    currentVertex.y = y;
    currentVertex.z = 0.f;
    drawVertex(&currentVertex);
}

void egPosition2v(const float *pPos)
{
    if (!bIsInBatch) return;
    memcpy(&currentVertex.x, pPos, 8);
    currentVertex.z = 0.f;
    drawVertex(&currentVertex);
}

void egPosition3(float x, float y, float z)
{
    if (!bIsInBatch) return;
    currentVertex.x = x;
    currentVertex.y = y;
    currentVertex.z = z;
    drawVertex(&currentVertex);
}

void egPosition3v(const float *pPos)
{
    if (!bIsInBatch) return;
    memcpy(&currentVertex.x, pPos, 12);
    drawVertex(&currentVertex);
}

void egTarget2(float x, float y)
{
}

void egTarget2v(const float *pPos)
{
}

void egTarget3(float x, float y, float z)
{
}

void egTarget3v(const float *pPos)
{
}

void egRadius(float radius)
{
}

void egRadius2(float innerRadius, float outterRadius)
{
}

void egRadius2v(const float *pRadius)
{
}

void egFalloutExponent(float exponent)
{
}

void egMultiply(float multiply)
{
}

void egSpecular(float intensity, float shininess)
{
}

void egSelfIllum(float intensity)
{
}

EGTexture createTexture(SEGTexture2D *pTexture)
{
    pBoundDevice->textures = realloc(pBoundDevice->textures, sizeof(SEGTexture2D) * (pBoundDevice->textureCount + 1));
    memcpy(pBoundDevice->textures + pBoundDevice->textureCount, pTexture, sizeof(SEGTexture2D));
    return ++pBoundDevice->textureCount;
}

EGTexture egCreateTexture1D(uint32_t dimension, const void *pData, EG_FORMAT dataFormat)
{
    return 0;
}

EGTexture egCreateTexture2D(uint32_t width, uint32_t height, const void *pData, uint32_t dataType, EG_TEXTURE_FLAGS flags)
{
    if (!pBoundDevice) return 0;
    SEGTexture2D texture2D = {0};
    uint8_t *pConvertedData = (uint8_t *)pData;
    texture2DFromData(&texture2D, pConvertedData, width, height, flags & EG_GENERATE_MIPMAPS ? TRUE : FALSE);
    if (!texture2D.pTexture) return 0;
    return createTexture(&texture2D);
}

EGTexture egCreateTexture3D(uint32_t width, uint32_t height, uint32_t depth, const void *pData, uint32_t dataType)
{
    return 0;
}

EGTexture egCreateCubeMap(uint32_t dimension, const void *pData, EG_FORMAT dataFormat, EG_TEXTURE_FLAGS flags)
{
    return 0;
}

void egDestroyTexture(EGTexture *pTexture)
{
}

void egBindDiffuse(EGTexture texture)
{
    if (!pBoundDevice) return;
    if (texture > pBoundDevice->textureCount || !texture) return;
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 0, 1, &pBoundDevice->textures[texture - 1].pResourceView);
}

void egBindNormal(EGTexture texture)
{
    if (!pBoundDevice) return;
    if (texture >= pBoundDevice->textureCount || !texture) return;
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 1, 1, &pBoundDevice->textures[texture - 1].pResourceView);
}

void egBindMaterial(EGTexture texture)
{
    if (!pBoundDevice) return;
    if (texture >= pBoundDevice->textureCount || !texture) return;
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShaderResources(pBoundDevice->pDeviceContext, 2, 1, &pBoundDevice->textures[texture - 1].pResourceView);
}

void egEnable(EG_ENABLE_BITS stateBits)
{
}

void egDisable(EG_ENABLE_BITS sateBits)
{
}

void egStatePush()
{
}

void egStatePop()
{
}

void egCube(float size)
{
}

void egSphere(float radius, uint32_t slices, uint32_t stacks)
{
}

void egCylinder(float bottomRadius, float topRadius, float height, uint32_t slices)
{
}

void egTube(float outterRadius, float innerRadius, float height, uint32_t slices)
{
}

void egTorus(float radius, float innerRadius, uint32_t slices, uint32_t stacks)
{
}
