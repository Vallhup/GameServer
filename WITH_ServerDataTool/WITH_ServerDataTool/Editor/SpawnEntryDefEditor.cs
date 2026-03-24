using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;
using WITH_ServerDataTool.Editor;
using WITH_ServerDataTool.ID;
using WITH_ServerDataTool.Utility;

namespace WITH_ServerDataTool.Definition
{
    public sealed class SpawnEntryDefEditor : UserControl
    {
        private readonly ISpawnEntryEditorLookupProvider _lookupProvider;

        private TableLayoutPanel _layout;

        private DefinitionReferenceComboBox<CharacterId> _cmbCharacterId;
        private NumericUpDown _numX;
        private NumericUpDown _numY;
        private NumericUpDown _numZ;
        private NumericUpDown _numYawRad;
        private CheckBox _chkIsPlayerSpawn;

        private SpawnEntryDef _value;
        private bool _suppressEvents;

        public event EventHandler ValueChanged;

        public SpawnEntryDefEditor(ISpawnEntryEditorLookupProvider lookupProvider)
        {
            if (lookupProvider == null)
                throw new ArgumentNullException("lookupProvider");

            _lookupProvider = lookupProvider;

            BuildUi();
            WireEvents();
            SetEditorEnabled(false);
        }

        public void SetLookupItems()
        {
            _cmbCharacterId.SetItems(_lookupProvider.GetCharacterItems());
        }

        public void SetValue(SpawnEntryDef value)
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

                _cmbCharacterId.SetSelectedId(_value.CharacterId);
                _numX.Value = ClampDecimal((decimal)_value.X, _numX);
                _numY.Value = ClampDecimal((decimal)_value.Y, _numY);
                _numZ.Value = ClampDecimal((decimal)_value.Z, _numZ);
                _numYawRad.Value = ClampDecimal((decimal)_value.YawRad, _numYawRad);
                _chkIsPlayerSpawn.Checked = _value.IsPlayerSpawn;
            }
            finally
            {
                _suppressEvents = false;
            }
        }

        private void BuildUi()
        {
            _layout = new TableLayoutPanel();
            _layout.Dock = DockStyle.Fill;
            _layout.Padding = new Padding(6);
            _layout.Margin = new Padding(0);
            _layout.ColumnCount = 2;
            _layout.RowCount = 6;

            _layout.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 120F));
            _layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100F));

            for (int i = 0; i < 6; ++i)
                _layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));

            _cmbCharacterId = new DefinitionReferenceComboBox<CharacterId>();
            _cmbCharacterId.Dock = DockStyle.Fill;

            _numX = CreateSignedFloatInput();
            _numY = CreateSignedFloatInput();
            _numZ = CreateSignedFloatInput();
            _numYawRad = CreateSignedFloatInput();

            _chkIsPlayerSpawn = new CheckBox();
            _chkIsPlayerSpawn.Text = "Enabled";
            _chkIsPlayerSpawn.Dock = DockStyle.Left;
            _chkIsPlayerSpawn.AutoSize = true;

            AddRow(0, "Character Id", _cmbCharacterId);
            AddRow(1, "X", _numX);
            AddRow(2, "Y", _numY);
            AddRow(3, "Z", _numZ);
            AddRow(4, "Yaw Rad", _numYawRad);
            AddRow(5, "Is Player Spawn", _chkIsPlayerSpawn);

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

        private static NumericUpDown CreateSignedFloatInput()
        {
            NumericUpDown num = new NumericUpDown();
            num.Dock = DockStyle.Fill;
            num.Minimum = -1000000M;
            num.Maximum = 1000000M;
            num.DecimalPlaces = 3;
            num.Increment = 0.001M;
            return num;
        }

        private void WireEvents()
        {
            _cmbCharacterId.SelectedIdChanged += OnAnyControlChanged;
            _numX.ValueChanged += OnAnyControlChanged;
            _numY.ValueChanged += OnAnyControlChanged;
            _numZ.ValueChanged += OnAnyControlChanged;
            _numYawRad.ValueChanged += OnAnyControlChanged;
            _chkIsPlayerSpawn.CheckedChanged += OnAnyControlChanged;
        }

        private void OnAnyControlChanged(object sender, EventArgs e)
        {
            if (_suppressEvents || _value == null)
                return;

            _value.CharacterId = _cmbCharacterId.GetSelectedId();
            _value.X = (float)_numX.Value;
            _value.Y = (float)_numY.Value;
            _value.Z = (float)_numZ.Value;
            _value.YawRad = (float)_numYawRad.Value;
            _value.IsPlayerSpawn = _chkIsPlayerSpawn.Checked;

            if (ValueChanged != null)
                ValueChanged(this, EventArgs.Empty);
        }

        private void ClearControls()
        {
            _cmbCharacterId.SelectedIndex = -1;
            _numX.Value = 0;
            _numY.Value = 0;
            _numZ.Value = 0;
            _numYawRad.Value = 0;
            _chkIsPlayerSpawn.Checked = false;
        }

        private void SetEditorEnabled(bool enabled)
        {
            _layout.Enabled = enabled;
        }

        private static decimal ClampDecimal(decimal value, NumericUpDown input)
        {
            if (value < input.Minimum) return input.Minimum;
            if (value > input.Maximum) return input.Maximum;
            return value;
        }
    }
}