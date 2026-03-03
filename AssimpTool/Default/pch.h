#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <iostream>
#include <filesystem>
#include <cassert>

#include <directxtk/SimpleMath.h>

using namespace std;
using namespace DirectX::SimpleMath;

using int32 = int32_t;
using uint32 = uint32_t;
using Vec2 = Vector2;
using Vec3 = Vector3;
using Vec4 = Vector4;

#include "Assimp_Macro.h"

// Assimp
#include <Assimp/Importer.hpp>
#include <Assimp/scene.h>
#include <Assimp/postprocess.h>

// spdlog
#include <spdlog/spdlog.h>

using namespace Assimp;
