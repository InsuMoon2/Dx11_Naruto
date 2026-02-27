#pragma once

namespace Engine
{
	typedef struct tagEngineDesc
    {
        HWND        hWnd;
        EWinMode    winMode;

        uint32      numLevels;
        uint32      viewportWidth;
        uint32      viewportHeight;

    } ENGINE_DESC;

    typedef struct tagEditorDesc
    {
        HWND        hWnd;
        EWinMode    winMode;

        uint32      viewportWidth;
        uint32      viewportHeight;

    } EDITOR_DESC;


    struct FBlackboardKeyInfo
    {
        string name;
        EBlackboardValueType type;
    };

    struct FLightDesc
    {
        ELightType  type;

        Vec4        direction;

        Vec4        position;
        float       range;

        Color       diffuse;
        Color       ambient;
        Color       specular;
    };

    struct FAssetMeta
    {
        string  guid;
        string  type;           // prefab, texture, behavior 등등

        wstring relativePath;
        wstring fullPath;
    };
}
