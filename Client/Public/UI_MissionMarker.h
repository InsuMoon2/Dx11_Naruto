#pragma once

#include "UIObject.h"

NS_BEGIN(Engine)
class VIBuffer_Rect;
NS_END

NS_BEGIN(Client)

class Background;

class UI_MissionMarker final : public UIObject
{
    GENERATED_BODY(UI_MissionMarker)

public:
    struct FUIMissionMarkerDesc : public FUIDesc
    {
        uint32 textureIndex = 0; 
    };

public:
    explicit UI_MissionMarker(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit UI_MissionMarker(const UI_MissionMarker& rhs);
    virtual ~UI_MissionMarker() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;

    void Update(float timeDelta) override;
    void Late_Update(float timeDelta) override;
    HRESULT Render() override;

public:
    void Set_TargetObject(Weak<GameObject> targetObject);
    void Set_TargetWorldPosition(const Vec3& worldPosition);

    void Clear_Target();

protected:
    HRESULT Ready_Components() override;

private:
    bool Resolve_TargetWorldPosition(Vec3& outWorldPosition) const;
    // 마커 표시 거리를 판정하기 위해 로컬 플레이어 위치를 우선, 없으면 카메라 위치를 대체로 구할 때 호출한다.
    bool Resolve_ViewerWorldPosition(Vec3& outWorldPosition) const;
    bool Project_WorldToUI(const Vec3& worldPosition, Vec2& outUIPosition, bool& outIsInsideScreen) const;

    Vec2 Clamp_ToScreenEdge(const Vec2& uiPosition) const;
    Vec2 Smooth_Follow(const Vec2& currentPosition, const Vec2& targetPosition, float timeDelta) const;

    HRESULT Render_AmbientHalo();

private:
    Shared<Shader>          _shaderCom;    
    Shared<Texture>         _textureCom;   
    Shared<VIBuffer_Rect>   _bufferCom;

    Weak<GameObject>        _targetObject;  
    Vec3                    _targetWorldPosition = Vec3::Zero; 
    bool                    _hasFixedWorldPosition = false; 

    Vec2                    _currentUIPosition = Vec2::Zero; 
    bool                    _hasCurrentUIPosition = false; 

    float                   _screenOffsetY = 180.f;
    float                   _edgeMargin = 48.f; 
    float                   _followSpeed = 14.f; 
    float                   _insideScale = 64.f + 64.f;
    float                   _edgeScale = 64.f + 32.f; 
    uint32                  _textureIndex = 0;

    Shared<Texture> _haloTextureCom;

    float _ambientHaloTime = 0.f;
    float _ambientHaloCycle = 0.55f;
    float _ambientHaloBaseAlpha = 0.65f;

    float _ambientHaloStartScale = 0.6f;
    float _ambientHaloEndScale = 1.2f;
    float _maxVisibleDistance = 200.f; // WaveTrigger 같은 월드 마커가 이 거리보다 멀어지면 화면에서 숨길 최대 표시 거리다.

public:
    static Shared<UI_MissionMarker> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
