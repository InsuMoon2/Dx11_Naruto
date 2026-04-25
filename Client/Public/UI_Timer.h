#pragma once

#include "UIObject.h"

NS_BEGIN(Engine)
class Shader;
class Texture;
class VIBuffer_Rect;
NS_END

NS_BEGIN(Client)

class UI_Timer final : public UIObject
{
    GENERATED_BODY(UI_Timer)

public:
    struct FTimerDesc : public UIObject::FUIDesc
    {
        float startSeconds = 600.f;
    };

public:
    explicit UI_Timer(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit UI_Timer(const UI_Timer& rhs);
    virtual ~UI_Timer() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Update(float timeDelta) override;
    HRESULT Render() override;

public:
    void Start_Timer(float seconds);
    void Set_TimerPaused(bool paused) { _isPaused = paused; }

    void Set_RemainingSeconds(float seconds);
    bool Is_TimeOver() const { return _remainingSeconds <= 0.f; }

private:
    void Update_Digits();

    void Refresh_DisplaySeconds();
    HRESULT Render_Texture(uint32 textureIndex, const Matrix& worldMatrix, float alpha);

    Matrix Make_DigitMatrix(int32 digitSlot) const;

protected:
    HRESULT Ready_Components() override;

private:
    Shared<Shader>        _shaderCom;   
    Shared<Texture>       _textureCom;  
    Shared<VIBuffer_Rect> _bufferCom;   

private:
    float _startSeconds = 600.f;     
    float _remainingSeconds = 600.f; 
    int32 _displaySeconds = -1;      
    bool  _isPaused = false;         

    int32 _digits[4] = { 1, 0, 0, 0 }; 
    bool  _showMinuteTens = true;

private:
    static constexpr uint32 TIMER_BG_TEXTURE_INDEX = 10;

    float _bgWidth = 128.f;
    float _bgHeight = 128.f;

    float _digitWidth = 92.f;
    float _digitHeight = 44.f;
    float _digitOffsetX = 0.f;
    float _digitOffsetY = -6.7f;

    float _minuteTensX = -70.f;
    float _minuteOnesX = -40.f;
    float _secondTensX = 5.3f;
    float _secondOnesX = 37.9f;

public:
    static Shared<UI_Timer> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
