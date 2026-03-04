#pragma once

NS_BEGIN(Editor)

class EditorWindow abstract
{
public:
    EditorWindow(const wstring& name);
    virtual ~EditorWindow() = default;

public:
    virtual void    Initialize() {}
    virtual void    Update(float timeDelta) {}
    virtual void    OnGui() = 0; 
    virtual void    Pre_Render() {};

public:
    const wstring&  Get_Name() const { return _name; }
    bool            IsActive() const { return _isActive; }
    void            Set_Active(bool active) { _isActive = active; }

    bool            IsDirty() const { return _isDirty; }
    void            MarkDirty() { _isDirty = true; }
    void            ClearDirty() { _isDirty = false; }

    bool            IsFocused() const { return _isFocused; }

    virtual bool    CanSave() const { return false; }  
    virtual void    Save() {}

protected:
    wstring      _name;

    bool        _isActive = true;
    bool        _isDirty = false;
    bool        _isFocused = false;
};

NS_END
