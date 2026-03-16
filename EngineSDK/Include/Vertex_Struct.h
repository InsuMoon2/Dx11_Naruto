

namespace Engine
{
    typedef struct FVertexTex
    {
        Vec3        position;
        Vec2        texCoord;

        static const uint32 numElements = { 2 }; // position, textcoord 2개

        static constexpr D3D11_INPUT_ELEMENT_DESC Elements[] =
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

        static const uint32 numElements = { 3 };

        static constexpr D3D11_INPUT_ELEMENT_DESC Elements[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
        };

    } VTXNORTEX;

    typedef struct FVertexMesh
    {
        Vec3    position;
        Vec3    normal;
        Vec3    tangent;
        Vec2    texcoord;

        static const uint32 numElements = { 4 };

        static constexpr D3D11_INPUT_ELEMENT_DESC Elements[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0}
        };

    } VTXMESH;

    typedef struct FVertexAnimationMesh
    {
        Vec3    position;
        Vec3    normal;
        Vec3    tangent;
        Vec2    texcoord;

        XMUINT4 blendIndex;
        Vec4    blendWeight;

        static const uint32 numElements = { 6 };

        static constexpr D3D11_INPUT_ELEMENT_DESC Elements[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0},
            { "BLENDINDEX", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0},
            { "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 60, D3D11_INPUT_PER_VERTEX_DATA, 0}
        };

    } VTXANIM;

}
