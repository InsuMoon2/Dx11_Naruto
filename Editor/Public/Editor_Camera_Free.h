#pragma once

#include "Camera_Free.h"

NS_BEGIN(Editor)

class Editor_Camera_Free : public Camera_Free
{
    GENERATED_BODY(Editor_Camera_Free)

public:
    struct FEditorCameraDesc : public FCameraFreeDesc
    {
        // 추가할게 있나
    };

public:
    explicit Editor_Camera_Free(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Editor_Camera_Free(const Editor_Camera_Free& rhs);
    virtual ~Editor_Camera_Free() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    
    void    Priority_Update(float timeDelta) override;
    
    Matrix  Get_ViewMatrix() const;

private:
    

public:
    static Shared<Editor_Camera_Free> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};

NS_END
