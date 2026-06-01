#include "pch.h"

#include <cassert>
#include <chrono>
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

    void Test_VillageTerrainDefMatchesMigratedClientRawFormat()
    {
        const WorldDef villageDef = CreateVillageWorldDef(1);

        assert(villageDef.map.terrainHeight.has_value());

        const TerrainHeightRawDef& terrainDef =
            villageDef.map.terrainHeight.value();

        assert(terrainDef.path == "../Map/Village_Terrain.raw");
        assert(terrainDef.width == 2049);
        assert(terrainDef.height == 2049);
        assert(terrainDef.originX == 0.0f);
        assert(terrainDef.originZ == 0.0f);
        assert(terrainDef.cellSizeX == 1.0f);
        assert(terrainDef.cellSizeZ == 1.0f);
        assert(terrainDef.heightScale == 159.4766f / 65535.0f);
        assert(terrainDef.heightOffset == 0.0f);
        assert(!terrainDef.flipZ);
        assert(terrainDef.sampleFormat == TerrainHeightSampleFormat::UInt16LE);

        const float worldSizeX =
            terrainDef.cellSizeX * static_cast<float>(terrainDef.width - 1u);
        const float worldSizeZ =
            terrainDef.cellSizeZ * static_cast<float>(terrainDef.height - 1u);

        assert(worldSizeX == 2048.0f);
        assert(worldSizeZ == 2048.0f);
    }
}

void RunTerrainHeightRuntimeSmokeTests()
{
    Test_LoadsAndBilinearSamplesUInt16Raw();
    std::cout << "[PASS] Test_LoadsAndBilinearSamplesUInt16Raw\n";

    Test_FlipZSamplesLogicalRowsFromOppositeSide();
    std::cout << "[PASS] Test_FlipZSamplesLogicalRowsFromOppositeSide\n";

    Test_RejectsInvalidByteCount();
    std::cout << "[PASS] Test_RejectsInvalidByteCount\n";

    Test_RejectsOversizedDeclaredGrid();
    std::cout << "[PASS] Test_RejectsOversizedDeclaredGrid\n";

    Test_WorldRuntimeTerrainHeightProviderInjection();
    std::cout << "[PASS] Test_WorldRuntimeTerrainHeightProviderInjection\n";

    Test_VillageTerrainDefMatchesMigratedClientRawFormat();
    std::cout << "[PASS] Test_VillageTerrainDefMatchesMigratedClientRawFormat\n";

    std::cout << "\nAll TerrainHeightRuntime smoke tests passed.\n";
}
