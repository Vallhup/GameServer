using System;
using System.Collections.Generic;
using System.Windows.Forms;
using WITH_ServerDataTool.Editor;
using WITH_ServerDataTool.ID;
using WITH_ServerDataTool.Utility;

namespace WITH_ServerDataTool.Definition
{
    public sealed class WorldDefEditor : DefinitionEditorBase<WorldDef>
    {
        private readonly IWorldEditorLookupProvider _lookupProvider;

        private EditorLayoutPanel _layout;

        private DefinitionReferenceComboBox<WorldId> _cmbId;
        private TextBox _txtName;

        private TextBox _txtCollisionMapPath;
        private TextBox _txtHeightMapPath;
        private NumericUpDown _numCollisionWidth;
        private NumericUpDown _numCollisionHeight;
        private NumericUpDown _numWorldSize;
        private NumericUpDown _numHeightScale;

        private DefinitionReferenceComboBox<SpawnSetId> _cmbSpawnSetId;

        private bool _suppressEvents;

        public WorldDefEditor(IWorldEditorLookupProvider lookupProvider)
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

            _cmbId = new DefinitionReferenceComboBox<WorldId>();
            _cmbId.Dock = DockStyle.Fill;

            _txtName = new TextBox();
            _txtName.Dock = DockStyle.Fill;

            _txtCollisionMapPath = new TextBox();
            _txtCollisionMapPath.Dock = DockStyle.Fill;

            _txtHeightMapPath = new TextBox();
            _txtHeightMapPath.Dock = DockStyle.Fill;

            _numCollisionWidth = new NumericUpDown();
            _numCollisionWidth.Dock = DockStyle.Fill;
            _numCollisionWidth.Minimum = 0;
            _numCollisionWidth.Maximum = 1000000;

            _numCollisionHeight = new NumericUpDown();
            _numCollisionHeight.Dock = DockStyle.Fill;
            _numCollisionHeight.Minimum = 0;
            _numCollisionHeight.Maximum = 1000000;

            _numWorldSize = new NumericUpDown();
            _numWorldSize.Dock = DockStyle.Fill;
            _numWorldSize.Minimum = 0;
            _numWorldSize.Maximum = 1000000;
            _numWorldSize.DecimalPlaces = 3;
            _numWorldSize.Increment = 0.001M;

            _numHeightScale = new NumericUpDown();
            _numHeightScale.Dock = DockStyle.Fill;
            _numHeightScale.Minimum = -1000000;
            _numHeightScale.Maximum = 1000000;
            _numHeightScale.DecimalPlaces = 3;
            _numHeightScale.Increment = 0.001M;

            _cmbSpawnSetId = new DefinitionReferenceComboBox<SpawnSetId>();
            _cmbSpawnSetId.Dock = DockStyle.Fill;

            _layout.AddRow("Id", _cmbId);
            _layout.AddRow("Name", _txtName);
            _layout.AddRow("Collision Map Path", _txtCollisionMapPath);
            _layout.AddRow("Height Map Path", _txtHeightMapPath);
            _layout.AddRow("Collision Width", _numCollisionWidth);
            _layout.AddRow("Collision Height", _numCollisionHeight);
            _layout.AddRow("World Size", _numWorldSize);
            _layout.AddRow("Height Scale", _numHeightScale);
            _layout.AddRow("Spawn Set Id", _cmbSpawnSetId);

            Controls.Clear();
            Controls.Add(_layout);
        }

        private void WireEvents()
        {
            _cmbId.SelectedIdChanged += OnAnyValueChanged;
            _txtName.TextChanged += OnAnyValueChanged;
            _txtCollisionMapPath.TextChanged += OnAnyValueChanged;
            _txtHeightMapPath.TextChanged += OnAnyValueChanged;
            _numCollisionWidth.ValueChanged += OnAnyValueChanged;
            _numCollisionHeight.ValueChanged += OnAnyValueChanged;
            _numWorldSize.ValueChanged += OnAnyValueChanged;
            _numHeightScale.ValueChanged += OnAnyValueChanged;
            _cmbSpawnSetId.SelectedIdChanged += OnAnyValueChanged;
        }

        private void OnAnyValueChanged(object sender, EventArgs e)
        {
            if (_suppressEvents)
                return;

            MarkDirty();
        }

        protected override void OnBeforeBind(
            DefinitionKind kind,
            WorldDef source,
            WorldDef workingCopy)
        {
            _cmbId.SetItems(_lookupProvider.GetWorldItems());
            _cmbSpawnSetId.SetItems(_lookupProvider.GetSpawnSetItems());
        }

        protected override WorldDef CloneDefinition(WorldDef source)
        {
            WorldDef clone = new WorldDef();
            clone.Id = source.Id;
            clone.Name = source.Name;
            clone.SpawnSetId = source.SpawnSetId;
            clone.Map = CloneMap(source.Map);
            return clone;
        }

        protected override void CopyDefinition(WorldDef from, WorldDef to)
        {
            to.Id = from.Id;
            to.Name = from.Name;
            to.SpawnSetId = from.SpawnSetId;
            to.Map = CloneMap(from.Map);
        }

        protected override void LoadToControls(WorldDef value)
        {
            _suppressEvents = true;
            try
            {
                EnsureMap(value);

                _cmbId.SetSelectedId(value.Id);
                _txtName.Text = value.Name ?? string.Empty;
                _txtCollisionMapPath.Text = value.Map.CollisionMapPath ?? string.Empty;
                _txtHeightMapPath.Text = value.Map.HeightMapPath ?? string.Empty;
                _numCollisionWidth.Value = ClampDecimal(value.Map.CollisionWidth, _numCollisionWidth);
                _numCollisionHeight.Value = ClampDecimal(value.Map.CollisionHeight, _numCollisionHeight);
                _numWorldSize.Value = ClampDecimal((decimal)value.Map.WorldSize, _numWorldSize);
                _numHeightScale.Value = ClampDecimal((decimal)value.Map.HeightScale, _numHeightScale);
                _cmbSpawnSetId.SetSelectedId(value.SpawnSetId);
            }
            finally
            {
                _suppressEvents = false;
            }
        }

        protected override void SaveFromControls(WorldDef value)
        {
            EnsureMap(value);

            value.Id = _cmbId.GetSelectedId();
            value.Name = _txtName.Text ?? string.Empty;
            value.Map.CollisionMapPath = _txtCollisionMapPath.Text ?? string.Empty;
            value.Map.HeightMapPath = _txtHeightMapPath.Text ?? string.Empty;
            value.Map.CollisionWidth = Decimal.ToInt32(_numCollisionWidth.Value);
            value.Map.CollisionHeight = Decimal.ToInt32(_numCollisionHeight.Value);
            value.Map.WorldSize = (float)_numWorldSize.Value;
            value.Map.HeightScale = (float)_numHeightScale.Value;
            value.SpawnSetId = _cmbSpawnSetId.GetSelectedId();
        }

        protected override void ClearControls()
        {
            _suppressEvents = true;
            try
            {
                _cmbId.SelectedIndex = -1;
                _txtName.Text = string.Empty;
                _txtCollisionMapPath.Text = string.Empty;
                _txtHeightMapPath.Text = string.Empty;
                _numCollisionWidth.Value = 0;
                _numCollisionHeight.Value = 0;
                _numWorldSize.Value = 0;
                _numHeightScale.Value = 0;
                _cmbSpawnSetId.SelectedIndex = -1;
            }
            finally
            {
                _suppressEvents = false;
            }
        }

        private static void EnsureMap(WorldDef value)
        {
            if (value.Map == null)
                value.Map = new MapDef();
        }

        private static MapDef CloneMap(MapDef source)
        {
            MapDef clone = new MapDef();

            if (source == null)
                return clone;

            clone.CollisionMapPath = source.CollisionMapPath;
            clone.HeightMapPath = source.HeightMapPath;
            clone.CollisionWidth = source.CollisionWidth;
            clone.CollisionHeight = source.CollisionHeight;
            clone.WorldSize = source.WorldSize;
            clone.HeightScale = source.HeightScale;
            return clone;
        }

        private static decimal ClampDecimal(int value, NumericUpDown input)
        {
            return ClampDecimal((decimal)value, input);
        }

        private static decimal ClampDecimal(decimal value, NumericUpDown input)
        {
            if (value < input.Minimum) return input.Minimum;
            if (value > input.Maximum) return input.Maximum;
            return value;
        }
    }
}