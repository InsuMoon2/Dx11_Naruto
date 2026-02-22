#pragma once

#include "Engine_Typedef.h"

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

    typedef struct FVertexTex
    {
        Vec3        position;
        Vec2        texCoord;

        static const unsigned int Vertex_Desc_Layout_Count = { 2 }; // position, textcoord 2개

        static constexpr D3D11_INPUT_ELEMENT_DESC	Vertex_Desc_Layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
        };

    } VTXTEX;

    typedef struct FVertexNormalTex
    {
        Vec3    position;
        Vec3    normal;
        Vec2    texCoord;

        static constexpr D3D11_INPUT_ELEMENT_DESC NormalTex_Layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
        };

    } VTXNORTEX;

    struct FBlackboardKeyInfo
    {
        string name;
        EBlackboardValueType type;
    };
}
