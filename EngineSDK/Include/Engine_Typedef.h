#ifndef Engine_Typedef_h__
#define Engine_Typedef_h__

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

    // SimpleMath 벡터/행렬
    using vec2      = DirectX::SimpleMath::Vector2;
    using vec3      = DirectX::SimpleMath::Vector3;
    using vec4      = DirectX::SimpleMath::Vector4;
    using matrix    = DirectX::SimpleMath::Matrix;

    using quat      = DirectX::SimpleMath::Quaternion;
    using color     = DirectX::SimpleMath::Color;
    using ray       = DirectX::SimpleMath::Ray;
    using plane     = DirectX::SimpleMath::Plane;

    // DirectX COM 객체 타입
    template<typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    using Device            = ID3D11Device;
    using DeviceContext     = ID3D11DeviceContext;
    using SwapChain         = IDXGISwapChain;
    using RenderTarget      = ID3D11RenderTargetView;
    using DepthStencil      = ID3D11DepthStencilView;
    using Texture2D         = ID3D11Texture2D;
    using Buffer            = ID3D11Buffer;
    using ShaderResource    = ID3D11ShaderResourceView;

}

#endif // Engine_Typedef_h__
