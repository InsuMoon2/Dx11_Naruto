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

        // StaticMesh or SkeletalMesh
        string  modelType;
    };

    struct FKeyFrame
    {
        float   time = 0.f;

        Vec3    scale = Vec3(1.f, 1.f, 1.f);
        Quat    rotation = Quat::Identity;
        Vec3    translation = Vec3::Zero;
    };
    
}
