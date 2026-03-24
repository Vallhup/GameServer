using System;
using WITH_ServerDataTool.Definition;

namespace WITH_ServerDataTool.Editor
{
    public sealed class DefinitionEditorFactory
    {
        private readonly ICharacterEditorLookupProvider _characterLookupProvider;
        private readonly IActionEditorLookupProvider _actionLookupProvider;
        private readonly IAnimationResourceEditorLookupProvider _animationLookupProvider;
        private readonly IBuffEditorLookupProvider _buffEditorLookupProvider;
        private readonly IWorldEditorLookupProvider _worldLookupProvider;
        private readonly ISpawnSetEditorLookupProvider _spawnSetLookupProvider;
        private readonly ISpawnEntryEditorLookupProvider _spawnEntryLookupProvider;

        public DefinitionEditorFactory(
            ICharacterEditorLookupProvider characterLookupProvider,
            IActionEditorLookupProvider actionLookupProvider,
            IAnimationResourceEditorLookupProvider animationLookupProvider,
            IBuffEditorLookupProvider buffEditorLookupProvider,
            IWorldEditorLookupProvider worldLookupProvider,
            ISpawnSetEditorLookupProvider spawnSetEditorLookupProvider,
            ISpawnEntryEditorLookupProvider spawnEntryLookupProvider)
        {
            if (characterLookupProvider == null)
                throw new ArgumentNullException("characterLookupProvider");

            _characterLookupProvider = characterLookupProvider;

            if (actionLookupProvider == null)
                throw new ArgumentNullException("actionLookupProvider");

            _actionLookupProvider = actionLookupProvider;

            if (animationLookupProvider == null)
                throw new ArgumentNullException("animationLookupProvider");

            _animationLookupProvider = animationLookupProvider;

            if (buffEditorLookupProvider == null)
                throw new ArgumentNullException("buffEditorLookupProvider");

            _buffEditorLookupProvider = buffEditorLookupProvider;

            if (worldLookupProvider == null)
                throw new ArgumentNullException("worldLookupProvider");

            _worldLookupProvider = worldLookupProvider;

            if (spawnSetEditorLookupProvider == null)
                throw new ArgumentNullException("spawnSetEditorLookupProvider");

            _spawnSetLookupProvider = spawnSetEditorLookupProvider;

            if (spawnEntryLookupProvider == null)
                throw new ArgumentNullException("spawnEntryLookupProvider");

            _spawnEntryLookupProvider = spawnEntryLookupProvider;
        }

        public IDefinitionEditor CreateEditor(DefinitionKind kind)
        {
            switch (kind)
            {
                case DefinitionKind.Characters:
                    return new CharacterDefEditor(_characterLookupProvider);

                case DefinitionKind.Actions:
                    return new ActionDefEditor(_actionLookupProvider);

                case DefinitionKind.Animations:
                    return new AnimationResourceDefEditor(_animationLookupProvider);

                case DefinitionKind.Buffs:
                    return new BuffDefEditor(_buffEditorLookupProvider);

                case DefinitionKind.Worlds:
                    return new WorldDefEditor(_worldLookupProvider);

                case DefinitionKind.SpawnSets:
                    return new SpawnSetDefEditor(_spawnSetLookupProvider, _spawnEntryLookupProvider);

                default:
                    return new PropertyGridEditor();
            }
        }
    }
}
