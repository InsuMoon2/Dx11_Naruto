#pragma once

#include "UIObject.h"

NS_BEGIN(Engine)
class VIBuffer_Rect;
NS_END

NS_BEGIN(Client)

class UI_AnnounceCombo : public UIObject
{
    GENERATED_BODY(UI_AnnounceCombo)

public:
    explicit UI_AnnounceCombo(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit UI_AnnounceCombo(const UI_AnnounceCombo& rhs);
    virtual ~UI_AnnounceCombo() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

public:
    void    Add_Combo();
    void    On_PlayerComboHit(uint32 combo);
    void    Set_AnnouncePosition(float x, float y);

private:
    HRESULT Ready_Components() override;
    void    Update_Matrices();
    float   Compute_DigitPopScale() const;
    static Vec2 Rotate_RenderOffset(const Vec2& localOffset, float degree);

private:
    Shared<Shader>          _shaderCom;
    Shared<Texture>         _textureCom;
    Shared<VIBuffer_Rect>   _bufferCom;

private:
    uint32                  _comboCount = 0;
    float                   _decayTimer = 0.f;

    Matrix                  _hitMatrix = Matrix::Identity;
    Matrix                  _hitsMatrix = Matrix::Identity;

    Shared<Texture>         _textureHit;

    vector<Matrix>          _digitMatrices;
    Shared<Texture>         _textureDigits;

    FDelegateHandle         _comboHitHandle;

private:
    float                   _digitPopTimer = 0.f;

    static constexpr float  DIGIT_POP_DURATION = 0.12f;
    static constexpr float  DIGIT_POP_START_SCALE = 1.2f;

    static constexpr uint32 HIT_TEXTURE_INDEX = 0;
    static constexpr uint32 HITS_TEXTURE_INDEX = 1;

private:
    static constexpr float  HIT_BASE_WIDTH = 552.f;
    static constexpr float  HIT_BASE_HEIGHT = 164.f;
    static constexpr float  HITS_BASE_WIDTH = 256.f;
    static constexpr float  HITS_BASE_HEIGHT = 72.f;
    static constexpr float  DIGIT_BASE_WIDTH = 108.f;
    static constexpr float  DIGIT_BASE_HEIGHT = 128.f;

    static constexpr float  HIT_RENDER_SCALE = 0.32f;
    static constexpr float  HITS_RENDER_SCALE = 0.62f;
    static constexpr float  DIGIT_RENDER_SCALE = 0.46f;

    static constexpr float  DIGIT_ADVANCE_RATIO = 0.66f;
    static constexpr float  DIGIT_HITS_SPACING = 5.f;

    static constexpr float  ANNOUNCE_ROTATION_DEGREE = 30.f;

    static constexpr float  ANNOUNCE_GROUP_OFFSET_X = 0.f;
    static constexpr float  ANNOUNCE_GROUP_OFFSET_Y = -12.f;

    static constexpr float  HIT_LOCAL_OFFSET_Y  = 5.f;
    static constexpr float  HITS_LOCAL_OFFSET_Y = -8.f;

public:
    static Shared<UI_AnnounceCombo> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
