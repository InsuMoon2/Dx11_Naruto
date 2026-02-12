#pragma once

// 기존 new 매크로 임시 해제
#ifdef new
#pragma push_macro("new")
#undef new
#endif

// Protobuf 헤더 include
#include "Protocol.pb.h"
#include "Enum.pb.h"
#include "Struct.pb.h"

// new 매크로 복원
#ifdef _DEBUG
#pragma pop_macro("new")
#endif
