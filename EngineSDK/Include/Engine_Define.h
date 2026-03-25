#pragma once

// DirectX
#include <DirectXCollision.h>
#include <DirectXMath.h>
#include <d3d11.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include <dxgi1_2.h>
#include <directxtk/SimpleMath.h>
#include <wrl.h>

#include <directxtk/DDSTextureLoader.h>
#include <directxtk/WICTextureLoader.h>

#include <DirectXColors.h>
#include <DirectXTK/Effects.h>
#include <DirectXTK/PrimitiveBatch.h>
#include <DirectXTK/SpriteBatch.h>
#include <DirectXTK/SpriteFont.h>
#include <DirectXTK/VertexTypes.h>

#include "Effects11/d3dx11effect.h"
#include <d3dcompiler.h>

using namespace DirectX;

// spdlog
#include "spdlog/sinks/base_sink.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/msvc_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

// STL
#include <algorithm>
#include <array>
#include <ctime>
#include <filesystem>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;
namespace fs = std::filesystem;

// Engine Header
#include "Engine_Enum.h"
#include "Engine_Function.h"
#include "Engine_Macro.h"
#include "Engine_Struct.h"
#include "Engine_Typedef.h"
#include "Property_Types.h"
#include "Reflection_Macro.h"
#include "Vertex_Struct.h"
#include "Text_Types.h"
#include "UI_AnimTypes.h"
#include "ModelBin_Types.h"
#include "Camera_Types.h"

// Win
#include <Windows.h>
#include <assert.h>
// #include <optional>

// UUID Library (stduuid)
#define UUID_SYSTEM_GENERATOR
#include <stduuid/uuid.h>

// Protobuf
#include "Enum.pb.h"
#include "Protocol.pb.h"
#include "Struct.pb.h"


#include <magic_enum/magic_enum.hpp>

// Magic Enum : 범위 지정
template <> struct magic_enum::customize::enum_range<Protocol::ComponentID> {
  static constexpr int min = 0;
  static constexpr int max = 2000;
};

// using namespace Protocol;

#ifdef _DEBUG

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <stdlib.h>


// Json
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "Utils.h"

// 메모리 누수 감지
#ifndef DBG_NEW

#define DBG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DBG_NEW

#endif
#endif

using namespace Engine;
