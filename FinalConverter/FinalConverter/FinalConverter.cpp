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

    //vector<wstring> names = { L"Block_1", L"Block_2", L"Charge", L"Death", L"GetHit_1", L"GetHit_2", L"GetHit_3", L"Idle_1", L"Idle_2", L"Idle_3", L"Idle_4", L"Jump_1", L"Jump_2",
    //L"Melee_1", L"Melee_2", L"Melee_3", L"Melee_4", L"Melee_5", L"Melee_6", L"Roar", L"Roar_2", L"Run", L"stun", L"Walk_Back", L"Walk_Forward", L"Walk_Forward_Slow", L"Walk_Left", L"Walk_Right" };

    //for (int i = 0; i < 28; i++)
    //{
    //    wstring inputFbx = L"../FBX/Demon_Executioner_" + names[i] + L".fbx";
    //    wstring fbxDir = L"../FBX";
    //    wstring outputBase = L"../FBXOutput/monster_Demon_Executioner_" + names[i];           // 출력 기본 이름

    //    // 애니메이션 전용 FBX의 경우, 메시의 스켈레톤을 기준으로 매핑
    //    wstring referenceSkeleton = L"../FBXOutput/monster_DemonExecutioner.skel";  // 메시에서 추출한 스켈레톤

    //    FBXLoader loader;
    //    if (!loader.LoadFbx(inputFbx)) {
    //        wcout << L"FBX 로딩 실패!" << endl;
    //        return -1;
    //    }

    //    wcout << L"FBX 로딩 완료" << endl;

    //    Exporter exporter;
    //    if (!exporter.ExportAll(loader, outputBase, fbxDir, referenceSkeleton)) {
    //        wcout << L"바이너리 변환 실패!" << endl;
    //        return -1;
    //    }

    //    wcout << i << L"번째 파일 변환 완료!" << endl;
    //}

    //-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    // 몬스터 T-pose 추출 코드
    //-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    wstring inputFbx = L"../FBX/monster_DemonExecutioner.fbx";          // 입력 FBX 파일
    wstring fbxDir = L"../FBX";
    wstring outputBase = L"../FBXOutput/monster_DemonExecutioner";           // 출력 기본 이름
    
    // 애니메이션 전용 FBX의 경우, 메시의 스켈레톤을 기준으로 매핑
    wstring referenceSkeleton = L"";  // 메시에서 추출한 스켈레톤
    
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
    
    wcout << L"변환 완료!" << endl;
    return 0;
}
