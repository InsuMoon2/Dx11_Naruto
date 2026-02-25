#pragma once

#include "string"

// 기본 타입
using uint8 = unsigned char;

using int16 = signed short;
using uint16 = unsigned short;

using int32 = signed int;
using uint32 = unsigned int;

using int64 = signed long long;
using uint64 = unsigned long long;

// 문자열
using wstring = std::wstring;
using string = std::string;

using tchar = wchar_t;

template<typename Key, typename Value>
using umap = std::unordered_map<Key, Value>;

template<typename T>
using uset = std::unordered_set<T>;

// 스마트 포인터
template<typename T>
using Shared = std::shared_ptr<T>;

template<typename T>
using Weak = std::weak_ptr<T>;

template<typename T>
using Unique = std::unique_ptr<T>;

