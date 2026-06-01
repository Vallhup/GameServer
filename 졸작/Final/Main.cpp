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

static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static void InitWindow(HINSTANCE hInstance, const int nCmdShow, HWND* hwnd);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    TIMER.Initialize();
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

    /*WinSize.x = GetSystemMetrics(SM_CXSCREEN);
    WinSize.y = GetSystemMetrics(SM_CYSCREEN);*/

    WinSize.x = 1920;
    WinSize.y = 1080;

    RECT rc = { 0, 0, WinSize.x, WinSize.y };
    AdjustWindowRect(&rc, WS_POPUP, FALSE);

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
        if (wParam == VK_F1 && ImGui::GetCurrentContext() != nullptr) {
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
            INPUT.SetMousePosition(XMFLOAT2(static_cast<float>(LOWORD(lParam)), static_cast<float>(HIWORD(lParam))));
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