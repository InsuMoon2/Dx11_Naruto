#include "pch.h"
#include "UI_Timer.h"
#include "Shader.h"
#include "VIBuffer_Rect.h"
#include "Texture.h"
#include "GameObject_Factory.h"

REGISTER_GAMEOBJECT(UI_Timer, Protocol::OBJECT_TYPE_UI_TIMER)
IMPLEMENT_REFLECTION(UI_Timer)

bool UI_Timer::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "UI_Timer";

    PROPERTY_FLOAT("BG Width", _bgWidth, 1.f, 512.f);
    PROPERTY_FLOAT("BG Height", _bgHeight, 1.f, 512.f);

    PROPERTY_FLOAT("Digit Width", _digitWidth, 1.f, 256.f);
    PROPERTY_FLOAT("Digit Height", _digitHeight, 1.f, 256.f);
    PROPERTY_FLOAT("Digit Offset X", _digitOffsetX, -512.f, 512.f);
    PROPERTY_FLOAT("Digit Offset Y", _digitOffsetY, -512.f, 512.f);

    PROPERTY_FLOAT("Minute Tens X", _minuteTensX, -512.f, 512.f);
    PROPERTY_FLOAT("Minute Ones X", _minuteOnesX, -512.f, 512.f);
    PROPERTY_FLOAT("Second Tens X", _secondTensX, -512.f, 512.f);
    PROPERTY_FLOAT("Second Ones X", _secondOnesX, -512.f, 512.f);

    return true;
}

UI_Timer::UI_Timer(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : UIObject(device, context)
{
}

UI_Timer::UI_Timer(const UI_Timer& rhs)
    : UIObject(rhs)
    , _startSeconds(rhs._startSeconds)
    , _remainingSeconds(rhs._remainingSeconds)
    , _displaySeconds(rhs._displaySeconds)
    , _isPaused(rhs._isPaused)
    , _showMinuteTens(rhs._showMinuteTens)
    , _bgWidth(rhs._bgWidth)
    , _bgHeight(rhs._bgHeight)
    , _digitWidth(rhs._digitWidth)
    , _digitHeight(rhs._digitHeight)
    , _digitOffsetX(rhs._digitOffsetX)
    , _digitOffsetY(rhs._digitOffsetY)
    , _minuteTensX(rhs._minuteTensX)
    , _minuteOnesX(rhs._minuteOnesX)
    , _secondTensX(rhs._secondTensX)
    , _secondOnesX(rhs._secondOnesX)
{
    memcpy(_digits, rhs._digits, sizeof(_digits));
}

HRESULT UI_Timer::Initialize_Prototype()
{
    return UIObject::Initialize_Prototype();
}

HRESULT UI_Timer::Initialize(void* arg)
{
    CHECK_FAILED(UIObject::Initialize(arg), E_FAIL);

    auto* desc = static_cast<FTimerDesc*>(arg);
    if (desc)
    {
        _startSeconds = max(0.f, desc->startSeconds);
        _remainingSeconds = _startSeconds;
    }

    CHECK_FAILED(Ready_Components(), E_FAIL);

    Refresh_DisplaySeconds();

    return S_OK;
}

void UI_Timer::Update(float timeDelta)
{
    UIObject::Update(timeDelta);

    if (!_isPaused && _remainingSeconds > 0.f)
    {
        _remainingSeconds = max(0.f, _remainingSeconds - timeDelta);
        Refresh_DisplaySeconds();
    }

    __super::Update_Transform();
}

HRESULT UI_Timer::Render()
{
    if (!_isVisible)
        return S_OK;

    CHECK_NULL(_shaderCom, E_FAIL);
    CHECK_NULL(_textureCom, E_FAIL);
    CHECK_NULL(_bufferCom, E_FAIL);

    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ViewMatrix", ETransformState::View), E_FAIL);
    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ProjMatrix", ETransformState::Proj), E_FAIL);

    Matrix bgMatrix =
        Matrix::CreateScale(_bgWidth, _bgHeight, 1.f) *
        Matrix::CreateTranslation(
            _worldMatrix.Translation().x,
            _worldMatrix.Translation().y,
            _worldMatrix.Translation().z);

    CHECK_FAILED(Render_Texture(TIMER_BG_TEXTURE_INDEX, bgMatrix, 1.f), E_FAIL);

    for (int32 i = 0; i < 4; ++i)
    {
        if (i == 0 && !_showMinuteTens)
            continue;

        const uint32 digitTextureIndex = static_cast<uint32>(_digits[i]);
        CHECK_FAILED(Render_Texture(digitTextureIndex, Make_DigitMatrix(i), 1.f), E_FAIL);
    }

    return S_OK;
}

void UI_Timer::Start_Timer(float seconds)
{
    _startSeconds = max(0.f, seconds);
    _remainingSeconds = _startSeconds;
    _displaySeconds = -1;
    _isPaused = false;

    Refresh_DisplaySeconds();
}

void UI_Timer::Set_RemainingSeconds(float seconds)
{
    _remainingSeconds = max(0.f, seconds);

    Refresh_DisplaySeconds();
}

void UI_Timer::Refresh_DisplaySeconds()
{
    const int32 nextDisplaySeconds = static_cast<int32>(ceilf(max(0.f, _remainingSeconds)));

    if (_displaySeconds == nextDisplaySeconds)
        return;

    _displaySeconds = nextDisplaySeconds;

    Update_Digits();
}

void UI_Timer::Update_Digits()
{
    const int32 totalSeconds = max(0, _displaySeconds);

    const int32 minutes = totalSeconds / 60;
    const int32 seconds = totalSeconds % 60;

    const int32 clampedMinutes = min(minutes, 99);

    _showMinuteTens = clampedMinutes >= 10;

    _digits[0] = clampedMinutes / 10;
    _digits[1] = clampedMinutes % 10;
    _digits[2] = seconds / 10;
    _digits[3] = seconds % 10;
}

HRESULT UI_Timer::Render_Texture(uint32 textureIndex, const Matrix& worldMatrix, float alpha)
{
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &worldMatrix), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_Alpha", &alpha, sizeof(float)), E_FAIL);
    CHECK_FAILED(_textureCom->Bind_SRV(_shaderCom, "g_Texture", textureIndex), E_FAIL);

    CHECK_FAILED(_shaderCom->Begin_Pass(0), E_FAIL);
    CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_bufferCom->Render(), E_FAIL);

    return S_OK;
}

Matrix UI_Timer::Make_DigitMatrix(int32 digitSlot) const
{
    float x = 0.f;

    switch (digitSlot)
    {
    case 0:
        x = _minuteTensX;
        break;

    case 1:
        x = _minuteOnesX;
        break;

    case 2:
        x = _secondTensX;
        break;

    case 3:
        x = _secondOnesX;
        break;

    default:
        break;
    }

    const Vec3 center = _worldMatrix.Translation();

    return Matrix::CreateScale(_digitWidth, _digitHeight, 1.f) *
        Matrix::CreateTranslation(
            center.x + _digitOffsetX + x,
            center.y + _digitOffsetY,
            center.z + 0.001f);
}

HRESULT UI_Timer::Ready_Components()
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_TEXTURE_TIMER, _textureCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_UI, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_RECT, _bufferCom), E_FAIL);

    return S_OK;
}

Shared<UI_Timer> UI_Timer::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<UI_Timer>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create Prototype : UI_Timer");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> UI_Timer::Clone(void* arg)
{
    auto clone = make_shared<UI_Timer>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : UI_Timer");
        return nullptr;
    }

    return clone;
}

void UI_Timer::Free()
{
    UIObject::Free();
}
