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

    //-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    // 몬스터 애니메이션 추출 코드
    //-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    vector<wstring> names = { L"0_idle", L"1_walk", L"2_run", L"3_attack_combo1", L"3_attack_combo2", L"3_attack_combo3", L"3_attack_special", L"4_roll",
    L"5_parry", L"6_stun", L"7_hit", L"8_guard", L"9_drinking", L"a0_death", };

    for (int i = 0; i < 14; i++)
    {
        wstring inputFbx = L"../FBX/knight6_animation" + names[i] + L".fbx";
        wstring fbxDir = L"../FBX";
        wstring outputBase = L"../FBXOutput/knight6_animation" + names[i];           // 출력 기본 이름

        // 애니메이션 전용 FBX의 경우, 메시의 스켈레톤을 기준으로 매핑
        wstring referenceSkeleton = L"../FBXOutput/knight6.skel";  // 메시에서 추출한 스켈레톤

        FBXLoader loader;
        if (!loader.LoadFbx(inputFbx)) {
            wcout << L"FBX 로딩 실패!" << endl;
            return -1;
        }

        wcout << L"FBX 로딩 완료" << endl;

        Exporter exporter;
        if (!exporter.ExportAll(loader, outputBase, fbxDir, referenceSkeleton)) {
            wcout << L"바이너리 변환 실패!" << endl;
            return -1;
        }

        wcout << i << L"번째 파일 변환 완료!" << endl;
    }

    //-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    // 몬스터 T-pose 추출 코드
    //-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    //wstring inputFbx = L"../FBX/monster_Tank.fbx";          // 입력 FBX 파일
    //wstring fbxDir = L"../FBX";
    //wstring outputBase = L"../FBXOutput/monster_Tank";           // 출력 기본 이름
    //
    //// 애니메이션 전용 FBX의 경우, 메시의 스켈레톤을 기준으로 매핑
    //wstring referenceSkeleton = L"";  // 메시에서 추출한 스켈레톤
    //
    //FBXLoader loader;
    //if (!loader.LoadFbx(inputFbx)) {
    //    wcout << L"FBX 로딩 실패!" << endl;
    //    return -1;
    //}
    //
    //wcout << L"FBX 로딩 완료" << endl;
    //
    //Exporter exporter;
    //if (!exporter.ExportAll(loader, outputBase, fbxDir, referenceSkeleton)) {
    //    wcout << L"바이너리 변환 실패!" << endl;
    //    return -1;
    //}
    //
    //wcout << L"변환 완료!" << endl;
    return 0;
}
