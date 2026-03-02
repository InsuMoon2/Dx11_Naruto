#include "pch.h"
#include "framework.h"
#include "Editor.h"

#include <locale.h>
#include <tchar.h>

#include "Editor_MainApp.h"

#define MAX_LOADSTRING 100

FILE* debug;

HWND g_hWnd;
HINSTANCE hInst;                             
WCHAR szTitle[MAX_LOADSTRING];               
WCHAR szWindowClass[MAX_LOADSTRING];         

namespace EditorApp
{
    unsigned int    g_winSizeX = 1600;
    unsigned int    g_winSizeY = 900;
}

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                        _In_opt_ HINSTANCE hPrevInstance,
                        _In_ LPWSTR    lpCmdLine,
                        _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_EDITORAPP, szWindowClass, MAX_LOADSTRING);

    unique_ptr<Editor_MainApp> mainApp = { nullptr };

    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_EDITORAPP));

    MSG msg;
    mainApp = Editor_MainApp::Create();

    if (!mainApp)
        return FALSE;

    GAME->Add_Timer(L"Timer_Default");
    GAME->Add_Timer(L"Timer_60FPS");

    float timeAcc = {};

    // 메인 루프
    while (true)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (WM_QUIT == msg.message)
                break;

            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);

                DispatchMessage(&msg);
            }
        }

        timeAcc += GAME->Compute_TimeDelta(L"Timer_Default");

        if (timeAcc >= 1.f / 60.f)
        {
            float dt = GAME->Compute_TimeDelta(L"Timer_60FPS");

            mainApp->Priority_Update(dt);
            mainApp->Update(dt);
            mainApp->Late_Update(dt);
            mainApp->Render();
            timeAcc = 0.f;
        }
    }

    mainApp->Free();
    mainApp.reset();
    return (int)msg.wParam;
}

//
//  함수: MyRegisterClass()
//
//  용도: 창 클래스를 등록합니다.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_EDITORAPP));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = nullptr;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    RECT rc = { 0, 0, static_cast<LONG>(EditorApp::g_winSizeX), static_cast<LONG>(EditorApp::g_winSizeY) };

    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    int windowWidth = rc.right - rc.left;
    int windowHeight = rc.bottom - rc.top;
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenWidth - windowWidth) / 2;
    int posY = (screenHeight - windowHeight) / 2;

    HWND hWnd = CreateWindowW(
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        posX, posY,
        windowWidth, windowHeight,
        nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    g_hWnd = hWnd;

    return TRUE;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
        return true;

    switch (message)
    {
    case WM_CREATE:
    {
        // 디버그 콘솔
        AllocConsole();
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);
        DWORD mode;
        GetConsoleMode(hConsole, &mode);
        SetConsoleMode(hConsole, mode & ~ENABLE_QUICK_EDIT_MODE);
        HWND consoleWindow = GetConsoleWindow();
        MoveWindow(consoleWindow, 0, 250, 500, 300, TRUE);
        _tfreopen_s(&debug, _T("CONOUT$"), _T("w"), stdout);
        _tfreopen_s(&debug, _T("CONIN$"), _T("r"), stdin);
        _tfreopen_s(&debug, _T("CONERR"), _T("w"), stderr);
        _tsetlocale(LC_ALL, _T(""));
    }
    break;

    case WM_CLOSE:                          
    {
        FreeConsole();
        DestroyWindow(hWnd);
    }

    case WM_MOUSEWHEEL:
        INPUT->Set_MouseWheel(GET_WHEEL_DELTA_WPARAM(wParam) / 120.f);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
