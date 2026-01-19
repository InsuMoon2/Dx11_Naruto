#pragma once

#include <Windows.h>

extern HWND g_hWnd;
extern HINSTANCE g_hInst;


namespace Client
{
    const unsigned int		g_winSizeX = { 1280 };
    const unsigned int		g_winSizeY = { 720 };

    enum class LEVEL { STATIC, LOADING, LOGO, GAMEPLAY, END };
}

using namespace Client;
