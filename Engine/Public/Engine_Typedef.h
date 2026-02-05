#pragma once

#include <directxtk/SimpleMath.h>

namespace Engine
{
    // 기본 타입
    using int8      = signed char;
    using uint8     = unsigned char;

    using int16     = signed short;
    using uint16    = unsigned short;

    using int32     = signed int;
    using uint32    = unsigned int;

    using int64     = signed long long;
    using uint64    = unsigned long long;

    // 문자열
    using wstring   = std::wstring;
    using string    = std::string;

    using tchar     = wchar_t;

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

    // SimpleMath 벡터/행렬
    using Vec2      = DirectX::SimpleMath::Vector2;
    using Vec3      = DirectX::SimpleMath::Vector3;
    using Vec4      = DirectX::SimpleMath::Vector4;
    using Matrix    = DirectX::SimpleMath::Matrix;

    using Quat      = DirectX::SimpleMath::Quaternion;
    using Color     = DirectX::SimpleMath::Color;
    using Ray       = DirectX::SimpleMath::Ray;
    using Plane     = DirectX::SimpleMath::Plane;

    // DirectX COM 객체 타입
    template<typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    using Device                = ID3D11Device;
    using DeviceContext         = ID3D11DeviceContext;
    using SwapChain             = IDXGISwapChain;
    using RenderTargetView      = ID3D11RenderTargetView;
    using DepthStencil          = ID3D11DepthStencilView;
    using Texture2D             = ID3D11Texture2D;
    using Buffer                = ID3D11Buffer;
    using ShaderResourceView    = ID3D11ShaderResourceView;
}
