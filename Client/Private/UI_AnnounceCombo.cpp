#include "pch.h"
#include "UI_AnnounceCombo.h"
#include "GameObject_Factory.h"
#include "Texture.h"
#include "Shader.h"
#include "VIBuffer_Rect.h"
#include "GameInstance.h"

REGISTER_GAMEOBJECT(UI_AnnounceCombo, Protocol::OBJECT_TYPE_UI_ANNOUNCE_COMBO)

UI_AnnounceCombo::UI_AnnounceCombo(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : UIObject(device, context)
{
}

UI_AnnounceCombo::UI_AnnounceCombo(const UI_AnnounceCombo& rhs)
    : UIObject(rhs)
{
}

HRESULT UI_AnnounceCombo::Initialize_Prototype()
{
    return S_OK;
}

HRESULT UI_AnnounceCombo::Initialize(void* arg)
{
    CHECK_FAILED(UIObject::Initialize(arg), E_FAIL);

    const float uiRefWidth = GAME->Get_UIReferenceWidth();
    const float uiRefHeight = GAME->Get_UIReferenceHeight();

    _sizeX = DIGIT_BASE_WIDTH;
    _sizeY = DIGIT_BASE_HEIGHT;
    _posX = uiRefWidth * 0.55f;
    _posY = uiRefHeight * 0.5f - 100.f;
    _isVisible = false;

    CHECK_FAILED(Ready_Components(), E_FAIL);

    auto& hub = GAME->Get_DelegateHub();
    _comboHitHandle = hub.OnPlayerComboHit.Add(this, &UI_AnnounceCombo::On_PlayerComboHit);

    return S_OK;
}

void UI_AnnounceCombo::Update(float timeDelta)
{
    if (!_isVisible)
        return;

    _decayTimer -= timeDelta;

    if (_digitPopTimer > 0.f)
    {
        _digitPopTimer = max(0.f, _digitPopTimer - timeDelta);
        Update_Matrices();
    }

    if (_decayTimer <= 0.f)
    {
        _comboCount = 0;
        _digitPopTimer = 0.f;
        _isVisible = false;
    }
}

void UI_AnnounceCombo::Late_Update(float timeDelta)
{
    if (!_isVisible)
        return;

    UIObject::Late_Update(timeDelta);
}

HRESULT UI_AnnounceCombo::Render()
{
    if (!_isVisible || _comboCount == 0)
        return S_OK;

    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ViewMatrix", ETransformState::View), E_FAIL);
    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ProjMatrix", ETransformState::Proj), E_FAIL);

    const float decayAlpha = (_decayTimer < 1.f) ? _decayTimer : 1.f;

    if (!_textureHit)
        return S_OK;

    if (_comboCount == 1)
    {
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_Alpha", &decayAlpha, sizeof(float)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &_hitMatrix), E_FAIL);
        CHECK_FAILED(_textureHit->Bind_SRV(_shaderCom, "g_Texture", HIT_TEXTURE_INDEX), E_FAIL);
        CHECK_FAILED(_shaderCom->Begin_Pass(0), E_FAIL);
        CHECK_FAILED(_bufferCom->Render(), E_FAIL);
        return S_OK;
    }

    if (_textureDigits)
    {
        const string comboStr = to_string(_comboCount);

        for (size_t i = 0; i < comboStr.length(); ++i)
        {
            if (i >= _digitMatrices.size())
                break;

            const int digit = comboStr[i] - '0';

            CHECK_FAILED(_shaderCom->Bind_RawValue("g_Alpha", &decayAlpha, sizeof(float)), E_FAIL);
            CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &_digitMatrices[i]), E_FAIL);
            CHECK_FAILED(_textureDigits->Bind_SRV(_shaderCom, "g_Texture", digit), E_FAIL);
            CHECK_FAILED(_shaderCom->Begin_Pass(0), E_FAIL);
            CHECK_FAILED(_bufferCom->Render(), E_FAIL);
        }
    }

    CHECK_FAILED(_shaderCom->Bind_RawValue("g_Alpha", &decayAlpha, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &_hitsMatrix), E_FAIL);
    CHECK_FAILED(_textureHit->Bind_SRV(_shaderCom, "g_Texture", HITS_TEXTURE_INDEX), E_FAIL);
    CHECK_FAILED(_shaderCom->Begin_Pass(0), E_FAIL);
    CHECK_FAILED(_bufferCom->Render(), E_FAIL);

    return S_OK;
}

void UI_AnnounceCombo::Add_Combo()
{
    _comboCount++;
    _decayTimer = 5.f;
    _digitPopTimer = DIGIT_POP_DURATION;
    _isVisible = true;

    Update_Matrices();
}

void UI_AnnounceCombo::On_PlayerComboHit(uint32 combo)
{
    _comboCount = combo;
    _decayTimer = 5.f;
    _digitPopTimer = DIGIT_POP_DURATION;
    _isVisible = (_comboCount > 0);

    if (_isVisible)
        Update_Matrices();
}

HRESULT UI_AnnounceCombo::Ready_Components()
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_TEXTURE_ANNOUNCE_HIT, _textureHit), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_TEXTURE_ANNOUNCE_DIGIT, _textureDigits), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_UI, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_RECT, _bufferCom), E_FAIL);

    return S_OK;
}

void UI_AnnounceCombo::Update_Matrices()
{
    const float uiRefWidth = GAME->Get_UIReferenceWidth();
    const float uiRefHeight = GAME->Get_UIReferenceHeight();

    const float hitWidth = HIT_BASE_WIDTH * HIT_RENDER_SCALE;
    const float hitHeight = HIT_BASE_HEIGHT * HIT_RENDER_SCALE;
    const float hitsWidth = HITS_BASE_WIDTH * HITS_RENDER_SCALE;
    const float hitsHeight = HITS_BASE_HEIGHT * HITS_RENDER_SCALE;

    const float groupPosX = _posX + ANNOUNCE_GROUP_OFFSET_X;
    const float groupPosY = _posY + ANNOUNCE_GROUP_OFFSET_Y;

    const float groupRenderX = groupPosX - (uiRefWidth * 0.5f);
    const float groupRenderY = -groupPosY + (uiRefHeight * 0.5f);

    const Matrix rotationMatrix =
        Matrix::CreateRotationZ(XMConvertToRadians(ANNOUNCE_ROTATION_DEGREE));

    _digitMatrices.clear();
    _hitMatrix = Matrix::Identity;
    _hitsMatrix = Matrix::Identity;

    // 1타 - HIT
    {
        const Vec2 hitRenderOffset = Rotate_RenderOffset(
            Vec2(0.f, HIT_LOCAL_OFFSET_Y), ANNOUNCE_ROTATION_DEGREE);

        _hitMatrix =
            Matrix::CreateScale(hitWidth, hitHeight, 1.f) *
            rotationMatrix *
            Matrix::CreateTranslation(
                groupRenderX + hitRenderOffset.x,
                groupRenderY + hitRenderOffset.y,
                0.f);
    }

    if (_comboCount <= 1)
        return;

    const string comboStr = to_string(_comboCount);

    const float digitPopScale = Compute_DigitPopScale();
    const float digitWidth = DIGIT_BASE_WIDTH * DIGIT_RENDER_SCALE * digitPopScale;
    const float digitHeight = DIGIT_BASE_HEIGHT * DIGIT_RENDER_SCALE * digitPopScale;
    const float digitAdvance = digitWidth * DIGIT_ADVANCE_RATIO;

    const float digitBlockWidth =
        (comboStr.length() > 0)
        ? ((static_cast<float>(comboStr.length()) - 1.f) * digitAdvance + digitWidth)
        : 0.f;

    const float totalWidth = digitBlockWidth + DIGIT_HITS_SPACING + hitsWidth;
    const float leftLocalX = -(totalWidth * 0.5f) + 65.f;

    for (size_t i = 0; i < comboStr.length(); ++i)
    {
        const float localCenterX = leftLocalX + (digitWidth * 0.5f) + (static_cast<float>(i) * digitAdvance);
        const Vec2 digitRenderOffset = Rotate_RenderOffset(
            Vec2(localCenterX, 0.f),
            ANNOUNCE_ROTATION_DEGREE);

        Matrix digitMatrix =
            Matrix::CreateScale(digitWidth, digitHeight, 1.f) *
            rotationMatrix *
            Matrix::CreateTranslation(
                groupRenderX + digitRenderOffset.x,
                groupRenderY + digitRenderOffset.y,
                0.f);

        _digitMatrices.push_back(digitMatrix);
    }

    const float hitsLocalCenterX = leftLocalX + digitBlockWidth + DIGIT_HITS_SPACING + (hitsWidth * 0.5f);
    const Vec2 hitsRenderOffset = Rotate_RenderOffset(
        Vec2(hitsLocalCenterX, HITS_LOCAL_OFFSET_Y),
        ANNOUNCE_ROTATION_DEGREE);

    _hitsMatrix =
        Matrix::CreateScale(hitsWidth, hitsHeight, 1.f) *
        rotationMatrix *
        Matrix::CreateTranslation(
            groupRenderX + hitsRenderOffset.x,
            groupRenderY + hitsRenderOffset.y,
            0.f);
}

float UI_AnnounceCombo::Compute_DigitPopScale() const
{
    if (_digitPopTimer <= 0.f)
        return 1.f;

    const float t = 1.f - (_digitPopTimer / DIGIT_POP_DURATION);
    return ::lerp(DIGIT_POP_START_SCALE, 1.f, t);
}

Vec2 UI_AnnounceCombo::Rotate_RenderOffset(const Vec2& localOffset, float degree)
{
    const float radian = XMConvertToRadians(degree);
    const float cosine = cosf(radian);
    const float sine = sinf(radian);

    return Vec2(
        localOffset.x * cosine - localOffset.y * sine,
        localOffset.x * sine + localOffset.y * cosine);
}

Shared<UI_AnnounceCombo> UI_AnnounceCombo::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<UI_AnnounceCombo>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : UI_AnnounceCombo");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> UI_AnnounceCombo::Clone(void* arg)
{
    auto clone = make_shared<UI_AnnounceCombo>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : UI_AnnounceCombo");
        return nullptr;
    }

    return clone;
}

void UI_AnnounceCombo::Free()
{
    auto& hub = GAME->Get_DelegateHub();
    hub.OnPlayerComboHit.Remove(_comboHitHandle);

    UIObject::Free();
}
