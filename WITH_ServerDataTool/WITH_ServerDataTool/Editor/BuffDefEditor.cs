using System;
using System.Collections.Generic;
using System.Windows.Forms;
using WITH_ServerDataTool.Editor;
using WITH_ServerDataTool.ID;

namespace WITH_ServerDataTool.Definition
{
    public sealed class BuffDefEditor : DefinitionEditorBase<BuffDef>
    {
        private readonly IBuffEditorLookupProvider _lookupProvider;

        private EditorLayoutPanel _layout;

        private DefinitionReferenceComboBox<BuffId> _cmbId;
        private TextBox _txtName;
        private ComboBox _cmbPolicy;

        private TableLayoutPanel _modifierLayout;
        private Panel _modifierListPanel;
        private ListBox _lstModifiers;
        private TableLayoutPanel _modifierButtonPanel;
        private Button _btnAddModifier;
        private Button _btnRemoveModifier;
        private Button _btnMoveUpModifier;
        private Button _btnMoveDownModifier;
        private BuffModifierDefEditor _modifierEditor;

        private bool _suppressEvents;

        public BuffDefEditor(IBuffEditorLookupProvider lookupProvider)
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

            _cmbId = new DefinitionReferenceComboBox<BuffId>();
            _cmbId.Dock = DockStyle.Fill;

            _txtName = new TextBox();
            _txtName.Dock = DockStyle.Fill;

            _cmbPolicy = new ComboBox();
            _cmbPolicy.Dock = DockStyle.Fill;
            _cmbPolicy.DropDownStyle = ComboBoxStyle.DropDownList;
            AddEnumItems(_cmbPolicy, typeof(BuffPolicyId));

            BuildModifierUi();

            _layout.AddRow("Id", _cmbId);
            _layout.AddRow("Name", _txtName);
            _layout.AddRow("Policy", _cmbPolicy);
            _layout.AddRow("Modifiers", _modifierLayout);

            Controls.Clear();
            Controls.Add(_layout);
        }

        private void BuildModifierUi()
        {
            _modifierLayout = new TableLayoutPanel();
            _modifierLayout.Dock = DockStyle.Fill;
            _modifierLayout.Margin = new Padding(0);
            _modifierLayout.Padding = new Padding(0);
            _modifierLayout.ColumnCount = 2;
            _modifierLayout.RowCount = 1;
            _modifierLayout.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 220F));
            _modifierLayout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100F));
            _modifierLayout.RowStyles.Add(new RowStyle(SizeType.Percent, 100F));

            _modifierListPanel = new Panel();
            _modifierListPanel.Dock = DockStyle.Fill;
            _modifierListPanel.Margin = new Padding(0, 0, 6, 0);

            _lstModifiers = new ListBox();
            _lstModifiers.Dock = DockStyle.Fill;

            _modifierButtonPanel = new TableLayoutPanel();
            _modifierButtonPanel.Dock = DockStyle.Bottom;
            _modifierButtonPanel.Height = 34;
            _modifierButtonPanel.ColumnCount = 4;
            _modifierButtonPanel.RowCount = 1;
            _modifierButtonPanel.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 25F));
            _modifierButtonPanel.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 25F));
            _modifierButtonPanel.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 25F));
            _modifierButtonPanel.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 25F));

            _btnAddModifier = new Button();
            _btnAddModifier.Text = "Add";
            _btnAddModifier.Dock = DockStyle.Fill;

            _btnRemoveModifier = new Button();
            _btnRemoveModifier.Text = "Remove";
            _btnRemoveModifier.Dock = DockStyle.Fill;

            _btnMoveUpModifier = new Button();
            _btnMoveUpModifier.Text = "Up";
            _btnMoveUpModifier.Dock = DockStyle.Fill;

            _btnMoveDownModifier = new Button();
            _btnMoveDownModifier.Text = "Down";
            _btnMoveDownModifier.Dock = DockStyle.Fill;

            _modifierButtonPanel.Controls.Add(_btnAddModifier, 0, 0);
            _modifierButtonPanel.Controls.Add(_btnRemoveModifier, 1, 0);
            _modifierButtonPanel.Controls.Add(_btnMoveUpModifier, 2, 0);
            _modifierButtonPanel.Controls.Add(_btnMoveDownModifier, 3, 0);

            _modifierListPanel.Controls.Add(_lstModifiers);
            _modifierListPanel.Controls.Add(_modifierButtonPanel);

            _modifierEditor = new BuffModifierDefEditor();
            _modifierEditor.Dock = DockStyle.Fill;

            _modifierLayout.Controls.Add(_modifierListPanel, 0, 0);
            _modifierLayout.Controls.Add(_modifierEditor, 1, 0);
        }

        private void WireEvents()
        {
            _cmbId.SelectedIdChanged += OnAnyValueChanged;
            _txtName.TextChanged += OnAnyValueChanged;
            _cmbPolicy.SelectedIndexChanged += OnAnyValueChanged;

            _btnAddModifier.Click += OnAddModifierClicked;
            _btnRemoveModifier.Click += OnRemoveModifierClicked;
            _btnMoveUpModifier.Click += OnMoveUpModifierClicked;
            _btnMoveDownModifier.Click += OnMoveDownModifierClicked;
            _lstModifiers.SelectedIndexChanged += OnModifierSelectionChanged;
            _modifierEditor.ValueChanged += OnModifierEditorValueChanged;
        }

        protected override void OnBeforeBind(
            DefinitionKind kind,
            BuffDef source, 
            BuffDef workingCopy)
        {
            _cmbId.SetItems(_lookupProvider.GetBuffItems());
        }

        private void OnAnyValueChanged(object sender, EventArgs e)
        {
            if (_suppressEvents)
                return;

            MarkDirty();
        }

        private void OnAddModifierClicked(object sender, EventArgs e)
        {
            if (WorkingCopy == null)
                return;

            if (WorkingCopy.Modifiers == null)
                WorkingCopy.Modifiers = new List<BuffModifierDef>();

            WorkingCopy.Modifiers.Add(new BuffModifierDef());

            RefreshModifierList();
            _lstModifiers.SelectedIndex = WorkingCopy.Modifiers.Count - 1;
            MarkDirty();
        }

        private void OnRemoveModifierClicked(object sender, EventArgs e)
        {
            if (WorkingCopy == null || WorkingCopy.Modifiers == null)
                return;

            int index = _lstModifiers.SelectedIndex;
            if (index < 0 || index >= WorkingCopy.Modifiers.Count)
                return;

            WorkingCopy.Modifiers.RemoveAt(index);
            RefreshModifierList();

            if (WorkingCopy.Modifiers.Count > 0)
                _lstModifiers.SelectedIndex = Math.Min(index, WorkingCopy.Modifiers.Count - 1);
            else
                _modifierEditor.SetValue(null);

            MarkDirty();
        }

        private void OnMoveUpModifierClicked(object sender, EventArgs e)
        {
            if (WorkingCopy == null || WorkingCopy.Modifiers == null)
                return;

            int index = _lstModifiers.SelectedIndex;
            if (index <= 0)
                return;

            BuffModifierDef temp = WorkingCopy.Modifiers[index - 1];
            WorkingCopy.Modifiers[index - 1] = WorkingCopy.Modifiers[index];
            WorkingCopy.Modifiers[index] = temp;

            RefreshModifierList();
            _lstModifiers.SelectedIndex = index - 1;
            MarkDirty();
        }

        private void OnMoveDownModifierClicked(object sender, EventArgs e)
        {
            if (WorkingCopy == null || WorkingCopy.Modifiers == null)
                return;

            int index = _lstModifiers.SelectedIndex;
            if (index < 0 || index >= WorkingCopy.Modifiers.Count - 1)
                return;

            BuffModifierDef temp = WorkingCopy.Modifiers[index + 1];
            WorkingCopy.Modifiers[index + 1] = WorkingCopy.Modifiers[index];
            WorkingCopy.Modifiers[index] = temp;

            RefreshModifierList();
            _lstModifiers.SelectedIndex = index + 1;
            MarkDirty();
        }

        private void OnModifierSelectionChanged(object sender, EventArgs e)
        {
            if (WorkingCopy == null || WorkingCopy.Modifiers == null)
            {
                _modifierEditor.SetValue(null);
                return;
            }

            int index = _lstModifiers.SelectedIndex;
            if (index < 0 || index >= WorkingCopy.Modifiers.Count)
            {
                _modifierEditor.SetValue(null);
                return;
            }

            _modifierEditor.SetValue(WorkingCopy.Modifiers[index]);
        }

        private void OnModifierEditorValueChanged(object sender, EventArgs e)
        {
            int index = _lstModifiers.SelectedIndex;
            if (index < 0)
                return;

            RefreshModifierList(index);
            MarkDirty();
        }

        protected override BuffDef CloneDefinition(BuffDef source)
        {
            BuffDef clone = new BuffDef();
            clone.Id = source.Id;
            clone.Name = source.Name;
            clone.Policy = source.Policy;
            clone.Modifiers = new List<BuffModifierDef>();

            if (source.Modifiers != null)
            {
                for (int i = 0; i < source.Modifiers.Count; ++i)
                    clone.Modifiers.Add(CloneModifier(source.Modifiers[i]));
            }

            return clone;
        }

        protected override void CopyDefinition(BuffDef from, BuffDef to)
        {
            to.Id = from.Id;
            to.Name = from.Name;
            to.Policy = from.Policy;

            if (to.Modifiers == null)
                to.Modifiers = new List<BuffModifierDef>();
            else
                to.Modifiers.Clear();

            if (from.Modifiers != null)
            {
                for (int i = 0; i < from.Modifiers.Count; ++i)
                    to.Modifiers.Add(CloneModifier(from.Modifiers[i]));
            }
        }

        protected override void LoadToControls(BuffDef value)
        {
            _suppressEvents = true;
            try
            {
                _cmbId.SetSelectedId(value.Id);
                _txtName.Text = value.Name ?? string.Empty;
                SetComboSelectedEnum(_cmbPolicy, value.Policy);

                RefreshModifierList();

                if (value.Modifiers != null && value.Modifiers.Count > 0)
                    _lstModifiers.SelectedIndex = 0;
                else
                    _modifierEditor.SetValue(null);
            }
            finally
            {
                _suppressEvents = false;
            }
        }

        protected override void SaveFromControls(BuffDef value)
        {
            value.Id = _cmbId.GetSelectedId();
            value.Name = _txtName.Text ?? string.Empty;
            value.Policy = GetSelectedEnumValue<BuffPolicyId>(_cmbPolicy);
        }

        protected override void ClearControls()
        {
            _suppressEvents = true;
            try
            {
                _cmbId.SelectedIndex = -1;
                _txtName.Text = string.Empty;
                SetComboSelectedEnum(_cmbPolicy, BuffPolicyId.None);
                _lstModifiers.Items.Clear();
                _modifierEditor.SetValue(null);
            }
            finally
            {
                _suppressEvents = false;
            }
        }

        private void RefreshModifierList()
        {
            RefreshModifierList(_lstModifiers.SelectedIndex);
        }

        private void RefreshModifierList(int preferredIndex)
        {
            _lstModifiers.BeginUpdate();
            try
            {
                _lstModifiers.Items.Clear();

                if (WorkingCopy == null || WorkingCopy.Modifiers == null)
                {
                    _modifierEditor.SetValue(null);
                    return;
                }

                for (int i = 0; i < WorkingCopy.Modifiers.Count; ++i)
                    _lstModifiers.Items.Add(BuildModifierDisplayText(i, WorkingCopy.Modifiers[i]));

                if (WorkingCopy.Modifiers.Count == 0)
                {
                    _modifierEditor.SetValue(null);
                    return;
                }

                if (preferredIndex < 0)
                    preferredIndex = 0;
                if (preferredIndex >= WorkingCopy.Modifiers.Count)
                    preferredIndex = WorkingCopy.Modifiers.Count - 1;

                _lstModifiers.SelectedIndex = preferredIndex;
            }
            finally
            {
                _lstModifiers.EndUpdate();
            }
        }

        private static string BuildModifierDisplayText(int index, BuffModifierDef modifier)
        {
            if (modifier == null)
                return index.ToString() + " : <null>";

            return string.Format(
                "#{0} {1} {2} {3:0.###}",
                index,
                modifier.Effect,
                modifier.StatId,
                modifier.Value);
        }

        private static BuffModifierDef CloneModifier(BuffModifierDef source)
        {
            BuffModifierDef clone = new BuffModifierDef();

            if (source == null)
                return clone;

            clone.Effect = source.Effect;
            clone.StatId = source.StatId;
            clone.Value = source.Value;
            return clone;
        }

        private static void AddEnumItems(ComboBox combo, Type enumType)
        {
            Array values = Enum.GetValues(enumType);
            for (int i = 0; i < values.Length; ++i)
                combo.Items.Add(values.GetValue(i));

            if (combo.Items.Count > 0)
                combo.SelectedIndex = 0;
        }

        private static void SetComboSelectedEnum<TEnum>(ComboBox combo, TEnum value)
        {
            for (int i = 0; i < combo.Items.Count; ++i)
            {
                object item = combo.Items[i];
                if (item != null && item.Equals(value))
                {
                    combo.SelectedIndex = i;
                    return;
                }
            }

            if (combo.Items.Count > 0)
                combo.SelectedIndex = 0;
        }

        private static TEnum GetSelectedEnumValue<TEnum>(ComboBox combo)
        {
            if (combo.SelectedItem == null)
                return default(TEnum);

            return (TEnum)combo.SelectedItem;
        }
    }
}