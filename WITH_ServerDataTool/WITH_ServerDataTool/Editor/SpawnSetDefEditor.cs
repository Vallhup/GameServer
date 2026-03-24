using System;
using System.Collections.Generic;
using System.Windows.Forms;
using WITH_ServerDataTool.Editor;
using WITH_ServerDataTool.ID;

namespace WITH_ServerDataTool.Definition
{
    public sealed class SpawnSetDefEditor : DefinitionEditorBase<SpawnSetDef>
    {
        private readonly ISpawnSetEditorLookupProvider _setLookupProvider;
        private readonly ISpawnEntryEditorLookupProvider _entryLookupProvider;
        
        private EditorLayoutPanel _layout;

        private DefinitionReferenceComboBox<SpawnSetId> _cmbId;
        private TextBox _txtName;

        private TableLayoutPanel _entryLayout;
        private Panel _entryListPanel;
        private ListBox _lstEntries;
        private TableLayoutPanel _entryButtonPanel;
        private Button _btnAddEntry;
        private Button _btnRemoveEntry;
        private Button _btnMoveUpEntry;
        private Button _btnMoveDownEntry;
        private SpawnEntryDefEditor _entryEditor;

        private bool _suppressEvents;

        public SpawnSetDefEditor(
            ISpawnSetEditorLookupProvider setLookupProvider,
            ISpawnEntryEditorLookupProvider entryLookupProvider)
        {
            if (setLookupProvider == null)
                throw new ArgumentNullException("lookupProvider");

            _setLookupProvider = setLookupProvider;

            if (entryLookupProvider == null)
                throw new ArgumentNullException("lookupProvider");

            _entryLookupProvider = entryLookupProvider;

            BuildUi();
            WireEvents();
        }

        private void BuildUi()
        {
            _layout = new EditorLayoutPanel();
            _layout.Dock = DockStyle.Fill;
            _layout.Padding = new Padding(8);

            _cmbId = new DefinitionReferenceComboBox<SpawnSetId>();
            _cmbId.Dock = DockStyle.Fill;

            _txtName = new TextBox();
            _txtName.Dock = DockStyle.Fill;

            BuildEntryUi();

            _layout.AddRow("Id", _cmbId);
            _layout.AddRow("Name", _txtName);
            _layout.AddRow("Entries", _entryLayout);

            Controls.Clear();
            Controls.Add(_layout);
        }

        private void BuildEntryUi()
        {
            _entryLayout = new TableLayoutPanel();
            _entryLayout.Dock = DockStyle.Fill;
            _entryLayout.Margin = new Padding(0);
            _entryLayout.Padding = new Padding(0);
            _entryLayout.ColumnCount = 2;
            _entryLayout.RowCount = 1;
            _entryLayout.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 220F));
            _entryLayout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100F));
            _entryLayout.RowStyles.Add(new RowStyle(SizeType.Percent, 100F));

            _entryListPanel = new Panel();
            _entryListPanel.Dock = DockStyle.Fill;
            _entryListPanel.Margin = new Padding(0, 0, 6, 0);

            _lstEntries = new ListBox();
            _lstEntries.Dock = DockStyle.Fill;

            _entryButtonPanel = new TableLayoutPanel();
            _entryButtonPanel.Dock = DockStyle.Bottom;
            _entryButtonPanel.Height = 34;
            _entryButtonPanel.ColumnCount = 4;
            _entryButtonPanel.RowCount = 1;
            _entryButtonPanel.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 25F));
            _entryButtonPanel.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 25F));
            _entryButtonPanel.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 25F));
            _entryButtonPanel.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 25F));

            _btnAddEntry = new Button();
            _btnAddEntry.Text = "Add";
            _btnAddEntry.Dock = DockStyle.Fill;

            _btnRemoveEntry = new Button();
            _btnRemoveEntry.Text = "Remove";
            _btnRemoveEntry.Dock = DockStyle.Fill;

            _btnMoveUpEntry = new Button();
            _btnMoveUpEntry.Text = "Up";
            _btnMoveUpEntry.Dock = DockStyle.Fill;

            _btnMoveDownEntry = new Button();
            _btnMoveDownEntry.Text = "Down";
            _btnMoveDownEntry.Dock = DockStyle.Fill;

            _entryButtonPanel.Controls.Add(_btnAddEntry, 0, 0);
            _entryButtonPanel.Controls.Add(_btnRemoveEntry, 1, 0);
            _entryButtonPanel.Controls.Add(_btnMoveUpEntry, 2, 0);
            _entryButtonPanel.Controls.Add(_btnMoveDownEntry, 3, 0);

            _entryListPanel.Controls.Add(_lstEntries);
            _entryListPanel.Controls.Add(_entryButtonPanel);

            _entryEditor = new SpawnEntryDefEditor(_entryLookupProvider);
            _entryEditor.Dock = DockStyle.Fill;

            _entryLayout.Controls.Add(_entryListPanel, 0, 0);
            _entryLayout.Controls.Add(_entryEditor, 1, 0);
        }

        private void WireEvents()
        {
            _cmbId.SelectedIdChanged += OnAnyValueChanged;
            _txtName.TextChanged += OnAnyValueChanged;

            _btnAddEntry.Click += OnAddEntryClicked;
            _btnRemoveEntry.Click += OnRemoveEntryClicked;
            _btnMoveUpEntry.Click += OnMoveUpEntryClicked;
            _btnMoveDownEntry.Click += OnMoveDownEntryClicked;
            _lstEntries.SelectedIndexChanged += OnEntrySelectionChanged;
            _entryEditor.ValueChanged += OnEntryEditorValueChanged;
        }

        private void OnAnyValueChanged(object sender, EventArgs e)
        {
            if (_suppressEvents)
                return;

            MarkDirty();
        }

        private void OnAddEntryClicked(object sender, EventArgs e)
        {
            if (WorkingCopy == null)
                return;

            if (WorkingCopy.Entries == null)
                WorkingCopy.Entries = new List<SpawnEntryDef>();

            WorkingCopy.Entries.Add(new SpawnEntryDef());

            RefreshEntryList();
            _lstEntries.SelectedIndex = WorkingCopy.Entries.Count - 1;
            MarkDirty();
        }

        private void OnRemoveEntryClicked(object sender, EventArgs e)
        {
            if (WorkingCopy == null || WorkingCopy.Entries == null)
                return;

            int index = _lstEntries.SelectedIndex;
            if (index < 0 || index >= WorkingCopy.Entries.Count)
                return;

            WorkingCopy.Entries.RemoveAt(index);
            RefreshEntryList();

            if (WorkingCopy.Entries.Count > 0)
                _lstEntries.SelectedIndex = Math.Min(index, WorkingCopy.Entries.Count - 1);
            else
                _entryEditor.SetValue(null);

            MarkDirty();
        }

        private void OnMoveUpEntryClicked(object sender, EventArgs e)
        {
            if (WorkingCopy == null || WorkingCopy.Entries == null)
                return;

            int index = _lstEntries.SelectedIndex;
            if (index <= 0)
                return;

            SpawnEntryDef temp = WorkingCopy.Entries[index - 1];
            WorkingCopy.Entries[index - 1] = WorkingCopy.Entries[index];
            WorkingCopy.Entries[index] = temp;

            RefreshEntryList();
            _lstEntries.SelectedIndex = index - 1;
            MarkDirty();
        }

        private void OnMoveDownEntryClicked(object sender, EventArgs e)
        {
            if (WorkingCopy == null || WorkingCopy.Entries == null)
                return;

            int index = _lstEntries.SelectedIndex;
            if (index < 0 || index >= WorkingCopy.Entries.Count - 1)
                return;

            SpawnEntryDef temp = WorkingCopy.Entries[index + 1];
            WorkingCopy.Entries[index + 1] = WorkingCopy.Entries[index];
            WorkingCopy.Entries[index] = temp;

            RefreshEntryList();
            _lstEntries.SelectedIndex = index + 1;
            MarkDirty();
        }

        private void OnEntrySelectionChanged(object sender, EventArgs e)
        {
            if (WorkingCopy == null || WorkingCopy.Entries == null)
            {
                _entryEditor.SetValue(null);
                return;
            }

            int index = _lstEntries.SelectedIndex;
            if (index < 0 || index >= WorkingCopy.Entries.Count)
            {
                _entryEditor.SetValue(null);
                return;
            }

            _entryEditor.SetValue(WorkingCopy.Entries[index]);
        }

        private void OnEntryEditorValueChanged(object sender, EventArgs e)
        {
            int index = _lstEntries.SelectedIndex;
            if (index < 0)
                return;

            RefreshEntryList(index);
            MarkDirty();
        }

        protected override void OnBeforeBind(
            DefinitionKind kind,
            SpawnSetDef source,
            SpawnSetDef workingCopy)
        {
            _cmbId.SetItems(_setLookupProvider.GetSpawnSetItems());
            _entryEditor.SetLookupItems();
        }

        protected override SpawnSetDef CloneDefinition(SpawnSetDef source)
        {
            SpawnSetDef clone = new SpawnSetDef();
            clone.Id = source.Id;
            clone.Name = source.Name;
            clone.Entries = new List<SpawnEntryDef>();

            if (source.Entries != null)
            {
                for (int i = 0; i < source.Entries.Count; ++i)
                    clone.Entries.Add(CloneEntry(source.Entries[i]));
            }

            return clone;
        }

        protected override void CopyDefinition(SpawnSetDef from, SpawnSetDef to)
        {
            to.Id = from.Id;
            to.Name = from.Name;

            if (to.Entries == null)
                to.Entries = new List<SpawnEntryDef>();
            else
                to.Entries.Clear();

            if (from.Entries != null)
            {
                for (int i = 0; i < from.Entries.Count; ++i)
                    to.Entries.Add(CloneEntry(from.Entries[i]));
            }
        }

        protected override void LoadToControls(SpawnSetDef value)
        {
            _suppressEvents = true;
            try
            {
                _cmbId.SetSelectedId(value.Id);
                _txtName.Text = value.Name ?? string.Empty;

                RefreshEntryList();

                if (value.Entries != null && value.Entries.Count > 0)
                    _lstEntries.SelectedIndex = 0;
                else
                    _entryEditor.SetValue(null);
            }
            finally
            {
                _suppressEvents = false;
            }
        }

        protected override void SaveFromControls(SpawnSetDef value)
        {
            value.Id = _cmbId.GetSelectedId();
            value.Name = _txtName.Text ?? string.Empty;
        }

        protected override void ClearControls()
        {
            _suppressEvents = true;
            try
            {
                _cmbId.SelectedIndex = -1;
                _txtName.Text = string.Empty;
                _lstEntries.Items.Clear();
                _entryEditor.SetValue(null);
            }
            finally
            {
                _suppressEvents = false;
            }
        }

        private void RefreshEntryList()
        {
            RefreshEntryList(_lstEntries.SelectedIndex);
        }

        private void RefreshEntryList(int preferredIndex)
        {
            _lstEntries.BeginUpdate();
            try
            {
                _lstEntries.Items.Clear();

                if (WorkingCopy == null || WorkingCopy.Entries == null)
                {
                    _entryEditor.SetValue(null);
                    return;
                }

                for (int i = 0; i < WorkingCopy.Entries.Count; ++i)
                    _lstEntries.Items.Add(BuildEntryDisplayText(i, WorkingCopy.Entries[i]));

                if (WorkingCopy.Entries.Count == 0)
                {
                    _entryEditor.SetValue(null);
                    return;
                }

                if (preferredIndex < 0)
                    preferredIndex = 0;
                if (preferredIndex >= WorkingCopy.Entries.Count)
                    preferredIndex = WorkingCopy.Entries.Count - 1;

                _lstEntries.SelectedIndex = preferredIndex;
            }
            finally
            {
                _lstEntries.EndUpdate();
            }
        }

        private static string BuildEntryDisplayText(int index, SpawnEntryDef entry)
        {
            if (entry == null)
                return index.ToString() + " : <null>";

            return string.Format(
                "#{0} {1} ({2:0.###}, {3:0.###}, {4:0.###}) Player={5}",
                index,
                entry.CharacterId,
                entry.X,
                entry.Y,
                entry.Z,
                entry.IsPlayerSpawn);
        }

        private static SpawnEntryDef CloneEntry(SpawnEntryDef source)
        {
            SpawnEntryDef clone = new SpawnEntryDef();

            if (source == null)
                return clone;

            clone.CharacterId = source.CharacterId;
            clone.X = source.X;
            clone.Y = source.Y;
            clone.Z = source.Z;
            clone.YawRad = source.YawRad;
            clone.IsPlayerSpawn = source.IsPlayerSpawn;
            return clone;
        }
    }
}