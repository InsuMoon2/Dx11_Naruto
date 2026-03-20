import re

def patch_file():
    path = r"d:\GitDesktop\Dx11_Naruto\AssimpTool\Private\Converter.cpp"
    with open(path, "r", encoding="utf-8") as f:
        content = f.read()
    
    # 1. Insert Write_AnimBin correctly
    target1 = "    if (!Write_MaterialJson(materialPath))\n        return false;\n\n    if (!Write_ModelMeta(meshPath, _resolvedModelType))"
    replacement1 = "    if (!Write_MaterialJson(materialPath))\n        return false;\n\n    if (!Write_AnimBin(dstBasePath))\n        return false;\n\n    if (!Write_ModelMeta(meshPath, _resolvedModelType))"
    
    # If the file has carriage returns \r\n, adjust
    target1_rn = target1.replace("\n", "\r\n")
    replacement1_rn = replacement1.replace("\n", "\r\n")
    
    if target1 in content:
        content = content.replace(target1, replacement1)
    elif target1_rn in content:
        content = content.replace(target1_rn, replacement1_rn)
    else:
        print("Failed to find chunk 1")

    # 2. Modify animationCount
    target2 = "header.animationCount = static_cast<uint32>(_animations.size());"
    replacement2 = "header.animationCount = 0;"
    if target2 in content:
        content = content.replace(target2, replacement2)
    else:
        print("Failed to find chunk 2")

    # 3. Replace the actual loop at the end of Write_MeshBin
    # We look for "3. Animation Clip Array" and substring all the way to "return true;\n}"
    # Wait, the string is "// [추가] 3. Animation Clip Array"
    start_str = "// [추가] 3. Animation Clip Array"
    idx = content.find(start_str)
    if idx != -1:
        # Find the END of Write_MeshBin function
        # It's right before "EConvertModelType Converter::Resolve_ModelType"
        end_str = "EConvertModelType Converter::Resolve_ModelType"
        idx_end = content.find(end_str, idx)
        if idx_end != -1:
            # Reconstruct the string from idx to idx_end
            new_logic = """    // 애니메이션 처리는 Write_AnimBin으로 옮겨졌습니다.

    return true;
}

bool Converter::Write_AnimBin(const wstring& dstBasePath)
{
    if (_animations.empty())
        return true;

    for (const auto& clip : _animations)
    {
        string safeName = clip.name;
        for (char& c : safeName)
        {
            if (c == '|' || c == ':' || c == '*' || c == '?' || c == '<' || c == '>' || c == ' ')
                c = '_';
        }

        wstring animPath = dstBasePath + L"_" + wstring(safeName.begin(), safeName.end()) + L".animbin";

        BinaryWriter writer;
        if (!writer.Open(animPath))
        {
            LOG_ERROR("Failed to open animbin output");
            continue;
        }

        FAnimationFileHeader header{};
        header.animationCount = 1;
        writer.Write(header);

        writer.WriteString(clip.name);

        FAnimationClipBin clipBin{};
        clipBin.duration = clip.duration;
        clipBin.ticksPerSecond = clip.ticksPerSecond;
        clipBin.channelCount = static_cast<uint32>(clip.channels.size());
        writer.Write(clipBin);

        for (const auto& channel : clip.channels)
        {
            writer.WriteString(channel.nodeName);

            FAnimationChannelBin channelBin{};
            channelBin.boneIndex = channel.boneIndex;
            channelBin.keyFrameCount = static_cast<uint32>(channel.keyFrames.size());
            writer.Write(channelBin);

            writer.WriteBytes(channel.keyFrames.data(),
                sizeof(FKeyFrameBin) * channel.keyFrames.size());
        }
    }

    return true;
}

"""
            # Replace the middle part
            part1 = content[:idx]
            part2 = content[idx_end:]
            content = part1 + new_logic + part2
        else:
            print("Failed to find end str for chunk 3")
    else:
        print("Failed to find start str for chunk 3")

    with open(path, "w", encoding="utf-8") as f:
        f.write(content)
        
    print("Patch applied successfully.")

patch_file()
