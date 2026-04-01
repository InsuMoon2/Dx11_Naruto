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

private:
    HRESULT Ready_Components() override;
    void    Update_Matrices();

private:
    Shared<Shader>          _shaderCom;
    Shared<Texture>         _textureCom;
    Shared<VIBuffer_Rect>   _bufferCom;

private:
    uint32                  _comboCount = 0;
    float                   _decayTimer = 0.f;

    Matrix                  _hitMatrix;
    Shared<Texture>         _textureHit;

    vector<Matrix>          _digitMatrices;
    Shared<Texture>         _textureDigits;

    FDelegateHandle         _comboHitHandle;

public:
    static Shared<UI_AnnounceCombo> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};

NS_END
