using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Windows.Forms;

using WITH_ServerDataTool.Definition;
using WITH_ServerDataTool.Utility;

namespace WITH_ServerDataTool.Editor
{
    public sealed class MainForm : Form
    {
        private readonly EditorController _controller;
        private readonly DefinitionEditorFactory _editorFactory;

        private ToolStrip _toolStrip;
        private ToolStripButton _btnNew;
        private ToolStripButton _btnOpen;
        private ToolStripButton _btnSave;
        private ToolStripButton _btnSaveAs;
        private ToolStripButton _btnValidate;
        private ToolStripButton _btnAdd;
        private ToolStripButton _btnRemove;

        private SplitContainer _splitMain;
        private SplitContainer _splitHorizontal;
        private SplitContainer _splitLeftRight;

        private ListBox _lstKinds;
        private ListBox _lstItems;
        private ListView _lvValidation;

        private DefinitionKind _currentKind;
        private bool _hasCurrentKind;

        private DefinitionEditorHost _editorHost;

        private bool _suppressItemSelectionChanged;
        private int _lastSelectedItemIndex = -1;

        public MainForm(EditorController controller)
        {
            if (controller == null)
                throw new ArgumentNullException("controller");

            _controller = controller;

            CharacterEditorLookupProvider characterLookupProvider =
                new CharacterEditorLookupProvider(controller);

            ActionEditorLookupProvider actionLookupProvider =
                new ActionEditorLookupProvider(controller);

            AnimationResourceEditorLookupProvider animationLookupProvider =
                new AnimationResourceEditorLookupProvider(controller);

            BuffEditorLookupProvider buffLookupProvider =
                new BuffEditorLookupProvider(controller);

            WorldEditorLookupProvider worldEditorLookupProvider =
                new WorldEditorLookupProvider(controller);

            SpawnSetEditorLookupProvider spawnSetLookupProvider =
                new SpawnSetEditorLookupProvider(controller);

            SpawnEntryEditorLookupProvider spawnEntryLookupProvider =
                new SpawnEntryEditorLookupProvider(controller);

            _editorFactory = 
                new DefinitionEditorFactory(
                    characterLookupProvider,
                    actionLookupProvider,
                    animationLookupProvider,
                    buffLookupProvider,
                    worldEditorLookupProvider,
                    spawnSetLookupProvider,
                    spawnEntryLookupProvider);

            InitializeComponent();
            InitializeLayout();
            InitializeEvents();

            Shown += OnMainFormShown;

            LoadKinds();
            SelectFirstKind();
            RefreshTitle();
        }

        private void OnMainFormShown(object sender, EventArgs e)
        {
            ApplyInitialSplitterDistances();
        }

        private void ApplyInitialSplitterDistances()
        {
            _splitHorizontal.FixedPanel = FixedPanel.Panel1;
            _splitLeftRight.FixedPanel = FixedPanel.Panel1;

            _splitHorizontal.Panel1MinSize = 90;
            _splitHorizontal.Panel2MinSize = 300;

            _splitLeftRight.Panel1MinSize = 160;
            _splitLeftRight.Panel2MinSize = 300;

            if (_splitMain.Height > 250)
                _splitMain.SplitterDistance = Math.Min(620, _splitMain.Height - 180);

            if (_splitHorizontal.Width > 450)
            {
                int maxHorizontal = _splitHorizontal.Width - _splitHorizontal.Panel2MinSize;
                _splitHorizontal.SplitterDistance = Math.Max(
                    _splitHorizontal.Panel1MinSize,
                    Math.Min(120, maxHorizontal));
            }

            if (_splitLeftRight.Width > 500)
            {
                int maxLeftRight = _splitLeftRight.Width - _splitLeftRight.Panel2MinSize;
                _splitLeftRight.SplitterDistance = Math.Max(
                    _splitLeftRight.Panel1MinSize,
                    Math.Min(260, maxLeftRight));
            }
        }

        protected override void OnFormClosing(FormClosingEventArgs e)
        {
            if (ConfirmDiscardChanges() == false)
            {
                e.Cancel = true;
                return;
            }

            base.OnFormClosing(e);
        }

        private void InitializeComponent()
        {
            _toolStrip = new ToolStrip();
            _btnNew = new ToolStripButton();
            _btnOpen = new ToolStripButton();
            _btnSave = new ToolStripButton();
            _btnSaveAs = new ToolStripButton();
            _btnValidate = new ToolStripButton();
            _btnAdd = new ToolStripButton();
            _btnRemove = new ToolStripButton();

            _splitMain = new SplitContainer();
            _splitHorizontal = new SplitContainer();
            _splitLeftRight = new SplitContainer();

            _lstKinds = new ListBox();
            _lstItems = new ListBox();
            _lvValidation = new ListView();
            _editorHost = new DefinitionEditorHost();

            SuspendLayout();

            _btnNew.Text = "New";
            _btnOpen.Text = "Open";
            _btnSave.Text = "Save";
            _btnSaveAs.Text = "Save As";
            _btnValidate.Text = "Validate";
            _btnAdd.Text = "Add";
            _btnRemove.Text = "Remove";

            ClientSize = new System.Drawing.Size(1400, 900);
            MinimumSize = new System.Drawing.Size(1100, 700);
            Name = "MainForm";
            Text = "WITH Server Data Tool";

            ResumeLayout(false);
            PerformLayout();
        }

        private void InitializeLayout()
        {
            _toolStrip.GripStyle = ToolStripGripStyle.Hidden;
            _toolStrip.Items.Add(_btnNew);
            _toolStrip.Items.Add(_btnOpen);
            _toolStrip.Items.Add(_btnSave);
            _toolStrip.Items.Add(_btnSaveAs);
            _toolStrip.Items.Add(new ToolStripSeparator());
            _toolStrip.Items.Add(_btnValidate);
            _toolStrip.Items.Add(new ToolStripSeparator());
            _toolStrip.Items.Add(_btnAdd);
            _toolStrip.Items.Add(_btnRemove);

            _splitMain.Dock = DockStyle.Fill;
            _splitMain.Orientation = Orientation.Horizontal;

            _splitHorizontal.Dock = DockStyle.Fill;
            _splitHorizontal.Orientation = Orientation.Vertical;

            _splitLeftRight.Dock = DockStyle.Fill;
            _splitLeftRight.Orientation = Orientation.Vertical;

            _lstKinds.Dock = DockStyle.Fill;
            _lstItems.Dock = DockStyle.Fill;
            _editorHost.Dock = DockStyle.Fill;

            _lvValidation.Dock = DockStyle.Fill;
            _lvValidation.View = View.Details;
            _lvValidation.FullRowSelect = true;
            _lvValidation.GridLines = true;
            _lvValidation.HideSelection = false;
            _lvValidation.Columns.Add("Severity", 100);
            _lvValidation.Columns.Add("Path", 320);
            _lvValidation.Columns.Add("Message", 800);

            _splitHorizontal.Panel1.Controls.Add(_lstKinds);
            _splitHorizontal.Panel2.Controls.Add(_splitLeftRight);

            _splitLeftRight.Panel1.Controls.Add(_lstItems);
            _splitLeftRight.Panel2.Controls.Add(_editorHost);

            _splitMain.Panel1.Controls.Add(_splitHorizontal);
            _splitMain.Panel2.Controls.Add(_lvValidation);

            Controls.Add(_splitMain);
            Controls.Add(_toolStrip);
        }

        private void InitializeEvents()
        {
            _btnNew.Click += OnNewClicked;
            _btnOpen.Click += OnOpenClicked;
            _btnSave.Click += OnSaveClicked;
            _btnSaveAs.Click += OnSaveAsClicked;
            _btnValidate.Click += OnValidateClicked;
            _btnAdd.Click += OnAddClicked;
            _btnRemove.Click += OnRemoveClicked;

            _lstKinds.SelectedIndexChanged += OnKindSelectionChanged;
            _lstItems.SelectedIndexChanged += OnItemSelectionChanged;
            _lvValidation.DoubleClick += OnValidationDoubleClick;

            _editorHost.CurrentEditorValueChanged += OnCurrentEditorValueChanged;
        }

        private void LoadKinds()
        {
            _lstKinds.Items.Clear();

            Array values = Enum.GetValues(typeof(DefinitionKind));
            for (int i = 0; i < values.Length; ++i)
            {
                DefinitionKind kind = (DefinitionKind)values.GetValue(i);
                _lstKinds.Items.Add(new DefinitionKindItem(kind));
            }
        }

        private void SelectFirstKind()
        {
            if (_lstKinds.Items.Count > 0)
                _lstKinds.SelectedIndex = 0;
        }

        private void RefreshTitle()
        {
            string fileName = string.IsNullOrEmpty(_controller.CurrentFilePath)
                ? "(new file)"
                : Path.GetFileName(_controller.CurrentFilePath);

            if (_controller.IsDirty)
                fileName += " *";

            Text = "WITH Server Data Tool - " + fileName;
        }

        private void RefreshItemList()
        {
            int selectedIndex = _lstItems.SelectedIndex;

            _lstItems.BeginUpdate();
            try
            {
                _lstItems.Items.Clear();
                ClearEditor();

                if (_hasCurrentKind == false)
                    return;

                IList items = _controller.GetItems(_currentKind);
                for (int i = 0; i < items.Count; ++i)
                {
                    object item = items[i];
                    _lstItems.Items.Add(new DefinitionListItem(_currentKind, i, item));
                }

                if (_lstItems.Items.Count == 0)
                    return;

                if (selectedIndex >= 0 && selectedIndex < _lstItems.Items.Count)
                    _lstItems.SelectedIndex = selectedIndex;
                else
                    _lstItems.SelectedIndex = 0;
            }
            finally
            {
                _lstItems.EndUpdate();
            }
        }

        private void RefreshValidationList(ValidationResult result)
        {
            _lvValidation.BeginUpdate();
            try
            {
                _lvValidation.Items.Clear();

                if (result == null)
                    return;

                IList<ValidationMessage> messages = result.Messages;
                for (int i = 0; i < messages.Count; ++i)
                {
                    ValidationMessage msg = messages[i];
                    ListViewItem item = new ListViewItem(GetSeverityText(msg.Severity));
                    item.SubItems.Add(msg.Path ?? string.Empty);
                    item.SubItems.Add(msg.Message ?? string.Empty);
                    item.Tag = msg;
                    _lvValidation.Items.Add(item);
                }
            }
            finally
            {
                _lvValidation.EndUpdate();
            }
        }

        private string GetSeverityText(ValidationSeverity severity)
        {
            switch (severity)
            {
                case ValidationSeverity.Error:
                    return "Error";
                case ValidationSeverity.Warning:
                    return "Warning";
                default:
                    return severity.ToString();
            }
        }

        private void SelectValidationTarget(string path)
        {
            if (string.IsNullOrEmpty(path))
                return;

            DefinitionKind kind;
            int index;
            if (ValidationPathNavigator.TryParseRoot(path, out kind, out index) == false)
                return;

            for (int i = 0; i < _lstKinds.Items.Count; ++i)
            {
                DefinitionKindItem kindItem = _lstKinds.Items[i] as DefinitionKindItem;
                if (kindItem == null)
                    continue;

                if (kindItem.Kind == kind)
                {
                    _lstKinds.SelectedIndex = i;
                    break;
                }
            }

            if (index >= 0 && index < _lstItems.Items.Count)
                _lstItems.SelectedIndex = index;
        }

        private void OnNewClicked(object sender, EventArgs e)
        {
            if (ConfirmDiscardChanges() == false)
                return;

            _controller.NewData();
            RefreshItemList();
            RefreshValidationList(_controller.LastValidation);
            RefreshTitle();
        }

        private void OnOpenClicked(object sender, EventArgs e)
        {
            if (ConfirmDiscardChanges() == false)
                return;

            using (OpenFileDialog dlg = new OpenFileDialog())
            {
                dlg.Filter = "JSON Files (*.json)|*.json|All Files (*.*)|*.*";
                dlg.Title = "Open Game Data JSON";

                if (dlg.ShowDialog(this) != DialogResult.OK)
                    return;

                try
                {
                    _controller.LoadFromFile(dlg.FileName);
                    RefreshItemList();
                    RefreshValidationList(_controller.LastValidation);
                    RefreshTitle();
                }
                catch (Exception ex)
                {
                    MessageBox.Show(
                        this,
                        ex.Message,
                        "Open Failed",
                        MessageBoxButtons.OK,
                        MessageBoxIcon.Error);
                }
            }
        }

        private bool ValidateBeforeSave()
        {
            if (_editorHost.CurrentEditor != null)
            {
                _editorHost.CurrentEditor.Commit();
                RefreshCurrentItemText();
            }

            ValidationResult result = _controller.Validate();
            RefreshValidationList(result);

            if (result.IsValid)
                return true;

            DialogResult dr = MessageBox.Show(
                this,
                "검증 오류가 있습니다. 그래도 저장하시겠습니까?",
                "Validation Failed",
                MessageBoxButtons.YesNo,
                MessageBoxIcon.Warning);

            return dr == DialogResult.Yes;
        }

        private void OnSaveClicked(object sender, EventArgs e)
        {
            try
            {
                if (ValidateBeforeSave() == false)
                    return;

                if (string.IsNullOrEmpty(_controller.CurrentFilePath))
                {
                    SaveAsAndReturnSuccess();
                    return;
                }

                _controller.Save();
                RefreshTitle();
            }
            catch (Exception ex)
            {
                MessageBox.Show(
                    this,
                    ex.Message,
                    "Save Failed",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
            }
        }

        private void OnSaveAsClicked(object sender, EventArgs e)
        {
            SaveAsAndReturnSuccess();
        }

        private bool SaveAsAndReturnSuccess()
        {
            try
            {
                if (ValidateBeforeSave() == false)
                    return false;

                using (SaveFileDialog dlg = new SaveFileDialog())
                {
                    dlg.Filter = "JSON Files (*.json)|*.json|All Files (*.*)|*.*";
                    dlg.Title = "Save Game Data As";

                    if (string.IsNullOrEmpty(_controller.CurrentFilePath) == false)
                        dlg.FileName = Path.GetFileName(_controller.CurrentFilePath);

                    if (dlg.ShowDialog(this) != DialogResult.OK)
                        return false;

                    _controller.SaveToFile(dlg.FileName);
                    RefreshTitle();
                    return true;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(
                    this,
                    ex.Message,
                    "Save As Failed",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                return false;
            }
        }

        private void OnValidateClicked(object sender, EventArgs e)
        {
            try
            {
                if (_editorHost.CurrentEditor != null)
                {
                    _editorHost.CurrentEditor.Commit();
                    RefreshCurrentItemText();
                }

                ValidationResult result = _controller.Validate();
                RefreshValidationList(result);

                string text = result.ToDisplayString();
                MessageBox.Show(
                    this,
                    text,
                    "Validation Result",
                    MessageBoxButtons.OK,
                    result.IsValid ? MessageBoxIcon.Information : MessageBoxIcon.Warning);
            }
            catch (Exception ex)
            {
                MessageBox.Show(
                    this,
                    ex.Message,
                    "Validate Failed",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
            }
        }

        private void OnAddClicked(object sender, EventArgs e)
        {
            if (_hasCurrentKind == false)
                return;

            try
            {
                object newItem = _controller.CreateNewItem(_currentKind);
                _controller.AddItem(_currentKind, newItem);

                RefreshItemList();

                for (int i = 0; i < _lstItems.Items.Count; ++i)
                {
                    DefinitionListItem item = _lstItems.Items[i] as DefinitionListItem;
                    if (item != null && object.ReferenceEquals(item.Value, newItem))
                    {
                        _lstItems.SelectedIndex = i;
                        break;
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(
                    this,
                    ex.Message,
                    "Add Failed",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
            }
        }

        private void OnRemoveClicked(object sender, EventArgs e)
        {
            if (_hasCurrentKind == false)
                return;

            DefinitionListItem selected = _lstItems.SelectedItem as DefinitionListItem;
            if (selected == null)
                return;

            DialogResult dr = MessageBox.Show(
                this,
                "선택한 항목을 삭제하시겠습니까?",
                "Remove Item",
                MessageBoxButtons.YesNo,
                MessageBoxIcon.Warning);

            if (dr != DialogResult.Yes)
                return;

            try
            {
                _controller.RemoveItem(_currentKind, selected.Value);
                RefreshItemList();
            }
            catch (Exception ex)
            {
                MessageBox.Show(
                    this,
                    ex.Message,
                    "Remove Failed",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
            }
        }

        private void OnKindSelectionChanged(object sender, EventArgs e)
        {
            DefinitionKindItem item = _lstKinds.SelectedItem as DefinitionKindItem;
            if (item == null)
            {
                _hasCurrentKind = false;
                ClearEditor();
                _lstItems.Items.Clear();
                return;
            }

            _currentKind = item.Kind;
            _hasCurrentKind = true;
            RefreshItemList();
        }

        private void OnItemSelectionChanged(object sender, EventArgs e)
        {
            if (_suppressItemSelectionChanged) return;

            try
            {
                if (_editorHost.CurrentEditor != null && _lastSelectedItemIndex >= 0)
                {
                    _editorHost.CurrentEditor.Commit();
                    RefreshCurrentItemText();
                }
            }
            catch (Exception ex)
            {
                _suppressItemSelectionChanged = true;
                try
                {
                    _lstItems.SelectedIndex = _lastSelectedItemIndex;
                }
                finally
                {
                    _suppressItemSelectionChanged = false;
                }

                MessageBox.Show(
                    this,
                    ex.Message,
                    "Commit Failed",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);

                return;
            }

            DefinitionListItem item = _lstItems.SelectedItem as DefinitionListItem;
            if (item == null)
            {
                ClearEditor();
                return;
            }

            ShowEditor(item.Kind, item.Value);
        }

        private void ShowEditor(DefinitionKind kind, object value)
        {
            IDefinitionEditor editor = _editorFactory.CreateEditor(kind);
            editor.Bind(kind, value);
            _editorHost.SetEditor(editor);
        }

        private void ClearEditor()
        {
            _editorHost.ClearEditor();
        }

        private void OnCurrentEditorValueChanged(object sender, EventArgs e)
        {
            _controller.MarkDirty();
            RefreshTitle();
            //RefreshCurrentItemText();
        }

        private void RefreshCurrentItemText()
        {
            int selectedIndex = _lstItems.SelectedIndex;
            if (selectedIndex < 0)
                return;

            DefinitionListItem oldItem = _lstItems.Items[selectedIndex] as DefinitionListItem;
            if (oldItem == null)
                return;

            _suppressItemSelectionChanged = true;
            _lstItems.BeginUpdate();
            try
            {
                DefinitionListItem newItem =
                    new DefinitionListItem(oldItem.Kind, oldItem.Index, oldItem.Value);

                _lstItems.Items[selectedIndex] = newItem;
                _lstItems.SelectedIndex = selectedIndex;
            }
            finally
            {
                _lstItems.EndUpdate();
                _suppressItemSelectionChanged = false;
            }
        }

        private void OnValidationDoubleClick(object sender, EventArgs e)
        {
            if (_lvValidation.SelectedItems.Count <= 0)
                return;

            ListViewItem selected = _lvValidation.SelectedItems[0];
            ValidationMessage msg = selected.Tag as ValidationMessage;
            if (msg == null)
                return;

            SelectValidationTarget(msg.Path);
        }

        private bool ConfirmDiscardChanges()
        {
            if (_controller.IsDirty == false)
                return true;

            DialogResult dr = MessageBox.Show(
                this,
                "저장되지 않은 변경 사항이 있습니다.\n저장하시겠습니까?",
                "Unsaved Changes",
                MessageBoxButtons.YesNoCancel,
                MessageBoxIcon.Warning);

            if (dr == DialogResult.Cancel)
                return false;

            if (dr == DialogResult.No)
                return true;

            try
            {
                if (string.IsNullOrEmpty(_controller.CurrentFilePath))
                {
                    return SaveAsAndReturnSuccess();
                }

                if (ValidateBeforeSave() == false)
                    return false;

                _controller.Save();
                RefreshTitle();
                return true;
            }
            catch (Exception ex)
            {
                MessageBox.Show(
                    this,
                    ex.Message,
                    "Save Failed",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                return false;
            }
        }

        internal sealed class DefinitionKindItem
        {
            public DefinitionKind Kind { get; private set; }

            public DefinitionKindItem(DefinitionKind kind)
            {
                Kind = kind;
            }

            public override string ToString()
            {
                return Kind.ToString();
            }
        }

        internal sealed class DefinitionListItem
        {
            public DefinitionKind Kind { get; private set; }
            public int Index { get; private set; }
            public object Value { get; private set; }

            public DefinitionListItem(DefinitionKind kind, int index, object value)
            {
                Kind = kind;
                Index = index;
                Value = value;
            }

            public override string ToString()
            {
                return EditorDisplayText.GetItemText(Kind, Value);
            }
        }

        internal static class EditorDisplayText
        {
            public static string GetItemText(DefinitionKind kind, object item)
            {
                if (item == null)
                    return "(null)";

                switch (kind)
                {
                    case DefinitionKind.Characters:
                        {
                            CharacterDef def = item as CharacterDef;
                            if (def == null) return "(invalid CharacterDef)";
                            return SafeText(def.Id) + " - " + SafeText(def.Name);
                        }

                    case DefinitionKind.Actions:
                        {
                            ActionDef def = item as ActionDef;
                            if (def == null) return "(invalid ActionDef)";
                            return SafeText(def.Id) + " - " + SafeText(def.Name);
                        }

                    case DefinitionKind.Animations:
                        {
                            AnimationResourceDef def = item as AnimationResourceDef;
                            if (def == null) return "(invalid AnimationResourceDef)";
                            return SafeText(def.Id) + " - " + SafeText(def.Name);
                        }

                    case DefinitionKind.Buffs:
                        {
                            BuffDef def = item as BuffDef;
                            if (def == null) return "(invalid BuffDef)";
                            return SafeText(def.Id) + " - " + SafeText(def.Name);
                        }

                    case DefinitionKind.Worlds:
                        {
                            WorldDef def = item as WorldDef;
                            if (def == null) return "(invalid WorldDef)";
                            return SafeText(def.Id) + " - " + SafeText(def.Name);
                        }

                    case DefinitionKind.SpawnSets:
                        {
                            SpawnSetDef def = item as SpawnSetDef;
                            if (def == null) return "(invalid SpawnSetDef)";
                            return SafeText(def.Id) + " - " + SafeText(def.Name);
                        }

                    default:
                        return item.ToString();
                }
            }

            private static string SafeText(object value)
            {
                return value == null ? string.Empty : value.ToString();
            }
        }

        internal static class ValidationPathNavigator
        {
            public static bool TryParseRoot(string path, out DefinitionKind kind, out int index)
            {
                kind = default(DefinitionKind);
                index = -1;

                if (string.IsNullOrEmpty(path))
                    return false;

                if (TryParse(path, "Characters", DefinitionKind.Characters, out kind, out index))
                    return true;

                if (TryParse(path, "Actions", DefinitionKind.Actions, out kind, out index))
                    return true;

                if (TryParse(path, "Animations", DefinitionKind.Animations, out kind, out index))
                    return true;

                if (TryParse(path, "Buffs", DefinitionKind.Buffs, out kind, out index))
                    return true;

                if (TryParse(path, "Worlds", DefinitionKind.Worlds, out kind, out index))
                    return true;

                if (TryParse(path, "SpawnSets", DefinitionKind.SpawnSets, out kind, out index))
                    return true;

                return false;
            }

            private static bool TryParse(
                string path,
                string prefix,
                DefinitionKind parsedKind,
                out DefinitionKind kind,
                out int index)
            {
                kind = default(DefinitionKind);
                index = -1;

                if (path.StartsWith(prefix + "[") == false)
                    return false;

                int start = prefix.Length + 1;
                int end = path.IndexOf(']', start);
                if (end < 0)
                    return false;

                string numText = path.Substring(start, end - start);
                int parsedIndex;
                if (int.TryParse(numText, out parsedIndex) == false)
                    return false;

                kind = parsedKind;
                index = parsedIndex;
                return true;
            }
        }
    }
}