#include "pch.h"
#include "Debug_Manager.h"

#include "DebugDraw.h"
#include "GameInstance.h"
#include "Mesh.h"
#include "Model.h"

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

    updateEntries(_depthMeshes);
    updateEntries(_overlayMeshes);
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

    if (_depthBoxes.empty() &&
        _depthSpheres.empty() &&
        _depthLines.empty() &&
        _depthMeshes.empty())
    {
        return S_OK;
    }

    CHECK_FAILED(Prepare_RenderState(), E_FAIL);

    _batch->Begin();

    Render_BoxEntries(_depthBoxes);
    Render_SphereEntries(_depthSpheres);
    Render_LineEntries(_depthLines);
    Render_MeshEntries(_depthMeshes);

    _batch->End();

    return S_OK;
}

HRESULT Debug_Manager::Render_Overlay()
{
    if (!_isEnabled)
        return S_OK;

    if (_overlayBoxes.empty() &&
        _overlaySpheres.empty() &&
        _overlayLines.empty() &&
        _overlayMeshes.empty())
    {
        return S_OK;
    }

    CHECK_FAILED(Prepare_RenderState(), E_FAIL);

    _batch->Begin();

    Render_BoxEntries(_overlayBoxes);
    Render_SphereEntries(_overlaySpheres);
    Render_LineEntries(_overlayLines);
    Render_MeshEntries(_overlayMeshes);

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

void Debug_Manager::Draw_TraceLine(const FDebugTraceLineDesc& desc)
{
    // [추가] 공통 렌더 옵션을 한 번만 맞춰서 trace 전체 표현에 재사용한다.
    FDebugRenderStyle sharedStyle{};
    sharedStyle.duration = desc.duration;
    sharedStyle.depthEnabled = desc.depthEnabled;

    if (!desc.isHit)
    {
        // [추가] miss일 때는 전체 구간을 missColor 선 하나로 표시한다.
        FDebugLineDesc lineDesc{};
        lineDesc.start = desc.start;
        lineDesc.end = desc.end;
        lineDesc.style = sharedStyle;
        lineDesc.style.color = desc.missColor;

        Draw_Line(lineDesc);
        return;
    }

    // [추가] hit일 때는 start -> hitPoint를 hitColor로 표시한다.
    FDebugLineDesc hitLineDesc{};
    hitLineDesc.start = desc.start;
    hitLineDesc.end = desc.hitPoint;
    hitLineDesc.style = sharedStyle;
    hitLineDesc.style.color = desc.hitColor;
    Draw_Line(hitLineDesc);

    if (desc.drawRemainderOnHit)
    {
        // [추가] 필요하면 hit 뒤 남은 구간도 remainderColor로 표시해서 막힌 위치를 더 쉽게 본다.
        FDebugLineDesc remainderDesc{};
        remainderDesc.start = desc.hitPoint;
        remainderDesc.end = desc.end;
        remainderDesc.style = sharedStyle;
        remainderDesc.style.color = desc.remainderColor;
        Draw_Line(remainderDesc);
    }

    if (desc.drawHitPoint)
    {
        // [추가] 히트 지점을 구로 표시해서 벽에 실제로 맞은 위치를 한눈에 보이게 한다.
        FDebugSphereDesc sphereDesc{};
        sphereDesc.center = desc.hitPoint;
        sphereDesc.radius = desc.hitPointRadius;
        sphereDesc.style = sharedStyle;
        sphereDesc.style.color = desc.hitPointColor;
        Draw_Sphere(sphereDesc);
    }

    if (desc.drawHitNormal)
    {
        // [추가] 히트 노멀 방향을 짧은 보조선으로 그려서 부착/반사/정면 여부를 바로 확인한다.
        Vec3 safeNormal = desc.hitNormal;
        if (safeNormal.LengthSquared() <= FLT_EPSILON)
            safeNormal = Vec3::Up;

        safeNormal = safeNormal;
        safeNormal.Normalize();

        FDebugLineDesc normalDesc{};
        normalDesc.start = desc.hitPoint;
        normalDesc.end = desc.hitPoint + safeNormal * desc.hitNormalLength;
        normalDesc.style = sharedStyle;
        normalDesc.style.color = desc.hitNormalColor;
        Draw_Line(normalDesc);
    }
}

void Debug_Manager::Draw_Mesh(const FDebugMeshDesc& desc)
{
    if (!desc.model) return;

    FDebugMeshEntry entry{};
    entry.model = desc.model;
    entry.worldMatrix = desc.worldMatrix;
    entry.color = desc.style.color;
    entry.lifetime.isOneFrame = (desc.style.duration <= 0.f);
    entry.lifetime.remainingTime = max(0.f, desc.style.duration);

    if (desc.style.depthEnabled)
        _depthMeshes.push_back(entry);
    else
        _overlayMeshes.push_back(entry);
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

void Debug_Manager::Render_MeshEntries(const vector<FDebugMeshEntry>& entries)
{
    for (const FDebugMeshEntry& entry : entries)
    {
        XMMATRIX world = XMLoadFloat4x4(&entry.worldMatrix);
        XMVECTOR color = XMLoadFloat4(&entry.color);

        for (const auto& mesh : entry.model->Get_Meshes())
        {
            const auto& pos = mesh->Get_CPUPositions();
            const auto& idx = mesh->Get_CPUIndices();
            if (pos.empty() || idx.empty()) continue;
            for (size_t i = 0; i + 2 < idx.size(); i += 3)
            {
                XMVECTOR v0 = XMVector3Transform(XMLoadFloat3(&pos[idx[i]]), world);
                XMVECTOR v1 = XMVector3Transform(XMLoadFloat3(&pos[idx[i + 1]]), world);
                XMVECTOR v2 = XMVector3Transform(XMLoadFloat3(&pos[idx[i + 2]]), world);

                DirectX::VertexPositionColor vpc0(v0, color);
                DirectX::VertexPositionColor vpc1(v1, color);
                DirectX::VertexPositionColor vpc2(v2, color);
                _batch->DrawLine(vpc0, vpc1);
                _batch->DrawLine(vpc1, vpc2);
                _batch->DrawLine(vpc2, vpc0);
            }
        }
    }
}

void Debug_Manager::Clear()
{
    _depthBoxes.clear();
    _depthSpheres.clear();
    _depthLines.clear();
    _depthMeshes.clear();

    _overlayBoxes.clear();
    _overlaySpheres.clear();
    _overlayLines.clear();
    _overlayMeshes.clear();

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
