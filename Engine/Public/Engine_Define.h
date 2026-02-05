#pragma once

// DirectX
#include <d3d11.h>
#include <DirectXMath.h>
#include <directxtk/SimpleMath.h>
#include <wrl.h>

#include <directxtk/DDSTextureLoader.h>
#include <directxtk/WICTextureLoader.h>

using namespace DirectX;

// spdlog
#include "spdlog/spdlog.h"
#include "spdlog/sinks/base_sink.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/msvc_sink.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

// STL
#include <memory>
#include <iostream>
#include <array>
#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <ctime>
#include <queue>

using namespace std;

// Magic Enum
#include <magic_enum/magic_enum.hpp>

// Engine Header
#include "Engine_Enum.h"
#include "Engine_Macro.h"
#include "Engine_Struct.h"
#include "Engine_Typedef.h"
#include "Engine_Function.h"


// Win
#include <Windows.h>
#include <assert.h>
//#include <optional>

#ifdef _DEBUG

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

// Json
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// ImGui
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include "Utils.h"

// Protobuf
#include "Struct.pb.h"
#include "Enum.pb.h"
#include "Protocol.pb.h"

// 메모리 누수 감지
#ifndef DBG_NEW 

#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ ) 
#define new DBG_NEW 

#endif
#endif

using namespace Engine;
using namespace Protocol;
