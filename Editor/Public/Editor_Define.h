#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
//#define _HAS_STD_BYTE 0

#include <Windows.h>
// 표준 라이브러리
#include <filesystem>
#include <format>
#include <crtdbg.h>
#include <cstdlib>

// ImGui
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "imgui_internal.h"
#include "implot.h"
#include <imgui-node-editor/imgui_node_editor.h>
#include <ImGuizmo.h>

#include "Client_Defines.h"
#include "EditorInstance.h"
#include "GameInstance.h"

extern HWND g_hWnd;

namespace EditorApp
{
    extern unsigned int g_winSizeX;
    extern unsigned int g_winSizeY;
}

namespace Client {}
namespace Editor {}

using namespace EditorApp;
using namespace Client;
using namespace Editor;

#include <filesystem>
#include <format>

namespace fs = std::filesystem;

#include "Input_Manager.h"
#include "Editor_Macro.h"
#include "Editor_Enum.h"
#include "Editor_Struct.h"

// ============================================
//        메모리 누수 감지
// ============================================
#define _CRTDEBG_MAP_ALLOC
#include <crtdbg.h>
#include <cstdlib>

#ifdef _DEBUG
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif
