#pragma once

#include "Core_pch.h"

namespace Server {};

#include "Server_Macro.h"
#include "Server_Typedef.h"
#include "Server_SimpleMath.h"

#include "Protocol.pb.h"
#include "Enum.pb.h"
#include "Struct.pb.h"

#define LOG_INFO(...)    spdlog::info(__VA_ARGS__)
#define LOG_WARN(...)    spdlog::warn(__VA_ARGS__)
#define LOG_ERROR(...)   spdlog::error(__VA_ARGS__)

using namespace Server;
