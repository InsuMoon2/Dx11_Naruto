#pragma once

#include <fstream>

NS_BEGIN(Assimp)

class BinaryWriter
{
public:
    BinaryWriter() = default;
    ~BinaryWriter();

public:
    bool Open(const wstring& filePath);
    void Close();

    bool IsOpen();

    template<typename T>
    void Write(const T& value)
    {
        _stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
    }

    void WriteBytes(const void* data, size_t size);
    void WriteString(const string& value);

private:
    ofstream _stream;

};

NS_END
