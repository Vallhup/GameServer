using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using WITH_ServerDataTool.Definition;
using WITH_ServerDataTool.Utility;

namespace WITH_ServerDataTool.Editor
{
    public enum DefinitionKind
    {
        Characters,
        Actions,
        Animations,
        Buffs,
        Worlds,
        SpawnSets
    }

    public sealed class EditorController
    {
        private readonly IGameDataSerializer _serializer;

        public GameDataSet Data { get; private set; }
        public ValidationResult LastValidation { get; private set; }
        public string CurrentFilePath { get; private set; }

        public bool IsDirty { get; private set; }

        public EditorController(IGameDataSerializer serializer)
        {
            if (serializer == null)
                throw new ArgumentNullException("serializer");

            _serializer = serializer;
            NewData();
        }

        public void NewData()
        {
            Data = new GameDataSet();
            Data.Characters = new List<CharacterDef>();
            Data.Actions = new List<ActionDef>();
            Data.Animations = new List<AnimationResourceDef>();
            Data.Buffs = new List<BuffDef>();
            Data.Worlds = new List<WorldDef>();
            Data.SpawnSets = new List<SpawnSetDef>();

            LastValidation = new ValidationResult();
            CurrentFilePath = null;

            IsDirty = false;
        }

        public void LoadFromFile(string path)
        {
            if (string.IsNullOrEmpty(path))
                throw new ArgumentException("path");

            string text = File.ReadAllText(path);
            Data = _serializer.Deserialize(text);
            CurrentFilePath = path;
            LastValidation = new ValidationResult();

            IsDirty = false;
        }

        public void SaveToFile(string path)
        {
            if (string.IsNullOrEmpty(path))
                throw new ArgumentException("path");

            string text = _serializer.Serialize(Data);
            File.WriteAllText(path, text);
            CurrentFilePath = path;

            IsDirty = false;
        }

        public void Save()
        {
            if (string.IsNullOrEmpty(CurrentFilePath))
                throw new InvalidOperationException("현재 파일 경로가 없습니다.");

            SaveToFile(CurrentFilePath);
        }

        public ValidationResult Validate()
        {
            LastValidation = GameDataValidator.ValidateAll(Data);
            return LastValidation;
        }

        public void MarkDirty()
        {
            IsDirty = true;
        }

        public IList GetItems(DefinitionKind kind)
        {
            switch (kind)
            {
                case DefinitionKind.Characters: return Data.Characters;
                case DefinitionKind.Actions: return Data.Actions;
                case DefinitionKind.Animations: return Data.Animations;
                case DefinitionKind.Buffs: return Data.Buffs;
                case DefinitionKind.Worlds: return Data.Worlds;
                case DefinitionKind.SpawnSets: return Data.SpawnSets;
                default:
                    throw new InvalidOperationException("알 수 없는 DefinitionKind 입니다.");
            }
        }

        public object CreateNewItem(DefinitionKind kind)
        {
            switch (kind)
            {
                case DefinitionKind.Characters:
                    return new CharacterDef();

                case DefinitionKind.Actions:
                    return new ActionDef
                    {
                        Id = new ActionDefKey(),
                        Segments = new List<ActionSegmentDef>()
                    };

                case DefinitionKind.Animations:
                    return new AnimationResourceDef();

                case DefinitionKind.Buffs:
                    return new BuffDef
                    {
                        Modifiers = new List<BuffModifierDef>()
                    };

                case DefinitionKind.Worlds:
                    return new WorldDef
                    {
                        Map = new MapDef()
                    };

                case DefinitionKind.SpawnSets:
                    return new SpawnSetDef
                    {
                        Entries = new List<SpawnEntryDef>()
                    };

                default:
                    throw new InvalidOperationException("알 수 없는 DefinitionKind 입니다.");
            }
        }

        public void AddItem(DefinitionKind kind, object item)
        {
            IList list = GetItems(kind);
            list.Add(item);
            IsDirty = true;
        }

        public object AddNewItem(DefinitionKind kind)
        {
            object item = CreateNewItem(kind);
            AddItem(kind, item);
            return item;
        }

        public void RemoveItem(DefinitionKind kind, object item)
        {
            IList list = GetItems(kind);
            list.Remove(item);
            IsDirty = true;
        }
    }
}
