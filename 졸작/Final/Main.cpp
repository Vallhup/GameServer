#include "pch.h"
#include "Input.h"
#include "Timer.h"
#include "Engine.h"
#include "imgui.h"
#include "ImGuiManager.h"
#include "ClientConnectionListener.h"

static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static void InitWindow(HINSTANCE hInstance, const int nCmdShow, HWND* hwnd);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    HWND hwnd = nullptr;
    InitWindow(hInstance, nCmdShow, &hwnd);

    GET(Timer).Initialize();

    ClientConnectionListener listener;

    Engine& game = GET(Engine);
    game.Initialize(hwnd, "127.0.0.1", 7000, listener);

    MSG msg{};

    while (true)
    {
        GET(Input).Renew();
        GET(Timer).Update();
        const float deltatime = GET(Timer).GetDeltaTime();

        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
            {            
                game.Shutdown();
                return 0;
            }
        }

        game.Update(deltatime);
        game.Render();
    }

    return 0;
}

void InitWindow(HINSTANCE hInstance, const int nCmdShow, HWND* hwnd)
{
    ASSERT(hInstance != nullptr);

    WinSize.x = 1280;
    WinSize.y = 720;

    RECT rc = { 0, 0, WinSize.x, WinSize.y };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    const TCHAR* appName = _T("Final");

    WNDCLASSEXW wcex = {
        .cbSize = sizeof(WNDCLASSEX),
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = WndProc,
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .hInstance = hInstance,
        .hIcon = nullptr,
        .hCursor = LoadCursor(nullptr, IDC_ARROW),
        .hbrBackground = nullptr,
        .lpszMenuName = nullptr,
        .lpszClassName = appName,
        .hIconSm = nullptr
    };

    if (not RegisterClassExW(&wcex)) {
        MASSERT(false, "Regiser failed!");
    }

    *hwnd = CreateWindow(wcex.lpszClassName, wcex.lpszClassName,
        WS_OVERLAPPEDWINDOW,
        100, 100, rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hInstance, nullptr);

    ShowWindow(*hwnd, nCmdShow);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    bool imguiWantsKeyboard = false;
    bool imguiWantsMouse = false;

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
        if (wParam == VK_ESCAPE) {
            DestroyWindow(hWnd);
            return 0;
        }
        if (wParam == VK_F1 && ImGui::GetCurrentContext() != nullptr) {
            GET(ImGuiManager).SetEnabled(!GET(ImGuiManager).IsEnabled());
            return 0;
        }
        [[fallthrough]];
    case WM_KEYUP:
        if (!imguiWantsKeyboard)
            GET(Input).SetKey(static_cast<size_t>(wParam), static_cast<bool>(WM_KEYUP - message));
        return 0;

    case WM_MOUSEMOVE:
        if (!imguiWantsMouse)
            GET(Input).SetMousePosition(XMFLOAT2(static_cast<float>(LOWORD(lParam)), static_cast<float>(HIWORD(lParam))));
        return 0;

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
        if (!imguiWantsMouse)
            GET(Input).SetMouseButton(MouseButton::LEFT, static_cast<bool>(WM_LBUTTONUP - message));
        return 0;

    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        if (!imguiWantsMouse)
            GET(Input).SetMouseButton(MouseButton::RIGHT, static_cast<bool>(WM_RBUTTONUP - message));
        return 0;

    case WM_MOUSEWHEEL:
        if (!imguiWantsMouse) {
            const short delta = GET_WHEEL_DELTA_WPARAM(wParam);
            GET(Input).SetMouseWheelDelta((int)delta);
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