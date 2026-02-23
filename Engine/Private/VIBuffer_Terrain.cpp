#include "pch.h"
#include "VIBuffer_Terrain.h"

VIBuffer_Terrain::VIBuffer_Terrain(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : VIBuffer(device, context)
{

}

VIBuffer_Terrain::VIBuffer_Terrain(const VIBuffer_Terrain& rhs)
    : VIBuffer(rhs)
    , _numVerticesX(rhs._numVerticesX)
    , _numVerticesZ(rhs._numVerticesZ)
{

}

VIBuffer_Terrain::~VIBuffer_Terrain()
{
}

HRESULT VIBuffer_Terrain::Initialize_Prototype(const wstring& heightMapPath)
{
    // 1. 하이트맵 BMP 로드
    HANDLE hFile = CreateFile(heightMapPath.c_str(),
        GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    BITMAPFILEHEADER    fileHeader;
    BITMAPINFOHEADER    infoHeader;
    DWORD               dwByte = {};

    ReadFile(hFile, &fileHeader, sizeof(fileHeader), &dwByte, nullptr);
    ReadFile(hFile, &infoHeader, sizeof(infoHeader), &dwByte, nullptr);

    _numVerticesX = infoHeader.biWidth;
    _numVerticesZ = infoHeader.biHeight;

    // 픽셀 데이터 읽기 (BGRA 4바이트씩)
    unsigned long* pixel = new unsigned long[_numVerticesX * _numVerticesZ];
    ReadFile(hFile, pixel, sizeof(unsigned long) * _numVerticesX * _numVerticesZ, &dwByte, nullptr);
    CloseHandle(hFile);

    // 2. 정점 버퍼 생성
    _numVertexBuffers = 1;
    _numVertices = _numVerticesX * _numVerticesZ;
    _vertexStride = sizeof(FVertexNormalTex);
    _primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    FVertexNormalTex* vertices = new FVertexNormalTex[_numVertices];
    ZeroMemory(vertices, sizeof(FVertexNormalTex) * _numVertices);

    for (uint32 z = 0; z < _numVerticesZ; ++z)
    {
        for (uint32 x = 0; x < _numVerticesX; ++x)
        {
            uint32 idx = z * _numVerticesX + x;

            vertices[idx].position = Vec3(
                float(x),
                float(pixel[idx] & 0x000000ff) / 10.f,  // R채널 높이
                float(z)
            );

            vertices[idx].normal = Vec3(0.f, 0.f, 0.f);  // 이후 계산

            vertices[idx].texCoord = Vec2(
                float(x) / (_numVerticesX - 1),
                float(z) / (_numVerticesZ - 1)
            );
        }
    }
     
    Safe_Delete_Array(pixel);

    // 3. 인덱스 버퍼 생성
    _numIndices = (_numVerticesX - 1) * (_numVerticesZ - 1) * 6;
    _indexStride = sizeof(uint32);

    uint32* indices = new uint32[_numIndices];
    uint32  idx = 0;

    for (uint32 z = 0; z < _numVerticesZ - 1; ++z)
    {
        for (uint32 x = 0; x < _numVerticesX - 1; ++x)
        {
            // 삼각형 1 (왼쪽 위)
            indices[idx++] = (z + 1) * _numVerticesX + x;
            indices[idx++] = (z + 1) * _numVerticesX + (x + 1);
            indices[idx++] = z * _numVerticesX + (x + 1);

            // 삼각형 2 (오른쪽 아래)
            indices[idx++] = (z + 1) * _numVerticesX + x;
            indices[idx++] = z * _numVerticesX + (x + 1);
            indices[idx++] = z * _numVerticesX + x;
        }
    }

    // 4. 법선 벡터 계산
    for (uint32 i = 0; i < _numIndices; i += 3)
    {
        uint32 i0 = indices[i + 0];
        uint32 i1 = indices[i + 1];
        uint32 i2 = indices[i + 2];

        Vec3 v0 = vertices[i0].position;
        Vec3 v1 = vertices[i1].position;
        Vec3 v2 = vertices[i2].position;

        Vec3 edge1 = v1 - v0;
        Vec3 edge2 = v2 - v0;

        Vec3 faceNormal = edge1.Cross(edge2);

        vertices[i0].normal += faceNormal;
        vertices[i1].normal += faceNormal;
        vertices[i2].normal += faceNormal;
    }

    for (uint32 i = 0; i < _numVertices; ++i)
    {
        vertices[i].normal.Normalize();
    }

    // 5. D3D11 버퍼 생성
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = _vertexStride * _numVertices;
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertices;

    _device->CreateBuffer(&vbDesc, &vbData, _vertexBuffer.GetAddressOf());

    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.ByteWidth = _indexStride * _numIndices;
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices;
    _device->CreateBuffer(&ibDesc, &ibData, _indexBuffer.GetAddressOf());

    Safe_Delete_Array(vertices);
    Safe_Delete_Array(indices);

    return S_OK;
}

HRESULT VIBuffer_Terrain::Initialize(void* arg)
{
    return VIBuffer::Initialize(arg);
}

void VIBuffer_Terrain::BeginPlay()
{
    VIBuffer::BeginPlay();
}

Shared<VIBuffer_Terrain> VIBuffer_Terrain::Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
                                                  const wstring& heightMapPath)
{
    auto instance = make_shared<VIBuffer_Terrain>(device, context);

    if (FAILED(instance->Initialize_Prototype(heightMapPath)))
    {
        MSG_BOX("Failed to Created : VIBuffer_Terrain");

        return nullptr;
    }

    return instance;
}

Shared<Component> VIBuffer_Terrain::Clone(void* arg)
{
    auto clone = make_shared<VIBuffer_Terrain>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : VIBuffer_Terrain");

        return nullptr;
    }

    return clone;
}

void VIBuffer_Terrain::Free()
{
    VIBuffer::Free();
}
