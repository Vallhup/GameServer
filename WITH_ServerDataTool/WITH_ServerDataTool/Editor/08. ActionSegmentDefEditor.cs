using System;
using System.Drawing;
using System.Windows.Forms;

namespace WITH_ServerDataTool.Definition
{
    public sealed class ActionSegmentDefEditor : UserControl
    {
        private TableLayoutPanel _rootLayout;

        private NumericUpDown _numStartNormalizedTime;
        private NumericUpDown _numEndNormalizedTime;

        private ComboBox _cmbMoveMode;
        private NumericUpDown _numMoveDistance;
        private NumericUpDown _numMoveMaxSpeed;
        private NumericUpDown _numMoveMaxTravel;
        private NumericUpDown _numMoveStopRange;
        private CheckBox _chkMoveLockDir;

        private ComboBox _cmbYawMode;
        private NumericUpDown _numTurnSpeedRad;
        private NumericUpDown _numYawEpsRad;

        private ComboBox _cmbVerticalMode;
        private NumericUpDown _numDeltaY;

        private bool _suppressEvents;
        private ActionSegmentDef _value;

        public event EventHandler ValueChanged;

        public ActionSegmentDefEditor()
        {
            BuildUi();
            WireEvents();
            SetEditorEnabled(false);
        }

        public void SetValue(ActionSegmentDef value)
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

                EnsureNestedObjects();
                SetEditorEnabled(true);

                _numStartNormalizedTime.Value =
                    ClampDecimal((decimal)_value.StartNormalizedTime, _numStartNormalizedTime);
                _numEndNormalizedTime.Value =
                    ClampDecimal((decimal)_value.EndNormalizedTime, _numEndNormalizedTime);

                SetComboSelectedEnum(_cmbMoveMode, _value.MoveMode);
                _numMoveDistance.Value =
                    ClampDecimal((decimal)_value.MoveParams.Distance, _numMoveDistance);
                _numMoveMaxSpeed.Value =
                    ClampDecimal((decimal)_value.MoveParams.MaxSpeed, _numMoveMaxSpeed);
                _numMoveMaxTravel.Value =
                    ClampDecimal((decimal)_value.MoveParams.MaxTravel, _numMoveMaxTravel);
                _numMoveStopRange.Value =
                    ClampDecimal((decimal)_value.MoveParams.StopRange, _numMoveStopRange);
                _chkMoveLockDir.Checked = _value.MoveParams.LockDir;

                SetComboSelectedEnum(_cmbYawMode, _value.YawMode);
                _numTurnSpeedRad.Value =
                    ClampDecimal((decimal)_value.YawParams.TurnSpeedRad, _numTurnSpeedRad);
                _numYawEpsRad.Value =
                    ClampDecimal((decimal)_value.YawParams.YawEpsRad, _numYawEpsRad);

                SetComboSelectedEnum(_cmbVerticalMode, _value.VerticalMode);
                _numDeltaY.Value =
                    ClampDecimal((decimal)_value.VerticalParams.DeltaY, _numDeltaY);

                UpdateModeUi();
            }
            finally
            {
                _suppressEvents = false;
            }
        }

        public ActionSegmentDef GetValue()
        {
            return _value;
        }

        private void BuildUi()
        {
            _rootLayout = new TableLayoutPanel();
            _rootLayout.Dock = DockStyle.Fill;
            _rootLayout.Padding = new Padding(6);
            _rootLayout.Margin = new Padding(0);
            _rootLayout.ColumnCount = 2;
            _rootLayout.RowCount = 13;
            _rootLayout.AutoScroll = true;

            _rootLayout.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 150F));
            _rootLayout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100F));

            for (int i = 0; i < 13; ++i)
                _rootLayout.RowStyles.Add(new RowStyle(SizeType.AutoSize));

            _numStartNormalizedTime = CreateFloatInput(0M, 1M, 3, 0.001M);
            _numEndNormalizedTime = CreateFloatInput(0M, 1M, 3, 0.001M);

            _cmbMoveMode = CreateEnumComboBox(typeof(MoveModeId));
            _numMoveDistance = CreateSignedFloatInput();
            _numMoveMaxSpeed = CreateSignedFloatInput();
            _numMoveMaxTravel = CreateSignedFloatInput();
            _numMoveStopRange = CreateSignedFloatInput();
            _chkMoveLockDir = CreateCheckBox();

            _cmbYawMode = CreateEnumComboBox(typeof(YawModeId));
            _numTurnSpeedRad = CreateSignedFloatInput();
            _numYawEpsRad = CreateSignedFloatInput();

            _cmbVerticalMode = CreateEnumComboBox(typeof(VerticalModeId));
            _numDeltaY = CreateSignedFloatInput();

            AddRow(0, "Start Normalized Time", _numStartNormalizedTime);
            AddRow(1, "End Normalized Time", _numEndNormalizedTime);

            AddRow(2, "Move Mode", _cmbMoveMode);
            AddRow(3, "Move Distance", _numMoveDistance);
            AddRow(4, "Move Max Speed", _numMoveMaxSpeed);
            AddRow(5, "Move Max Travel", _numMoveMaxTravel);
            AddRow(6, "Move Stop Range", _numMoveStopRange);
            AddRow(7, "Move Lock Dir", _chkMoveLockDir);

            AddRow(8, "Yaw Mode", _cmbYawMode);
            AddRow(9, "Turn Speed Rad", _numTurnSpeedRad);
            AddRow(10, "Yaw Eps Rad", _numYawEpsRad);

            AddRow(11, "Vertical Mode", _cmbVerticalMode);
            AddRow(12, "Delta Y", _numDeltaY);

            Controls.Clear();
            Controls.Add(_rootLayout);
        }

        private void AddRow(int rowIndex, string labelText, Control editor)
        {
            Label label = CreateLabel(labelText);
            _rootLayout.Controls.Add(label, 0, rowIndex);
            _rootLayout.Controls.Add(editor, 1, rowIndex);
        }

        private static Label CreateLabel(string text)
        {
            Label label = new Label();
            label.Text = text;
            label.Dock = DockStyle.Fill;
            label.TextAlign = ContentAlignment.MiddleLeft;
            label.AutoSize = true;
            marginify(label);
            return label;
        }

        private static void marginify(Control c)
        {
            c.Margin = new Padding(3, 6, 3, 3);
        }

        private static CheckBox CreateCheckBox()
        {
            CheckBox checkBox = new CheckBox();
            checkBox.Text = "Enabled";
            checkBox.Dock = DockStyle.Left;
            checkBox.AutoSize = true;
            checkBox.Margin = new Padding(3, 6, 3, 3);
            return checkBox;
        }

        private static NumericUpDown CreateFloatInput(
            decimal min,
            decimal max,
            int decimalPlaces,
            decimal increment)
        {
            NumericUpDown num = new NumericUpDown();
            num.Dock = DockStyle.Fill;
            num.Minimum = min;
            num.Maximum = max;
            num.DecimalPlaces = decimalPlaces;
            num.Increment = increment;
            num.Margin = new Padding(3);
            return num;
        }

        private static NumericUpDown CreateSignedFloatInput()
        {
            NumericUpDown num = new NumericUpDown();
            num.Dock = DockStyle.Fill;
            num.Minimum = -1000000M;
            num.Maximum = 1000000M;
            num.DecimalPlaces = 3;
            num.Increment = 0.001M;
            num.Margin = new Padding(3);
            return num;
        }

        private static ComboBox CreateEnumComboBox(Type enumType)
        {
            ComboBox combo = new ComboBox();
            combo.Dock = DockStyle.Fill;
            combo.DropDownStyle = ComboBoxStyle.DropDownList;
            combo.Margin = new Padding(3);

            Array values = Enum.GetValues(enumType);
            for (int i = 0; i < values.Length; ++i)
                combo.Items.Add(values.GetValue(i));

            if (combo.Items.Count > 0)
                combo.SelectedIndex = 0;

            return combo;
        }

        private void WireEvents()
        {
            _numStartNormalizedTime.ValueChanged += OnAnyControlChanged;
            _numEndNormalizedTime.ValueChanged += OnAnyControlChanged;

            _cmbMoveMode.SelectedIndexChanged += OnAnyControlChanged;
            _numMoveDistance.ValueChanged += OnAnyControlChanged;
            _numMoveMaxSpeed.ValueChanged += OnAnyControlChanged;
            _numMoveMaxTravel.ValueChanged += OnAnyControlChanged;
            _numMoveStopRange.ValueChanged += OnAnyControlChanged;
            _chkMoveLockDir.CheckedChanged += OnAnyControlChanged;

            _cmbYawMode.SelectedIndexChanged += OnAnyControlChanged;
            _numTurnSpeedRad.ValueChanged += OnAnyControlChanged;
            _numYawEpsRad.ValueChanged += OnAnyControlChanged;

            _cmbVerticalMode.SelectedIndexChanged += OnAnyControlChanged;
            _numDeltaY.ValueChanged += OnAnyControlChanged;
        }

        private void OnAnyControlChanged(object sender, EventArgs e)
        {
            if (_suppressEvents || _value == null)
                return;

            ApplyControlsToValue();
            UpdateModeUi();

            if (ValueChanged != null)
                ValueChanged(this, EventArgs.Empty);
        }

        private void ApplyControlsToValue()
        {
            EnsureNestedObjects();

            _value.StartNormalizedTime = (float)_numStartNormalizedTime.Value;
            _value.EndNormalizedTime = (float)_numEndNormalizedTime.Value;

            _value.MoveMode = GetSelectedEnumValue<MoveModeId>(_cmbMoveMode);
            _value.MoveParams.Distance = (float)_numMoveDistance.Value;
            _value.MoveParams.MaxSpeed = (float)_numMoveMaxSpeed.Value;
            _value.MoveParams.MaxTravel = (float)_numMoveMaxTravel.Value;
            _value.MoveParams.StopRange = (float)_numMoveStopRange.Value;
            _value.MoveParams.LockDir = _chkMoveLockDir.Checked;

            _value.YawMode = GetSelectedEnumValue<YawModeId>(_cmbYawMode);
            _value.YawParams.TurnSpeedRad = (float)_numTurnSpeedRad.Value;
            _value.YawParams.YawEpsRad = (float)_numYawEpsRad.Value;

            _value.VerticalMode = GetSelectedEnumValue<VerticalModeId>(_cmbVerticalMode);
            _value.VerticalParams.DeltaY = (float)_numDeltaY.Value;
        }

        private void EnsureNestedObjects()
        {
            if (_value == null)
                return;

            if (_value.MoveParams == null)
                _value.MoveParams = new MoveParamsDef();

            if (_value.YawParams == null)
                _value.YawParams = new YawParamsDef();

            if (_value.VerticalParams == null)
                _value.VerticalParams = new VerticalParamsDef();
        }

        private void UpdateModeUi()
        {
            MoveModeId moveMode = GetSelectedEnumValue<MoveModeId>(_cmbMoveMode);
            bool moveEnabled = moveMode != MoveModeId.None;

            _numMoveDistance.Enabled = moveEnabled;
            _numMoveMaxSpeed.Enabled = moveEnabled;
            _numMoveMaxTravel.Enabled = moveEnabled;
            _numMoveStopRange.Enabled = moveEnabled;
            _chkMoveLockDir.Enabled = moveEnabled;

            YawModeId yawMode = GetSelectedEnumValue<YawModeId>(_cmbYawMode);
            bool yawEnabled = yawMode != YawModeId.None;

            _numTurnSpeedRad.Enabled = yawEnabled;
            _numYawEpsRad.Enabled = yawEnabled;

            VerticalModeId verticalMode = GetSelectedEnumValue<VerticalModeId>(_cmbVerticalMode);
            bool verticalEnabled = verticalMode != VerticalModeId.None;

            _numDeltaY.Enabled = verticalEnabled;
        }

        private void ClearControls()
        {
            _numStartNormalizedTime.Value = 0;
            _numEndNormalizedTime.Value = 0;

            SetComboSelectedEnum(_cmbMoveMode, MoveModeId.None);
            _numMoveDistance.Value = 0;
            _numMoveMaxSpeed.Value = 0;
            _numMoveMaxTravel.Value = 0;
            _numMoveStopRange.Value = 0;
            _chkMoveLockDir.Checked = false;

            SetComboSelectedEnum(_cmbYawMode, YawModeId.None);
            _numTurnSpeedRad.Value = 0;
            _numYawEpsRad.Value = 0;

            SetComboSelectedEnum(_cmbVerticalMode, VerticalModeId.None);
            _numDeltaY.Value = 0;

            UpdateModeUi();
        }

        private void SetEditorEnabled(bool enabled)
        {
            _rootLayout.Enabled = enabled;
            _numDeltaY.Enabled = enabled;
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