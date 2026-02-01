#pragma once

#include "Core_Pch.h"
#include "Editor_Define.h"

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include "Client_Defines.h"

#include "Service.h"
#include <assert.h>

#include "Protocol_Wrapper.h"

#include "Client_PacketHandler.h"
#include "NetworkManager.h"

#include <format>
#include <filesystem>
namespace fs = std::filesystem;

#include "Input_Manager.h"

#define _CRTDEBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif
