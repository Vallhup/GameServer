using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;
using WITH_ServerDataTool.ID;

namespace WITH_ServerDataTool.Editor
{
    public sealed class DefinitionReferenceItem<TId>
    {
        private readonly TId _id;
        private readonly string _displayText;

        public DefinitionReferenceItem(TId id, string displayText)
        {
            _id = id;
            _displayText = string.IsNullOrEmpty(displayText)
                ? Convert.ToString(id)
                : displayText;
        }

        public TId Id
        {
            get { return _id; }
        }

        public override string ToString()
        {
            return _displayText;
        }
    }

    public class DefinitionReferenceComboBox<TId> : ComboBox
    {
        private bool _allowEmpty;

        public event EventHandler SelectedIdChanged;

        public DefinitionReferenceComboBox()
        {
            DropDownStyle = ComboBoxStyle.DropDownList;
            FormattingEnabled = true;
            IntegralHeight = false;
            SelectedIndexChanged += OnSelectedIndexChangedInternal;
        }

        public bool AllowEmpty
        {
            get { return _allowEmpty; }
            set { _allowEmpty = value; }
        }

        public void SetItems(IEnumerable<DefinitionReferenceItem<TId> > items)
        {
            BeginUpdate();
            try
            {
                Items.Clear();

                if (_allowEmpty)
                    Items.Add("(None)");

                if (items != null)
                {
                    foreach (DefinitionReferenceItem<TId> item in items)
                        Items.Add(item);
                }

                if (Items.Count > 0)
                    SelectedIndex = 0;
            }
            finally
            {
                EndUpdate();
            }
        }

        public TId GetSelectedId()
        {
            object item = SelectedItem;
            if (item == null)
                return default(TId);

            DefinitionReferenceItem<TId> refItem =
                item as DefinitionReferenceItem<TId>;

            if (refItem == null)
                return default(TId);

            return refItem.Id;
        }

        public void SetSelectedId(TId id)
        {
            for (int i = 0; i < Items.Count; ++i)
            {
                DefinitionReferenceItem<TId> refItem =
                    Items[i] as DefinitionReferenceItem<TId>;

                if (refItem == null)
                    continue;

                if (EqualityComparer<TId>.Default.Equals(refItem.Id, id))
                {
                    SelectedIndex = i;
                    return;
                }
            }

            if (_allowEmpty && Items.Count > 0)
                SelectedIndex = 0;
            else
                SelectedIndex = -1;
        }

        private void OnSelectedIndexChangedInternal(object sender, EventArgs e)
        {
            EventHandler handler = SelectedIdChanged;
            if (handler != null)
                handler(this, EventArgs.Empty);
        }
    }

	public class SimpleListEditor<TItem> : UserControl
	{
		private ListBox _listBox;
		private FlowLayoutPanel _buttonPanel;
		private Button _btnAdd;
		private Button _btnRemove;
		private Button _btnUp;
		private Button _btnDown;

		private IList<TItem> _items;
		private Func<TItem> _createItem;
		private Func<TItem, string> _displayTextSelector;

		public event EventHandler SelectedItemChanged;
		public event EventHandler ListChanged;

		public SimpleListEditor()
		{
			BuildUi();
		}

		public void Initialize(
			IList<TItem> items,
			Func<TItem> createItem,
			Func<TItem, string> displayTextSelector)
		{
			_items = items;
			_createItem = createItem;
			_displayTextSelector = displayTextSelector;
			RefreshItems();
		}

		public TItem SelectedItem
		{
			get
			{
				if (_listBox.SelectedIndex < 0 || _items == null)
					return default(TItem);

				if (_listBox.SelectedIndex >= _items.Count)
					return default(TItem);

				return _items[_listBox.SelectedIndex];
			}
		}

		public int SelectedIndex
		{
			get { return _listBox.SelectedIndex; }
			set { _listBox.SelectedIndex = value; }
		}

		public void RefreshItems()
		{
			_listBox.BeginUpdate();
			try
			{
				_listBox.Items.Clear();

				if (_items == null)
					return;

				for (int i = 0; i < _items.Count; ++i)
				{
					TItem item = _items[i];
					string text = _displayTextSelector != null
						? _displayTextSelector(item)
						: Convert.ToString(item);

					_listBox.Items.Add(text);
				}
			}
			finally
			{
				_listBox.EndUpdate();
			}

			UpdateButtonState();
		}

		private void BuildUi()
		{
			_listBox = new ListBox();
			_buttonPanel = new FlowLayoutPanel();
			_btnAdd = new Button();
			_btnRemove = new Button();
			_btnUp = new Button();
			_btnDown = new Button();

			_listBox.Dock = DockStyle.Fill;
			_listBox.IntegralHeight = false;
			_listBox.SelectedIndexChanged += OnListBoxSelectedIndexChanged;

			_buttonPanel.Dock = DockStyle.Bottom;
			_buttonPanel.Height = 32;
			_buttonPanel.FlowDirection = FlowDirection.LeftToRight;
			_buttonPanel.WrapContents = false;

			_btnAdd.Text = "Add";
			_btnRemove.Text = "Remove";
			_btnUp.Text = "Up";
			_btnDown.Text = "Down";

			_btnAdd.Width = 70;
			_btnRemove.Width = 70;
			_btnUp.Width = 50;
			_btnDown.Width = 50;

			_btnAdd.Click += OnAddClicked;
			_btnRemove.Click += OnRemoveClicked;
			_btnUp.Click += OnUpClicked;
			_btnDown.Click += OnDownClicked;

			_buttonPanel.Controls.Add(_btnAdd);
			_buttonPanel.Controls.Add(_btnRemove);
			_buttonPanel.Controls.Add(_btnUp);
			_buttonPanel.Controls.Add(_btnDown);

			Controls.Add(_listBox);
			Controls.Add(_buttonPanel);

			UpdateButtonState();
		}

		private void UpdateButtonState()
		{
			bool hasItems = _items != null && _items.Count > 0;
			bool hasSelection = hasItems &&
				_listBox.SelectedIndex >= 0 &&
				_listBox.SelectedIndex < _items.Count;

			_btnRemove.Enabled = hasSelection;
			_btnUp.Enabled = hasSelection && _listBox.SelectedIndex > 0;
			_btnDown.Enabled = hasSelection &&
				_listBox.SelectedIndex >= 0 &&
				_listBox.SelectedIndex < _items.Count - 1;
		}

		private void OnListBoxSelectedIndexChanged(object sender, EventArgs e)
		{
			UpdateButtonState();

			EventHandler selectedHandler = SelectedItemChanged;
			if (selectedHandler != null)
				selectedHandler(this, EventArgs.Empty);
		}

		private void OnAddClicked(object sender, EventArgs e)
		{
			if (_items == null || _createItem == null)
				return;

			TItem item = _createItem();
			_items.Add(item);

			RefreshItems();
			_listBox.SelectedIndex = _items.Count - 1;

			RaiseListChanged();
		}

		private void OnRemoveClicked(object sender, EventArgs e)
		{
			if (_items == null)
				return;

			int index = _listBox.SelectedIndex;
			if (index < 0 || index >= _items.Count)
				return;

			_items.RemoveAt(index);

			RefreshItems();

			if (_items.Count > 0)
				_listBox.SelectedIndex = Math.Min(index, _items.Count - 1);

			RaiseListChanged();
		}

		private void OnUpClicked(object sender, EventArgs e)
		{
			if (_items == null)
				return;

			int index = _listBox.SelectedIndex;
			if (index <= 0 || index >= _items.Count)
				return;

			TItem temp = _items[index - 1];
			_items[index - 1] = _items[index];
			_items[index] = temp;

			RefreshItems();
			_listBox.SelectedIndex = index - 1;

			RaiseListChanged();
		}

		private void OnDownClicked(object sender, EventArgs e)
		{
			if (_items == null)
				return;

			int index = _listBox.SelectedIndex;
			if (index < 0 || index >= _items.Count - 1)
				return;

			TItem temp = _items[index + 1];
			_items[index + 1] = _items[index];
			_items[index] = temp;

			RefreshItems();
			_listBox.SelectedIndex = index + 1;

			RaiseListChanged();
		}

		private void RaiseListChanged()
		{
			UpdateButtonState();

			EventHandler handler = ListChanged;
			if (handler != null)
				handler(this, EventArgs.Empty);
		}
	}

	public static class EnumReferenceItems
	{
		public static List<DefinitionReferenceItem<TEnum>> Create<TEnum>()
			where TEnum : struct
		{
			Type enumType = typeof(TEnum);
			if (enumType.IsEnum == false)
				throw new InvalidOperationException(
					enumType.FullName + " is not an enum type.");

			Array values = Enum.GetValues(enumType);
			List<DefinitionReferenceItem<TEnum>> items =
				new List<DefinitionReferenceItem<TEnum>>(values.Length);

			for (int i = 0; i < values.Length; ++i)
			{
				object raw = values.GetValue(i);
				TEnum value = (TEnum)raw;
				string text = Enum.GetName(enumType, raw);

				items.Add(new DefinitionReferenceItem<TEnum>(value, text));
			}

			return items;
		}
	}

	public sealed class ReferenceListEditor<TId> : UserControl
	{
		private readonly ListBox _listBox;
		private readonly DefinitionReferenceComboBox<TId> _comboBox;
		private readonly FlowLayoutPanel _buttonPanel;
		private readonly Button _btnAdd;
		private readonly Button _btnRemove;
		private readonly Button _btnUp;
		private readonly Button _btnDown;

		private IList<TId> _items;

		public event EventHandler ListChanged;

		public ReferenceListEditor()
		{
			_listBox = new ListBox();
			_comboBox = new DefinitionReferenceComboBox<TId>();
			_buttonPanel = new FlowLayoutPanel();
			_btnAdd = new Button();
			_btnRemove = new Button();
			_btnUp = new Button();
			_btnDown = new Button();

			SuspendLayout();

			_comboBox.Dock = DockStyle.Top;
			_comboBox.Height = 24;

			_listBox.Dock = DockStyle.Fill;
			_listBox.IntegralHeight = false;
			_listBox.SelectedIndexChanged += OnSelectionChanged;

			_buttonPanel.Dock = DockStyle.Bottom;
			_buttonPanel.Height = 32;
			_buttonPanel.FlowDirection = FlowDirection.LeftToRight;
			_buttonPanel.WrapContents = false;

			_btnAdd.Text = "Add";
			_btnRemove.Text = "Remove";
			_btnUp.Text = "Up";
			_btnDown.Text = "Down";

			_btnAdd.Width = 70;
			_btnRemove.Width = 70;
			_btnUp.Width = 50;
			_btnDown.Width = 50;

			_btnAdd.Click += OnAddClicked;
			_btnRemove.Click += OnRemoveClicked;
			_btnUp.Click += OnUpClicked;
			_btnDown.Click += OnDownClicked;

			_buttonPanel.Controls.Add(_btnAdd);
			_buttonPanel.Controls.Add(_btnRemove);
			_buttonPanel.Controls.Add(_btnUp);
			_buttonPanel.Controls.Add(_btnDown);

			Controls.Add(_listBox);
			Controls.Add(_buttonPanel);
			Controls.Add(_comboBox);

			ResumeLayout(false);

			UpdateButtonState();
		}

		public void Bind(
			IList<TId> items,
			IEnumerable<DefinitionReferenceItem<TId>> candidates)
		{
			_items = items;
			_comboBox.SetItems(candidates);
			RefreshList();
		}

		public void ClearItems()
		{
			_items = null;
			_listBox.Items.Clear();
			UpdateButtonState();
		}

		public void RefreshList()
		{
			_listBox.BeginUpdate();
			try
			{
				_listBox.Items.Clear();

				if (_items != null)
				{
					for (int i = 0; i < _items.Count; ++i)
						_listBox.Items.Add(_items[i]);
				}
			}
			finally
			{
				_listBox.EndUpdate();
			}

			UpdateButtonState();
		}

		private void OnSelectionChanged(object sender, EventArgs e)
		{
			UpdateButtonState();
		}

		private void OnAddClicked(object sender, EventArgs e)
		{
			if (_items == null)
				return;

			TId selectedId = _comboBox.GetSelectedId();

			if (ContainsItem(selectedId))
				return;

			_items.Add(selectedId);
			RefreshList();
			_listBox.SelectedIndex = _items.Count - 1;
			RaiseListChanged();
		}

		private bool ContainsItem(TId value)
		{
			if (_items == null)
				return false;

			EqualityComparer<TId> comparer = EqualityComparer<TId>.Default;
			for (int i = 0; i < _items.Count; ++i)
			{
				if (comparer.Equals(_items[i], value))
					return true;
			}

			return false;
		}

		private void OnRemoveClicked(object sender, EventArgs e)
		{
			if (_items == null)
				return;

			int index = _listBox.SelectedIndex;
			if (index < 0 || index >= _items.Count)
				return;

			_items.RemoveAt(index);
			RefreshList();

			if (_items.Count > 0)
				_listBox.SelectedIndex = Math.Min(index, _items.Count - 1);

			RaiseListChanged();
		}

		private void OnUpClicked(object sender, EventArgs e)
		{
			if (_items == null)
				return;

			int index = _listBox.SelectedIndex;
			if (index <= 0 || index >= _items.Count)
				return;

			TId temp = _items[index - 1];
			_items[index - 1] = _items[index];
			_items[index] = temp;

			RefreshList();
			_listBox.SelectedIndex = index - 1;
			RaiseListChanged();
		}

		private void OnDownClicked(object sender, EventArgs e)
		{
			if (_items == null)
				return;

			int index = _listBox.SelectedIndex;
			if (index < 0 || index >= _items.Count - 1)
				return;

			TId temp = _items[index + 1];
			_items[index + 1] = _items[index];
			_items[index] = temp;

			RefreshList();
			_listBox.SelectedIndex = index + 1;
			RaiseListChanged();
		}

		private void UpdateButtonState()
		{
			bool hasItems = _items != null && _items.Count > 0;
			bool hasSelection =
				hasItems &&
				_listBox.SelectedIndex >= 0 &&
				_listBox.SelectedIndex < _listBox.Items.Count;

			_btnRemove.Enabled = hasSelection;
			_btnUp.Enabled = hasSelection && _listBox.SelectedIndex > 0;
			_btnDown.Enabled = hasSelection && _listBox.SelectedIndex < _listBox.Items.Count - 1;
		}

		private void RaiseListChanged()
		{
			UpdateButtonState();

			EventHandler handler = ListChanged;
			if (handler != null)
				handler(this, EventArgs.Empty);
		}
	}



	public sealed class EditorLayoutPanel : TableLayoutPanel
	{
		private int _rowCount;

		public EditorLayoutPanel()
		{
			ColumnCount = 2;
			RowCount = 0;
			Dock = DockStyle.Fill;
			AutoScroll = true;
			Padding = new Padding(8);

			ColumnStyles.Clear();
			ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 140f));
			ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100f));
		}

		public void AddRow(string labelText, Control editor)
		{
			if (editor == null)
				return;

			RowStyles.Add(new RowStyle(SizeType.AutoSize));
			RowCount++;

			Label label = new Label();
			label.Text = labelText;
			label.AutoSize = true;
			label.TextAlign = ContentAlignment.MiddleLeft;
			label.Anchor = AnchorStyles.Left | AnchorStyles.Top;
			label.Margin = new Padding(3, 6, 6, 3);

			editor.Anchor = AnchorStyles.Left | AnchorStyles.Right | AnchorStyles.Top;
			editor.Margin = new Padding(3);
			editor.Width = 200;

			Controls.Add(label, 0, _rowCount);
			Controls.Add(editor, 1, _rowCount);

			if (editor is not TextBox &&
				editor is not ComboBox &&
				editor is not DefinitionReferenceComboBox<CharacterId> &&
				editor is not DefinitionReferenceComboBox<EntityCategoryId> &&
				editor is not DefinitionReferenceComboBox<FactionId> &&
				editor is not DefinitionReferenceComboBox<AIArchetypeId>)
			{
				editor.Height = Math.Max(editor.Height, 120);
			}

			_rowCount++;
		}
	}

	public sealed class DefinitionEditorHost : Panel
	{
		private IDefinitionEditor _currentEditor;

		public event EventHandler CurrentEditorValueChanged;

		public DefinitionEditorHost()
		{
			Dock = DockStyle.Fill;
		}

		public IDefinitionEditor CurrentEditor
		{
			get { return _currentEditor; }
		}

		public void SetEditor(IDefinitionEditor editor)
		{
			if (_currentEditor != null)
			{
				_currentEditor.ValueChanged -= OnCurrentEditorValueChanged;

				Control oldControl = _currentEditor.EditorControl;
				Controls.Remove(oldControl);

				if (oldControl != null)
					oldControl.Dispose();
			}

			_currentEditor = editor;

			if (_currentEditor != null)
			{
				Control control = _currentEditor.EditorControl;
				control.Dock = DockStyle.Fill;
				Controls.Add(control);
				_currentEditor.ValueChanged += OnCurrentEditorValueChanged;
			}
		}

		public void ClearEditor()
		{
			SetEditor(null);
		}

		private void OnCurrentEditorValueChanged(object sender, EventArgs e)
		{
			EventHandler handler = CurrentEditorValueChanged;
			if (handler != null)
				handler(this, EventArgs.Empty);
		}
	}
}
