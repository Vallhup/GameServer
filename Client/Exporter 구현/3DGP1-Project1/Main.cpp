#include "pch.h"
#include "Input.h"
#include "Timer.h"
#include "Engine.h"

static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static void InitWindow(HINSTANCE hInstance, const int nCmdShow, HWND* hwnd);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    HWND hwnd = nullptr;
    InitWindow(hInstance, nCmdShow, &hwnd);

    Engine& game = GET(Engine);
    game.Initialize(hwnd);

    GET(Timer).Initialize();

    MSG msg{};

    while (true)
    {
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
        game.Render(deltatime);
    }

    return 0;
}

void InitWindow(HINSTANCE hInstance, const int nCmdShow, HWND* hwnd)
{
    ASSERT(hInstance != nullptr);

    const TCHAR* appName = _T("과제2(2019180051 이정호)");

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

    RECT winRect = { 0, 0, static_cast<LONG>(WinSize.x), static_cast<LONG>(WinSize.y) };
    AdjustWindowRect(&winRect, WS_OVERLAPPEDWINDOW, false);

    *hwnd = CreateWindow(wcex.lpszClassName, wcex.lpszClassName,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        winRect.right - winRect.left, winRect.bottom - winRect.top,
        nullptr, nullptr, hInstance, nullptr);

    ShowWindow(*hwnd, nCmdShow);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_KEYDOWN:
    case WM_KEYUP:
        GET(Input).SetKey(static_cast<size_t>(wParam), static_cast<bool>(WM_KEYUP - message));
        return 0;
    case WM_MOUSEMOVE:
        GET(Input).SetMousePosition(XMFLOAT2(static_cast<float>(LOWORD(lParam)), static_cast<float>(HIWORD(lParam))));
        return 0;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
        GET(Input).SetMouseButton(MouseButton::LEFT, static_cast<bool>(WM_LBUTTONUP - message));
        return 0;
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        GET(Input).SetMouseButton(MouseButton::RIGHT, static_cast<bool>(WM_RBUTTONUP - message));
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