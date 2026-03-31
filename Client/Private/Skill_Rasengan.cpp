#include "pch.h"
#include "Skill_Rasengan.h"
#include "GameObject_Factory.h"

REGISTER_GAMEOBJECT_CATEGORY(Skill_Rasengan, Protocol::OBJECT_TYPE_SKILL_RASENGAN, "SkillSpawn");

Skill_Rasengan::Skill_Rasengan(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : SkillObject(device, context)
{
}

Skill_Rasengan::Skill_Rasengan(const Skill_Rasengan& rhs)
    : SkillObject(rhs)
{
}

HRESULT Skill_Rasengan::Initialize_Prototype()
{
    return SkillObject::Initialize_Prototype();
}

HRESULT Skill_Rasengan::Initialize(void* arg)
{
    CHECK_FAILED(SkillObject::Initialize(arg), E_FAIL);

    return S_OK;
}

void Skill_Rasengan::Update(float timeDelta)
{
    SkillObject::Update(timeDelta);

    if (Is_Destroy())
        return;



}

Shared<GameObject> Skill_Rasengan::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Skill_Rasengan>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : Skill_Rasengan");

        return nullptr;
    }

    return instance;
}

Shared<GameObject> Skill_Rasengan::Clone(void* arg)
{
    auto clone = make_shared<Skill_Rasengan>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Skill_Rasengan");

        return nullptr;
    }

    return clone;
}

void Skill_Rasengan::Free()
{
    SkillObject::Free();
}
