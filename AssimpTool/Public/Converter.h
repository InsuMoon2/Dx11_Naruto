#pragma once

NS_BEGIN(Assimp)

class Converter
{
public:
    explicit Converter();
    virtual ~Converter() = default;

public:
    void ReadAssetFile(const wstring& filePath);

private:
    shared_ptr<Assimp::Importer> _importer;
    const aiScene*               _scene = nullptr;

public:
    static unique_ptr<Converter> Create();

};

NS_END
