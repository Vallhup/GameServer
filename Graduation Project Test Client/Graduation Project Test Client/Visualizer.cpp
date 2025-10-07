#include "Visualizer.h"
#include "Service.h"

typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int interval);
typedef int (WINAPI* PFNWGLGETSWAPINTERVALEXTPROC)(void);

Visualizer::Visualizer(int width, int height, bool isFull)
	: _hWnd(nullptr), _hDC(nullptr), _hRC(nullptr), _active(true), _isFull(isFull), _base(0)
{
    _hInstance = GetModuleHandle(nullptr);
    WNDCLASSEXW wcex = {
        .cbSize = sizeof(WNDCLASSEX),
        .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc = VisulizerProc,
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .hInstance = _hInstance,
        .hIcon = nullptr,
        .hCursor = LoadCursor(nullptr, IDC_ARROW),
        .hbrBackground = nullptr,
        .lpszMenuName = nullptr,
        .lpszClassName = WINDOW_NAME.data(),
        .hIconSm = nullptr
    };
    RegisterClassEx(&wcex);

    DWORD dwExStyle;
    DWORD dwStyle;

    if (_isFull) {
        dwExStyle = WS_EX_APPWINDOW;
        dwStyle = WS_POPUP;
        ShowCursor(FALSE);
    }

    else {
        dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
        dwStyle = WS_OVERLAPPEDWINDOW;
    }

    RECT windowRect{ 0, 0, width, height };
    AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

    _hWnd = CreateWindowEx(dwExStyle, wcex.lpszClassName, wcex.lpszClassName,
        dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        0, 0, width, height, NULL, NULL, _hInstance, NULL);
    SetWindowLongPtr(_hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR), 1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA, 16,
        0, 0, 0, 0, 0, 0,							// Color Bits Ignored
        0,											// No Alpha Buffer
        0,											// Shift Bit Ignored
        0,											// No Accumulation Buffer
        0, 0, 0, 0,									// Accumulation Bits Ignored
        16,											// 16Bit Z-Buffer (Depth Buffer)  
        0,											// No Stencil Buffer
        0,											// No Auxiliary Buffer
        PFD_MAIN_PLANE,								// Main Drawing Layer
        0,											// Reserved
        0, 0, 0										// Layer Masks Ignored
    };

    _hDC = GetDC(_hWnd);
    int pixelFormat = ChoosePixelFormat(_hDC, &pfd);
    SetPixelFormat(_hDC, pixelFormat, &pfd);

    _hRC = wglCreateContext(_hDC);
    wglMakeCurrent(_hDC, _hRC);

    ShowWindow(_hWnd, SW_SHOW);
    UpdateWindow(_hWnd);
    ResizeGLWindow(width, height);

    InitOpenGL();
}

Visualizer::~Visualizer()
{
    KillGLWindow();
    Stop();
}

void Visualizer::Start()
{
    /*bool expected{ false };
    if (_running.compare_exchange_strong(expected, true)) {
        _renderThread = std::thread([this, &width, &height]()
            {
                Update();
            });
    }*/
}

void Visualizer::Stop()
{
}

void Visualizer::Render()
{
    UpdateClientPositions(Service::Instance().GetClientManager().GetClientList());

    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    glColor3f(1.0f, 1.0f, 0.0f);
    glRasterPos2f(50.0f, -275.0f);
    glPrint("Active Clients : [%d]", _clientSnapshot.size());
        
    glColor3f(1.0f, 1.0f, 1.0f);
    glPointSize(3.0f);

    glBegin(GL_POINTS);
    for (auto& client : _clientSnapshot) {
        const vec3 glPos = client->GetPos();
        glVertex2f(glPos.x, glPos.z);
    }
    glEnd();

    SwapBuffers(_hDC);
}

void Visualizer::UpdateClientPositions(const std::vector<std::shared_ptr<Client>>& clients)
{
	std::lock_guard<std::mutex> lock(_clientMutex);
    _clientSnapshot = clients;
}

void Visualizer::ResizeGLWindow(GLsizei width, GLsizei height)
{
    if (height == 0) height = 1;

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

	const float aspect = static_cast<float>(width) / static_cast<float>(height);
    glOrtho(-300.0f, 300.0f, -300.0f, 300.0f, -1.0f, 1.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void Visualizer::InitOpenGL()
{
    glShadeModel(GL_SMOOTH);
    glClearColor(0, 0, 0, 1.0f);
    glDisable(GL_DEPTH_TEST);

    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT =
        (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

    if (wglSwapIntervalEXT) {
        wglSwapIntervalEXT(1);
    }

    BuildFont();
}

void Visualizer::BuildFont()
{
    HFONT	font;										
    HFONT	oldfont;								

    _base = glGenLists(96);							

    font = CreateFont(-24, 0, 0, 0,								
        FW_BOLD, FALSE, FALSE, FALSE,							
        ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,			
        ANTIALIASED_QUALITY, FF_DONTCARE | DEFAULT_PITCH,	
        L"Courier New");				

    oldfont = (HFONT)SelectObject(_hDC, font);          
    wglUseFontBitmaps(_hDC, 32, 96, _base);				
    SelectObject(_hDC, oldfont);						
    DeleteObject(font);									
}

void Visualizer::glPrint(const char* fmt, ...)
{
    char		text[256];						
    va_list		ap;								

    if (fmt == NULL)							
        return;									

    va_start(ap, fmt);							
    vsprintf_s(text, fmt, ap);					
    va_end(ap);									

    glPushAttrib(GL_LIST_BIT);					
    glListBase(_base - 32);						
    glCallLists((GLsizei)strlen(text), GL_UNSIGNED_BYTE, text);
    glPopAttrib();
}

void Visualizer::KillGLWindow()
{
    if (_isFull) {
        ChangeDisplaySettings(NULL, 0);
        ShowCursor(TRUE);
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(_hRC);

    ReleaseDC(_hWnd, _hDC);

    DestroyWindow(_hWnd);

    UnregisterClass(WINDOW_NAME.data(), _hInstance);

    glDeleteLists(_base, 96);
}

LRESULT Visualizer::VisulizerProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_SIZE: {
        if (auto vis = reinterpret_cast<Visualizer*>(GetWindowLongPtr(hWnd, GWLP_USERDATA))) {
            vis->ResizeGLWindow(LOWORD(lParam), HIWORD(lParam));
        }
        return 0;
    }
    case WM_CLOSE:
    case WM_DESTROY:
        Service::Instance().Stop();
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}
