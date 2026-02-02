#pragma once

#define _HAS_STD_BYTE 0

// ImGui
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "implot.h"

namespace Editor
{ }

using namespace Editor;

#define EDITOR EditorInstance::GetInstance()
