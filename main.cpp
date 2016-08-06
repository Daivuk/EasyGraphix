#include <assert.h>
#include <Windows.h>
#include "eg.h"
#include "LodePNG.h"

#define RESOLUTION_W 1280
#define RESOLUTION_H 720

HWND        windowHandle;
EGDevice    device;
EGTexture   diffuse;
EGTexture   normal;
EGTexture   material;
EGTexture   diffuseFloor;
EGTexture   normalFloor;
EGTexture   materialFloor;
EGTexture   alphaTest;
EGState     state3d;
EGState     state2d;
EGTexture   ogreTextures[3];

struct sMeshVertex
{
    float x, y, z;
    float nx, ny, nz;
    float u, v;
    float r, g, b;
};

struct sMesh
{
    sMeshVertex *pVertices;
    unsigned short *pIndices;
    unsigned long nbIndices;
};

sMesh ogreMesh;

void init()
{
    // Create device
    device = egCreateDevice(windowHandle);

    // Load textures
    unsigned int w, h;
    {
        std::vector<unsigned char> image;
        auto ret = lodepng::decode(image, w, h, "d01.png");
        assert(!ret);
        diffuseFloor = egCreateTexture2D(w, h, image.data(), EG_U8 | EG_RGBA, EG_GENERATE_MIPMAPS);
    }
    {
        std::vector<unsigned char> image;
        auto ret = lodepng::decode(image, w, h, "n01.png");
        assert(!ret);
        normalFloor = egCreateTexture2D(w, h, image.data(), EG_U8 | EG_RGBA, EG_GENERATE_MIPMAPS);
    }
    {
        std::vector<unsigned char> image;
        auto ret = lodepng::decode(image, w, h, "m01.png");
        assert(!ret);
        materialFloor = egCreateTexture2D(w, h, image.data(), EG_U8 | EG_RGBA, EG_GENERATE_MIPMAPS);
    }
    {
        std::vector<unsigned char> image;
        auto ret = lodepng::decode(image, w, h, "d02.png");
        assert(!ret);
        diffuse = egCreateTexture2D(w, h, image.data(), EG_U8 | EG_RGBA, EG_GENERATE_MIPMAPS);
    }
    {
        std::vector<unsigned char> image;
        auto ret = lodepng::decode(image, w, h, "n02.png");
        assert(!ret);
        normal = egCreateTexture2D(w, h, image.data(), EG_U8 | EG_RGBA, EG_GENERATE_MIPMAPS);
    }
    {
        std::vector<unsigned char> image;
        auto ret = lodepng::decode(image, w, h, "m02.png");
        assert(!ret);
        material = egCreateTexture2D(w, h, image.data(), EG_U8 | EG_RGBA, EG_GENERATE_MIPMAPS);
    }
    {
        std::vector<unsigned char> image;
        auto ret = lodepng::decode(image, w, h, "alphatest.png");
        assert(!ret);
        alphaTest = egCreateTexture2D(w, h, image.data(), EG_U8 | EG_RGBA, EG_GENERATE_MIPMAPS);
    }

    // Ogre resources
    {
        std::vector<unsigned char> image;
        auto ret = lodepng::decode(image, w, h, "ogre_dif.png");
        assert(!ret);
        ogreTextures[0] = egCreateTexture2D(w, h, image.data(), EG_U8 | EG_RGBA, EG_GENERATE_MIPMAPS);
    }
    {
        std::vector<unsigned char> image;
        auto ret = lodepng::decode(image, w, h, "ogre_normal.png");
        assert(!ret);
        ogreTextures[1] = egCreateTexture2D(w, h, image.data(), EG_U8 | EG_RGBA, EG_GENERATE_MIPMAPS);
    }
    {
        std::vector<unsigned char> image;
        auto ret = lodepng::decode(image, w, h, "ogre_spec.png");
        assert(!ret);
        ogreTextures[2] = egCreateTexture2D(w, h, image.data(), EG_U8 | EG_RGBA, EG_GENERATE_MIPMAPS);
    }
    {
        FILE *pFic;
        fopen_s(&pFic, "ogre.mesh", "rb");

        unsigned long version;
        fread(&version, 4, 1, pFic);
        unsigned long nbVertices;
        fread(&nbVertices, 4, 1, pFic);
        ogreMesh.pVertices = new sMeshVertex[nbVertices];
        fread(&ogreMesh.nbIndices, 4, 1, pFic);
        ogreMesh.pIndices = new unsigned short[ogreMesh.nbIndices];
        fread(ogreMesh.pVertices, 4, nbVertices * 11, pFic);
        fread(ogreMesh.pIndices, 2, ogreMesh.nbIndices, pFic);
    }

    // Create state objects
    egDisable(EG_ALL);
    egEnable(EG_DEPTH_TEST | EG_DEPTH_WRITE | EG_CULL);
    egEnable(EG_LIGHTING);
    //egEnable(EG_CAST_SHADOW);
    egEnable(EG_HDR | EG_BLOOM /*| EG_BLUR | EG_VIGNETTE*/);
    egBlur(32);
    egVignette(8);
    state3d = egCreateState();

    egDisable(EG_ALL);
    egEnable(EG_BLUR);
    egBlur(16);
    state2d = egCreateState();
}

void shutdown()
{
    egDestroyDevice(&device);
}

void update()
{
}

void draw()
{
    // Clear screen
    egClearColor(0, 0, 0, 1);
    egClear(EG_CLEAR_ALL);

#if 1 // Grimrock asset test
    // Setup matrices
    egModelIdentity();
    egSet3DViewProj(-1, 2.5f, -2.5f, 0, 1.5f, 0, 0, 1, 0, 70, .1f, 10000.f);
    //egSet3DViewProj(-0.5f, 2.5f, -1.5f, 0, 2.5f, 0, 0, 1, 0, 70, .1f, 10000.f);

    // Setup 3d states
    egBindState(state3d);

#if 1 // Background
    egColor3(1, 1, 1);
    egBindDiffuse(0);
    egBindNormal(0);
    egBindMaterial(0);
    egModelPush();
    {
        egModelTranslate(0, 20, 0);
        egModelRotate(180, 1, 0, 0);
        egTangent(1, 0, 0);
        egBinormal(0, -1, 0);
        egNormal(0, 0, 1);
        egColor3(0, 0, 0);
        egBegin(EG_QUADS);
        {
            egTexCoord(0, 0);
            egPosition3(-100, 100, 0);
            egTexCoord(0, 200 / 5);
            egPosition3(-100, -100, 0);
            egTexCoord(200 / 5, 200 / 5);
            egPosition3(100, -100, 0);
            egTexCoord(200 / 5, 0);
            egPosition3(100, 100, 0);
        }
        egEnd();
    }
    egModelPop();
#endif


    egBindDiffuse(ogreTextures[0]);
    egBindNormal(ogreTextures[1]);
    egBindMaterial(ogreTextures[2]);
    egColor3(1, 1, 1);

    egEnable(EG_GENERATE_TANGENT_BINORMAL);
    egBegin(EG_TRIANGLES);
    for (unsigned int i = 0; i < ogreMesh.nbIndices; ++i)
    {
        auto pVert = ogreMesh.pVertices + ogreMesh.pIndices[i];
        egNormal(pVert->nx, pVert->ny, pVert->nz);
        egTexCoord(pVert->u, pVert->v);
        egPosition3(pVert->x, pVert->y, pVert->z);
    }
    egEnd();
    egDisable(EG_GENERATE_TANGENT_BINORMAL);

    egBegin(EG_AMBIENTS);
    egColor3(.15f, .15f, .15f);
    egEnd();

    egBegin(EG_OMNIS);

    egColor3(1, .85f, .5f);
    egMultiply(2);
    egRadius(200);
    egPosition3(-10, 0, 3);

    egColor3(.5f, .85f, 1);
    egMultiply(6);
    egRadius(200);
    egPosition3(10, 10, -3);

    egEnd();

    egPostProcess();
#endif

#if 0 // Basic test
#if 1 // 3d
    // Setup matrices
    egModelIdentity();
    egSet3DViewProj(-5, -5, 3, 0, 0, -2, 0, 0, 1, 70, .1f, 10000.f);

    // Setup 3d states
    egBindState(state3d);

    // Draw shit up
    static float rotation = 0.f;
    rotation += 1.f;

#if 1 // floor
    egColor3(1, 1, 1);
    egBindDiffuse(diffuseFloor);
    egBindNormal(normalFloor);
    egBindMaterial(materialFloor);
    egModelPush();
    {
        egModelTranslate(0, 0, -1.5);
        egTangent(1, 0, 0);
        egBinormal(0, -1, 0);
        egNormal(0, 0, 1);
        egBegin(EG_QUADS);
        {
            egTexCoord(0, 0);
            egPosition3(-100, 100, 0);
            egTexCoord(0, 200 / 5);
            egPosition3(-100, -100, 0);
            egTexCoord(200 / 5, 200 / 5);
            egPosition3(100, -100, 0);
            egTexCoord(200 / 5, 0);
            egPosition3(100, 100, 0);
        }
        egEnd();
    }
    egModelPop();
#endif

#if 1 // primitives
    egModelPush();
    {
        egColor3(1, 1, 1);
        egModelRotate(rotation, 0, 0, 1);
        egBindDiffuse(diffuse);
        egBindNormal(normal);
        egBindMaterial(material);
        egCube(3);

        egModelTranslate(-5, 2, 0);
        egSphere(1.5f, 24, 12, 4);

        egModelTranslate(7, -7, -1.5f);
        egTube(1.5f, 1.0f, 3, 24, 3);
    }
    egModelPop();
#endif

    egBegin(EG_AMBIENTS);
    {
        egColor3(.35f, .25f, .35f); // This will render an ambient pass
    }
    egEnd();

#if 0
    egBegin(EG_OMNIS);
    {
        egFalloffExponent(1);
        egMultiply(3);
        egRadius(100);

        egColor3(1, .75f, .5f);
        egPosition3(-15, 15, 20); // This will render the light
    }
    egEnd();

    egBegin(EG_OMNIS);
    {
        egRadius(40);
        egColor3(.5f, .75f, 1);
        egPosition3(15, -15, 10);

        egRadius(10);

        egMultiply(10);
        egColor3(.5f, .75f, 1);
        egPosition3(0, -4, -2.0f); // This will render the light

        egMultiply(3);
        egColor3(1, .75f, .5f);
        egPosition3(-4, 0, -2.0f); // This will render the light

        egMultiply(10);
        egColor3(.75f, .5f, 1);
        egPosition3(5, 20, -2.0f); // This will render the light
    }
    egEnd();
#endif
    egPostProcess();
#endif
#if 1 // 2d
    egSet2DViewProj(-999, 999);
    egBindState(state2d);
    egBindDiffuse(0);
    egBindNormal(0);
    egBindMaterial(0);
    egColor3(1, 1, 1);
    egBegin(EG_QUADS);
    {
        egColor3(1, 0, 0);
        egPosition2(100, 100 + 300);
        egPosition2(100, 100 + 16 + 300);
        egPosition2(100 + 16, 100 + 16 + 300);
        egPosition2(100 + 16, 100 + 300);

        egColor3(0, 1, 0);
        egPosition2(200, 100 + 300);
        egPosition2(200, 100 + 32 + 300);
        egPosition2(200 + 32, 100 + 32 + 300);
        egPosition2(200 + 32, 100 + 300);

        egColor3(0, 0, 1);
        egPosition2(400, 100 + 300);
        egPosition2(400, 100 + 64 + 300);
        egPosition2(400 + 64, 100 + 64 + 300);
        egPosition2(400 + 64, 100 + 300);

        egColor3(1, 1, 0);
        egPosition2(800, 100 + 300);
        egPosition2(800, 100 + 128 + 300);
        egPosition2(800 + 128, 100 + 128 + 300);
        egPosition2(800 + 128, 100 + 300);
    }
    egEnd();

    egEnable(EG_BLEND);
    egBlendFunc(EG_SRC_ALPHA, EG_ONE_MINUS_SRC_ALPHA);
    egPostProcess();
#endif
#endif

    egSwap();
}

LRESULT CALLBACK WinProc(HWND handle, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_DESTROY ||
        msg == WM_CLOSE)
    {
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(handle, msg, wparam, lparam);
}

void createWindow()
{
    // Define window style
    WNDCLASS wc = {0};
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WinProc;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"EasyGraphixSampleWNDC";
    RegisterClass(&wc);

    // Centered position
    auto screenW = GetSystemMetrics(SM_CXSCREEN);
    auto screenH = GetSystemMetrics(SM_CYSCREEN);
    auto posX = (screenW - RESOLUTION_W) / 2;
    auto posY = (screenH - RESOLUTION_H) / 2;

    // Create the window
    windowHandle = CreateWindow(L"EasyGraphixSampleWNDC", L"Easy Graphix Sample",
                                WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
                                posX, posY, RESOLUTION_W, RESOLUTION_H,
                                nullptr, nullptr, nullptr, nullptr);
}

int CALLBACK WinMain(
    _In_  HINSTANCE hInstance,
    _In_  HINSTANCE hPrevInstance,
    _In_  LPSTR lpCmdLine,
    _In_  int nCmdShow)
{
    // Initialize
    createWindow();
    init();

    // Main loop
    MSG msg;
    while (true)
    {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
            {
                break;
            }
        }

        update();
        draw();
    }

    shutdown();

    return 0;
}
