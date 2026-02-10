#pragma once

#include <Windows.h>
#include "GameInstance.h"
#include "EditorInstance.h"

//#include "Client_Defines.h"


extern HWND g_hWnd;

namespace EditorApp
{
    extern unsigned int g_winSizeX;
    extern unsigned int g_winSizeY;
    enum class ELevelType { Static, Loading, Logo, GamePlay, END };
}

namespace Client
{ }

using namespace EditorApp;
using namespace Client;

using Message = google::protobuf::Message;

// Network
//#include "Service.h"
//#include "Protocol_Wrapper.h"
//#include "Client_PacketHandler.h"
//#include "NetworkManager.h"

#include <format>
#include <filesystem>

namespace fs = std::filesystem;

#include "Input_Manager.h"

// ============================================
//        메모리 누수 감지
// ============================================
#define _CRTDEBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>
#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif
