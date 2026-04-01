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

    float uiRefWidth = GAME->Get_UIReferenceWidth();
    float uiRefHeight = GAME->Get_UIReferenceHeight();

    _sizeX = 100.f;
    _sizeY = 100.f;
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

    if (_decayTimer <= 0.f)
    {
        _comboCount = 0;
        _isVisible = false;
    }
}

void UI_AnnounceCombo::Late_Update(float timeDelta)
{
    if (!_isVisible) return;

    UIObject::Late_Update(timeDelta);
}

HRESULT UI_AnnounceCombo::Render()
{
    if (!_isVisible || _comboCount == 0) return S_OK;

    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ViewMatrix", ETransformState::View), E_FAIL);
    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ProjMatrix", ETransformState::Proj), E_FAIL);

    float alpha = (_decayTimer < 1.0f) ? _decayTimer : 1.0f;
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_Alpha", &alpha, sizeof(float)), E_FAIL);

    // HIT 로고 그리기
    if (_textureHit)
    {
        _shaderCom->Bind_Matrix("g_WorldMatrix", &_hitMatrix);
        _textureHit->Bind_SRV(_shaderCom, "g_Texture", 0);

        _shaderCom->Begin_Pass(0);
        _bufferCom->Render();
    }

    // 숫자를 String으로 만들고 각 자리를 분리해서 렌더링
    if (_textureDigits)
    {
        string comboStr = to_string(_comboCount);
        for (size_t i = 0; i < comboStr.length(); i++)
        {
            // 문자를 0~9 정수 인덱스로 파싱
            int digit = comboStr[i] - '0';

            _shaderCom->Bind_Matrix("g_WorldMatrix", &_digitMatrices[i]);
            _textureDigits->Bind_SRV(_shaderCom, "g_Texture", digit); // 해당 숫자 인덱스 이미지 로드

            _shaderCom->Begin_Pass(0);
            _bufferCom->Render();
        }
    }
    return S_OK;
}

void UI_AnnounceCombo::Add_Combo()
{
    _comboCount++;
    _decayTimer = 5.f;  // 콤보가 들어오면 타이머 계속 연장
    _isVisible = true;

    Update_Matrices();
}

void UI_AnnounceCombo::On_PlayerComboHit(uint32 combo)
{
    _comboCount = combo;
    _decayTimer = 5.f;
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
    float uiRefWidth = GAME->Get_UIReferenceWidth();
    float uiRefHeight = GAME->Get_UIReferenceHeight();

    // HIT 텍스트 위치
    float hitSizeX = 120.f;
    float hitSizeY = 50.f;
    float hitPosX = _posX;
    float hitPosY = _posY - 50.f;

    _hitMatrix = Matrix::CreateScale(hitSizeX, hitSizeY, 1.f) *
        Matrix::CreateTranslation(hitPosX - (uiRefWidth * 0.5f), -hitPosY + (uiRefHeight * 0.5f), 0.f);

    // 숫자 위치 자릿수 배열 정리
    string comboStr = to_string(_comboCount);
    _digitMatrices.clear();

    float digitSizeX = 80.f;
    float digitSizeY = 80.f;

    float totalWidth = comboStr.length() * digitSizeX * 0.8f;
    float startX = _posX - (totalWidth * 0.5f) + (digitSizeX * 0.4f);

    for (size_t i = 0; i < comboStr.length(); i++)
    {
        float curX = startX + (i * digitSizeX * 0.8f);
        float curY = _posY;

        Matrix m = Matrix::CreateScale(digitSizeX, digitSizeY, 1.f) *
            Matrix::CreateTranslation(curX - (uiRefWidth * 0.5f), -curY + (uiRefHeight * 0.5f), 0.f);

        _digitMatrices.push_back(m);
    }
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
