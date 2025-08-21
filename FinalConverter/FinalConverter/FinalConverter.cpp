#include "pch.h"
#include "FBXLoader.h"
#include "Exporter.h"

int wmain(int argc, wchar_t* argv[])
{
    _wsetlocale(LC_ALL, L"korean");
    SetConsoleOutputCP(CP_UTF8);

    wstring inputFbx = L"../FBX/knight5_Idle.fbx";          // 입력 FBX 파일
    wstring fbxDir = L"../FBX";
    wstring outputBase = L"../FBXOutput/knight5_Idle";           // 출력 기본 이름

    FBXLoader loader;
    if (!loader.LoadFbx(inputFbx)) {
        wcout << L"FBX 로딩 실패!" << endl;
        return -1;
    }

    wcout << L"FBX 로딩중..." << endl;

    Exporter exporter;
    if (!exporter.ExportAll(loader, outputBase, fbxDir)) {
        wcout << L"바이너리 변환 실패!" << endl;
        return -1;
    }

    wcout << L"변환 완료!" << endl;
    return 0;
}
