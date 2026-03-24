using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using WITH_ServerDataTool.Definition;
using WITH_ServerDataTool.ID;

namespace WITH_ServerDataTool.Utility
{
    public static class GameDataValidator
    {
        public static ValidationResult ValidateAll(GameDataSet data)
        {
            ValidationResult result = new ValidationResult();

            if (data == null)
            {
                result.AddError("GameDataSet", "데이터셋이 null 입니다.");
                return result;
            }

            ValidateCharacters(data, result);
            ValidateActions(data, result);
            ValidateAnimations(data, result);
            ValidateBuffs(data, result);
            ValidateWorlds(data, result);
            ValidateSpawnSets(data, result);

            ValidateCrossReferences(data, result);

            return result;
        }

        private static void ValidateCharacters(GameDataSet data, ValidationResult result)
        {
            HashSet<CharacterId> ids = new HashSet<CharacterId>();

            for (int i = 0; i < data.Characters.Count; ++i)
            {
                CharacterDef def = data.Characters[i];
                string path = "Characters[" + i + "]";

                if (def == null)
                {
                    result.AddError(path, "CharacterDef가 null 입니다.");
                    continue;
                }

                if (def.Id == CharacterId.None)
                {
                    result.AddError(path + ".Id", "CharacterId는 None일 수 없습니다.");
                }

                if (ids.Contains(def.Id))
                {
                    result.AddError(path + ".Id", "중복된 CharacterId 입니다: " + def.Id);
                }
                else
                {
                    ids.Add(def.Id);
                }

                if (string.IsNullOrEmpty(def.Name))
                {
                    result.AddError(path + ".Name", "Name이 비어 있습니다.");
                }

                if (def.CategoryId != EntityCategoryId.Character)
                {
                    result.AddError(path + ".CategoryId", "CharacterDef의 CategoryId는 Character여야 합니다.");
                }

                ValidateDistinctList(def.DefaultBuffIds, path + ".DefaultBuffIds", result);
            }
        }

        private static void ValidateActions(GameDataSet data, ValidationResult result)
        {
            HashSet<string> keys = new HashSet<string>();

            for (int i = 0; i < data.Actions.Count; ++i)
            {
                ActionDef def = data.Actions[i];
                string path = "Actions[" + i + "]";

                if (def == null)
                {
                    result.AddError(path, "ActionDef가 null 입니다.");
                    continue;
                }

                if (def.Id.CharacterId == CharacterId.None)
                    result.AddError(path + ".Id.CharacterId", "CharacterId는 None일 수 없습니다.");

                if (def.Id.ActionLocalId == 0)
                    result.AddError(path + ".Id.ActionDefId", "ActionDefId는 None일 수 없습니다.");

                if (def.ActionId == ActionId.None)
                    result.AddError(path + ".ActionId", "ActionId는 None일 수 없습니다.");

                if (string.IsNullOrEmpty(def.Name))
                    result.AddError(path + ".Name", "Name이 비어 있습니다.");

                if (def.DurationSec < 0.0f)
                    result.AddError(path + ".DurationSec", "DurationSec는 0 이상이어야 합니다.");

                string key = def.Id.CharacterId.ToString() + ":" + def.Id.ActionLocalId.ToString();
                if (keys.Contains(key))
                    result.AddError(path + ".Id", "같은 CharacterId + ActionDefId 조합이 중복되었습니다: " + key);
                else
                    keys.Add(key);

                if (def.AnimationId == AnimationId.None)
                    result.AddError(path + ".AnimationId", "AnimationId는 None일 수 없습니다.");

                ValidateActionSegments(def, path + ".Segments", result);
            }
        }

        private static void ValidateAnimations(GameDataSet data, ValidationResult result)
        {
            HashSet<AnimationId> ids = new HashSet<AnimationId>();

            for (int i = 0; i < data.Animations.Count; ++i)
            {
                AnimationResourceDef def = data.Animations[i];
                string path = "Animations[" + i + "]";

                if (def == null)
                {
                    result.AddError(path, "AnimationResourceDef가 null 입니다.");
                    continue;
                }

                if (def.Id == AnimationId.None)
                {
                    result.AddError(path + ".Id", "AnimationId는 None일 수 없습니다.");
                }

                if (ids.Contains(def.Id))
                {
                    result.AddError(path + ".Id", "중복된 AnimationId 입니다: " + def.Id);
                }
                else
                {
                    ids.Add(def.Id);
                }

                if (string.IsNullOrEmpty(def.Name))
                {
                    result.AddError(path + ".Name", "Name이 비어 있습니다.");
                }

                if (string.IsNullOrEmpty(def.SourcePath))
                {
                    result.AddError(path + ".SourcePath", "SourcePath가 비어 있습니다.");
                }

                if (def.Fps < 0.0f)
                {
                    result.AddError(path + ".Fps", "Fps는 0 이상이어야 합니다.");
                }

                if (def.NumFrames < 0)
                {
                    result.AddError(path + ".NumFrames", "NumFrames는 0 이상이어야 합니다.");
                }
            }
        }

        private static void ValidateBuffs(GameDataSet data, ValidationResult result)
        {
            HashSet<BuffId> ids = new HashSet<BuffId>();

            for (int i = 0; i < data.Buffs.Count; ++i)
            {
                BuffDef def = data.Buffs[i];
                string path = "Buffs[" + i + "]";

                if (def == null)
                {
                    result.AddError(path, "BuffDef가 null 입니다.");
                    continue;
                }

                if (def.Id == BuffId.None)
                    result.AddError(path + ".Id", "BuffId는 None일 수 없습니다.");

                if (ids.Contains(def.Id))
                    result.AddError(path + ".Id", "중복된 BuffId 입니다: " + def.Id);
                else
                    ids.Add(def.Id);

                if (string.IsNullOrEmpty(def.Name))
                    result.AddError(path + ".Name", "Name이 비어 있습니다.");

                if (def.Modifiers == null || def.Modifiers.Count == 0)
                    result.AddError(path + ".Modifiers", "Modifier가 최소 1개는 있어야 합니다.");

                if (def.Modifiers != null)
                {
                    for (int j = 0; j < def.Modifiers.Count; ++j)
                    {
                        BuffModifierDef mod = def.Modifiers[j];
                        string modPath = path + ".Modifiers[" + j + "]";

                        if (mod == null)
                        {
                            result.AddError(modPath, "BuffModifierDef가 null 입니다.");
                            continue;
                        }

                        if (mod.Effect == BuffEffectId.None)
                            result.AddError(modPath + ".Effect", "Effect는 None일 수 없습니다.");

                        if (mod.StatId == StatId.None)
                            result.AddError(modPath + ".StatId", "StatId는 None일 수 없습니다.");
                    }
                }
            }
        }

        private static void ValidateWorlds(GameDataSet data, ValidationResult result)
        {
            HashSet<WorldId> ids = new HashSet<WorldId>();

            for (int i = 0; i < data.Worlds.Count; ++i)
            {
                WorldDef def = data.Worlds[i];
                string path = "Worlds[" + i + "]";

                if (def == null)
                {
                    result.AddError(path, "WorldDef가 null 입니다.");
                    continue;
                }

                if (def.Id == WorldId.None)
                    result.AddError(path + ".Id", "WorldId는 None일 수 없습니다.");

                if (ids.Contains(def.Id))
                    result.AddError(path + ".Id", "중복된 WorldId 입니다: " + def.Id);
                else
                    ids.Add(def.Id);

                if (string.IsNullOrEmpty(def.Name))
                    result.AddError(path + ".Name", "Name이 비어 있습니다.");

                if (def.Map == null)
                {
                    result.AddError(path + ".Map", "MapDef가 null 입니다.");
                }
                else
                {
                    ValidateMap(path + ".Map", def.Map, result);
                }

                if (def.SpawnSetId == SpawnSetId.None)
                    result.AddError(path + ".SpawnSetId", "SpawnSetId는 None일 수 없습니다.");
            }
        }

        private static void ValidateSpawnSets(GameDataSet data, ValidationResult result)
        {
            HashSet<SpawnSetId> ids = new HashSet<SpawnSetId>();

            for (int i = 0; i < data.SpawnSets.Count; ++i)
            {
                SpawnSetDef def = data.SpawnSets[i];
                string path = "SpawnSets[" + i + "]";

                if (def == null)
                {
                    result.AddError(path, "SpawnSetDef가 null 입니다.");
                    continue;
                }

                if (def.Id == SpawnSetId.None)
                    result.AddError(path + ".Id", "SpawnSetId는 None일 수 없습니다.");

                if (ids.Contains(def.Id))
                    result.AddError(path + ".Id", "중복된 SpawnSetId 입니다: " + def.Id);
                else
                    ids.Add(def.Id);

                if (string.IsNullOrEmpty(def.Name))
                    result.AddError(path + ".Name", "Name이 비어 있습니다.");

                if (def.Entries == null || def.Entries.Count == 0)
                    result.AddError(path + ".Entries", "SpawnEntry가 최소 1개는 있어야 합니다.");

                if (def.Entries != null)
                {
                    for (int j = 0; j < def.Entries.Count; ++j)
                    {
                        SpawnEntryDef entry = def.Entries[j];
                        string entryPath = path + ".Entries[" + j + "]";

                        if (entry == null)
                        {
                            result.AddError(entryPath, "SpawnEntryDef가 null 입니다.");
                            continue;
                        }

                        if (entry.CharacterId == CharacterId.None)
                            result.AddError(entryPath + ".CharacterId", "CharacterId는 None일 수 없습니다.");
                    }
                }
            }
        }

        private static void ValidateCrossReferences(GameDataSet data, ValidationResult result)
        {
            HashSet<CharacterId> characterIds = new HashSet<CharacterId>();
            HashSet<AnimationId> animationIds = new HashSet<AnimationId>();
            HashSet<BuffId> buffIds = new HashSet<BuffId>();
            HashSet<SpawnSetId> spawnSetIds = new HashSet<SpawnSetId>();
            HashSet<string> actionKeys = new HashSet<string>();

            for (int i = 0; i < data.Characters.Count; ++i)
                if (data.Characters[i] != null)
                    characterIds.Add(data.Characters[i].Id);

            for (int i = 0; i < data.Animations.Count; ++i)
                if (data.Animations[i] != null)
                    animationIds.Add(data.Animations[i].Id);

            for (int i = 0; i < data.Buffs.Count; ++i)
                if (data.Buffs[i] != null)
                    buffIds.Add(data.Buffs[i].Id);

            for (int i = 0; i < data.SpawnSets.Count; ++i)
                if (data.SpawnSets[i] != null)
                    spawnSetIds.Add(data.SpawnSets[i].Id);

            for (int i = 0; i < data.Actions.Count; ++i)
            {
                ActionDef action = data.Actions[i];
                if (action == null) continue;

                string key =
                    action.Id.CharacterId.ToString() + ":" +
                    action.Id.ActionLocalId.ToString();

                actionKeys.Add(key);
            }

            for (int i = 0; i < data.Characters.Count; ++i)
            {
                CharacterDef ch = data.Characters[i];
                if (ch == null) continue;

                string path = "Characters[" + i + "]";

                if (ch.DefaultBuffIds != null)
                {
                    for (int j = 0; j < ch.DefaultBuffIds.Count; ++j)
                    {
                        if (!buffIds.Contains(ch.DefaultBuffIds[j]))
                        {
                            result.AddError(
                                path + ".DefaultBuffIds[" + j + "]",
                                "존재하지 않는 BuffId 참조입니다: " + ch.DefaultBuffIds[j]);
                        }
                    }
                }
            }

            for (int i = 0; i < data.Actions.Count; ++i)
            {
                ActionDef action = data.Actions[i];
                if (action == null) continue;

                string path = "Actions[" + i + "]";

                if (!characterIds.Contains(action.Id.CharacterId))
                    result.AddError(path + ".Id.CharacterId", "존재하지 않는 CharacterId 참조입니다: " + action.Id.CharacterId);

                if (!animationIds.Contains(action.AnimationId))
                    result.AddError(path + ".AnimationId", "존재하지 않는 AnimationId 참조입니다: " + action.AnimationId);
            }

            for (int i = 0; i < data.Worlds.Count; ++i)
            {
                WorldDef world = data.Worlds[i];
                if (world == null) continue;

                string path = "Worlds[" + i + "]";

                if (!spawnSetIds.Contains(world.SpawnSetId))
                    result.AddError(path + ".SpawnSetId", "존재하지 않는 SpawnSetId 참조입니다: " + world.SpawnSetId);
            }

            for (int i = 0; i < data.SpawnSets.Count; ++i)
            {
                SpawnSetDef set = data.SpawnSets[i];
                if (set == null || set.Entries == null) continue;

                string path = "SpawnSets[" + i + "]";

                for (int j = 0; j < set.Entries.Count; ++j)
                {
                    SpawnEntryDef entry = set.Entries[j];
                    if (entry == null) continue;

                    if (!characterIds.Contains(entry.CharacterId))
                    {
                        result.AddError(
                            path + ".Entries[" + j + "].CharacterId",
                            "존재하지 않는 CharacterId 참조입니다: " + entry.CharacterId);
                    }
                }
            }
        }

        private static void ValidateDistinctList<T>(List<T> values, string path, ValidationResult result)
        {
            if (values == null) return;

            HashSet<T> set = new HashSet<T>();
            for (int i = 0; i < values.Count; ++i)
            {
                if (set.Contains(values[i]))
                {
                    result.AddError(path + "[" + i + "]", "중복 값입니다: " + values[i]);
                }
                else
                {
                    set.Add(values[i]);
                }
            }
        }

        private static void ValidateMap(string path, MapDef def, ValidationResult result)
        {
            if (string.IsNullOrEmpty(def.CollisionMapPath))
                result.AddError(path + ".CollisionMapPath", "CollisionMapPath가 비어 있습니다.");

            if (string.IsNullOrEmpty(def.HeightMapPath))
                result.AddError(path + ".HeightMapPath", "HeightMapPath가 비어 있습니다.");

            if (def.CollisionWidth <= 0)
                result.AddError(path + ".CollisionWidth", "CollisionWidth는 1 이상이어야 합니다.");

            if (def.CollisionHeight <= 0)
                result.AddError(path + ".CollisionHeight", "CollisionHeight는 1 이상이어야 합니다.");

            if (def.WorldSize <= 0.0f)
                result.AddError(path + ".WorldSize", "WorldSize는 0보다 커야 합니다.");

            if (def.HeightScale < 0.0f)
                result.AddError(path + ".HeightScale", "HeightScale은 0 이상이어야 합니다.");
        }

        private static void ValidateActionSegments(ActionDef def, string path, ValidationResult result)
        {
            if (def.Segments == null)
                return;

            float prevEnd = -1.0f;

            for (int i = 0; i < def.Segments.Count; ++i)
            {
                ActionSegmentDef seg = def.Segments[i];
                string segPath = path + "[" + i + "]";

                if (seg == null)
                {
                    result.AddError(segPath, "ActionSegmentDef가 null 입니다.");
                    continue;
                }

                if (seg.StartNormalizedTime < 0.0f || seg.StartNormalizedTime > 1.0f)
                    result.AddError(segPath + ".StartNormalizedTime", "StartNormalizedTime은 [0,1] 범위여야 합니다.");

                if (seg.EndNormalizedTime < 0.0f || seg.EndNormalizedTime > 1.0f)
                    result.AddError(segPath + ".EndNormalizedTime", "EndNormalizedTime은 [0,1] 범위여야 합니다.");

                if (seg.StartNormalizedTime >= seg.EndNormalizedTime)
                    result.AddError(segPath, "segment는 Start < End 여야 합니다.");

                if (seg.StartNormalizedTime < prevEnd)
                    result.AddError(segPath, "segment 시간이 이전 segment와 겹칩니다.");

                prevEnd = seg.EndNormalizedTime;
            }
        }
    }
}