#pragma once

NS_BEGIN(Engine)
class Component;
NS_END

NS_BEGIN(Editor)

class Component_Inspector
{
public:
    explicit Component_Inspector() = default;
    virtual ~Component_Inspector() = default;

public:
    virtual void Draw_Inspector(shared_ptr<Component> component) = 0;
    virtual uint32 Get_ComponentType() const = 0;

protected:
    bool Draw_Header(const string& name);
    bool Draw_Float(const string& label, float& value);
    void Draw_ProgressBar(float current, float max, const string& label = "");
    void Draw_ReadOnly(const string& label, float value);

    // JSON 수정 헬퍼
    void BeginEdit();
    void EndEdit(shared_ptr<Component> component);

private:
    bool _isDirty = false;

};

NS_END
