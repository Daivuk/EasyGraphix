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
    egClearColor(.25f, .5f, 1, 1);
    egClear(EG_CLEAR_COLOR);
    egSet2DViewProj(-999, 999);

    egBindDiffuse(texture);

    egBegin(EG_QUADS);
    egTexCoord(0, 0);
    egPosition2(200, 200);
    egTexCoord(0, 1);
    egPosition2(200, 400);
    egTexCoord(1, 1);
    egPosition2(400, 400);
    egTexCoord(1, 0);
    egPosition2(400, 200);
    egEnd();

    egBegin(EG_QUADS);
    egTexCoord(0, 0);
    egPosition2(500, 200);
    egTexCoord(0, 1);
    egPosition2(500, 300);
    egTexCoord(1, 1);
    egPosition2(600, 300);
    egTexCoord(1, 0);
    egPosition2(600, 200);
    egEnd();

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
