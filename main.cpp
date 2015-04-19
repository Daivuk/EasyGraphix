#include <assert.h>
#include <Windows.h>
#include "eg.h"
#include "LodePNG.h"

#define RESOLUTION_W 1280
#define RESOLUTION_H 720

HWND        windowHandle;
EGDevice    device;
EGTexture   texture;

void init()
{
    device = egCreateDevice(windowHandle);

    std::vector<unsigned char> image;
    unsigned int w, h;
    auto ret = lodepng::decode(image, w, h, "stone.png");
    assert(!ret);
    byte* pData = &(image[0]);

    texture = egCreateTexture2D(w, h, pData, EG_U8 | EG_RGBA, EG_GENERATE_MIPMAPS);
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
    egClearColor(0, 0, 0, 1);
    egClear(EG_CLEAR_COLOR | EG_CLEAR_DEPTH_STENCIL);

    egModelIdentity();

    egSet3DViewProj(-15, -15, 10, 0, 0, 0, 0, 0, 1, 90, .1f, 10000.f);
    egEnable(EG_DEPTH_TEST);

    static float rotation = 0.f;
    rotation += 1.f;

    egColor3(1, 1, 1);
    egBindDiffuse(0);
    egModelPush();
    egModelScale(200, 200, 1);
    egModelTranslate(0, 0, -5);
    egCube(1);
    egModelPop();

    egBindDiffuse(texture);
    egModelRotate(rotation, 0, 0, 1);
    egColor3(1, .5f, .5f);
    egCube(5);

    egStatePush();
    egModelPush();
    egDisable(EG_DEPTH_TEST);
    egModelTranslate(0, -10, 0);
    egColor3(.5f, 1, .5f);
    egCube(5);
    egModelPop();
    egStatePop();

    egModelTranslate(-10, 0, 0);
    egColor3(.5f, .5f, 1);
    egCube(5);

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
