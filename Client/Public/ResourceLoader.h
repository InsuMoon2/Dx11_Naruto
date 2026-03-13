#pragma once

#include "Base.h"
#include "Client_Struct.h"

NS_BEGIN(Client)
    class ResourceLoader : public Base
{
public:
    explicit ResourceLoader(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~ResourceLoader() = default;

public:
    HRESULT Initialize();

    HRESULT Load_TextureTable(const wstring& tablePath);
    HRESULT Load_ShaderTable(const wstring& tablePath);
    HRESULT Load_TerrainTable(const wstring& tablePath);
    HRESULT Load_ModelTable(const wstring& tablePath);
    HRESULT Load_SkillTable(const wstring& tablePath);

public:
    struct FInputLayoutInfo
    {
        const D3D11_INPUT_ELEMENT_DESC* desc;
        uint32 count;
    };

    static FInputLayoutInfo Get_InputLayout(const string& name);

    HRESULT Build_TextureJobs(const wstring& tablePath, vector<FLoadJob>& outJobs);
    HRESULT Build_ShaderJobs(const wstring& tablePath, vector<FLoadJob>& outJobs);
    HRESULT Build_TerrainJobs(const wstring& tablePath, vector<FLoadJob>& outJobs);
    HRESULT Build_ModelJobs(const wstring& tablePath, vector<FLoadJob>& outJobs);
    HRESULT Build_SkillJobs(const wstring& tablePath, vector<FLoadJob>& outJobs);

private:
    HRESULT Load_Textures(const json& data);
    HRESULT Load_Shaders(const json& data);
    HRESULT Load_Terrains(const json& data);

    // 안정화되면 삭제
    HRESULT Load_Model(const json& data);
    HRESULT Load_Skills(const json& data);

    uint32  Get_ComponentID_From_String(const string& idStr);
    uint32  Get_LevelIndex_From_String(const string& levelName);

private:
    ComPtr<Device>          _device;
    ComPtr<DeviceContext>   _context;

public:
    static shared_ptr<ResourceLoader> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    void Free() override;
};

NS_END
