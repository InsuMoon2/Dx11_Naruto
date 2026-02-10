#include "pch.h"
#include "Client.h"

#include "MainApp.h"
#include "GameInstance.h"

#include <locale.h>
#include <tchar.h>

FILE* debug;

#define MAX_LOADSTRING 100

// 전역 변수:
HWND g_hWnd;
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

namespace Client
{
    unsigned int    g_winSizeX = 1600;
    unsigned int    g_winSizeY = 900;
}

struct LaunchParams
{
    int32 playerIndex = 0;
    int32 windowX = CW_USEDEFAULT;
    int32 windowY = CW_USEDEFAULT;
    int32 windowWidth  = g_winSizeX;   
    int32 windowHeight = g_winSizeY;
};

LaunchParams ParseCommandLine(LPWSTR lpCmdLine)
{
    LaunchParams params;
    wstring cmdLine(lpCmdLine);

    // --player0
    size_t playerPos = cmdLine.find(L"--player");
    if (playerPos != wstring::npos)
        params.playerIndex = _wtoi(cmdLine.c_str() + playerPos + 8);

    // --x100
    size_t xPos = cmdLine.find(L"--x");
    if (xPos != wstring::npos && (xPos == 0 || cmdLine[xPos - 1] == L' '))
        params.windowX = _wtoi(cmdLine.c_str() + xPos + 3);

    // --y100
    size_t yPos = cmdLine.find(L"--y");
    if (yPos != wstring::npos && (yPos == 0 || cmdLine[yPos - 1] == L' '))
        params.windowY = _wtoi(cmdLine.c_str() + yPos + 3);

    // --width800
    size_t widthPos = cmdLine.find(L"--width");
    if (widthPos != wstring::npos)
        params.windowWidth = _wtoi(cmdLine.c_str() + widthPos + 7);

    // --height600
    size_t heightPos = cmdLine.find(L"--height");
    if (heightPos != wstring::npos)
        params.windowHeight = _wtoi(cmdLine.c_str() + heightPos + 8);

    return params;
}

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int, const LaunchParams&);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    //UNREFERENCED_PARAMETER(lpCmdLine);

    LPWSTR fullCmdLine = GetCommandLineW();

    wstring cmdStr(fullCmdLine);
    size_t firstSpace = cmdStr.find(L' ');
    wstring args = (firstSpace != wstring::npos) ? cmdStr.substr(firstSpace + 1) : L"";

    LaunchParams params = ParseCommandLine(const_cast<LPWSTR>(args.c_str()));

    g_winSizeX = params.windowWidth;
    g_winSizeY = params.windowHeight;

    // TODO: 여기에 코드를 입력합니다.
    unique_ptr<MainApp> mainApp = { nullptr };

    // 전역 문자열을 초기화합니다.
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CLIENT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 애플리케이션 초기화를 수행합니다:
    if (!InitInstance (hInstance, nCmdShow, params))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CLIENT));

    MSG msg;

    mainApp = MainApp::Create();
    CHECK_NULL(mainApp, FALSE);


    GAME->Add_Timer(L"Timer_Default");
    GAME->Add_Timer(L"Timer_60FPS");

    float   timeAcc = { };

    // 기본 메시지 루프입니다:
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
            mainApp->Priority_Update(GAME->Compute_TimeDelta(L"Timer_60FPS"));
            mainApp->Update(GAME->Compute_TimeDelta(L"Timer_60FPS"));
            mainApp->Late_Update(GAME->Compute_TimeDelta(L"Timer_60FPS"));
            mainApp->Render();

            timeAcc = 0.f;
        }
    }

    mainApp->Free();
    mainApp.reset();

    return (int) msg.wParam;
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
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CLIENT));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_CLIENT);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, const LaunchParams& params)
{
   hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

   wstring windowTitle = wstring(szTitle) + L" - Player " + to_wstring(params.playerIndex + 1);

   RECT rc = { 0, 0, static_cast<LONG>(params.windowWidth), static_cast<LONG>(params.windowHeight) };

   AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

   int windowWidth = rc.right - rc.left;
   int windowHeight = rc.bottom - rc.top;

   int posX = params.windowX;
   int posY = params.windowY;


   if (posX == CW_USEDEFAULT || posY == CW_USEDEFAULT)
   {
       int screenWidth = GetSystemMetrics(SM_CXSCREEN);
       int screenHeight = GetSystemMetrics(SM_CYSCREEN);
       posX = (screenWidth - windowWidth) / 2;
       posY = (screenHeight - windowHeight) / 2;
   }

   HWND hWnd = CreateWindowW(
       szWindowClass,
       windowTitle.c_str(),
       WS_OVERLAPPEDWINDOW,
       posX,
       posY,
       windowWidth,
       windowHeight,
       nullptr, nullptr, hInstance, nullptr);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   g_hWnd = hWnd;

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
#pragma region Debug Log
    case WM_CREATE:
    {
        AllocConsole();
        SetConsoleOutputCP(CP_UTF8);  // UTF-8 출력
        SetConsoleCP(CP_UTF8);        // UTF-8 입력

        HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);
        DWORD mode;
        GetConsoleMode(hConsole, &mode);
        SetConsoleMode(hConsole, mode & ~ENABLE_QUICK_EDIT_MODE);

        HWND consoleWindow = GetConsoleWindow();
        MoveWindow(consoleWindow, 000, 250, 500, 300, TRUE);

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
    break;
        
#pragma endregion

    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 메뉴 선택을 구문 분석합니다:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 여기에 hdc를 사용하는 그리기 코드를 추가합니다...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
