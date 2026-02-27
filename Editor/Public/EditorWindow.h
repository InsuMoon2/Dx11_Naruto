#pragma once

NS_BEGIN(Editor)

class EditorWindow abstract
{
public:
    EditorWindow(const wstring& name);
    virtual ~EditorWindow() = default;

public:
    virtual void    Initialize() { }
    virtual void    Update(float timeDelta) { }
    virtual void    OnGui() = 0;
    virtual void    Pre_Render() {};

public:
    const wstring&  Get_Name() const { return _name; }
    bool            IsActive() const { return _isActive; }
    void            Set_Active(bool active) { _isActive = active; }

protected:
    wstring  _name;
    bool     _isActive = true;

};

NS_END
