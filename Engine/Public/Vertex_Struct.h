

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
        // [추가] layered material의 blend 텍스처가 참조하는 보조 UV 세트다.
        Vec2    texcoord1;

        static const uint32 numElements = { 5 };

        static constexpr D3D11_INPUT_ELEMENT_DESC Elements[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0},
            { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0}
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

    typedef struct FVertexPos
    {
        Vec3 position;

        static const uint32 numElements = { 1 };


        static constexpr D3D11_INPUT_ELEMENT_DESC Elements[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
    } VTXPOS;

    typedef struct FVertexParticleInstnace
    {
        Vec4 right;
        Vec4 up;
        Vec4 look;
        Vec4 translation;
        Vec2 lifetime;      // x = 최대 수명, y = 현재 누적된 시간으로 사용
    } VTXPARTICLE_INSTANCE;

    typedef struct FVertexParticlePointInstanceDesc
    {
        static const uint32 numElements = { 6 };

        static constexpr D3D11_INPUT_ELEMENT_DESC Elements[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },

            { "WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    1, 64, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        };
    } VTXPARTICLE_POINT_INSTANCE_DESC;

}
