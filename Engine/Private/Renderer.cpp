#include "pch.h"
#include "Renderer.h"
#include "GameInstance.h"
#include "GameObject.h"
#include "Shader.h"
#include "UIObject.h"
#include "UI_Text.h"
#include "VIBuffer_Rect.h"

static constexpr float SHADOW_RENDER_SCALE = 2.f;
static uint32 Compute_ShadowRenderExtent(float viewportExtent)
{
    return max(1u, static_cast<uint32>(viewportExtent * SHADOW_RENDER_SCALE));
}

Renderer::Renderer(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : _device(device), _context(context)
{

}

Renderer::~Renderer()
{

}

#ifdef _DEBUG
// Shadow pass 실패 로그에서 어느 오브젝트가 문제인지 바로 알 수 있도록 이름/타입/GUID를 묶어 만든다.
static string Make_ShadowLogObjectLabel(const shared_ptr<GameObject>& gameObject)
{
    if (!gameObject)
        return "<null>";

    const string objectName = Utils::ToString(gameObject->Get_Name()); // 로그에서 사람이 읽을 수 있는 오브젝트 이름이다.
    const string objectTypeName = string(magic_enum::enum_name(gameObject->Get_ObjectType())); // protobuf object type을 같이 남긴다.
    const string objectGuid = gameObject->Get_GUID(); // 같은 이름의 복제 오브젝트를 구분하기 위한 GUID다.

    return format("name='{}', type='{}', guid='{}'", objectName, objectTypeName, objectGuid);
}

// Shadow frustum이 실제 caster를 보고 있는지 빠르게 확인하기 위해 오브젝트 원점의 light clip/NDC 좌표를 계산한다.
static bool Try_ComputeShadowNdcForObject(
    const shared_ptr<GameObject>& gameObject,
    Vec3& outWorldPos,
    Vec3& outShadowNdc)
{
    if (!gameObject)
        return false;

    const auto transform = gameObject->Get_Transform();
    const Matrix* shadowView = GAME->Get_ShadowViewMatrix();
    const Matrix* shadowProj = GAME->Get_ShadowProjMatrix();
    if (!transform || !shadowView || !shadowProj)
        return false;

    outWorldPos = transform->Get_WorldPosition();

    const XMVECTOR worldPos = XMVectorSet(outWorldPos.x, outWorldPos.y, outWorldPos.z, 1.f);
    const XMVECTOR lightViewPos = XMVector4Transform(worldPos, *shadowView);
    const XMVECTOR lightClipPos = XMVector4Transform(lightViewPos, *shadowProj);

    XMFLOAT4 clipFloat4{};
    XMStoreFloat4(&clipFloat4, lightClipPos);

    const float invW = 1.f / max(fabsf(clipFloat4.w), 0.0001f);
    outShadowNdc = Vec3(
        clipFloat4.x * invW,
        clipFloat4.y * invW,
        clipFloat4.z * invW);

    return true;
}
#endif

HRESULT Renderer::Initialize()
{
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    if (FAILED(_device->CreateBlendState(&blendDesc, _uiBlendState.GetAddressOf())))
        return E_FAIL;

    // --------------------------------------------------
    // 기본 3D depth on state
    // --------------------------------------------------
    D3D11_DEPTH_STENCIL_DESC defaultDepthDesc = {};
    defaultDepthDesc.DepthEnable = TRUE;
    defaultDepthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    defaultDepthDesc.DepthFunc = D3D11_COMPARISON_LESS;
    defaultDepthDesc.StencilEnable = FALSE;

    if (FAILED(_device->CreateDepthStencilState(&defaultDepthDesc, _defaultDepthState.GetAddressOf())))
        return E_FAIL;

    // --------------------------------------------------
    // UI depth off state
    // --------------------------------------------------
    D3D11_DEPTH_STENCIL_DESC uiDepthDesc = {};
    uiDepthDesc.DepthEnable = FALSE;
    uiDepthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    uiDepthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    uiDepthDesc.StencilEnable = FALSE;

    if (FAILED(_device->CreateDepthStencilState(&uiDepthDesc, _uiDepthDisabledState.GetAddressOf())))
        return E_FAIL;

    CHECK_FAILED(Ready_RenderTarget(), E_FAIL);

    return S_OK;
}

void Renderer::Add_RenderGroup(ERenderGroup renderType, shared_ptr<GameObject> gameObject)
{
    CHECK_NULL(gameObject);

    _renderObjects[ETOI(renderType)].emplace_back(gameObject);
}

void Renderer::Backup_RenderGroup()
{
    for (int i = 0; i < ETOI(ERenderGroup::END); ++i)
    {
        _backupRenderObjects[i] = _renderObjects[i];
        _renderObjects[i].clear();
    }
}

void Renderer::Restore_RenderGroup()
{
    for (int i = 0; i < ETOI(ERenderGroup::END); ++i)
    {
        _renderObjects[i] = _backupRenderObjects[i];
    }
}

void Renderer::Resize_DeferredViewport(uint32 width, uint32 height)
{
    if (width == 0 || height == 0)
        return;

    _worldMatrix = Matrix::CreateScale(static_cast<float>(width), static_cast<float>(height), 1.f);
    _viewMatrix = Matrix::Identity;
    _projMatrix = XMMatrixOrthographicLH(static_cast<float>(width), static_cast<float>(height), 0.f, 1.f);

#ifdef _DEBUG
    Update_RTDebugLayout(width, height);
#endif
}

void Renderer::Set_BlurStrength(float strength)
{
    _blurStrength = ::clamp(strength, 0.f, 1.f);
}

void Renderer::Set_BlurDirection(const Vec2& direction)
{
    Vec2 normalized = direction;

    if (normalized.LengthSquared() <= FLT_EPSILON)
        normalized = Vec2(0.f, 1.f);
    else
        normalized.Normalize();

    _blurDirection = normalized;
}

#ifdef _DEBUG
void Renderer::Add_DebugRenderGroup(Shared<Component> debugComponent)
{
    _debugComponents.push_back(debugComponent);
}
#endif

void Renderer::Draw(bool renderDebugPrimitives, bool renderColliders, bool renderRTDebug)
{
    _drawCallCount = 0;

    Render_BackgroundUI();
    Apply_Default3DState();
    Render_Priority();

    {
        Update_ShadowState();

        Render_Shadow();
    }

    Render_NonBlend();

    Render_Lights();
    Render_Combined();
    Render_NonLight();

    // 일반 Blend보다 굴절을 먼저 호출해야함
    Render_ScreenDistortion();

    Render_Blend();

    Apply_Default3DState();

    if (renderDebugPrimitives)
        GAME->Render_DebugDepth();

    Apply_UIState();

#ifdef _DEBUG
    if (renderColliders)
        GAME->Render_Colliders();
#endif

    if (renderDebugPrimitives)
        GAME->Render_DebugOverlay();

    Apply_Default3DState();

    Render_UI();

#ifdef _DEBUG
    if (renderRTDebug)
        Render_Debug();
#endif
}

HRESULT Renderer::Draw_Preview(bool renderColliders)
{
    _drawCallCount = 0;

    Apply_Default3DState();
    Render_Priority();

    if (renderColliders)
        GAME->Render_Colliders();

    for (auto& renderObject : _renderObjects[ETOI(ERenderGroup::NonBlend)])
    {
        if (renderObject)
        {
            renderObject->Render();
            _drawCallCount++;
        }
    }
    _renderObjects[ETOI(ERenderGroup::NonBlend)].clear();

    for (auto& renderObject : _renderObjects[ETOI(ERenderGroup::NonLight)])
    {
        if (renderObject)
        {
            renderObject->Render();
            _drawCallCount++;
        }
    }
    _renderObjects[ETOI(ERenderGroup::NonLight)].clear();

    Render_Blend();

    _renderObjects[ETOI(ERenderGroup::BackgroundUI)].clear();
    _renderObjects[ETOI(ERenderGroup::UI)].clear();

    return S_OK;
}

void Renderer::Render_BackgroundUI()
{
    Apply_UIState();

    _renderObjects[ETOI(ERenderGroup::BackgroundUI)].sort([](const Shared<GameObject>& src, const Shared<GameObject>& dst) {
        auto uiSrc = dynamic_pointer_cast<UIObject>(src);
        auto uiDst = dynamic_pointer_cast<UIObject>(dst);
        return uiSrc->Get_ZOrder() < uiDst->Get_ZOrder();
        });

    for (auto& renderObject : _renderObjects[ETOI(ERenderGroup::BackgroundUI)])
    {
        if (renderObject)
        {
            renderObject->Render();
            _drawCallCount++;
        }
    }

    _renderObjects[ETOI(ERenderGroup::BackgroundUI)].clear();

    Apply_Default3DState();
}

void Renderer::Render_Priority()
{
    for (auto& renderObject : _renderObjects[ETOI(ERenderGroup::Priority)])
    {
        if (renderObject)
            renderObject->Render();

        _drawCallCount++;
    }

    _renderObjects[ETOI(ERenderGroup::Priority)].clear();
}

void Renderer::Sort_NonBlendRenderObjects()
{
    auto& renderObjects = _renderObjects[ETOI(ERenderGroup::NonBlend)];

    renderObjects.sort([](const Shared<GameObject>& lhs, const Shared<GameObject>& rhs)
        {
            if (lhs == rhs)
                return false;

            if (!lhs)
                return false;

            if (!rhs)
                return true;

            const bool lhsSortable = lhs->Is_RenderBatchSortable();
            const bool rhsSortable = rhs->Is_RenderBatchSortable();

            if (lhsSortable != rhsSortable)
                return lhsSortable && !rhsSortable;

            if (!lhsSortable)
                return false;

            const uint64 lhsPrimaryKey = lhs->Get_RenderBatchPrimaryKey();
            const uint64 rhsPrimaryKey = rhs->Get_RenderBatchPrimaryKey();
            if (lhsPrimaryKey != rhsPrimaryKey)
                return lhsPrimaryKey < rhsPrimaryKey;

            const uint64 lhsSecondaryKey = lhs->Get_RenderBatchSecondaryKey();
            const uint64 rhsSecondaryKey = rhs->Get_RenderBatchSecondaryKey();
            if (lhsSecondaryKey != rhsSecondaryKey)
                return lhsSecondaryKey < rhsSecondaryKey;

            return false;
        });
}

void Renderer::Render_NonBlend()
{
    CHECK_FAILED(GAME->Begin_MRT(L"MRT_GameObjects"));

    Sort_NonBlendRenderObjects();

    for (auto& renderObject : _renderObjects[ETOI(ERenderGroup::NonBlend)])
    {
        if (renderObject)
            renderObject->Render();

        _drawCallCount++;
    }

    _renderObjects[ETOI(ERenderGroup::NonBlend)].clear();

    GAME->End_MRT();
}

void Renderer::Render_Blend()
{
    _context->OMSetBlendState(nullptr, nullptr, 0xffffffff);

    for (auto& renderObject : _renderObjects[ETOI(ERenderGroup::Blend)])
    {
        if (renderObject)
            renderObject->Render();

        _drawCallCount++;
    }

    _renderObjects[ETOI(ERenderGroup::Blend)].clear();

    _context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
}

void Renderer::Render_UI()
{
    Apply_UIState();

    _renderObjects[ETOI(ERenderGroup::UI)].sort([](const Shared<GameObject>& src, const Shared<GameObject>& dst)
        {
            auto uiSrc = dynamic_pointer_cast<UIObject>(src);
            auto uiDst = dynamic_pointer_cast<UIObject>(dst);

            if (uiSrc->Get_UILayer() != uiDst->Get_UILayer())
            {
                return uiSrc->Get_UILayer() < uiDst->Get_UILayer();
            }

            // 같은 레이어면, ZOrder 기준
            return uiSrc->Get_ZOrder() < uiDst->Get_ZOrder();

        });

    for (auto& renderObject : _renderObjects[ETOI(ERenderGroup::UI)])
    {
        if (!renderObject)
            continue;

        // DWrite는 별도 패스에서 처리해 D3D UI와 같은 백버퍼에 섞어 그리지 않는다.
        if (dynamic_pointer_cast<UI_Text>(renderObject))
            continue;

        renderObject->Render();

        _drawCallCount++;
    }

    ComPtr<ID3D11RenderTargetView> savedRenderTargetView; // UI_Text 전까지 실제로 그리던 RTV를 복원하기 위해 저장한다.
    ComPtr<ID3D11DepthStencilView> savedDepthStencilView; // RTV와 짝이 맞는 depth target도 함께 복원한다.
    UINT savedViewportCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE; // 에디터 Scene/Game View 전용 viewport를 유지하기 위해 개수를 저장한다.
    D3D11_VIEWPORT savedViewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {}; // UI_Text가 끝난 뒤 원래 viewport를 되돌릴 버퍼다.

    _context->OMGetRenderTargets(1, savedRenderTargetView.GetAddressOf(), savedDepthStencilView.GetAddressOf());
    _context->RSGetViewports(&savedViewportCount, savedViewports);

    ID3D11RenderTargetView* nullRTV = nullptr;
    _context->OMSetRenderTargets(1, &nullRTV, nullptr);

    GAME->Begin_UIText();

    for (auto& renderObject : _renderObjects[ETOI(ERenderGroup::UI)])
    {
        auto uiText = dynamic_pointer_cast<UI_Text>(renderObject);
        if (!uiText)
            continue;

        uiText->Render();
        _drawCallCount++;
    }

    _renderObjects[ETOI(ERenderGroup::UI)].clear();

    GAME->End_UIText();

    ID3D11RenderTargetView* restoreRTV = savedRenderTargetView.Get();
    _context->OMSetRenderTargets(1, &restoreRTV, savedDepthStencilView.Get());

    if (savedViewportCount > 0)
        _context->RSSetViewports(savedViewportCount, savedViewports);

    Apply_Default3DState();
}

void Renderer::Render_Lights()
{
    if (FAILED(GAME->Begin_MRT(L"MRT_LightAcc")))
        return;

    bool canRender = true;

    if (FAILED(_deferredShader->Bind_Matrix("g_WorldMatrix", &_worldMatrix)))
        canRender = false;

    if (FAILED(_deferredShader->Bind_Matrix("g_ViewMatrix", &_viewMatrix)))
        canRender = false;

    if (FAILED(_deferredShader->Bind_Matrix("g_ProjMatrix", &_projMatrix)))
        canRender = false;

    // Depth 기반 월드 위치 복원을 위한 역행렬 넘겨주기
    if (canRender && FAILED(_deferredShader->Bind_Matrix("g_ViewMatrixInverse", GAME->Get_TransformInverse(ETransformState::View))))
        canRender = false;

    if (canRender && FAILED(_deferredShader->Bind_Matrix("g_ProjMatrixInverse", GAME->Get_TransformInverse(ETransformState::Proj))))
        canRender = false;

    if (canRender && FAILED(GAME->Bind_RT_ShaderResource(_deferredShader, "g_NormalTexture", L"Target_Normal")))
        canRender = false;

    if (canRender && FAILED(GAME->Bind_RT_ShaderResource(_deferredShader, "g_DepthTexture", L"Target_Depth")))
        canRender = false;

    // 카메라 위치
    if (canRender && FAILED(_deferredShader->Bind_RawValue("g_CamPosition", GAME->Get_CamPosition(), sizeof(Vec4))))
        canRender = false;

    if (canRender)
    {
        _viBuffer->Bind_Resources();

        if (FAILED(GAME->Render_Lights(_deferredShader, _viBuffer)))
            canRender = false;
    }

    GAME->End_MRT();
}


void Renderer::Render_NonLight()
{
    for (auto& renderObject : _renderObjects[ETOI(ERenderGroup::NonLight)])
    {
        if (renderObject)
            renderObject->Render();
        _drawCallCount++;
    }

    _renderObjects[ETOI(ERenderGroup::NonLight)].clear();
}

void Renderer::Render_Combined()
{
    if (FAILED(_deferredShader->Bind_Matrix("g_WorldMatrix", &_worldMatrix))) return;
    if (FAILED(_deferredShader->Bind_Matrix("g_ViewMatrix", &_viewMatrix)))  return;
    if (FAILED(_deferredShader->Bind_Matrix("g_ProjMatrix", &_projMatrix)))  return;

    if (FAILED(GAME->Bind_RT_ShaderResource(_deferredShader, "g_DiffuseTexture", L"Target_Diffuse"))) return;
    if (FAILED(GAME->Bind_RT_ShaderResource(_deferredShader, "g_NormalTexture", L"Target_Normal"))) return;
    if (FAILED(GAME->Bind_RT_ShaderResource(_deferredShader, "g_ShadeTexture", L"Target_Shade")))   return;
    if (FAILED(GAME->Bind_RT_ShaderResource(_deferredShader, "g_DepthTexture", L"Target_Depth"))) return;
    if (FAILED(GAME->Bind_RT_ShaderResource(_deferredShader, "g_SpecularTexture", L"Target_Specular"))) return;

    if (FAILED(_deferredShader->Bind_Matrix("g_ViewMatrixInverse", GAME->Get_TransformInverse(ETransformState::View)))) return;
    if (FAILED(_deferredShader->Bind_Matrix("g_ProjMatrixInverse", GAME->Get_TransformInverse(ETransformState::Proj)))) return;
    if (FAILED(GAME->Bind_ShadowMatrices(_deferredShader, "g_LightViewMatrix", "g_LightProjMatrix"))) return;
    if (FAILED(GAME->Bind_RT_ShaderResource(_deferredShader, "g_LightDepthTexture", L"Target_LightDepth"))) return;

    const int lightCastsShadow = _shadowEnabled ? 1 : 0;
    if (FAILED(_deferredShader->Bind_RawValue("g_LightCastsShadow", &lightCastsShadow, sizeof(int)))) return;

    const FLightDesc* shadowDesc = GAME->Get_ShadowLightDesc();
    if (shadowDesc)
    {
        if (FAILED(_deferredShader->Bind_RawValue("g_ShadowBias", &shadowDesc->shadowBias, sizeof(float)))) return;
        if (FAILED(_deferredShader->Bind_RawValue("g_ShadowStrength", &shadowDesc->shadowStrength, sizeof(float)))) return;
        if (FAILED(_deferredShader->Bind_RawValue("g_ShadowSoftness", &shadowDesc->shadowSoftness, sizeof(float)))) return;
    }

    // 툰셰이딩 외곽선 처리
    {
        const float outlineInvViewportSize[2] =
        {
            1.f / max(1.f, static_cast<float>(GAME->Get_ViewportWidth())),
            1.f / max(1.f, static_cast<float>(GAME->Get_ViewportHeight()))
        };

        const float outlineNormalThreshold = 0.32f;
        const float outlineStrength = 0.45f;
        const Vec4 outlineColor = Vec4(0.04f, 0.05f, 0.08f, 1.f);

        if (FAILED(_deferredShader->Bind_RawValue("g_OutlineInvViewportSize", outlineInvViewportSize, sizeof(outlineInvViewportSize))))
            return;

        if (FAILED(_deferredShader->Bind_RawValue("g_PostOutlineNormalThreshold", &outlineNormalThreshold, sizeof(float))))
            return;

        if (FAILED(_deferredShader->Bind_RawValue("g_PostOutlineStrength", &outlineStrength, sizeof(float))))
            return;

        if (FAILED(_deferredShader->Bind_RawValue("g_PostOutlineColor", &outlineColor, sizeof(Vec4))))
            return;
    }

    // 블러처리
    {
        const float blurStrength = _blurStrength;

        const float blurDirection[2] =
        {
            _blurDirection.x,
            _blurDirection.y
        };

        const float blurInvViewportSize[2] =
        {
            1.f / max(1.f, static_cast<float>(GAME->Get_ViewportWidth())),
            1.f / max(1.f, static_cast<float>(GAME->Get_ViewportHeight()))
        };

        if (FAILED(_deferredShader->Bind_RawValue("g_ScreenBlurStrength", &blurStrength, sizeof(float))))
            return;

        if (FAILED(_deferredShader->Bind_RawValue("g_ScreenBlurDirection", blurDirection, sizeof(blurDirection))))
            return;

        if (FAILED(_deferredShader->Bind_RawValue("g_ScreenBlurInvViewportSize", blurInvViewportSize, sizeof(blurInvViewportSize))))
            return;
    }


    _deferredShader->Begin_Pass(ETOI(EDeferred::Combined));

    _viBuffer->Bind_Resources();
    _viBuffer->Render();
}

void Renderer::Apply_Default3DState()
{
    _context->OMSetDepthStencilState(_defaultDepthState.Get(), 0);
    _context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
}

void Renderer::Apply_UIState()
{
    _context->OMSetDepthStencilState(_uiDepthDisabledState.Get(), 0);
    _context->OMSetBlendState(_uiBlendState.Get(), nullptr, 0xffffffff);

}

void Renderer::Render_Debug()
{
    for (auto& debugComponent : _debugComponents)
        debugComponent->Render_Debug();

    _debugComponents.clear();

    if (FAILED(_deferredShader->Bind_Matrix("g_ViewMatrix", &_viewMatrix))) return;
    if (FAILED(_deferredShader->Bind_Matrix("g_ProjMatrix", &_projMatrix))) return;

    if (FAILED(GAME->Render_RT_Debug(_viBuffer, _deferredShader, L"MRT_GameObjects"))) return;
    if (FAILED(GAME->Render_RT_Debug(_viBuffer, _deferredShader, L"MRT_LightAcc")))    return;
    if (FAILED(GAME->Render_RT_Debug(_viBuffer, _deferredShader, L"MRT_ShadowObjects"))) return;
}

// 에디터 Game/Scene View의 실제 RT 크기가 바뀔 때마다 디버그 타일을 화면 안쪽 왼쪽 상단 기준으로 다시 배치한다.
HRESULT Renderer::Update_RTDebugLayout(uint32 width, uint32 height)
{
    if (width == 0 || height == 0)
        return S_OK;

    if (!GAME->Is_EditorRuntime())
    {
        CHECK_FAILED(GAME->Ready_RT_Debug(L"Target_Diffuse", 150.f, 300.f, 150.f, 150.f), E_FAIL);
        CHECK_FAILED(GAME->Ready_RT_Debug(L"Target_Normal", 150.f, 450.f, 150.f, 150.f), E_FAIL);
        CHECK_FAILED(GAME->Ready_RT_Debug(L"Target_Shade", 300.f, 300.f, 150.f, 150.f), E_FAIL);
        CHECK_FAILED(GAME->Ready_RT_Debug(L"Target_Specular", 300.f, 450.f, 150.f, 150.f), E_FAIL);
        CHECK_FAILED(GAME->Ready_RT_Debug(L"Target_LightDepth", 450.f, 300.f, 150.f, 150.f), E_FAIL);
        return S_OK;
    }

    const float viewportWidth = static_cast<float>(width);
    const float viewportHeight = static_cast<float>(height);
    const float halfViewportWidth = viewportWidth * 0.5f;
    const float halfViewportHeight = viewportHeight * 0.5f;
    const float minViewportAxis = (viewportWidth < viewportHeight) ? viewportWidth : viewportHeight;

    const float padding = ((minViewportAxis * 0.02f) > 12.f) ? (minViewportAxis * 0.02f) : 12.f;
    const float gap = ((minViewportAxis * 0.012f) > 8.f) ? (minViewportAxis * 0.012f) : 8.f;

    float tileSize = viewportWidth * 0.18f;
    const float maxTileByWidth = (viewportWidth - padding * 2.f - gap) * 0.5f;
    const float maxTileByHeight = (viewportHeight - padding * 2.f - gap * 2.f) / 3.f;

    if (tileSize > maxTileByWidth)
        tileSize = maxTileByWidth;

    if (tileSize > maxTileByHeight)
        tileSize = maxTileByHeight;

    if (tileSize > 180.f)
        tileSize = 180.f;

    if (tileSize < 72.f)
        tileSize = 72.f;

    struct FRTDebugSlot
    {
        const wchar_t* targetTag; // 배치할 렌더타겟 이름이다.
        uint32 column; // 왼쪽 기준 열 인덱스다.
        uint32 row; // 위쪽 기준 행 인덱스다.
    };

    const FRTDebugSlot debugSlots[] =
    {
        { L"Target_Diffuse", 0u, 0u },
        { L"Target_Normal", 0u, 1u },
        { L"Target_Shade", 0u, 2u },
        { L"Target_Specular", 1u, 0u },
        { L"Target_LightDepth", 1u, 1u },
    };

    for (const FRTDebugSlot& debugSlot : debugSlots)
    {
        const float centerX =
            -halfViewportWidth + padding + tileSize * 0.5f + debugSlot.column * (tileSize + gap);
        const float centerY =
            halfViewportHeight - padding - tileSize * 0.5f - debugSlot.row * (tileSize + gap);

        CHECK_FAILED(GAME->Ready_RT_Debug(debugSlot.targetTag, centerX, centerY, tileSize, tileSize), E_FAIL);
    }

    return S_OK;
}

HRESULT Renderer::Ready_RenderTarget()
{
    const uint32 width = static_cast<uint32>(GAME->Get_ViewportWidth());
    const uint32 height = static_cast<uint32>(GAME->Get_ViewportHeight());

    CHECK_FAILED(GAME->Add_RenderTarget(L"Target_Diffuse",
        width, height, DXGI_FORMAT_R8G8B8A8_UNORM, Color(0, 0, 0, 0)), E_FAIL);

    CHECK_FAILED(GAME->Add_RenderTarget(L"Target_Normal",
        width, height, DXGI_FORMAT_R16G16B16A16_UNORM, Color(0, 0, 0, 0)), E_FAIL);

    CHECK_FAILED(GAME->Add_RenderTarget(L"Target_Shade",
        width, height, DXGI_FORMAT_R16G16B16A16_UNORM, Color(0, 0, 0, 0)), E_FAIL);

    CHECK_FAILED(GAME->Add_RenderTarget(L"Target_Depth",
        width, height, DXGI_FORMAT_R32G32B32A32_FLOAT, Color(0, 0, 0, 0)), E_FAIL);

    CHECK_FAILED(GAME->Add_RenderTarget(L"Target_Specular",
        width, height, DXGI_FORMAT_R16G16B16A16_UNORM, Color(0, 0, 0, 0)), E_FAIL);

    // 굴절 디스토션용
    CHECK_FAILED(GAME->Add_RenderTarget(L"Target_SceneColorCopy",
        width, height, DXGI_FORMAT_R8G8B8A8_UNORM, Color(0, 0, 0, 0)), E_FAIL);

    const uint32 shadowWidth = Compute_ShadowRenderExtent(GAME->Get_ViewportWidth()); 
    const uint32 shadowHeight = Compute_ShadowRenderExtent(GAME->Get_ViewportHeight());

    CHECK_FAILED(GAME->Add_RenderTarget(L"Target_LightDepth",
        shadowWidth, shadowHeight, DXGI_FORMAT_R32G32B32A32_FLOAT, Color(1.f, 1.f, 1.f, 1.f), false), E_FAIL);

    CHECK_FAILED(GAME->Add_MRT(L"MRT_GameObjects", L"Target_Diffuse"), E_FAIL); // SV_TARGET0
    CHECK_FAILED(GAME->Add_MRT(L"MRT_GameObjects", L"Target_Normal"), E_FAIL);  // SV_TARGET1
    CHECK_FAILED(GAME->Add_MRT(L"MRT_GameObjects", L"Target_Depth"), E_FAIL);   // SV_TARGET2

    CHECK_FAILED(GAME->Add_MRT(L"MRT_LightAcc", L"Target_Shade"), E_FAIL);      // SV_TARGET0
    CHECK_FAILED(GAME->Add_MRT(L"MRT_LightAcc", L"Target_Specular"), E_FAIL);   // SV_TARGET1

    CHECK_FAILED(GAME->Add_MRT(L"MRT_ShadowObjects", L"Target_LightDepth"), E_FAIL);

    // Deferred Shader, 풀스크린 Rect 생성
    _viBuffer = VIBuffer_Rect::Create(_device, _context);
    CHECK_NULL(_viBuffer, E_FAIL);

    _deferredShader = Shader::Create(_device, _context,
        L"../../Client/Bin/Shaders/Shader_Deferred.hlsl", VTXTEX::Elements, VTXTEX::numElements);

    if (!_deferredShader) return E_FAIL;

    // 직교투영 행렬 세팅
    _worldMatrix = Matrix::CreateScale(width, height, 1.f);
    _viewMatrix = Matrix::Identity;
    _projMatrix = XMMatrixOrthographicLH(width, height, 0.f, 1.f);

#ifdef _DEBUG
    CHECK_FAILED(Update_RTDebugLayout(width, height), E_FAIL);
#endif

    return S_OK;
}

void Renderer::Render_Shadow()
{
    auto& shadowStaticObjects = _renderObjects[ETOI(ERenderGroup::ShadowStatic)];
    auto& shadowDynamicObjects = _renderObjects[ETOI(ERenderGroup::ShadowDynamic)];

#ifdef _DEBUG
    static constexpr bool ENABLE_EXPENSIVE_SHADOW_RT_STATS = false;
    static uint32 s_shadowLogFrame = 0; 
    ++s_shadowLogFrame;
    const bool shouldLogShadow = (s_shadowLogFrame % 300u) == 1u; // 디버그 콘솔 I/O가 프레임을 흔들지 않도록 약 5초마다만 출력한다.
    const uint32 submittedShadowStaticCount = static_cast<uint32>(shadowStaticObjects.size()); // 이번 프레임 static caster 수다.
    const uint32 submittedShadowDynamicCount = static_cast<uint32>(shadowDynamicObjects.size()); // 이번 프레임 dynamic caster 수다.
    uint32 renderedShadowCount = 0; // Render_Shadow가 성공해 실제 draw를 시도한 caster 수다.
    uint32 noDrawShadowCount = 0; // Render_Shadow는 호출됐지만 drawable mesh가 없었던 caster 수다.
    uint32 skippedShadowNullCount = 0; // null 포인터라 shadow pass에서 건너뛴 슬롯 수다.
    uint32 failedShadowCount = 0; // Render_Shadow가 실패한 caster 수다.
#endif

    if (!_shadowEnabled)
    {
#ifdef _DEBUG
        if (shouldLogShadow)
        {
            LOG_WARN("[Shadow] skipped because shadow is disabled. staticSubmitted={}, dynamicSubmitted={}",
                submittedShadowStaticCount,
                submittedShadowDynamicCount);
        }
#endif
        shadowStaticObjects.clear();
        shadowDynamicObjects.clear();
        return;
    }

    CHECK_FAILED(GAME->Begin_MRT(L"MRT_ShadowObjects", _shadowDepthDSV));
    _context->RSSetViewports(1, &_shadowViewport);
    _context->ClearDepthStencilView(_shadowDepthDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

    auto renderShadowGroup = [&](auto& shadowObjects, const char* groupLabel)
    {
        for (auto& renderObject : shadowObjects)
        {
            if (!renderObject)
            {
#ifdef _DEBUG
                ++skippedShadowNullCount;
#endif
                continue;
            }

            const HRESULT shadowHr = renderObject->Render_Shadow(); // single shadow RT에 caster를 실제로 찍는 호출 결과다.
            if (shadowHr == S_FALSE)
            {
#ifdef _DEBUG
                ++noDrawShadowCount;
                if (shouldLogShadow)
                {
                    LOG_WARN("[Shadow] Render_Shadow had no drawable mesh. group={}, {}",
                        groupLabel,
                        Make_ShadowLogObjectLabel(renderObject));
                }
#endif
                continue;
            }

            if (FAILED(shadowHr))
            {
#ifdef _DEBUG
                ++failedShadowCount;
                LOG_ERROR("[Shadow] Render_Shadow failed. group={}, hr=0x{:08X}, {}",
                    groupLabel,
                    static_cast<uint32>(shadowHr),
                    Make_ShadowLogObjectLabel(renderObject));
#endif
                continue;
            }

#ifdef _DEBUG
            ++renderedShadowCount;
#endif
        }
    };

    renderShadowGroup(shadowStaticObjects, "Static");
    renderShadowGroup(shadowDynamicObjects, "Dynamic");

    GAME->End_MRT();

#ifdef _DEBUG
    if (shouldLogShadow)
    {
        const FLightDesc* shadowDesc = GAME->Get_ShadowLightDesc(); // 이번 프레임 single shadow pass를 구성한 directional light 설정이다.
        if (shadowDesc)
        {
            LOG_INFO("[ShadowDesc] dir=({:.3f}, {:.3f}, {:.3f}) center=({:.3f}, {:.3f}, {:.3f}) ortho=({:.3f}, {:.3f}) useShadowCamera={} eye=({:.3f}, {:.3f}, {:.3f}) target=({:.3f}, {:.3f}, {:.3f}) fovy={:.3f} aspect={:.3f} near={:.3f} far={:.3f}",
                shadowDesc->direction.x,
                shadowDesc->direction.y,
                shadowDesc->direction.z,
                shadowDesc->shadowCenter.x,
                shadowDesc->shadowCenter.y,
                shadowDesc->shadowCenter.z,
                shadowDesc->shadowOrthoWidth,
                shadowDesc->shadowOrthoHeight,
                shadowDesc->useShadowCamera,
                shadowDesc->shadowEye.x,
                shadowDesc->shadowEye.y,
                shadowDesc->shadowEye.z,
                shadowDesc->shadowTarget.x,
                shadowDesc->shadowTarget.y,
                shadowDesc->shadowTarget.z,
                shadowDesc->shadowFovY,
                shadowDesc->shadowAspect,
                shadowDesc->shadowNear,
                shadowDesc->shadowFar);
        }

        Vec3 staticWorldPos = Vec3::Zero; // 첫 static caster의 월드 원점이다.
        Vec3 staticShadowNdc = Vec3::Zero; // 첫 static caster 원점이 shadow NDC에서 어디에 있는지 보여준다.
        for (const auto& renderObject : shadowStaticObjects)
        {
            if (Try_ComputeShadowNdcForObject(renderObject, staticWorldPos, staticShadowNdc))
            {
                LOG_INFO("[ShadowProbe][Static] {} world=({:.3f}, {:.3f}, {:.3f}) ndc=({:.3f}, {:.3f}, {:.3f})",
                    Make_ShadowLogObjectLabel(renderObject),
                    staticWorldPos.x,
                    staticWorldPos.y,
                    staticWorldPos.z,
                    staticShadowNdc.x,
                    staticShadowNdc.y,
                    staticShadowNdc.z);
                break;
            }
        }

        Vec3 dynamicWorldPos = Vec3::Zero; // 첫 dynamic caster의 월드 원점이다.
        Vec3 dynamicShadowNdc = Vec3::Zero; // 첫 dynamic caster 원점이 shadow NDC에서 어디에 있는지 보여준다.
        for (const auto& renderObject : shadowDynamicObjects)
        {
            if (Try_ComputeShadowNdcForObject(renderObject, dynamicWorldPos, dynamicShadowNdc))
            {
                LOG_INFO("[ShadowProbe][Dynamic] {} world=({:.3f}, {:.3f}, {:.3f}) ndc=({:.3f}, {:.3f}, {:.3f})",
                    Make_ShadowLogObjectLabel(renderObject),
                    dynamicWorldPos.x,
                    dynamicWorldPos.y,
                    dynamicWorldPos.z,
                    dynamicShadowNdc.x,
                    dynamicShadowNdc.y,
                    dynamicShadowNdc.z);
                break;
            }
        }

        LOG_INFO("[Shadow] staticSubmitted={}, dynamicSubmitted={}, rendered={}, noDraw={}, nullSkipped={}, failed={}, viewport={}x{}",
            submittedShadowStaticCount,
            submittedShadowDynamicCount,
            renderedShadowCount,
            noDrawShadowCount,
            skippedShadowNullCount,
            failedShadowCount,
            _shadowViewport.Width,
            _shadowViewport.Height);

        if constexpr (ENABLE_EXPENSIVE_SHADOW_RT_STATS)
            GAME->Log_RT_DebugFloatStats(L"Target_LightDepth");
    }
#endif

    shadowStaticObjects.clear();
    shadowDynamicObjects.clear();
}

bool Renderer::Update_ShadowState()
{
    _shadowEnabled = GAME->Update_PrimaryShadowLight();
    if (!_shadowEnabled)
        return false;

    const FLightDesc* shadowDesc = GAME->Get_ShadowLightDesc();
    if (!shadowDesc)
    {
        _shadowEnabled = false;
        return false;
    }

    if (FAILED(Ensure_ShadowResources(*shadowDesc)))
    {
        _shadowEnabled = false;
        return false;
    }

    return true;
}

HRESULT Renderer::Ensure_ShadowResources(const FLightDesc& shadowDesc)
{
    (void)shadowDesc;

    const uint32 shadowWidth = Compute_ShadowRenderExtent(GAME->Get_ViewportWidth()); // shadow RT와 같은 고해상도 depth 폭이다.
    const uint32 shadowHeight = Compute_ShadowRenderExtent(GAME->Get_ViewportHeight()); // shadow RT와 같은 고해상도 depth 높이다.
    const uint32 desiredShadowMapSize = max(shadowWidth, shadowHeight); // 기존 단일 size 멤버와 호환하기 위해 더 큰 축을 보관한다.
    const bool sizeChanged =
        (_shadowMapSize != desiredShadowMapSize) ||
        (static_cast<uint32>(_shadowViewport.Width) != shadowWidth) ||
        (static_cast<uint32>(_shadowViewport.Height) != shadowHeight);
    if (!sizeChanged)
        return S_OK;

    _shadowMapSize = desiredShadowMapSize;

    _shadowViewport.TopLeftX = 0.f;
    _shadowViewport.TopLeftY = 0.f;
    _shadowViewport.Width = static_cast<float>(shadowWidth);
    _shadowViewport.Height = static_cast<float>(shadowHeight);
    _shadowViewport.MinDepth = 0.f;
    _shadowViewport.MaxDepth = 1.f;

    _shadowDepthTexture.Reset();
    _shadowDepthDSV.Reset();

    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = shadowWidth;
    depthDesc.Height = shadowHeight;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    CHECK_FAILED(_device->CreateTexture2D(&depthDesc, nullptr, _shadowDepthTexture.GetAddressOf()), E_FAIL);
    CHECK_FAILED(_device->CreateDepthStencilView(_shadowDepthTexture.Get(), nullptr, _shadowDepthDSV.GetAddressOf()), E_FAIL);

    return S_OK;
}

void Renderer::Render_ScreenDistortion()
{
    auto& renderObjects = _renderObjects[ETOI(ERenderGroup::ScreenDistortion)];

    if (renderObjects.empty())
        return;

    // 현재 렌더타겟 복사하고,
    if (FAILED(GAME->Copy_CurrentRenderTargetToRT(L"Target_SceneColorCopy")))
    {
        renderObjects.clear();
        return;
    }

    for (auto& renderObject : renderObjects)
    {
        if (renderObject)
            renderObject->Render();

        _drawCallCount++;
    }

    renderObjects.clear();
}

unique_ptr<Renderer> Renderer::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_unique<Renderer>(device, context);

    if (FAILED(instance->Initialize()))
    {
        return nullptr;
    }

    return instance;
}

void Renderer::Free()
{
    Base::Free();

    for (auto& renderObjects : _renderObjects)
    {
        renderObjects.clear();
    }
}
