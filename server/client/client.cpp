#include "pch.h"

char SERVER_ADDR[32] = {};

#define ID_EDIT 101
#define ID_BUTTON 102

const WCHAR* BOARD_IMAGE = L"chessboard.jpg";
const WCHAR* PIECE_IMAGE = L"chesspiece.png";

// GDI+ 초기화 및 종료를 위한 변수
ULONG_PTR gdiplusToken;

// GDI+ 초기화
void InitGdiPlus()
{
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}

// GDI+ 종료
void ShutdownGdiPlus()
{
    Gdiplus::GdiplusShutdown(gdiplusToken);
}

// 윈도우 프로시저 함수 선언
//LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    {
        //_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    //WSADATA WSAData;
    //if (WSAStartup(MAKEWORD(2, 2), &WSAData))
    //    return 0;

    //InitGdiPlus();

    //WNDCLASS wc = { 0 };
    //wc.lpfnWndProc = WindowProcedure; // 윈도우 프로시저 지정
    //wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    //wc.hInstance = hInstance;
    //wc.lpszClassName = TEXT("Game Server Programming Client Windodw Class");
    //wc.hCursor = LoadCursor(NULL, IDC_ARROW);  // 기본 커서 설정
    //wc.style = CS_HREDRAW | CS_VREDRAW;

    //// 클래스 등록
    //if (!RegisterClass(&wc)) {
    //    MessageBox(NULL, TEXT("Window Registration Failed!"), TEXT("Error"), MB_ICONERROR);
    //    return 0;
    //}

    //// 윈도우 생성
    //HWND hwnd = CreateWindow(
    //    wc.lpszClassName, // 윈도우 클래스 이름
    //    TEXT("Game Server Programming"), // 윈도우 제목
    //    WS_OVERLAPPEDWINDOW, // 윈도우 스타일
    //    CW_USEDEFAULT, CW_USEDEFAULT, WIN_X_SIZE, WIN_Y_SIZE, // 위치와 크기
    //    NULL, NULL, hInstance, NULL // 부모 창, 메뉴, 인스턴스 핸들
    //);

    //// 윈도우 생성에 실패한 경우
    //if (hwnd == NULL) {
    //    MessageBox(NULL, TEXT("Window Creation Failed!"), TEXT("Error"), MB_ICONERROR);
    //    return 0;
    //}

    //// 윈도우 보이기
    //ShowWindow(hwnd, nCmdShow);
    //UpdateWindow(hwnd);

    //// 메시지 루프
    //MSG Message;
    //while (GetMessage(&Message, NULL, 0, 0)) {
    //    TranslateMessage(&Message); // 키보드 메시지 처리
    //    DispatchMessage(&Message);  // 윈도우 프로시저로 메시지 전달
    //}

    //ShutdownGdiPlus();

    //return static_cast<int>(Message.wParam);
    }
    
    gClientCore->Init(hInstance, nCmdShow);
    gClientCore->MainLoop();
    gClientCore->Shutdown();

    return static_cast<int>(gClientCore->GetClientMessage().wParam);
}

RECT WinSize;

SOCKET clientSocket;
SOCKADDR_IN serverAddr;

std::unordered_map<RenderComponent*, const WCHAR*> renderComponents;

HWND hEdit, hButton;

// 윈도우 프로시저: 메시지를 처리하는 함수
//LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
//    static HDC hdc;
//    static HDC hdcMemCompatible; // double buffering을 위한 memory DC
//
//    HBITMAP hBitmap;
//
//    PAINTSTRUCT ps;
//
//    char recvBuffer[512];
//
//    WSABUF recvWsabuf[1];
//    DWORD recvBytes;
//    DWORD recvFlag = 0;
//
//    int32_t tempPos[2] = {};
//    size_t recvTotalBytes = sizeof(tempPos);
//
//    static int pieceXpos{ 1 }, pieceYpos{ 1 };
//
//    switch (msg) {
//    case WM_CREATE:
//        hEdit = CreateWindow(TEXT("EDIT"), TEXT(""),
//            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
//            10, 10, 200, 25, hwnd, (HMENU)ID_EDIT, NULL, NULL);
//
//        // 버튼 생성
//        hButton = CreateWindow(TEXT("BUTTON"), TEXT("확인"),
//            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
//            220, 10, 80, 25, hwnd, (HMENU)ID_BUTTON, NULL, NULL);
//
//        break;
//
//    case WM_COMMAND:
//        if (LOWORD(wp) == ID_BUTTON)
//        {
//            wchar_t inputAddress[32];
//            GetWindowText(hEdit, inputAddress, 32);
//
//            WideCharToMultiByte(CP_ACP, 0, inputAddress, -1, SERVER_ADDR, sizeof(SERVER_ADDR), NULL, NULL);
//
//            if (strlen(SERVER_ADDR) == 0)
//                MessageBoxA(hwnd, "서버 주소를 입력하세요!", "오류", MB_OK | MB_ICONERROR);
//
//            else {
//                DestroyWindow(hEdit);
//                DestroyWindow(hButton);
//
//                clientSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
//
//                serverAddr.sin_family = AF_INET;
//                serverAddr.sin_port = htons(SERVER_PORT);
//                inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr.s_addr);
//
//                WSAConnect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr),
//                    sizeof(serverAddr), 0, 0, 0, 0);
//
//                // Rendering Components input
//                renderComponents.emplace(std::make_pair(new RenderComponent, BOARD_IMAGE));
//                renderComponents.emplace(std::make_pair(new RenderComponent, PIECE_IMAGE));
//
//                InvalidateRect(hwnd, NULL, FALSE);
//                UpdateWindow(hwnd);
//            }
//        }
//
//        break;
//
//    case WM_PAINT:
//        hdc = BeginPaint(hwnd, &ps);
//
//        // double buffering
//        if (!hdcMemCompatible) {
//            hdcMemCompatible = CreateCompatibleDC(hdc);
//            GetClientRect(hwnd, &WinSize);
//            hBitmap = CreateCompatibleBitmap(hdc, WinSize.right, WinSize.bottom);
//            (HBITMAP)SelectObject(hdcMemCompatible, hBitmap);
//        }
//
//        // Rendering
//        for (const auto& p : renderComponents)
//        {
//            p.first->Init(hdcMemCompatible, p.second);
//
//            if (p.second == PIECE_IMAGE)
//                p.first->Draw(hdcMemCompatible,
//                    33 + ((pieceXpos - 1) * SQUARE_SIZE),
//                    38 + ((pieceYpos - 1) * SQUARE_SIZE),
//                    PIECE_X_SIZE, PIECE_Y_SIZE);
//
//            else
//                p.first->Draw(hdcMemCompatible, 0, 0, BOARD_SIZE, BOARD_SIZE);
//        }
//
//        BitBlt(hdc, 0, 0, 500, 500, hdcMemCompatible, 0, 0, SRCCOPY);
//
//        EndPaint(hwnd, &ps);
//
//        break;
//
//    case WM_KEYDOWN:
//        switch (wp) {
//        case VK_UP:
//        {
//
//            break;
//        }
//
//
//        }
//
//        InvalidateRect(hwnd, NULL, FALSE);
//        UpdateWindow(hwnd);
//
//        break;
//
//    case WM_DESTROY:
//        closesocket(clientSocket);
//        WSACleanup();
//        PostQuitMessage(0); 
//        break;
//
//    default:
//        return DefWindowProc(hwnd, msg, wp, lp);
//    }
//
//    return 0;
//}