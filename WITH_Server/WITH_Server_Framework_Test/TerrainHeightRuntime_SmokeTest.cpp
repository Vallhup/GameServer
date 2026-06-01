#include "pch.h"

#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>

#include "System.h"
#include "TerrainHeightRuntime.h"
#include "WorldExecutionModelTypes.h"
#include "WorldDef.h"
#include "WorldRuntime.h"
#include "WorldSystemServiceScope.h"

namespace
{
    std::filesystem::path MakeUniqueTempPath(const char* label)
    {
        const auto tick = std::chrono::steady_clock::now()
            .time_since_epoch()
            .count();

        return std::filesystem::temp_directory_path() /
            ("WITH_" + std::string(label) + "_" + std::to_string(tick) + ".raw");
    }

    class TempRawFile final
    {
    public:
        TempRawFile(
            const char* label,
            const std::initializer_list<uint16_t> samples)
            : _path(MakeUniqueTempPath(label))
        {
            Write(samples);
        }

        ~TempRawFile()
        {
            std::error_code ec;
            std::filesystem::remove(_path, ec);
        }

        TempRawFile(const TempRawFile&) = delete;
        TempRawFile& operator=(const TempRawFile&) = delete;

        const std::filesystem::path& Path() const noexcept { return _path; }

    private:
        void Write(const std::initializer_list<uint16_t> samples)
        {
            std::ofstream ofs(_path, std::ios::binary | std::ios::trunc);
            if (!ofs.is_open())
                throw std::runtime_error("failed to create terrain height test raw file");

            for (const uint16_t sample : samples)
            {
                const unsigned char bytes[2] =
                {
                    static_cast<unsigned char>(sample & 0xFFu),
                    static_cast<unsigned char>((sample >> 8u) & 0xFFu)
                };
                ofs.write(reinterpret_cast<const char*>(bytes), sizeof(bytes));
            }
        }

    private:
        std::filesystem::path _path;
    };

    TerrainHeightRawDef MakeTestDef()
    {
        TerrainHeightRawDef def{};
        def.width = 2;
        def.height = 2;
        def.originX = 10.0f;
        def.originZ = 20.0f;
        def.cellSizeX = 2.0f;
        def.cellSizeZ = 4.0f;
        def.heightScale = 0.5f;
        def.heightOffset = 1.0f;
        return def;
    }

    const SpawnPointDef* FindSpawnPoint(
        const WorldDef& worldDef,
        SpawnPointId spawnPointId)
    {
        for (const SpawnPointDef& spawnPoint : worldDef.map.spawnPoints)
        {
            if (spawnPoint.id == spawnPointId)
                return &spawnPoint;
        }

        return nullptr;
    }

    void AssertSpawnPointPosition(
        const WorldDef& worldDef,
        SpawnPointId spawnPointId,
        float expectedX,
        float expectedY,
        float expectedZ)
    {
        const SpawnPointDef* const spawnPoint =
            FindSpawnPoint(worldDef, spawnPointId);
        assert(spawnPoint != nullptr);

        constexpr float kEpsilon = 0.0001f;
        assert(std::fabs(spawnPoint->position.x - expectedX) < kEpsilon);
        assert(std::fabs(spawnPoint->position.y - expectedY) < kEpsilon);
        assert(std::fabs(spawnPoint->position.z - expectedZ) < kEpsilon);
    }

    WorldDef MakeWorldDef()
    {
        WorldDef def{};
        def.id = WorldDefId::Plaza;
        def.name = "TerrainHeightRuntimeSmoke";
        def.topology = { WorldKind::Hub, WorldInstanceType::Instanced };
        def.entryPolicy = {
            CreationPolicy::CreateOnDemand,
            JoinPolicy::FreeJoin,
            0,
            true,
            false,
            std::nullopt,
            std::nullopt
        };
        def.map.resourceId = 0;
        def.map.defaultPlayerSpawnPointId = SpawnPointIds::None;
        def.map.navMesh = std::nullopt;
        def.map.terrainHeight = std::nullopt;
        def.spawn = { SpawnSetId::None, std::nullopt };
        def.progressRule = {
            WorldClearConditionType::None,
            WorldFailConditionType::None,
            WorldCompletionActionType::None,
            std::nullopt,
            false
        };
        def.linkRules = {};
        return def;
    }

    WorldExecutionModel MakeExecutionModel()
    {
        WorldExecutionModel model{};
        model.key = 1;
        return model;
    }

    class StubTerrainHeightProvider final : public ITerrainHeightProvider
    {
    public:
        bool TrySampleHeight(
            float worldX,
            float worldZ,
            float& outHeight) const noexcept override
        {
            (void)worldX;
            (void)worldZ;
            outHeight = 777.0f;
            return true;
        }
    };

    void Test_LoadsAndBilinearSamplesUInt16Raw()
    {
        TempRawFile file("TerrainHeightRuntime_2x2", { 10, 20, 30, 40 });

        TerrainHeightRuntime runtime;
        TerrainHeightRawDef def = MakeTestDef();
        def.path = file.Path().string();

        assert(runtime.LoadFromFile(file.Path().string(), def));
        assert(runtime.IsReady());

        float height = 0.0f;
        assert(runtime.TrySampleHeight(11.0f, 22.0f, height));
        assert(height == 13.5f);

        assert(runtime.TrySampleHeight(12.0f, 24.0f, height));
        assert(height == 21.0f);

        assert(!runtime.TrySampleHeight(9.99f, 22.0f, height));

        runtime.Unload();
        assert(!runtime.IsReady());
    }

    void Test_FlipZSamplesLogicalRowsFromOppositeSide()
    {
        TempRawFile file("TerrainHeightRuntime_flip", { 10, 20, 30, 40 });

        TerrainHeightRuntime runtime;
        TerrainHeightRawDef def = MakeTestDef();
        def.path = file.Path().string();
        def.heightScale = 1.0f;
        def.heightOffset = 0.0f;
        def.flipZ = true;

        assert(runtime.LoadFromFile(file.Path().string(), def));

        float height = 0.0f;
        assert(runtime.TrySampleHeight(10.0f, 20.0f, height));
        assert(height == 30.0f);
    }

    void Test_RotationYSamplesUnityTerrainLocalSpace()
    {
        TempRawFile file("TerrainHeightRuntime_rotation_y", { 10, 20, 30, 40 });

        TerrainHeightRuntime runtime;
        TerrainHeightRawDef def = MakeTestDef();
        def.path = file.Path().string();
        def.originX = 0.0f;
        def.originZ = 0.0f;
        def.cellSizeX = 1.0f;
        def.cellSizeZ = 1.0f;
        def.heightScale = 1.0f;
        def.heightOffset = 0.0f;
        def.rotationYDegrees = 90.0f;

        assert(runtime.LoadFromFile(file.Path().string(), def));

        float height = 0.0f;
        assert(runtime.TrySampleHeight(1.0f, 0.0f, height));
        assert(height == 30.0f);
        assert(!runtime.TrySampleHeight(0.0f, 1.0f, height));
    }

    void Test_RejectsInvalidByteCount()
    {
        TempRawFile file("TerrainHeightRuntime_bad_size", { 1, 2, 3 });

        TerrainHeightRuntime runtime;
        TerrainHeightRawDef def = MakeTestDef();
        def.path = file.Path().string();

        assert(!runtime.LoadFromFile(file.Path().string(), def));
        assert(!runtime.IsReady());
    }

    void Test_RejectsOversizedDeclaredGrid()
    {
        TempRawFile file("TerrainHeightRuntime_oversized", { 1, 2, 3, 4 });

        TerrainHeightRuntime runtime;
        TerrainHeightRawDef def = MakeTestDef();
        def.path = file.Path().string();
        def.width = std::numeric_limits<uint32_t>::max();
        def.height = std::numeric_limits<uint32_t>::max();

        assert(!runtime.LoadFromFile(file.Path().string(), def));
        assert(!runtime.IsReady());
    }

    void Test_WorldRuntimeTerrainHeightProviderInjection()
    {
        WorldExecutionModel model = MakeExecutionModel();

        WorldDef noTerrainDef = MakeWorldDef();
        WorldRuntime noTerrainRuntime({
            .def = &noTerrainDef,
            .executionModel = &model
        });
        assert(noTerrainRuntime.Initialize());
        WorldSystemServiceScope noTerrainScope(noTerrainRuntime);
        assert(noTerrainScope.Services().terrainHeightProvider == nullptr);

        WorldDef failedTerrainDef = MakeWorldDef();
        failedTerrainDef.map.terrainHeight = MakeTestDef();
        failedTerrainDef.map.terrainHeight->path = MakeUniqueTempPath(
            "TerrainHeightRuntime_missing").string();
        WorldRuntime failedTerrainRuntime({
            .def = &failedTerrainDef,
            .executionModel = &model
        });
        assert(failedTerrainRuntime.Initialize());
        WorldSystemServiceScope failedTerrainScope(failedTerrainRuntime);
        assert(failedTerrainScope.Services().terrainHeightProvider == nullptr);

        TempRawFile file("TerrainHeightRuntime_injection", { 10, 20, 30, 40 });
        WorldDef loadedTerrainDef = MakeWorldDef();
        loadedTerrainDef.map.terrainHeight = MakeTestDef();
        loadedTerrainDef.map.terrainHeight->path = file.Path().string();
        WorldRuntime loadedTerrainRuntime({
            .def = &loadedTerrainDef,
            .executionModel = &model
        });
        assert(loadedTerrainRuntime.Initialize());

        WorldSystemServiceScope loadedTerrainScope(loadedTerrainRuntime);
        const ITerrainHeightProvider* provider =
            loadedTerrainScope.Services().terrainHeightProvider;
        assert(provider != nullptr);

        float height = 0.0f;
        assert(provider->TrySampleHeight(11.0f, 22.0f, height));
        assert(height == 13.5f);

        StubTerrainHeightProvider stubProvider;
        WorldSystemServices services{};
        services.terrainHeightProvider = &stubProvider;
        WorldSystemServiceScope explicitProviderScope(
            loadedTerrainRuntime,
            services);
        assert(
            explicitProviderScope.Services().terrainHeightProvider ==
            &stubProvider);

        assert(explicitProviderScope.Services().terrainHeightProvider->
            TrySampleHeight(0.0f, 0.0f, height));
        assert(height == 777.0f);
    }

    bool AlmostEqual(float lhs, float rhs)
    {
        return std::fabs(lhs - rhs) < 0.0001f;
    }

    void AssertTerrainDefMatchesRawImport(
        const TerrainHeightRawDef& terrainDef,
        const std::string& expectedPath,
        float expectedOriginX,
        float expectedOriginZ,
        float expectedTerrainWidth,
        float expectedTerrainLength,
        float expectedTerrainHeight,
        float expectedRotationYDegrees)
    {
        constexpr uint32_t kExpectedResolution = 2049;
        constexpr float kExpectedPositionY = 0.0f;

        assert(terrainDef.path == expectedPath);
        assert(terrainDef.width == kExpectedResolution);
        assert(terrainDef.height == kExpectedResolution);
        assert(AlmostEqual(terrainDef.originX, expectedOriginX));
        assert(AlmostEqual(terrainDef.originZ, expectedOriginZ));
        assert(AlmostEqual(
            terrainDef.cellSizeX,
            expectedTerrainWidth / static_cast<float>(kExpectedResolution - 1u)));
        assert(AlmostEqual(
            terrainDef.cellSizeZ,
            expectedTerrainLength / static_cast<float>(kExpectedResolution - 1u)));
        assert(AlmostEqual(
            terrainDef.heightScale,
            expectedTerrainHeight / 65535.0f));
        assert(AlmostEqual(terrainDef.heightOffset, kExpectedPositionY));
        assert(AlmostEqual(terrainDef.rotationYDegrees, expectedRotationYDegrees));
        assert(!terrainDef.flipZ);
        assert(terrainDef.sampleFormat == TerrainHeightSampleFormat::UInt16LE);
    }

    void Test_WorldTerrainDefsMatchUnityRawImportSettings()
    {
        const WorldDef plazaDef = CreatePlazaWorldDef(1);
        const WorldDef villageDef = CreateVillageWorldDef(1);
        const WorldDef castleDef = CreateCastleWorldDef(1);
        const WorldDef finalDef = CreateFinalWorldDef(1);

        assert(plazaDef.map.terrainHeight.has_value());
        assert(villageDef.map.terrainHeight.has_value());
        assert(castleDef.map.terrainHeight.has_value());
        assert(finalDef.map.terrainHeight.has_value());

        AssertTerrainDefMatchesRawImport(
            plazaDef.map.terrainHeight.value(),
            "../Map/Plaza_Terrain.raw",
            0.0f,
            0.0f,
            1016.0f,
            1016.0f,
            27.01563f,
            0.0f);
        AssertTerrainDefMatchesRawImport(
            villageDef.map.terrainHeight.value(),
            "../Map/Village_Terrain.raw",
            0.0f,
            0.0f,
            1023.0f,
            1023.0f,
            159.4766f,
            90.0f);
        AssertTerrainDefMatchesRawImport(
            castleDef.map.terrainHeight.value(),
            "../Map/Castle_Terrain.raw",
            0.0f,
            0.0f,
            650.2402f,
            650.2402f,
            79.28662f,
            0.0f);
        AssertTerrainDefMatchesRawImport(
            finalDef.map.terrainHeight.value(),
            "../Map/Cathedral_Terrain.raw",
            -57.9f,
            -102.2f,
            120.0f,
            120.0f,
            600.0f,
            0.0f);
    }

    void Test_VillageCombatSpawnPointsMatchContentLayout()
    {
        const WorldDef villageDef = CreateVillageWorldDef(1);

        assert(villageDef.spawn.initialSpawnSetId == SpawnSetId::VillageDefault);
        assert(FindSpawnPoint(villageDef, SpawnPointIds::VillageMonster02A) == nullptr);
        assert(FindSpawnPoint(villageDef, SpawnPointIds::VillageMonster02B) == nullptr);

        AssertSpawnPointPosition(
            villageDef,
            SpawnPointIds::VillageMonster01,
            212.904114f,
            56.205055f,
            620.986938f);
        AssertSpawnPointPosition(
            villageDef,
            SpawnPointIds::VillageMonster01A,
            214.304108f,
            56.205055f,
            621.786926f);
        AssertSpawnPointPosition(
            villageDef,
            SpawnPointIds::VillageMonster01B,
            211.704117f,
            56.205055f,
            622.286926f);
        AssertSpawnPointPosition(
            villageDef,
            SpawnPointIds::VillageMonster02,
            216.599396f,
            56.203743f,
            579.089844f);
        AssertSpawnPointPosition(
            villageDef,
            SpawnPointIds::VillageMonster03,
            263.797272f,
            58.984131f,
            556.220764f);
        AssertSpawnPointPosition(
            villageDef,
            SpawnPointIds::VillageMonster04,
            272.319183f,
            62.978153f,
            651.002991f);
        AssertSpawnPointPosition(
            villageDef,
            SpawnPointIds::VillageMonster05,
            240.033142f,
            57.021049f,
            606.717407f);
        AssertSpawnPointPosition(
            villageDef,
            SpawnPointIds::VillageMonster05A,
            241.433136f,
            57.021049f,
            607.517395f);
        AssertSpawnPointPosition(
            villageDef,
            SpawnPointIds::VillageMonster05B,
            238.833145f,
            57.021049f,
            608.017395f);
        AssertSpawnPointPosition(
            villageDef,
            SpawnPointIds::VillageMonster06,
            186.963013f,
            56.395004f,
            583.231812f);
        AssertSpawnPointPosition(
            villageDef,
            SpawnPointIds::VillageMonster06A,
            188.363007f,
            56.395004f,
            584.031799f);
        AssertSpawnPointPosition(
            villageDef,
            SpawnPointIds::VillageMonster06B,
            185.763016f,
            56.395004f,
            584.531799f);
        AssertSpawnPointPosition(
            villageDef,
            SpawnPointIds::VillageMonster07,
            263.688568f,
            62.369606f,
            620.089600f);
        AssertSpawnPointPosition(
            villageDef,
            SpawnPointIds::VillageMonster08,
            287.081970f,
            65.820259f,
            639.837524f);
        AssertSpawnPointPosition(
            villageDef,
            SpawnPointIds::VillageMonster08A,
            288.481964f,
            65.820259f,
            640.637512f);
        AssertSpawnPointPosition(
            villageDef,
            SpawnPointIds::VillageMonster09,
            239.990372f,
            56.680523f,
            540.022034f);
        AssertSpawnPointPosition(
            villageDef,
            SpawnPointIds::VillageMonster10,
            292.899323f,
            68.393768f,
            570.101746f);
    }

    void Test_CastleAdditionalCombatSpawnPointsMatchContentLayout()
    {
        const WorldDef castleDef = CreateCastleWorldDef(1);

        assert(castleDef.spawn.initialSpawnSetId == SpawnSetId::CastleDefault);
        AssertSpawnPointPosition(
            castleDef,
            SpawnPointIds::CastleMonster07,
            353.739288f,
            68.879623f,
            308.454926f);
        AssertSpawnPointPosition(
            castleDef,
            SpawnPointIds::CastleMonster07A,
            355.139282f,
            68.879623f,
            309.254913f);
        AssertSpawnPointPosition(
            castleDef,
            SpawnPointIds::CastleMonster07B,
            352.539276f,
            68.879623f,
            309.754913f);
        AssertSpawnPointPosition(
            castleDef,
            SpawnPointIds::CastleMonster08,
            367.923859f,
            68.897438f,
            328.193909f);
        AssertSpawnPointPosition(
            castleDef,
            SpawnPointIds::CastleMonster09,
            353.965179f,
            68.888054f,
            333.697205f);
        AssertSpawnPointPosition(
            castleDef,
            SpawnPointIds::CastleMonster10,
            311.321808f,
            69.173592f,
            321.829773f);
        AssertSpawnPointPosition(
            castleDef,
            SpawnPointIds::CastleMonster11,
            344.273621f,
            68.230255f,
            383.559265f);
        AssertSpawnPointPosition(
            castleDef,
            SpawnPointIds::CastleMonster12,
            318.663025f,
            67.961067f,
            371.518890f);
        AssertSpawnPointPosition(
            castleDef,
            SpawnPointIds::CastleMonster12A,
            320.063019f,
            67.961067f,
            372.318878f);
        AssertSpawnPointPosition(
            castleDef,
            SpawnPointIds::CastleMonster12B,
            317.463013f,
            67.961067f,
            372.818878f);
    }
}

void RunTerrainHeightRuntimeSmokeTests()
{
    Test_LoadsAndBilinearSamplesUInt16Raw();
    std::cout << "[PASS] Test_LoadsAndBilinearSamplesUInt16Raw\n";

    Test_FlipZSamplesLogicalRowsFromOppositeSide();
    std::cout << "[PASS] Test_FlipZSamplesLogicalRowsFromOppositeSide\n";

    Test_RotationYSamplesUnityTerrainLocalSpace();
    std::cout << "[PASS] Test_RotationYSamplesUnityTerrainLocalSpace\n";

    Test_RejectsInvalidByteCount();
    std::cout << "[PASS] Test_RejectsInvalidByteCount\n";

    Test_RejectsOversizedDeclaredGrid();
    std::cout << "[PASS] Test_RejectsOversizedDeclaredGrid\n";

    Test_WorldRuntimeTerrainHeightProviderInjection();
    std::cout << "[PASS] Test_WorldRuntimeTerrainHeightProviderInjection\n";

    Test_WorldTerrainDefsMatchUnityRawImportSettings();
    std::cout << "[PASS] Test_WorldTerrainDefsMatchUnityRawImportSettings\n";

    Test_VillageCombatSpawnPointsMatchContentLayout();
    std::cout << "[PASS] Test_VillageCombatSpawnPointsMatchContentLayout\n";

    Test_CastleAdditionalCombatSpawnPointsMatchContentLayout();
    std::cout << "[PASS] Test_CastleAdditionalCombatSpawnPointsMatchContentLayout\n";

    std::cout << "\nAll TerrainHeightRuntime smoke tests passed.\n";
}
