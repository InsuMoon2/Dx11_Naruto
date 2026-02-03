#pragma once

NS_BEGIN(Engine)

class Component;

class Component_Factory
{
private:
    Component_Factory() = delete;
    ~Component_Factory() = delete;

public:
    static void Initialize();
    static void Register(uint32 typeId, const wstring& prototypeTag);

private:
    static map<uint32, wstring> _prototypeMap;

public:
    static shared_ptr<Component> Create(uint32 typeId, uint32 levelIndex, void* arg = {});

};

NS_END
