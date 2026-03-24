using System;
using System.Collections.Generic;
using System.Windows.Forms;
using WITH_ServerDataTool.Definition;
using WITH_ServerDataTool.ID;

namespace WITH_ServerDataTool.Editor
{
    public sealed class CharacterDefEditor : DefinitionEditorBase<CharacterDef>
    {
        private readonly ICharacterEditorLookupProvider _lookupProvider;

        private EditorLayoutPanel _layout;

        private DefinitionReferenceComboBox<CharacterId> _cmbId;
        private TextBox _txtName;
        private DefinitionReferenceComboBox<EntityCategoryId> _cmbCategory;
        private DefinitionReferenceComboBox<FactionId> _cmbFaction;
        private DefinitionReferenceComboBox<AIArchetypeId> _cmbAIArchetype;
        private ReferenceListEditor<BuffId> _buffListEditor;

        public CharacterDefEditor(ICharacterEditorLookupProvider lookupProvider)
        {
            if (lookupProvider == null)
                throw new ArgumentNullException("lookupProvider");

            _lookupProvider = lookupProvider;

            BuildUi();
            WireEvents();
        }

        private void BuildUi()
        {
            _layout = new EditorLayoutPanel();
            _layout.Dock = DockStyle.Fill;
            _layout.Padding = new Padding(8);

            _cmbId = new DefinitionReferenceComboBox<CharacterId>();
            _txtName = new TextBox();
            _cmbCategory = new DefinitionReferenceComboBox<EntityCategoryId>();
            _cmbFaction = new DefinitionReferenceComboBox<FactionId>();
            _cmbAIArchetype = new DefinitionReferenceComboBox<AIArchetypeId>();
            _buffListEditor = new ReferenceListEditor<BuffId>();

            _cmbId.Dock = DockStyle.Fill;
            _txtName.Dock = DockStyle.Fill;
            _cmbCategory.Dock = DockStyle.Fill;
            _cmbFaction.Dock = DockStyle.Fill;
            _cmbAIArchetype.Dock = DockStyle.Fill;
            _buffListEditor.Dock = DockStyle.Fill;
            _buffListEditor.Height = 180;

            _layout.AddRow("Id", _cmbId);
            _layout.AddRow("Name", _txtName);
            _layout.AddRow("Category", _cmbCategory);
            _layout.AddRow("Default Faction", _cmbFaction);
            _layout.AddRow("AI Archetype", _cmbAIArchetype);
            _layout.AddRow("Default Buffs", _buffListEditor);

            Controls.Clear();
            Controls.Add(_layout);
        }

        private void WireEvents()
        {
            _cmbId.SelectedIdChanged += OnAnyValueChanged;
            _txtName.TextChanged += OnAnyValueChanged;
            _cmbCategory.SelectedIdChanged += OnAnyValueChanged;
            _cmbFaction.SelectedIdChanged += OnAnyValueChanged;
            _cmbAIArchetype.SelectedIdChanged += OnAnyValueChanged;
            _buffListEditor.ListChanged += OnAnyValueChanged;
        }

        private void OnAnyValueChanged(object sender, EventArgs e)
        {
            MarkDirty();
        }

        protected override void OnBeforeBind(
            DefinitionKind kind,
            CharacterDef source,
            CharacterDef workingCopy)
        {
            _cmbId.SetItems(_lookupProvider.GetCharacterItems());
            _cmbCategory.SetItems(_lookupProvider.GetEntityCategoryItems());
            _cmbFaction.SetItems(_lookupProvider.GetFactionItems());
            _cmbAIArchetype.SetItems(_lookupProvider.GetAIArchetypeItems());

            _buffListEditor.Bind(
                workingCopy.DefaultBuffIds,
                _lookupProvider.GetBuffItems());
        }

        protected override CharacterDef CloneDefinition(CharacterDef source)
        {
            CharacterDef clone = new CharacterDef();
            clone.Id = source.Id;
            clone.Name = source.Name;
            clone.CategoryId = source.CategoryId;
            clone.DefaultFactionId = source.DefaultFactionId;
            clone.AIArchetypeId = source.AIArchetypeId;

            clone.DefaultBuffIds = new List<BuffId>();
            if (source.DefaultBuffIds != null)
            {
                for (int i = 0; i < source.DefaultBuffIds.Count; ++i)
                    clone.DefaultBuffIds.Add(source.DefaultBuffIds[i]);
            }

            return clone;
        }

        protected override void CopyDefinition(CharacterDef from, CharacterDef to)
        {
            to.Id = from.Id;
            to.Name = from.Name;
            to.CategoryId = from.CategoryId;
            to.DefaultFactionId = from.DefaultFactionId;
            to.AIArchetypeId = from.AIArchetypeId;

            if (to.DefaultBuffIds == null)
                to.DefaultBuffIds = new List<BuffId>();
            else
                to.DefaultBuffIds.Clear();

            if (from.DefaultBuffIds != null)
            {
                for (int i = 0; i < from.DefaultBuffIds.Count; ++i)
                    to.DefaultBuffIds.Add(from.DefaultBuffIds[i]);
            }
        }

        protected override void LoadToControls(CharacterDef value)
        {
            _cmbId.SetSelectedId(value.Id);
            _txtName.Text = value.Name ?? string.Empty;
            _cmbCategory.SetSelectedId(value.CategoryId);
            _cmbFaction.SetSelectedId(value.DefaultFactionId);
            _cmbAIArchetype.SetSelectedId(value.AIArchetypeId);

            _buffListEditor.RefreshList();
        }

        protected override void SaveFromControls(CharacterDef value)
        {
            value.Id = _cmbId.GetSelectedId();
            value.Name = _txtName.Text ?? string.Empty;
            value.CategoryId = _cmbCategory.GetSelectedId();
            value.DefaultFactionId = _cmbFaction.GetSelectedId();
            value.AIArchetypeId = _cmbAIArchetype.GetSelectedId();
        }

        protected override void ClearControls()
        {
            _cmbId.SelectedIndex = -1;
            _txtName.Text = string.Empty;
            _cmbCategory.SelectedIndex = -1;
            _cmbFaction.SelectedIndex = -1;
            _cmbAIArchetype.SelectedIndex = -1;
            _buffListEditor.ClearItems();
        }
    }
}