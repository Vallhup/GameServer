using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using WITH_ServerDataTool.Definition;
using WITH_ServerDataTool.ID;

namespace WITH_ServerDataTool.Utility
{
    public sealed class DefinitionRegistry<TKey, TDef>
        where TDef : class, IDefinition<TKey>
    {
        private readonly Dictionary<TKey, TDef> _map;

        public DefinitionRegistry()
        {
            _map = new Dictionary<TKey, TDef>();
        }

        public int Count
        {
            get { return _map.Count; }
        }

        public IEnumerable<TDef> Values
        {
            get { return _map.Values; }
        }

        public void Add(TDef def)
        {
            if (def == null)
                throw new ArgumentNullException("def");

            if (_map.ContainsKey(def.Id))
                throw new InvalidOperationException(
                    "중복 Definition ID 입니다: " + def.Id);

            _map.Add(def.Id, def);
        }

        public bool Contains(TKey id)
        {
            return _map.ContainsKey(id);
        }

        public bool TryGet(TKey id, out TDef def)
        {
            return _map.TryGetValue(id, out def);
        }

        public TDef Get(TKey id)
        {
            TDef def;
            if (!_map.TryGetValue(id, out def))
                throw new KeyNotFoundException(
                    "Definition을 찾을 수 없습니다: " + id);

            return def;
        }
    }


    public sealed class GameDataRegistry
    {
        public DefinitionRegistry<CharacterId, CharacterDef> Characters { get; private set; }
        public DefinitionRegistry<ActionDefKey, ActionDef> Actions { get; private set; }
        public DefinitionRegistry<AnimationId, AnimationResourceDef> Animations { get; private set; }
        public DefinitionRegistry<BuffId, BuffDef> Buffs { get; private set; }
        public DefinitionRegistry<WorldId, WorldDef> Worlds { get; private set; }
        public DefinitionRegistry<SpawnSetId, SpawnSetDef> SpawnSets { get; private set; }

        public GameDataRegistry()
        {
            Characters = new DefinitionRegistry<CharacterId, CharacterDef>();
            Actions = new DefinitionRegistry<ActionDefKey, ActionDef>();
            Animations = new DefinitionRegistry<AnimationId, AnimationResourceDef>();
            Buffs = new DefinitionRegistry<BuffId, BuffDef>();
            Worlds = new DefinitionRegistry<WorldId, WorldDef>();
            SpawnSets = new DefinitionRegistry<SpawnSetId, SpawnSetDef>();
        }

        // --------------------------------------------------------------------
        // 기본 Contains / Get / TryGet
        // --------------------------------------------------------------------

        bool ContainsCharacter(CharacterId id)
        {
            return Characters.Contains(id);
        }

        public bool ContainsAction(ActionDefKey key)
        {
            return Actions.Contains(key);
        }

        public bool ContainsAction(CharacterId characterId, byte actionDefId)
        {
            return Actions.Contains(new ActionDefKey(characterId, actionDefId));
        }

        public bool ContainsAnimation(AnimationId id)
        {
            return Animations.Contains(id);
        }

        public bool ContainsBuff(BuffId id)
        {
            return Buffs.Contains(id);
        }

        public bool ContainsWorld(WorldId id)
        {
            return Worlds.Contains(id);
        }

        public bool ContainsSpawnSet(SpawnSetId id)
        {
            return SpawnSets.Contains(id);
        }

        public CharacterDef GetCharacter(CharacterId id)
        {
            return Characters.Get(id);
        }

        public ActionDef GetAction(ActionDefKey key)
        {
            return Actions.Get(key);
        }

        public ActionDef GetAction(CharacterId characterId, byte actionDefId)
        {
            return Actions.Get(new ActionDefKey(characterId, actionDefId));
        }

        public AnimationResourceDef GetAnimation(AnimationId id)
        {
            return Animations.Get(id);
        }

        public BuffDef GetBuff(BuffId id)
        {
            return Buffs.Get(id);
        }

        public WorldDef GetWorld(WorldId id)
        {
            return Worlds.Get(id);
        }

        public SpawnSetDef GetSpawnSet(SpawnSetId id)
        {
            return SpawnSets.Get(id);
        }

        public bool TryGetCharacter(CharacterId id, out CharacterDef def)
        {
            return Characters.TryGet(id, out def);
        }

        public bool TryGetAction(ActionDefKey key, out ActionDef def)
        {
            return Actions.TryGet(key, out def);
        }

        public bool TryGetAction(CharacterId characterId, byte actionDefId, out ActionDef def)
        {
            return Actions.TryGet(new ActionDefKey(characterId, actionDefId), out def);
        }

        public bool TryGetAnimation(AnimationId id, out AnimationResourceDef def)
        {
            return Animations.TryGet(id, out def);
        }

        public bool TryGetBuff(BuffId id, out BuffDef def)
        {
            return Buffs.TryGet(id, out def);
        }

        public bool TryGetWorld(WorldId id, out WorldDef def)
        {
            return Worlds.TryGet(id, out def);
        }

        public bool TryGetSpawnSet(SpawnSetId id, out SpawnSetDef def)
        {
            return SpawnSets.TryGet(id, out def);
        }

        // --------------------------------------------------------------------
        // 전체 열거
        // --------------------------------------------------------------------

        public IEnumerable<CharacterDef> GetAllCharacters()
        {
            return Characters.Values;
        }

        public IEnumerable<ActionDef> GetAllActions()
        {
            return Actions.Values;
        }

        public IEnumerable<AnimationResourceDef> GetAllAnimations()
        {
            return Animations.Values;
        }

        public IEnumerable<BuffDef> GetAllBuffs()
        {
            return Buffs.Values;
        }

        public IEnumerable<WorldDef> GetAllWorlds()
        {
            return Worlds.Values;
        }

        public IEnumerable<SpawnSetDef> GetAllSpawnSets()
        {
            return SpawnSets.Values;
        }

        // --------------------------------------------------------------------
        // Character 기반 조회
        // --------------------------------------------------------------------

        public List<BuffDef> GetDefaultBuffsOfCharacter(CharacterId characterId)
        {
            CharacterDef character = GetCharacter(characterId);
            return GetDefaultBuffsOfCharacter(character);
        }

        public List<BuffDef> GetDefaultBuffsOfCharacter(CharacterDef character)
        {
            if (character == null)
                throw new ArgumentNullException("character");

            List<BuffDef> list = new List<BuffDef>();

            if (character.DefaultBuffIds == null)
                return list;

            for (int i = 0; i < character.DefaultBuffIds.Count; ++i)
            {
                BuffId buffId = character.DefaultBuffIds[i];
                list.Add(GetBuff(buffId));
            }

            return list;
        }

        public bool HasDefaultBuff(CharacterId characterId, BuffId buffId)
        {
            CharacterDef character;
            if (TryGetCharacter(characterId, out character) == false)
                return false;

            if (character.DefaultBuffIds == null)
                return false;

            for (int i = 0; i < character.DefaultBuffIds.Count; ++i)
            {
                if (character.DefaultBuffIds[i].Equals(buffId))
                    return true;
            }

            return false;
        }

        // --------------------------------------------------------------------
        // Action 기반 조회
        // --------------------------------------------------------------------

        public CharacterDef GetOwnerCharacterOfAction(ActionDefKey key)
        {
            ActionDef action = GetAction(key);
            return GetCharacter(action.Id.CharacterId);
        }

        public CharacterDef GetOwnerCharacterOfAction(CharacterId characterId, byte actionDefId)
        {
            ActionDef action = GetAction(characterId, actionDefId);
            return GetCharacter(action.Id.CharacterId);
        }

        public AnimationResourceDef GetAnimationOfAction(ActionDefKey key)
        {
            ActionDef action = GetAction(key);
            return GetAnimation(action.AnimationId);
        }

        public AnimationResourceDef GetAnimationOfAction(CharacterId characterId, byte actionDefId)
        {
            ActionDef action = GetAction(characterId, actionDefId);
            return GetAnimation(action.AnimationId);
        }

        public List<ActionDef> GetActionsByAnimation(AnimationId animationId)
        {
            List<ActionDef> list = new List<ActionDef>();

            foreach (ActionDef action in Actions.Values)
            {
                if (action.AnimationId.Equals(animationId))
                    list.Add(action);
            }

            return list;
        }

        public List<ActionDef> GetActionsByCharacter(CharacterId characterId)
        {
            List<ActionDef> list = new List<ActionDef>();

            foreach (ActionDef action in Actions.Values)
            {
                if (action.Id.CharacterId.Equals(characterId))
                    list.Add(action);
            }

            return list;
        }

        public ActionDef FindActionByActionId(ActionId actionId)
        {
            if (actionId == ActionId.None)
                throw new ArgumentException("ActionId.None 은 조회에 사용할 수 없습니다.", "actionId");

            foreach (ActionDef action in Actions.Values)
            {
                if (action.ActionId.Equals(actionId))
                    return action;
            }

            return null;
        }

        public List<ActionDef> FindActionsByActionId(ActionId actionId)
        {
            if (actionId == ActionId.None)
                throw new ArgumentException("ActionId.None 은 조회에 사용할 수 없습니다.", "actionId");

            List<ActionDef> list = new List<ActionDef>();

            foreach (ActionDef action in Actions.Values)
            {
                if (action.ActionId.Equals(actionId))
                    list.Add(action);
            }

            return list;
        }

        // --------------------------------------------------------------------
        // Animation 기반 조회
        // --------------------------------------------------------------------

        public List<ActionDef> GetReferencingActions(AnimationId animationId)
        {
            return GetActionsByAnimation(animationId);
        }

        public bool IsAnimationUsed(AnimationId animationId)
        {
            foreach (ActionDef action in Actions.Values)
            {
                if (action.AnimationId.Equals(animationId))
                    return true;
            }

            return false;
        }

        // --------------------------------------------------------------------
        // Buff 기반 조회
        // --------------------------------------------------------------------

        public List<CharacterDef> GetCharactersUsingDefaultBuff(BuffId buffId)
        {
            List<CharacterDef> list = new List<CharacterDef>();

            foreach (CharacterDef character in Characters.Values)
            {
                if (character.DefaultBuffIds == null)
                    continue;

                for (int i = 0; i < character.DefaultBuffIds.Count; ++i)
                {
                    if (character.DefaultBuffIds[i].Equals(buffId))
                    {
                        list.Add(character);
                        break;
                    }
                }
            }

            return list;
        }

        public bool IsBuffUsedAsDefault(BuffId buffId)
        {
            foreach (CharacterDef character in Characters.Values)
            {
                if (character.DefaultBuffIds == null)
                    continue;

                for (int i = 0; i < character.DefaultBuffIds.Count; ++i)
                {
                    if (character.DefaultBuffIds[i].Equals(buffId))
                        return true;
                }
            }

            return false;
        }

        // --------------------------------------------------------------------
        // World / Map / SpawnSet 기반 조회
        // --------------------------------------------------------------------

        public MapDef GetMapOfWorld(WorldId worldId)
        {
            WorldDef world = GetWorld(worldId);
            return world.Map;
        }

        public SpawnSetDef GetSpawnSetOfWorld(WorldId worldId)
        {
            WorldDef world = GetWorld(worldId);
            return GetSpawnSet(world.SpawnSetId);
        }

        public bool TryGetSpawnSetOfWorld(WorldId worldId, out SpawnSetDef spawnSet)
        {
            spawnSet = null;

            WorldDef world;
            if (TryGetWorld(worldId, out world) == false)
                return false;

            return TryGetSpawnSet(world.SpawnSetId, out spawnSet);
        }

        public List<SpawnEntryDef> GetSpawnEntriesOfWorld(WorldId worldId)
        {
            SpawnSetDef spawnSet = GetSpawnSetOfWorld(worldId);

            if (spawnSet.Entries == null)
                return new List<SpawnEntryDef>();

            return new List<SpawnEntryDef>(spawnSet.Entries);
        }

        public List<CharacterDef> GetSpawnCharactersOfWorld(WorldId worldId)
        {
            SpawnSetDef spawnSet = GetSpawnSetOfWorld(worldId);
            return GetSpawnCharactersOfSpawnSet(spawnSet);
        }

        public List<CharacterDef> GetSpawnCharactersOfSpawnSet(SpawnSetId spawnSetId)
        {
            SpawnSetDef spawnSet = GetSpawnSet(spawnSetId);
            return GetSpawnCharactersOfSpawnSet(spawnSet);
        }

        public List<CharacterDef> GetSpawnCharactersOfSpawnSet(SpawnSetDef spawnSet)
        {
            if (spawnSet == null)
                throw new ArgumentNullException("spawnSet");

            List<CharacterDef> list = new List<CharacterDef>();

            if (spawnSet.Entries == null)
                return list;

            for (int i = 0; i < spawnSet.Entries.Count; ++i)
            {
                SpawnEntryDef entry = spawnSet.Entries[i];
                if (entry == null)
                    continue;

                list.Add(GetCharacter(entry.CharacterId));
            }

            return list;
        }

        public List<SpawnEntryDef> GetSpawnEntriesOfSpawnSet(SpawnSetId spawnSetId)
        {
            SpawnSetDef spawnSet = GetSpawnSet(spawnSetId);

            if (spawnSet.Entries == null)
                return new List<SpawnEntryDef>();

            return new List<SpawnEntryDef>(spawnSet.Entries);
        }

        public List<WorldDef> GetWorldsUsingSpawnSet(SpawnSetId spawnSetId)
        {
            List<WorldDef> list = new List<WorldDef>();

            foreach (WorldDef world in Worlds.Values)
            {
                if (world.SpawnSetId.Equals(spawnSetId))
                    list.Add(world);
            }

            return list;
        }

        public bool IsSpawnSetUsedByAnyWorld(SpawnSetId spawnSetId)
        {
            foreach (WorldDef world in Worlds.Values)
            {
                if (world.SpawnSetId.Equals(spawnSetId))
                    return true;
            }

            return false;
        }

        // --------------------------------------------------------------------
        // SpawnSet / SpawnEntry 기반 조회
        // --------------------------------------------------------------------

        public List<SpawnSetDef> GetSpawnSetsContainingCharacter(CharacterId characterId)
        {
            List<SpawnSetDef> list = new List<SpawnSetDef>();

            foreach (SpawnSetDef set in SpawnSets.Values)
            {
                if (set.Entries == null)
                    continue;

                for (int i = 0; i < set.Entries.Count; ++i)
                {
                    SpawnEntryDef entry = set.Entries[i];
                    if (entry == null)
                        continue;

                    if (entry.CharacterId.Equals(characterId))
                    {
                        list.Add(set);
                        break;
                    }
                }
            }

            return list;
        }

        public List<WorldDef> GetWorldsContainingCharacterInSpawn(CharacterId characterId)
        {
            List<WorldDef> list = new List<WorldDef>();

            foreach (WorldDef world in Worlds.Values)
            {
                SpawnSetDef set;
                if (TryGetSpawnSet(world.SpawnSetId, out set) == false)
                    continue;

                if (set.Entries == null)
                    continue;

                for (int i = 0; i < set.Entries.Count; ++i)
                {
                    SpawnEntryDef entry = set.Entries[i];
                    if (entry == null)
                        continue;

                    if (entry.CharacterId.Equals(characterId))
                    {
                        list.Add(world);
                        break;
                    }
                }
            }

            return list;
        }

        public bool IsCharacterUsedInAnySpawnSet(CharacterId characterId)
        {
            foreach (SpawnSetDef set in SpawnSets.Values)
            {
                if (set.Entries == null)
                    continue;

                for (int i = 0; i < set.Entries.Count; ++i)
                {
                    SpawnEntryDef entry = set.Entries[i];
                    if (entry == null)
                        continue;

                    if (entry.CharacterId.Equals(characterId))
                        return true;
                }
            }

            return false;
        }

        // --------------------------------------------------------------------
        // 참조 무결성 보조 조회
        // --------------------------------------------------------------------

        public bool HasBrokenReferences()
        {
            foreach (CharacterDef ch in Characters.Values)
            {
                if (ch.DefaultBuffIds != null)
                {
                    for (int i = 0; i < ch.DefaultBuffIds.Count; ++i)
                    {
                        if (ContainsBuff(ch.DefaultBuffIds[i]) == false)
                            return true;
                    }
                }
            }

            foreach (ActionDef action in Actions.Values)
            {
                if (ContainsCharacter(action.Id.CharacterId) == false)
                    return true;

                if (ContainsAnimation(action.AnimationId) == false)
                    return true;
            }

            foreach (WorldDef world in Worlds.Values)
            {
                if (ContainsSpawnSet(world.SpawnSetId) == false)
                    return true;
            }

            foreach (SpawnSetDef set in SpawnSets.Values)
            {
                if (set.Entries == null)
                    continue;

                for (int i = 0; i < set.Entries.Count; ++i)
                {
                    SpawnEntryDef entry = set.Entries[i];
                    if (entry == null)
                        continue;

                    if (ContainsCharacter(entry.CharacterId) == false)
                        return true;
                }
            }

            return false;
        }




        public static GameDataRegistry Build(GameDataSet data)
        {
            if (data == null)
                throw new ArgumentNullException("data");

            ValidationResult validation = Utility.GameDataValidator.ValidateAll(data);
            if (!validation.IsValid)
            {
                throw new InvalidOperationException(
                    "GameDataSet 검증 실패:\n" + validation.ToDisplayString());
            }

            GameDataRegistry registry = new GameDataRegistry();

            if (data.Characters != null)
            {
                for (int i = 0; i < data.Characters.Count; ++i)
                {
                    registry.Characters.Add(data.Characters[i]);
                }
            }

            if (data.Actions != null)
            {
                for (int i = 0; i < data.Actions.Count; ++i)
                {
                    registry.Actions.Add(data.Actions[i]);
                }
            }

            if (data.Animations != null)
            {
                for (int i = 0; i < data.Animations.Count; ++i)
                {
                    registry.Animations.Add(data.Animations[i]);
                }
            }

            if (data.Buffs != null)
            {
                for (int i = 0; i < data.Buffs.Count; ++i)
                {
                    registry.Buffs.Add(data.Buffs[i]);
                }
            }

            if (data.Worlds != null)
            {
                for (int i = 0; i < data.Worlds.Count; ++i)
                {
                    registry.Worlds.Add(data.Worlds[i]);
                }
            }

            if (data.SpawnSets != null)
            {
                for (int i = 0; i < data.SpawnSets.Count; ++i)
                {
                    registry.SpawnSets.Add(data.SpawnSets[i]);
                }
            }

            return registry;
        }
    }
}
