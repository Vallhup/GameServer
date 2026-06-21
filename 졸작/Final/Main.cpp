#include "pch.h"
#include "Input.h"
#include "Timer.h"
#include "Engine.h"
#include "imgui.h"
#include "ImGuiManager.h"
#include "ClientConnectionListener.h"
#include "SceneManager.h"
#include "Scene.h"
#include "Camera.h"
#include <chrono>
#include <thread>
#include <timeapi.h>
#include <wincodec.h>
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "windowscodecs.lib")

static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static void InitWindow(HINSTANCE hInstance, const int nCmdShow, HWND* hwnd);
static void LimitFrameRate(int fpsCap);
static HCURSOR CreateCursorFromPng(const wchar_t* path, int maxSize, float hotspotU, float hotspotV);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    TIMER.Initialize();
    timeBeginPeriod(1);
    const HRESULT comInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    ClientConnectionListener listener;

    AllocConsole();
    string ip;

    {
        ofstream console_out("CONOUT$");
        ifstream console_in("CONIN$");
        console_out << "Server IP: " << flush;
        getline(console_in, ip);
    }

    FreeConsole();

    HWND hwnd = nullptr;
    InitWindow(hInstance, nCmdShow, &hwnd);

    Engine& game = ENGINE;
    game.Initialize(hwnd, ip.c_str(), 7000, listener);

    TIMER.Reset();

    MSG msg{};

    while (true)
    {
        INPUT.Renew();
        TIMER.Update();
        const float deltatime = TIMER.GetDeltaTime();

        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
            {
                game.Shutdown();
                timeEndPeriod(1);

                if (SUCCEEDED(comInit)) 
                    CoUninitialize();   

                return 0;
            }
        }

        game.Update(deltatime);
        game.Render();

        LimitFrameRate(IMGUI.GetFrameLimitFps());
    }

    timeEndPeriod(1);
    return 0;
}

void InitWindow(HINSTANCE hInstance, const int nCmdShow, HWND* hwnd)
{
    ASSERT(hInstance != nullptr);

    /*WinSize.x = GetSystemMetrics(SM_CXSCREEN);
    WinSize.y = GetSystemMetrics(SM_CYSCREEN);*/

    WinSize.x = 1920;
    WinSize.y = 1080;

    RECT rc = { 0, 0, WinSize.x, WinSize.y };
    AdjustWindowRect(&rc, WS_POPUP, FALSE);

    const TCHAR* appName = _T("Final");

    HCURSOR customCursor = CreateCursorFromPng(L"../Assets/UI/Textures/Cursor.png", 48, 0.006f, 0.012f);

    WNDCLASSEXW wcex = {
        .cbSize = sizeof(WNDCLASSEX),
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = WndProc,
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .hInstance = hInstance,
        .hIcon = nullptr,
        .hCursor = customCursor ? customCursor : LoadCursor(nullptr, IDC_ARROW),
        .hbrBackground = nullptr,
        .lpszMenuName = nullptr,
        .lpszClassName = appName,
        .hIconSm = nullptr
    };

    if (not RegisterClassExW(&wcex)) {
        MASSERT(false, "Regiser failed!");
    }

    /**hwnd = CreateWindow(wcex.lpszClassName, wcex.lpszClassName,
        WS_POPUP,
        0, 0, WinSize.x, WinSize.y,
        nullptr, nullptr, hInstance, nullptr);*/

    *hwnd = CreateWindow(wcex.lpszClassName, wcex.lpszClassName,
        WS_POPUP,
        0, 0, rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hInstance, nullptr);

    ShowWindow(*hwnd, nCmdShow);
}

void LimitFrameRate(int fpsCap)
{
    static auto nextFrame = chrono::steady_clock::now();
    if (fpsCap > 0)
    {
        nextFrame += chrono::duration_cast<chrono::steady_clock::duration>(
            chrono::duration<double>(1.0 / fpsCap));
        const auto now = chrono::steady_clock::now();
        if (now < nextFrame)
        {
            const auto spinMargin = std::chrono::milliseconds(1);
            if (nextFrame - now > spinMargin)
                this_thread::sleep_until(nextFrame - spinMargin);    
            while (chrono::steady_clock::now() < nextFrame) {}       
        }
        else nextFrame = now;
    }
    else nextFrame = chrono::steady_clock::now();
}

//void InitWindow(HINSTANCE hInstance, const int nCmdShow, HWND* hwnd)
//{
//    ASSERT(hInstance != nullptr);
//
//    /*WinSize.x = GetSystemMetrics(SM_CXSCREEN);
//    WinSize.y = GetSystemMetrics(SM_CYSCREEN);*/
//
//    WinSize.x = 1280;
//    WinSize.y = 720;
//
//    // 타이틀바로 드래그 이동은 가능하게 하되, 리사이즈/최대화는 막아 클라이언트 해상도를 고정한다.
//    constexpr DWORD windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
//
//    RECT rc = { 0, 0, WinSize.x, WinSize.y };
//    AdjustWindowRect(&rc, windowStyle, FALSE);
//
//    const TCHAR* appName = _T("Final");
//
//    WNDCLASSEXW wcex = {
//        .cbSize = sizeof(WNDCLASSEX),
//        .style = CS_HREDRAW | CS_VREDRAW,
//        .lpfnWndProc = WndProc,
//        .cbClsExtra = 0,
//        .cbWndExtra = 0,
//        .hInstance = hInstance,
//        .hIcon = nullptr,
//        .hCursor = LoadCursor(nullptr, IDC_ARROW),
//        .hbrBackground = nullptr,
//        .lpszMenuName = nullptr,
//        .lpszClassName = appName,
//        .hIconSm = nullptr
//    };
//
//    if (not RegisterClassExW(&wcex)) {
//        MASSERT(false, "Regiser failed!");
//    }
//
//    /**hwnd = CreateWindow(wcex.lpszClassName, wcex.lpszClassName,
//        WS_POPUP,
//        0, 0, WinSize.x, WinSize.y,
//        nullptr, nullptr, hInstance, nullptr);*/
//
//    *hwnd = CreateWindow(wcex.lpszClassName, wcex.lpszClassName,
//        windowStyle,
//        CW_USEDEFAULT, 0, rc.right - rc.left, rc.bottom - rc.top,
//        nullptr, nullptr, hInstance, nullptr);
//
//    ShowWindow(*hwnd, nCmdShow);
//}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    bool imguiWantsKeyboard = false;
    bool imguiWantsMouse = false;

    if (message == WM_MOUSEMOVE)
    {
        RECT cr; GetClientRect(hWnd, &cr);
        const int cw = cr.right - cr.left;
        const int ch = cr.bottom - cr.top;
        if (cw > 0 && ch > 0 && (cw != WinSize.x || ch != WinSize.y))
        {
            const int sx = MulDiv(static_cast<short>(LOWORD(lParam)), WinSize.x, cw);
            const int sy = MulDiv(static_cast<short>(HIWORD(lParam)), WinSize.y, ch);
            lParam = MAKELPARAM(sx, sy);
        }
    }

    if (ImGui::GetCurrentContext() != nullptr)
    {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
            return true;

        ImGuiIO& io = ImGui::GetIO();
        imguiWantsKeyboard = io.WantCaptureKeyboard;
        imguiWantsMouse = io.WantCaptureMouse;
    }

    switch (message)
    {
    case WM_KEYDOWN:
        if (wParam == VK_F12 && ImGui::GetCurrentContext() != nullptr) {
            const bool enabled = !IMGUI.IsEnabled();
            IMGUI.SetEnabled(enabled);
            if (Scene* scene = SCENE_MANAGER->GetCurrentScene())
                if (Camera* camera = scene->GetCamera())
                    if (camera->IsCursorActive() != enabled)
                        camera->SetCursor(enabled);
            return 0;
        }
        [[fallthrough]];
    case WM_KEYUP:
        if (!imguiWantsKeyboard)
            INPUT.SetKey(static_cast<size_t>(wParam), static_cast<bool>(WM_KEYUP - message));
        return 0;

    case WM_MOUSEMOVE:
        if (!imguiWantsMouse) 
            INPUT.SetMousePosition(XMFLOAT2(static_cast<float>(static_cast<short>(LOWORD(lParam))), static_cast<float>(static_cast<short>(HIWORD(lParam)))));
        return 0;

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
        if (!imguiWantsMouse)
            INPUT.SetMouseButton(MouseButton::LEFT, static_cast<bool>(WM_LBUTTONUP - message));
        return 0;

    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        if (!imguiWantsMouse)
            INPUT.SetMouseButton(MouseButton::RIGHT, static_cast<bool>(WM_RBUTTONUP - message));
        return 0;

    case WM_MOUSEWHEEL:
        if (!imguiWantsMouse) {
            const short delta = GET_WHEEL_DELTA_WPARAM(wParam);
            INPUT.SetMouseWheelDelta((int)delta);
        }
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
    }
    break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

HCURSOR CreateCursorFromPng(const wchar_t* path, int maxSize, float hotspotU, float hotspotV)
{
    using Microsoft::WRL::ComPtr;

    ComPtr<IWICImagingFactory> factory;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr,
        CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))))
        return nullptr;

    ComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(factory->CreateDecoderFromFilename(path, nullptr, GENERIC_READ,
        WICDecodeMetadataCacheOnLoad, &decoder)))
        return nullptr;

    ComPtr<IWICBitmapFrameDecode> frame;
    if (FAILED(decoder->GetFrame(0, &frame)))
        return nullptr;

    UINT srcW = 0, srcH = 0;
    frame->GetSize(&srcW, &srcH);
    if (srcW == 0 || srcH == 0)
        return nullptr;

    const UINT srcMax = (srcW > srcH) ? srcW : srcH;
    const float scale = static_cast<float>(maxSize) / static_cast<float>(srcMax);
    int w = static_cast<int>(srcW * scale); if (w < 1) w = 1;
    int h = static_cast<int>(srcH * scale); if (h < 1) h = 1;

    ComPtr<IWICBitmapScaler> scaler;
    if (FAILED(factory->CreateBitmapScaler(&scaler)))
        return nullptr;
    scaler->Initialize(frame.Get(), w, h, WICBitmapInterpolationModeFant);

    ComPtr<IWICFormatConverter> converter;
    if (FAILED(factory->CreateFormatConverter(&converter)))
        return nullptr;
    converter->Initialize(scaler.Get(), GUID_WICPixelFormat32bppPBGRA,
        WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h;     
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP hColor = CreateDIBSection(nullptr, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!hColor)
        return nullptr;

    if (FAILED(converter->CopyPixels(nullptr, w * 4, w * h * 4, static_cast<BYTE*>(bits))))
    {
        DeleteObject(hColor);
        return nullptr;
    }

    HBITMAP hMask = CreateBitmap(w, h, 1, 1, nullptr);

    ICONINFO ii = {};
    ii.fIcon = FALSE;   
    ii.xHotspot = static_cast<DWORD>(hotspotU * w);
    ii.yHotspot = static_cast<DWORD>(hotspotV * h);
    ii.hbmColor = hColor;
    ii.hbmMask = hMask;

    HCURSOR cursor = static_cast<HCURSOR>(CreateIconIndirect(&ii));

    DeleteObject(hColor);
    DeleteObject(hMask);
    return cursor;
}