#pragma once

#define _HAS_STD_BYTE 0

#include "Core_Types.h"
#include "Core_Macro.h"
#include "Core_TLS.h"
#include "Core_Global.h"

// spdlog
#include "spdlog/spdlog.h"
#include "spdlog/sinks/base_sink.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/msvc_sink.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#define LOG_INFO(...)    spdlog::info(__VA_ARGS__)
#define LOG_WARN(...)    spdlog::warn(__VA_ARGS__)
#define LOG_ERROR(...)   spdlog::error(__VA_ARGS__)

#include <vector>
#include <list>
#include <queue>
#include <stack>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#include <winsock2.h>
#include <windows.h>
#include <iostream>

using namespace std;

#include <assert.h>
#include "SocketUtils.h"
#include "SendBuffer.h"
#include "Session.h"
#include "Service.h"

#pragma comment(lib, "ws2_32.lib")
