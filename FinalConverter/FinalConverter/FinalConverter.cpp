#include "pch.h"
#include "FBXLoader.h"
#include "Exporter.h"

int wmain(int argc, wchar_t* argv[])
{
    _wsetlocale(LC_ALL, L"korean");
    SetConsoleOutputCP(CP_UTF8);

    //vector<wstring> names = { L"candle", L"pillar", L"statue1", L"statue2", L"statue3", L"throne" };

    //for (int i = 1; i < 26; ++i) {
    //    if (i < 10)
    //    {
    //        wstring inputFbx = L"../FBX/mesh_0" + to_wstring(i) + L".fbx";          // 입력 FBX 파일
    //        wstring fbxDir = L"../FBX";
    //        wstring outputBase = L"../FBXOutput/mesh_0" + to_wstring(i);           // 출력 기본 이름

    //        FBXLoader loader;
    //        if (!loader.LoadFbx(inputFbx)) {
    //            wcout << L"FBX 로딩 실패!" << endl;
    //            return -1;
    //        }

    //        wcout << L"FBX 로딩중..." << endl;

    //        Exporter exporter;
    //        if (!exporter.ExportAll(loader, outputBase, fbxDir)) {
    //            wcout << L"바이너리 변환 실패!" << endl;
    //            return -1;
    //        }

    //        wcout << i << L"번째 파일 변환 완료!" << endl;
    //    }
    //    else if (i < 20)
    //    {
    //        wstring inputFbx = L"../FBX/mesh_" + to_wstring(i) + L".fbx";          // 입력 FBX 파일
    //        wstring fbxDir = L"../FBX";
    //        wstring outputBase = L"../FBXOutput/mesh_" + to_wstring(i);           // 출력 기본 이름

    //        FBXLoader loader;
    //        if (!loader.LoadFbx(inputFbx)) {
    //            wcout << L"FBX 로딩 실패!" << endl;
    //            return -1;
    //        }

    //        wcout << L"FBX 로딩중..." << endl;

    //        Exporter exporter;
    //        if (!exporter.ExportAll(loader, outputBase, fbxDir)) {
    //            wcout << L"바이너리 변환 실패!" << endl;
    //            return -1;
    //        }

    //        wcout << i << L"번째 파일 변환 완료!" << endl;
    //    }
    //    else
    //    {
    //        wstring inputFbx = L"../FBX/mesh_" + names[i - 20] + L".fbx";          // 입력 FBX 파일
    //        wstring fbxDir = L"../FBX";
    //        wstring outputBase = L"../FBXOutput/mesh_" + names[i - 20];           // 출력 기본 이름

    //        FBXLoader loader;
    //        if (!loader.LoadFbx(inputFbx)) {
    //            wcout << L"FBX 로딩 실패!" << endl;
    //            return -1;
    //        }

    //        wcout << L"FBX 로딩중..." << endl;

    //        Exporter exporter;
    //        if (!exporter.ExportAll(loader, outputBase, fbxDir)) {
    //            wcout << L"바이너리 변환 실패!" << endl;
    //            return -1;
    //        }

    //        wcout << i << L"번째 파일 변환 완료!" << endl;
    //    }
    //}

    wstring inputFbx = L"../FBX/knight6_Animation_Attack1.fbx";          // 입력 FBX 파일
    wstring fbxDir = L"../FBX";
    wstring outputBase = L"../FBXOutput/knight6_Animation_Attack1";           // 출력 기본 이름

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
