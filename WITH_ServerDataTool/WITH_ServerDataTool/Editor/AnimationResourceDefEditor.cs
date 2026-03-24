using System;
using System.Collections.Generic;
using System.Windows.Forms;
using WITH_ServerDataTool.Editor;
using WITH_ServerDataTool.ID;
using WITH_ServerDataTool.Utility;

namespace WITH_ServerDataTool.Definition
{
    public sealed class AnimationResourceDefEditor
        : DefinitionEditorBase<AnimationResourceDef>
    {
        private readonly IAnimationResourceEditorLookupProvider _lookupProvider;

        private EditorLayoutPanel _layout;

        private DefinitionReferenceComboBox<AnimationId> _cmbId;
        private TextBox _txtName;
        private TextBox _txtSourcePath;
        private NumericUpDown _numVersion;
        private NumericUpDown _numFps;
        private NumericUpDown _numNumFrames;

        private bool _suppressEvents;

        public AnimationResourceDefEditor(
            IAnimationResourceEditorLookupProvider lookupProvider)
        {
            if (lookupProvider == null)
                throw new ArgumentNullException("lookupProvider");

            _lookupProvider = lookupProvider;

            BuildUi();
            WireEvents();
        }

        protected override void OnBeforeBind(
            DefinitionKind kind,
            AnimationResourceDef source,
            AnimationResourceDef workingCopy)
        {
            _cmbId.SetItems(_lookupProvider.GetAnimationItems());
        }

        private void BuildUi()
        {
            _layout = new EditorLayoutPanel();
            _layout.Dock = DockStyle.Fill;
            _layout.Padding = new Padding(8);

            _cmbId = new DefinitionReferenceComboBox<AnimationId>();
            _cmbId.Dock = DockStyle.Fill;

            _txtName = new TextBox();
            _txtName.Dock = DockStyle.Fill;

            _txtSourcePath = new TextBox();
            _txtSourcePath.Dock = DockStyle.Fill;

            _numVersion = new NumericUpDown();
            _numVersion.Dock = DockStyle.Fill;
            _numVersion.Minimum = -1000000;
            _numVersion.Maximum = 1000000;

            _numFps = new NumericUpDown();
            _numFps.Dock = DockStyle.Fill;
            _numFps.Minimum = 0;
            _numFps.Maximum = 1000000;
            _numFps.DecimalPlaces = 3;
            _numFps.Increment = 0.001M;

            _numNumFrames = new NumericUpDown();
            _numNumFrames.Dock = DockStyle.Fill;
            _numNumFrames.Minimum = 0;
            _numNumFrames.Maximum = 1000000;

            _layout.AddRow("Id", _cmbId);
            _layout.AddRow("Name", _txtName);
            _layout.AddRow("Source Path", _txtSourcePath);
            _layout.AddRow("Version", _numVersion);
            _layout.AddRow("Fps", _numFps);
            _layout.AddRow("Num Frames", _numNumFrames);

            Controls.Clear();
            Controls.Add(_layout);
        }

        private void WireEvents()
        {
            _cmbId.SelectedIdChanged += OnAnyValueChanged;
            _txtName.TextChanged += OnAnyValueChanged;
            _txtSourcePath.TextChanged += OnAnyValueChanged;
            _numVersion.ValueChanged += OnAnyValueChanged;
            _numFps.ValueChanged += OnAnyValueChanged;
            _numNumFrames.ValueChanged += OnAnyValueChanged;
        }

        private void OnAnyValueChanged(object sender, EventArgs e)
        {
            if (_suppressEvents)
                return;

            MarkDirty();
        }

        protected override AnimationResourceDef CloneDefinition(AnimationResourceDef source)
        {
            AnimationResourceDef clone = new AnimationResourceDef();
            clone.Id = source.Id;
            clone.Name = source.Name;
            clone.SourcePath = source.SourcePath;
            clone.Version = source.Version;
            clone.Fps = source.Fps;
            clone.NumFrames = source.NumFrames;
            return clone;
        }

        protected override void CopyDefinition(AnimationResourceDef from, AnimationResourceDef to)
        {
            to.Id = from.Id;
            to.Name = from.Name;
            to.SourcePath = from.SourcePath;
            to.Version = from.Version;
            to.Fps = from.Fps;
            to.NumFrames = from.NumFrames;
        }

        protected override void LoadToControls(AnimationResourceDef value)
        {
            _suppressEvents = true;
            try
            {
                _cmbId.SetSelectedId(value.Id);
                _txtName.Text = value.Name ?? string.Empty;
                _txtSourcePath.Text = value.SourcePath ?? string.Empty;
                _numVersion.Value = ClampDecimal(value.Version, _numVersion);
                _numFps.Value = ClampDecimal((decimal)value.Fps, _numFps);
                _numNumFrames.Value = ClampDecimal(value.NumFrames, _numNumFrames);
            }
            finally
            {
                _suppressEvents = false;
            }
        }

        protected override void SaveFromControls(AnimationResourceDef value)
        {
            value.Id = _cmbId.GetSelectedId();
            value.Name = _txtName.Text ?? string.Empty;
            value.SourcePath = _txtSourcePath.Text ?? string.Empty;
            value.Version = Decimal.ToInt32(_numVersion.Value);
            value.Fps = (float)_numFps.Value;
            value.NumFrames = Decimal.ToInt32(_numNumFrames.Value);
        }

        protected override void ClearControls()
        {
            _suppressEvents = true;
            try
            {
                _cmbId.SelectedIndex = -1;
                _txtName.Text = string.Empty;
                _txtSourcePath.Text = string.Empty;
                _numVersion.Value = 0;
                _numFps.Value = 0;
                _numNumFrames.Value = 0;
            }
            finally
            {
                _suppressEvents = false;
            }
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