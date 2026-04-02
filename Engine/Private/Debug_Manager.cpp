#include "pch.h"
#include "Debug_Manager.h"

#include "DebugDraw.h"
#include "GameInstance.h"

NS_BEGIN(Engine)

Debug_Manager::Debug_Manager(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : _device(device)
    , _context(context)
{
}

HRESULT Debug_Manager::Initialize()
{
    _batch = make_shared<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(_context.Get());
    _effect = make_shared<DirectX::BasicEffect>(_device.Get());

    CHECK_NULL(_batch, E_FAIL);
    CHECK_NULL(_effect, E_FAIL);

    _effect->SetVertexColorEnabled(true);

    const void* shaderByteCode = nullptr;
    size_t shaderByteCodeLength = 0;
    _effect->GetVertexShaderBytecode(&shaderByteCode, &shaderByteCodeLength);

    if (FAILED(_device->CreateInputLayout(
        DirectX::VertexPositionColor::InputElements,
        DirectX::VertexPositionColor::InputElementCount,
        shaderByteCode,
        shaderByteCodeLength,
        _inputLayout.GetAddressOf())))
    {
        return E_FAIL;
    }

    return S_OK;
}

void Debug_Manager::Tick(float timeDelta)
{
    // 프레임 시작 시 이전 프레임의 1프레임 요청을 제거하고 유지 시간이 있는 요청은 남은 시간을 줄인다.
    auto updateEntries = [timeDelta]<typename TEntry>(vector<TEntry>& entries)
    {
        std::erase_if(entries, [timeDelta](TEntry& entry)
        {
            if (entry.lifetime.isOneFrame)
                return true;

            entry.lifetime.remainingTime -= timeDelta;
            return entry.lifetime.remainingTime <= 0.f;
        });
    };

    updateEntries(_depthBoxes);
    updateEntries(_depthSpheres);
    updateEntries(_depthLines);

    updateEntries(_overlayBoxes);
    updateEntries(_overlaySpheres);
    updateEntries(_overlayLines);
}

HRESULT Debug_Manager::Prepare_RenderState()
{
    CHECK_NULL(_effect, E_FAIL);

    _effect->SetWorld(Matrix::Identity);
    _effect->SetView(*GAME->Get_Transform(ETransformState::View));
    _effect->SetProjection(*GAME->Get_Transform(ETransformState::Proj));

    _context->IASetInputLayout(_inputLayout.Get());
    _effect->Apply(_context.Get());

    return S_OK;
}

HRESULT Debug_Manager::Render_Depth()
{
    if (!_isEnabled)
        return S_OK;

    if (_depthBoxes.empty() && _depthSpheres.empty() && _depthLines.empty())
        return S_OK;

    CHECK_FAILED(Prepare_RenderState(), E_FAIL);

    _batch->Begin();

    Render_BoxEntries(_depthBoxes);
    Render_SphereEntries(_depthSpheres);
    Render_LineEntries(_depthLines);

    _batch->End();

    return S_OK;
}

HRESULT Debug_Manager::Render_Overlay()
{
    if (!_isEnabled)
        return S_OK;

    if (_overlayBoxes.empty() && _overlaySpheres.empty() && _overlayLines.empty())
        return S_OK;

    CHECK_FAILED(Prepare_RenderState(), E_FAIL);

    _batch->Begin();

    Render_BoxEntries(_overlayBoxes);
    Render_SphereEntries(_overlaySpheres);
    Render_LineEntries(_overlayLines);

    _batch->End();

    return S_OK;
}

void Debug_Manager::Draw_Box(const FDebugBoxDesc& desc)
{
    FDebugBoxEntry entry{};
    entry.box = BoundingOrientedBox(desc.center, desc.extents, desc.rotation);
    entry.color = desc.style.color;
    entry.lifetime.isOneFrame = (desc.style.duration <= 0.f);
    entry.lifetime.remainingTime = max(0.f, desc.style.duration);

    if (desc.style.depthEnabled)
        _depthBoxes.push_back(entry);
    else
        _overlayBoxes.push_back(entry);
}

void Debug_Manager::Draw_Sphere(const FDebugSphereDesc& desc)
{
    FDebugSphereEntry entry{};
    entry.sphere = BoundingSphere(desc.center, desc.radius);
    entry.color = desc.style.color;
    entry.lifetime.isOneFrame = (desc.style.duration <= 0.f);
    entry.lifetime.remainingTime = max(0.f, desc.style.duration);

    if (desc.style.depthEnabled)
        _depthSpheres.push_back(entry);
    else
        _overlaySpheres.push_back(entry);
}

void Debug_Manager::Draw_Line(const FDebugLineDesc& desc)
{
    FDebugLineEntry entry{};
    entry.start = desc.start;
    entry.end = desc.end;
    entry.color = desc.style.color;
    entry.lifetime.isOneFrame = (desc.style.duration <= 0.f);
    entry.lifetime.remainingTime = max(0.f, desc.style.duration);

    if (desc.style.depthEnabled)
        _depthLines.push_back(entry);
    else
        _overlayLines.push_back(entry);
}

void Debug_Manager::Render_BoxEntries(const vector<FDebugBoxEntry>& entries)
{
    for (const FDebugBoxEntry& entry : entries)
    {
        DX::Draw(_batch.get(), entry.box, entry.color);
    }
}

void Debug_Manager::Render_SphereEntries(const vector<FDebugSphereEntry>& entries)
{
    for (const FDebugSphereEntry& entry : entries)
    {
        DX::Draw(_batch.get(), entry.sphere, entry.color);
    }
}

void Debug_Manager::Render_LineEntries(const vector<FDebugLineEntry>& entries)
{
    for (const FDebugLineEntry& entry : entries)
    {
        DirectX::VertexPositionColor startVertex(entry.start, entry.color);
        DirectX::VertexPositionColor endVertex(entry.end, entry.color);
        _batch->DrawLine(startVertex, endVertex);
    }
}

void Debug_Manager::Clear()
{
    _depthBoxes.clear();
    _depthSpheres.clear();
    _depthLines.clear();

    _overlayBoxes.clear();
    _overlaySpheres.clear();
    _overlayLines.clear();
}

Unique<Debug_Manager> Debug_Manager::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_unique<Debug_Manager>(device, context);

    if (FAILED(instance->Initialize()))
    {
        MSG_BOX("Failed to Create : Debug_Manager");
        return nullptr;
    }

    return instance;
}

void Debug_Manager::Free()
{
    Base::Free();

    Clear();

    _batch.reset();
    _effect.reset();
    _inputLayout.Reset();
}

NS_END
