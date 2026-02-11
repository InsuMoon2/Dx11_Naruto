#pragma once

#include "EditorInstance.h"
#include "GameInstance.h"
#include <Windows.h>

#include "Client_Defines.h"

extern HWND g_hWnd;

namespace EditorApp
{
    extern unsigned int g_winSizeX;
    extern unsigned int g_winSizeY;
} 

namespace Client {}

using namespace EditorApp;
using namespace Client;

#include <filesystem>
#include <format>

namespace fs = std::filesystem;

#include "Input_Manager.h"

// ============================================
//        메모리 누수 감지
// ============================================
#define _CRTDEBG_MAP_ALLOC
#include <crtdbg.h>
#include <cstdlib>

#ifdef _DEBUG
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif
