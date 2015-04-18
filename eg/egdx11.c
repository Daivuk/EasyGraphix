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
cbuffer mm:register(b0)\
{\
    matrix viewProj;\
}\
cbuffer mm:register(b1)\
{\
    matrix model;\
}\
struct sInput\
{\
    float3 position:POSITION;\
    float3 normal:NORMAL;\
    float2 texCoord:TEXCOORD;\
    float4 color:COLOR;\
};\
struct sOutput\
{\
    float4 position:SV_POSITION;\
    float3 normal:NORMAL;\
    float2 texCoord:TEXCOORD;\
    float4 color:COLOR;\
};\
sOutput main(sInput input)\
{\
    sOutput output;\
    float4 worldPosition = mul(float4(input.position, 1), model);\
    output.position = mul(worldPosition, viewProj);\
    output.normal = mul(float4(input.normal, 0), model).xyz;\
    output.texCoord = input.texCoord;\
    output.color = input.color;\
    return output;\
}";

const char *g_ps =
"\
Texture2D xDiffuse:register(t0);\
SamplerState sSampler:register(s0);\
struct sInput\
{\
    float4 position:SV_POSITION;\
    float3 normal:NORMAL;\
    float2 texCoord:TEXCOORD;\
    float4 color:COLOR;\
};\
float4 main(sInput input):SV_TARGET\
{\
    float4 diffuse = xDiffuse.Sample(sSampler, input.texCoord);\
    return diffuse * input.color;\
}";

ID3D11VertexShader  *pVS;
ID3D11PixelShader   *pPS;
ID3D11InputLayout   *pInputLayout;
ID3D11Buffer        *pCBViewProj;
ID3D11Buffer        *pCBModel;

typedef struct
{
    float x, y, z;
    float nx, ny, nz;
    float u, v;
    float r, g, b, a;
} SEGVertex;

// Devices
typedef struct
{
    IDXGISwapChain         *pSwapChain;
    ID3D11Device           *pDevice;
    ID3D11DeviceContext    *pDeviceContext;
    ID3D11RenderTargetView *pRenderTargetView;
    D3D11_TEXTURE2D_DESC    backBufferDesc;
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
void setLookAtMatrix(SEGMatrix *pMatrix, float* pos, float* dir, float* in_up)
{
    float front[3] = {dir[0], dir[1], dir[2]};
    float right[3];
    float up[3];
    float eye[3] = {-pos[0], -pos[1], -pos[2]};

    v3cross(front, in_up, right);
    v3cross(right, front, up);

    v3normalize(front);
    v3normalize(right);
    v3normalize(up);

    front[0] = -front[0];
    front[1] = -front[1];
    front[2] = -front[2];

    pMatrix->m[0] = right[0];
    pMatrix->m[1] = right[1];
    pMatrix->m[2] = right[2];
    pMatrix->m[3] = v3dot(right, eye);
    pMatrix->m[4] = up[0];
    pMatrix->m[5] = up[1];
    pMatrix->m[6] = up[2];
    pMatrix->m[7] = v3dot(up, eye);
    pMatrix->m[8] = front[0];
    pMatrix->m[9] = front[1];
    pMatrix->m[10] = front[2];
    pMatrix->m[11] = v3dot(front, eye);
    pMatrix->m[12] = 0.f;
    pMatrix->m[13] = 0.f;
    pMatrix->m[14] = 0.f;
    pMatrix->m[15] = 1.f;
}
void setProjectionMatrix(SEGMatrix *pMatrix, float fov, float aspect, float nearDist, float farDist, BOOL leftHanded)
{
    //
    // General form of the Projection Matrix
    //
    // uh = Cot( fov/2 ) == 1/Tan(fov/2)
    // uw / uh = 1/aspect
    // 
    //   uw         0       0       0
    //    0        uh       0       0
    //    0         0      f/(f-n)  1
    //    0         0    -fn/(f-n)  0
    //
    // Make result to be identity first
    setIdentityMatrix(pMatrix);

    // check for bad parameters to avoid divide by zero:
    // if found, assert and return an identity matrix.
    if (fov <= 0 || aspect == 0)
    {
        return;
    }

    float frustumDepth = farDist - nearDist;
    float oneOverDepth = 1 / frustumDepth;

    pMatrix->m[5] = 1.f / tanf(0.5f * fov);
    pMatrix->m[0] = (leftHanded ? 1 : -1) * pMatrix->m[5] / aspect;
    pMatrix->m[10] = farDist * oneOverDepth;
    pMatrix->m[11] = (-farDist * nearDist) * oneOverDepth;
    pMatrix->m[14] = 1;
    pMatrix->m[15] = 0;
}
void multMatrix(SEGMatrix *pM1, SEGMatrix *pM2, SEGMatrix *pMOut)
{
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            float n = 0;
            for (int k = 0; k < 4; k++)
                n += pM2->m[i + k * 4] * pM1->m[k + j * 4];
            pMOut->m[i + j * 4] = n;
        }
    }
}

void resetStates()
{
    worldMatricesStackCount = 0;

    egSet2DViewProj(-999, 999);
    egViewPort(0, 0, (uint32_t)pBoundDevice->backBufferDesc.Width, (uint32_t)pBoundDevice->backBufferDesc.Height);
    egModelIdentity();

    pBoundDevice->pDeviceContext->lpVtbl->IASetInputLayout(pBoundDevice->pDeviceContext, pInputLayout);
    pBoundDevice->pDeviceContext->lpVtbl->VSSetShader(pBoundDevice->pDeviceContext, pVS, NULL, 0);
    pBoundDevice->pDeviceContext->lpVtbl->PSSetShader(pBoundDevice->pDeviceContext, pPS, NULL, 0);
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

EGDevice egCreateDevice(HWND windowHandle)
{
    EGDevice                ret = 0;
    DXGI_SWAP_CHAIN_DESC    swapChainDesc;
    SEGDevice               device;
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
    result = device.pDevice->lpVtbl->CreateVertexShader(device.pDevice, pVSB->lpVtbl->GetBufferPointer(pVSB), pVSB->lpVtbl->GetBufferSize(pVSB), NULL, &pVS);
    if (result != S_OK) return 0;
    result = device.pDevice->lpVtbl->CreatePixelShader(device.pDevice, pPSB->lpVtbl->GetBufferPointer(pPSB), pPSB->lpVtbl->GetBufferSize(pPSB), NULL, &pPS);
    if (result != S_OK) return 0;

    // Input layout
    D3D11_INPUT_ELEMENT_DESC layout[4] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
    device.pDevice->lpVtbl->CreateInputLayout(device.pDevice, layout, 4, pVSB->lpVtbl->GetBufferPointer(pVSB), pVSB->lpVtbl->GetBufferSize(pVSB), &pInputLayout);

    // Create uniforms
    D3D11_BUFFER_DESC cbDesc = {64, D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE, 0, 0};
    device.pDevice->lpVtbl->CreateBuffer(device.pDevice, &cbDesc, NULL, &pCBViewProj);
    device.pDevice->lpVtbl->CreateBuffer(device.pDevice, &cbDesc, NULL, &pCBModel);

    ++deviceCount;
    if (deviceCount == 1) resetStates();
    return deviceCount;
}

void egDestroyDevice(EGDevice *pDeviceID)
{
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
    pBoundDevice = devices + (device - 1);
}

void egSwap()
{
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
    pCBViewProj->lpVtbl->QueryInterface(pCBViewProj, &IID_ID3D11Resource, &pRes);
    pBoundDevice->pDeviceContext->lpVtbl->Map(pBoundDevice->pDeviceContext, pRes, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
    memcpy(map.pData, viewProjMatrix.m, 64);
    pBoundDevice->pDeviceContext->lpVtbl->Unmap(pBoundDevice->pDeviceContext, pRes, 0);
    pRes->lpVtbl->Release(pRes);
    pBoundDevice->pDeviceContext->lpVtbl->VSSetConstantBuffers(pBoundDevice->pDeviceContext, 1, 1, &pCBViewProj);
}

void egSet2DViewProj(float nearClip, float farClip)
{
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
    if (!pBoundDevice) return;

    // Projection
    float aspect = (float)pBoundDevice->backBufferDesc.Width / (float)pBoundDevice->backBufferDesc.Height;
    setProjectionMatrix(&projectionMatrix, TO_RAD(fov), aspect, nearClip, farClip, FALSE);

    // View
    float dir[3] = {centerX - eyeX, centerY - eyeY, centerZ - eyeZ};
    setLookAtMatrix(&viewMatrix, &eyeX, dir, &upX);

    // Multiply them
    multMatrix(&viewMatrix, &projectionMatrix, &viewProjMatrix);

    updateViewProjCB();
}

void egViewPort(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    if (!pBoundDevice) return;

    viewPort[0] = x;
    viewPort[1] = y;
    viewPort[2] = width;
    viewPort[3] = height;

    D3D11_VIEWPORT d3dViewport = {(FLOAT)viewPort[0], (FLOAT)viewPort[1], (FLOAT)viewPort[2], (FLOAT)viewPort[3], 0.f, 1.f};
    pBoundDevice->pDeviceContext->lpVtbl->RSSetViewports(pBoundDevice->pDeviceContext, 1, &d3dViewport);
}

void egScissor(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
}

void updateModelCB()
{
    SEGMatrix *pModel = worldMatrices + worldMatricesStackCount;

    D3D11_MAPPED_SUBRESOURCE map;
    ID3D11Resource *pRes = NULL;
    pCBModel->lpVtbl->QueryInterface(pCBModel, &IID_ID3D11Resource, &pRes);
    pBoundDevice->pDeviceContext->lpVtbl->Map(pBoundDevice->pDeviceContext, pRes, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
    memcpy(map.pData, pModel->m, 64);
    pBoundDevice->pDeviceContext->lpVtbl->Unmap(pBoundDevice->pDeviceContext, pRes, 0);
    pRes->lpVtbl->Release(pRes);
    pBoundDevice->pDeviceContext->lpVtbl->VSSetConstantBuffers(pBoundDevice->pDeviceContext, 1, 1, &pCBModel);
}

void egModelIdentity()
{
    SEGMatrix *pModel = worldMatrices + worldMatricesStackCount;
    setIdentityMatrix(pModel);
    updateModelCB();
}

void egModelTranslate(float x, float y, float z)
{
}

void egModelTranslatev(const float *pAxis)
{
}

void egModelMult(const float *pMatrix)
{
}

void egModelRotate(float angle, float x, float y, float z)
{
}

void egModelRotatev(float angle, const float *pAxis)
{
}

void egModelScale(float x, float y, float z)
{
}

void egModelScalev(const float *pAxis)
{
}

void egModelPush()
{
    if (worldMatricesStackCount == MAX_WORLD_STACK - 1) return;
    SEGMatrix *pPrevious = worldMatrices + worldMatricesStackCount;
    ++worldMatricesStackCount;
    SEGMatrix *pNew = worldMatrices + worldMatricesStackCount;
    memcpy(pNew, pPrevious, sizeof(SEGMatrix));
}

void egModelPop()
{
    if (!worldMatricesStackCount) return;
    --worldMatricesStackCount;
    updateModelCB();
}

void egBegin(EG_TOPOLOGY topology)
{
}

void egEnd()
{
}

void egColor3(float r, float g, float b)
{
}

void egColor3v(const float *pRGB)
{
}

void egColor4(float r, float g, float b, float a)
{
}

void egColor4v(const float *pRGBA)
{
}

void egNormal(float nx, float ny, float nz)
{
}

void egNormalv(const float *pNormal)
{
}

void egTexCoord(float u, float v)
{
}

void egTexCoordv(const float *pTexCoord)
{
}

void egPosition2(float x, float y)
{
}

void egPosition2v(const float *pPos)
{
}

void egPosition3(float x, float y, float z)
{
}

void egPosition3v(const float *pPos)
{
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

EGTexture egCreateTexture1D(uint32_t dimension, const void *pData, EG_FORMAT dataFormat)
{
    return 0;
}

EGTexture egCreateTexture2D(uint32_t width, uint32_t height, const void *pData, EG_DATA_TYPE dataType, EG_TEXTURE_FLAGS flags)
{
    return 0;
}

EGTexture egCreateTexture3D(uint32_t width, uint32_t height, uint32_t depth, const void *pData, EG_DATA_TYPE dataType)
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
}

void egBindNormal(EGTexture texture)
{
}

void egBindMaterial(EGTexture texture)
{
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
