#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>

#include "Engine_Define.h"
#include "Core_Pch.h"

#include "targetver.h"
//#define WIN32_LEAN_AND_MEAN
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include "Client_Defines.h"
#include "Client_Enum.h"
#include "Client_Macro.h"
#include "Client_Struct.h"

#include <assert.h>

#include <format>

#include "Input_Manager.h"

#define _CRTDEBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif
