using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using WITH_ServerDataTool.Definition;
using WITH_ServerDataTool.ID;

namespace WITH_ServerDataTool.Editor
{
    public interface ICharacterEditorLookupProvider
    {
        IEnumerable<DefinitionReferenceItem<CharacterId>> GetCharacterItems();
        IEnumerable<DefinitionReferenceItem<EntityCategoryId>> GetEntityCategoryItems();
        IEnumerable<DefinitionReferenceItem<FactionId>> GetFactionItems();
        IEnumerable<DefinitionReferenceItem<AIArchetypeId>> GetAIArchetypeItems();
        IEnumerable<DefinitionReferenceItem<BuffId>> GetBuffItems();
    }

    public sealed class CharacterEditorLookupProvider
        : ICharacterEditorLookupProvider
    {
        private readonly EditorController _controller;

        public CharacterEditorLookupProvider(EditorController controller)
        {
            _controller = controller;
        }

        public IEnumerable<DefinitionReferenceItem<CharacterId>> GetCharacterItems()
        {
            return EnumReferenceItems.Create<CharacterId>();
        }

        public IEnumerable<DefinitionReferenceItem<EntityCategoryId>> GetEntityCategoryItems()
        {
            return EnumReferenceItems.Create<EntityCategoryId>();
        }

        public IEnumerable<DefinitionReferenceItem<FactionId>> GetFactionItems()
        {
            return EnumReferenceItems.Create<FactionId>();
        }

        public IEnumerable<DefinitionReferenceItem<AIArchetypeId>> GetAIArchetypeItems()
        {
            return EnumReferenceItems.Create<AIArchetypeId>();
        }

        public IEnumerable<DefinitionReferenceItem<BuffId>> GetBuffItems()
        {
            return EnumReferenceItems.Create<BuffId>();
        }
    }

    public interface IActionEditorLookupProvider
    {
        IEnumerable<DefinitionReferenceItem<CharacterId>> GetCharacterItems();
        IEnumerable<DefinitionReferenceItem<ActionId>> GetActionItems();
        IEnumerable<DefinitionReferenceItem<AnimationId>> GetAnimationItems();
    }

    public sealed class ActionEditorLookupProvider
        : IActionEditorLookupProvider
    {
        private readonly EditorController _controller;

        public ActionEditorLookupProvider(EditorController controller)
        {
            _controller = controller;
        }

        public IEnumerable<DefinitionReferenceItem<CharacterId>> GetCharacterItems()
        {
            return EnumReferenceItems.Create<CharacterId>();
        }

        public IEnumerable<DefinitionReferenceItem<ActionId>> GetActionItems()
        {
            return EnumReferenceItems.Create<ActionId>();
        }

        public IEnumerable<DefinitionReferenceItem<AnimationId>> GetAnimationItems()
        {
            return EnumReferenceItems.Create<AnimationId>();
        }
    }


    public interface IAnimationResourceEditorLookupProvider
    {
        IEnumerable<DefinitionReferenceItem<AnimationId>> GetAnimationItems();
    }

    public sealed class AnimationResourceEditorLookupProvider
        : IAnimationResourceEditorLookupProvider
    {
        private readonly EditorController _controller;

        public AnimationResourceEditorLookupProvider(EditorController controller)
        {
            _controller = controller;
        }

        public IEnumerable<DefinitionReferenceItem<AnimationId>> GetAnimationItems()
        {
            return EnumReferenceItems.Create<AnimationId>();
        }
    }

    public interface IBuffEditorLookupProvider
    {
        IEnumerable<DefinitionReferenceItem<BuffId>> GetBuffItems();
    }

    public sealed class BuffEditorLookupProvider
        : IBuffEditorLookupProvider
    {
        private readonly EditorController _controller;

        public BuffEditorLookupProvider(EditorController controller)
        {
            _controller = controller;
        }

        public IEnumerable<DefinitionReferenceItem<BuffId>> GetBuffItems()
        {
            return EnumReferenceItems.Create<BuffId>();
        }
    }

    public interface IWorldEditorLookupProvider
    {
        IEnumerable<DefinitionReferenceItem<WorldId>> GetWorldItems();
        IEnumerable<DefinitionReferenceItem<SpawnSetId>> GetSpawnSetItems();
    }

    public sealed class WorldEditorLookupProvider
        : IWorldEditorLookupProvider
    {
        private readonly EditorController _controller;

        public WorldEditorLookupProvider(EditorController controller)
        {
            _controller = controller;
        }

        public IEnumerable<DefinitionReferenceItem<WorldId>> GetWorldItems()
        {
            return EnumReferenceItems.Create<WorldId>();
        }

        public IEnumerable<DefinitionReferenceItem<SpawnSetId>> GetSpawnSetItems()
        {
            return EnumReferenceItems.Create<SpawnSetId>();
        }
    }

    public interface ISpawnSetEditorLookupProvider
    {
        IEnumerable<DefinitionReferenceItem<SpawnSetId>> GetSpawnSetItems();
    }

    public sealed class SpawnSetEditorLookupProvider
        : ISpawnSetEditorLookupProvider
    {
        private readonly EditorController _controller;

        public SpawnSetEditorLookupProvider(EditorController controller)
        {
            _controller = controller;
        }

        public IEnumerable<DefinitionReferenceItem<SpawnSetId>> GetSpawnSetItems()
        {
            return EnumReferenceItems.Create<SpawnSetId>();
        }
    }

    public interface ISpawnEntryEditorLookupProvider
    {
        IEnumerable<DefinitionReferenceItem<CharacterId>> GetCharacterItems();
    }

    public sealed class SpawnEntryEditorLookupProvider
        : ISpawnEntryEditorLookupProvider
    {
        private readonly EditorController _controller;

        public SpawnEntryEditorLookupProvider(EditorController controller)
        {
            _controller = controller;
        }

        public IEnumerable<DefinitionReferenceItem<CharacterId>> GetCharacterItems()
        {
            return EnumReferenceItems.Create<CharacterId>();
        }
    }
}
