#include "pch.h"
#include "BinaryWriter.h"

namespace fs = filesystem;

BinaryWriter::~BinaryWriter()
{
    Close();
}

bool BinaryWriter::Open(const wstring& filePath)
{
    fs::path path(filePath);
    fs::create_directories(path.parent_path());

    _stream.open(path, ios_base::binary | ios_base::out | ios_base::trunc);

    return _stream.is_open();
}

void BinaryWriter::Close()
{
    if (_stream.is_open())
    {
        _stream.close();
    }
}

bool BinaryWriter::IsOpen()
{
    return _stream.is_open();
}

void BinaryWriter::WriteBytes(const void* data, size_t size)
{
    if (size == 0)
        return;

    _stream.write(reinterpret_cast<const char*>(data), static_cast<streamsize>(size));
}

void BinaryWriter::WriteString(const string& value)
{
    uint32 length = static_cast<uint32>(value.size());
    Write(length);

    if (length > 0)
    {
        WriteBytes(value.data(), length);
    }

}
