using System;
using System.Drawing;
using System.Windows.Forms;

namespace WITH_ServerDataTool.Definition
{
    public sealed class BuffModifierDefEditor : UserControl
    {
        private TableLayoutPanel _layout;

        private ComboBox _cmbEffect;
        private ComboBox _cmbStatId;
        private NumericUpDown _numValue;

        private BuffModifierDef _value;
        private bool _suppressEvents;

        public event EventHandler ValueChanged;

        public BuffModifierDefEditor()
        {
            BuildUi();
            WireEvents();
            SetEditorEnabled(false);
        }

        public void SetValue(BuffModifierDef value)
        {
            _value = value;

            _suppressEvents = true;
            try
            {
                if (_value == null)
                {
                    ClearControls();
                    SetEditorEnabled(false);
                    return;
                }

                SetEditorEnabled(true);

                SetComboSelectedEnum(_cmbEffect, _value.Effect);
                SetComboSelectedEnum(_cmbStatId, _value.StatId);
                _numValue.Value = ClampDecimal((decimal)_value.Value, _numValue);
            }
            finally
            {
                _suppressEvents = false;
            }
        }

        public BuffModifierDef GetValue()
        {
            return _value;
        }

        private void BuildUi()
        {
            _layout = new TableLayoutPanel();
            _layout.Dock = DockStyle.Fill;
            _layout.Padding = new Padding(6);
            _layout.Margin = new Padding(0);
            _layout.ColumnCount = 2;
            _layout.RowCount = 3;

            _layout.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 120F));
            _layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100F));

            for (int i = 0; i < 3; ++i)
                _layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));

            _cmbEffect = CreateEnumComboBox(typeof(BuffEffectId));
            _cmbStatId = CreateEnumComboBox(typeof(StatId));

            _numValue = new NumericUpDown();
            _numValue.Dock = DockStyle.Fill;
            _numValue.Minimum = -1000000M;
            _numValue.Maximum = 1000000M;
            _numValue.DecimalPlaces = 3;
            _numValue.Increment = 0.001M;

            AddRow(0, "Effect", _cmbEffect);
            AddRow(1, "Stat Id", _cmbStatId);
            AddRow(2, "Value", _numValue);

            Controls.Clear();
            Controls.Add(_layout);
        }

        private void AddRow(int row, string labelText, Control editor)
        {
            Label label = new Label();
            label.Text = labelText;
            label.Dock = DockStyle.Fill;
            label.TextAlign = ContentAlignment.MiddleLeft;
            label.AutoSize = true;

            _layout.Controls.Add(label, 0, row);
            _layout.Controls.Add(editor, 1, row);
        }

        private static ComboBox CreateEnumComboBox(Type enumType)
        {
            ComboBox combo = new ComboBox();
            combo.Dock = DockStyle.Fill;
            combo.DropDownStyle = ComboBoxStyle.DropDownList;

            Array values = Enum.GetValues(enumType);
            for (int i = 0; i < values.Length; ++i)
                combo.Items.Add(values.GetValue(i));

            if (combo.Items.Count > 0)
                combo.SelectedIndex = 0;

            return combo;
        }

        private void WireEvents()
        {
            _cmbEffect.SelectedIndexChanged += OnAnyControlChanged;
            _cmbStatId.SelectedIndexChanged += OnAnyControlChanged;
            _numValue.ValueChanged += OnAnyControlChanged;
        }

        private void OnAnyControlChanged(object sender, EventArgs e)
        {
            if (_suppressEvents || _value == null)
                return;

            _value.Effect = GetSelectedEnumValue<BuffEffectId>(_cmbEffect);
            _value.StatId = GetSelectedEnumValue<StatId>(_cmbStatId);
            _value.Value = (float)_numValue.Value;

            if (ValueChanged != null)
                ValueChanged(this, EventArgs.Empty);
        }

        private void ClearControls()
        {
            SetComboSelectedEnum(_cmbEffect, BuffEffectId.None);
            SetComboSelectedEnum(_cmbStatId, StatId.None);
            _numValue.Value = 0;
        }

        private void SetEditorEnabled(bool enabled)
        {
            _layout.Enabled = enabled;
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

        private static decimal ClampDecimal(decimal value, NumericUpDown input)
        {
            if (value < input.Minimum) return input.Minimum;
            if (value > input.Maximum) return input.Maximum;
            return value;
        }
    }
}