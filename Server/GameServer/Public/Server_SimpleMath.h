#pragma once

#include <directxtk/SimpleMath.h>

// Engine 쪽과 같은 이름으로 SimpleMath 타입을 서버에서도 공용으로 사용하기 위한 별칭이다.
using Vec2 = DirectX::SimpleMath::Vector2;
using Vec3 = DirectX::SimpleMath::Vector3;
using Vec4 = DirectX::SimpleMath::Vector4;
using Matrix = DirectX::SimpleMath::Matrix;

// 회전/색상/기하 보조 타입도 같은 이름으로 맞춰 서버 로직과 클라 로직의 타입 감각을 통일한다.
using Quat = DirectX::SimpleMath::Quaternion;
using Color = DirectX::SimpleMath::Color;
using Ray = DirectX::SimpleMath::Ray;
using Plane = DirectX::SimpleMath::Plane;
