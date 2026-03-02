#include "pch.h"
#include "Inspector_Factory.h"
#include "CombatStat_Inspector.h"
#include "Transform_Inspector.h"
#include "Texture_Inspector.h"
#include "BehaviorTree_Inspector.h"
#include "Texture.h"

IMPLEMENT_SINGLETON(Inspector_Factory)

void Inspector_Factory::Initialize()
{
    _textureInspector = make_shared<Texture_Inspector>();

    Register_Inspector(Protocol::COMPONENT_TYPE_TRANSFORM, make_shared<Transform_Inspector>());
    //Register_Inspector(Protocol::COMPONENT_TYPE_COMBAT_STAT, make_shared<CombatStat_Inspector>());
    Register_Inspector(Protocol::COMPONENT_TYPE_TEXTURE_DEFAULT, make_shared<Texture_Inspector>());
    Register_Inspector(Protocol::COMPONENT_TYPE_AI, make_shared<BehaviorTree_Inspector>());

}

void Inspector_Factory::Register_Inspector(uint32 typeId, shared_ptr<Component_Inspector> insepctor)
{
    if (_inspectors.contains(typeId))
    {
        LOG_WARN("Inspector already : {}", typeId);

        return;
    }

    _inspectors.emplace(typeId, insepctor);
    //LOG_INFO("Inspector Registered: {}", typeId);
}

shared_ptr<Component_Inspector> Inspector_Factory::Get_Inspector(uint32 typeId)
{
    //LOG_INFO("Get_Insepctor called with id: {}, map size: {}", typeId, _inspectors.size());

    auto iter = _inspectors.find(typeId);
    if (iter == _inspectors.end())
        return nullptr;

    return iter->second;
}

Shared<Component_Inspector> Inspector_Factory::Get_Inspector_ByType(Shared<Component> component)
{
    if (dynamic_pointer_cast<Texture>(component))
        return _textureInspector;

    return nullptr;
}

bool Inspector_Factory::Has_Inspector(uint32 typeId)
{
    return _inspectors.contains(typeId);
}
