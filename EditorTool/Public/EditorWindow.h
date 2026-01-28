#pragma once

NS_BEGIN(Editor)

class EditorWindow abstract
{
public:
    EditorWindow(const wstring& name);
    virtual ~EditorWindow() = default;

public:
    virtual void    Initialize()    { }
    virtual void    Update()        { }
    virtual void    OnGui()         = 0;

public:
    const wstring&  Get_Name() const { return _name; }
    bool            IsActive() const { return _isActive; }
    void            Set_Active(bool active) { _isActive = active; }

protected:
    wstring  _name;
    bool    _isActive = true;

};

NS_END
