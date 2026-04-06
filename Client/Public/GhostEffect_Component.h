#pragma once

#include "Component.h"
#include "Shader.h"
#include "Model.h"

NS_BEGIN(Client)

class GhostEffect_Component : public Component
{
    GENERATED_COMPONENT(GhostEffect_Component, Protocol::COMPONENT_TYPE_GHOST_EFFECT)

public:
    explicit GhostEffect_Component(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit GhostEffect_Component(const GhostEffect_Component& rhs);
    virtual ~GhostEffect_Component() = default;

public:
    HRESULT Initialize(void* arg) override;
    void    BeginPlay() override;

    void    Update_GhostEffect(float timeDelta);
    HRESULT Render();

public:
    void Start_GhostEffect(float interval, float lifespan, Vec4 color, Vec4 rimColor);
    void Stop_GhostEffect();

    bool Has_ActiveGhosts() const { return !_ghostSnapshots.empty(); }

private:
    struct FGhostSnapshot
    {
        float lifespan = 0.f;
        float maxLifespan = 0.f;
        Matrix worldMatrix;
        vector<Matrix> boneMatrices;
        Vec4 color;
        Vec4 rimColor;
    };

    struct FGhostSettings
    {
        bool active = false;
        float captureTimer = 0.f;
        float captureInterval = 0.05f;
        float defaultLifespan = 0.3f;
        Vec4 color = Vec4(0.02f, 0.02f, 0.02f, 1.f);
        Vec4 rimColor = Vec4(0.1f, 0.4f, 1.0f, 1.f);
    };

private:
    FGhostSettings          _settings;
    vector<FGhostSnapshot>  _ghostSnapshots;
    Shared<Shader>          _ghostShader;

    Weak<Transform>     _ownerTransform;
    Weak<Model>         _ownerModel;

public:
    static Shared<GhostEffect_Component> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component> Clone(void* arg) override;
};

NS_END
