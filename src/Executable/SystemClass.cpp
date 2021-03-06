#include "stdafx.h"
#include "SystemClass.h"
#include "resource.h"
using std::wstring;

static LRESULT CALLBACK WndProc_temp(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static wstring LoadString(_In_opt_ HINSTANCE InstanceHandle, _In_ UINT uID);

static double getTimerFreq()
{
    LARGE_INTEGER freq;
    FALSE_ERROR( QueryPerformanceFrequency(&freq) );
    return static_cast<double>( freq.QuadPart );
}

static double getTimerCount()
{
    LARGE_INTEGER count;
    FALSE_ERROR( QueryPerformanceCounter(&count) );
    return static_cast<double>( count.QuadPart );
}



SystemClass::SystemClass(_In_ HINSTANCE InstanceHandle, _In_ size_t Width, _In_ size_t Height, _In_ bool FullScreen) :
    instanceHandle ( InstanceHandle ),
    displaySettingChanged ( FullScreen ),
    freq( getTimerFreq() ), last( 0.0 )
{
#pragma region initialize windowHandle
    // get the application title and name of the class from the Executable.rc
    wstring Title = LoadString(instanceHandle, IDS_APP_TITLE);
    wstring ClassName = LoadString(instanceHandle, IDC_EXECUTABLE);

    // get the width and height of primary display monitor
    int DisplayWidth, DisplayHeight;
    FALSE_WARNING( DisplayWidth = GetSystemMetrics(SM_CXSCREEN) );
    FALSE_WARNING( DisplayHeight = GetSystemMetrics(SM_CYSCREEN) );
    if ( !DisplayWidth ) DisplayWidth = Width;
    if ( !DisplayHeight ) DisplayHeight = Height;

    int screenWidth, screenHeight, posX, posY;
    DWORD windowStyle, windowExStyle;

    // set the basic window information
    WNDCLASSEX wcex = {};
    wcex.style 				= CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc		= WndProc_temp;
    wcex.lpszClassName	= ClassName.c_str();
    wcex.hInstance			= instanceHandle;
    wcex.hIcon = wcex.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
    wcex.hCursor			= LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground	= (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.cbWndExtra		= sizeof(this);
    wcex.cbSize 				= sizeof(wcex);
    FALSE_ERROR( RegisterClassEx(&wcex) );

    // set the screen settings depending on whether it is running in full screen or not
    if ( FullScreen )
    {
        // set the size of the screen to size of display
        screenWidth = DisplayWidth;
        screenHeight = DisplayHeight;
        // set the position of the window to the top left corner
        posX = 0;
        posY = 0;
        // set the style of the window
        windowExStyle = WS_EX_APPWINDOW;
        windowStyle = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

        // change the display settings to full screen
        DEVMODE ScreenSettings = {};
        ScreenSettings.dmSize = sizeof(DEVMODE);
        ScreenSettings.dmPelsWidth = (unsigned long)screenWidth;
        ScreenSettings.dmPelsHeight = (unsigned long)screenHeight;
        ScreenSettings.dmBitsPerPel = 32;
        ScreenSettings.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
        LONG result;
        NONZERO_ERROR( result = ChangeDisplaySettings(&ScreenSettings, CDS_FULLSCREEN) );
    }
    else
    {
        // set the size of the screen to default size
        screenWidth = Width;
        screenHeight = Height;
        // set the position of the window to the center of the display
        posX = ( DisplayWidth - screenWidth ) / 2;
        posY = ( DisplayHeight - screenHeight ) / 2;
        // set the style of the window
        windowExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
        windowStyle = WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    }
    FALSE_ERROR( windowHandle = CreateWindowEx( windowExStyle, ClassName.c_str(), Title.c_str(), windowStyle,
        posX, posY, screenWidth, screenHeight, NULL, NULL, instanceHandle, this) );
#pragma endregion

    // initialize game object
    game.reset(new GameClass(windowHandle, screenWidth, screenHeight, FullScreen));
}

SystemClass::~SystemClass()
{
    // discard game object
    game.reset();

    if ( displaySettingChanged )
    {
        NONZERO_WARNING( ChangeDisplaySettings(nullptr, 0) );
    }

    if ( IsWindow(windowHandle) )
    {
        FALSE_WARNING( DestroyWindow(windowHandle) );
        windowHandle = nullptr;
    }

    if ( instanceHandle )
    {
        wstring ClassName = LoadString(instanceHandle, IDC_EXECUTABLE);
        FALSE_WARNING( UnregisterClass(ClassName.c_str(), instanceHandle) );
    }
}

void SystemClass::Run(int ShowCommand)
{
    ShowWindow(windowHandle, ShowCommand);
    FALSE_WARNING( SetForegroundWindow(windowHandle) );
    FALSE_WARNING( SetFocus(windowHandle) );

    MSG message = {};

    while( true )
    {
        while ( PeekMessage(&message, nullptr, 0, 0, PM_REMOVE) )
        {
            if ( message.message == WM_QUIT ) goto onQuit;
            DispatchMessage(&message);
        }

        onIdle();

        // TODO : 윈도우가 비활성화되었을때의 처리가 여기에 들어가야한다
        // WaitMessage();
    }
onQuit:
    return;
}

LRESULT CALLBACK SystemClass::MessageHandler(HWND WindowHandle, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch ( Message )
    {
    case WM_PAINT:
        onIdle();
        ValidateRect(WindowHandle, nullptr);
        return 0;
    case WM_KEYDOWN:
        game->onKeyDown(wParam);
        return 0;
    case WM_KEYUP:
        game->onKeyUp(wParam);
        return 0;
    case WM_LBUTTONDOWN:
        game->onLButtonDown(LOWORD(lParam), LOWORD(lParam));
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(WindowHandle, Message, wParam, lParam);
    }
}

void SystemClass::onIdle()
{
    double now = getTimerCount();
    if ( last == 0.0 ) last = now;
    game->onFrame( (now-last)/freq );
    last = now;

    game->onDraw();
}



// temporary window procedure of new window
static LRESULT CALLBACK WndProc_temp(HWND WindowHandle, UINT Message, WPARAM wParam, LPARAM lParam)
{
    // it change the window's WndProc when it set GWL_USERDATA to address of a SystemClass object
    if ( Message == WM_NCCREATE )
    {
        LPCREATESTRUCT CreateStruct = reinterpret_cast<LPCREATESTRUCT>( lParam );
        SetWindowLongPtr(WindowHandle, GWL_USERDATA, reinterpret_cast<LONG_PTR>( CreateStruct->lpCreateParams ));
        SetWindowLongPtr(WindowHandle, GWL_WNDPROC, reinterpret_cast<LONG_PTR>( WndProc ));
        return WndProc(WindowHandle, Message, wParam, lParam);
    }

    return DefWindowProc(WindowHandle, Message, wParam, lParam);
}

// main window procedure
static LRESULT CALLBACK WndProc(HWND WindowHandle, UINT Message, WPARAM wParam, LPARAM lParam)
{
    SystemClass* system = reinterpret_cast<SystemClass*>( GetWindowLongPtr(WindowHandle, GWL_USERDATA) );
    return system->MessageHandler(WindowHandle, Message, wParam, lParam);
}

wstring LoadString(_In_opt_ HINSTANCE InstanceHandle, _In_ UINT uID)
{
    const wchar_t* src; int len;
    FALSE_ERROR( len = LoadString(InstanceHandle, uID, reinterpret_cast<LPWSTR>( &src ), 0) );
    return wstring(src, len);
}