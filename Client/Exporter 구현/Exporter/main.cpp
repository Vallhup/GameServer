#include "FBXloader.h"
#include "Exporter.h"

int wmain(int argc, wchar_t* argv[])
{
    std::wcout << L"argc: " << argc << std::endl;

    if (argc < 3)
    {
        std::wcout << L"사용법: Exporter.exe [입력파일.fbx] [출력파일.bin]" << std::endl;
        return -1;
    }

    std::wstring inputFbx = argv[1];
    std::wstring outputBin = argv[2];

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    FBXLoader loader;
    if (!loader.Load(inputFbx, vertices, indices))
    {
        std::wcout << L"FBX 로딩 실패: " << inputFbx << std::endl;
        return -1;
    }

    // 바이너리 형식 추출
    if (!ExportToBinary(outputBin, vertices, indices))
    {
        std::wcout << L"BIN 저장 실패: " << outputBin << std::endl;
        return -1;
    }

    // text 형식 추출
    /*if (!ExportToText(outputBin, vertices, indices))
    {
        std::wcout << L"BIN 저장 실패: " << outputBin << std::endl;
        return -1;
    }*/

    std::wcout << L"Export Completed" << outputBin << std::endl;

    return 0;
}