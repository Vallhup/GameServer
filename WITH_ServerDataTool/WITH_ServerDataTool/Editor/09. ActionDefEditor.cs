using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;
using WITH_ServerDataTool.Editor;
using WITH_ServerDataTool.ID;
using WITH_ServerDataTool.Utility;

namespace WITH_ServerDataTool.Definition
{
    public sealed class ActionDefEditor : DefinitionEditorBase<ActionDef>
    {
        private readonly IActionEditorLookupProvider _lookupProvider;

        private EditorLayoutPanel _layout;

        private DefinitionReferenceComboBox<CharacterId> _cmbCharacterId;
        private NumericUpDown _numActionDefId;

        private DefinitionReferenceComboBox<ActionId> _cmbActionId;
        private TextBox _txtName;

        private NumericUpDown _numPriority;
        private NumericUpDown _numDurationSec;
        private NumericUpDown _numInterruptMask;

        private CheckBox _chkCanMove;
        private CheckBox _chkCanGuard;
        private CheckBox _chkIsAttack;

        private DefinitionReferenceComboBox<AnimationId> _cmbAnimationId;
        private CheckBox _chkAnimationLoop;

        private TableLayoutPanel _segmentLayout;
        private Panel _segmentListPanel;
        private ListBox _lstSegments;
        private TableLayoutPanel _segmentButtonPanel;
        private Button _btnAddSegment;
        private Button _btnRemoveSegment;
        private Button _btnMoveUpSegment;
        private Button _btnMoveDownSegment;
        private ActionSegmentDefEditor _segmentEditor;

        private bool _suppressEvents;

        public ActionDefEditor(IActionEditorLookupProvider lookupProvider)
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

            _cmbCharacterId = new DefinitionReferenceComboBox<CharacterId>();
            _cmbCharacterId.Dock = DockStyle.Fill;

            _numActionDefId = new NumericUpDown();
            _numActionDefId.Dock = DockStyle.Fill;
            _numActionDefId.Minimum = 0;
            _numActionDefId.Maximum = 255;

            _cmbActionId = new DefinitionReferenceComboBox<ActionId>();
            _cmbActionId.Dock = DockStyle.Fill;

            _txtName = new TextBox();
            _txtName.Dock = DockStyle.Fill;

            _numPriority = new NumericUpDown();
            _numPriority.Dock = DockStyle.Fill;
            _numPriority.Minimum = -1000000;
            _numPriority.Maximum = 1000000;

            _numDurationSec = new NumericUpDown();
            _numDurationSec.Dock = DockStyle.Fill;
            _numDurationSec.Minimum = 0;
            _numDurationSec.Maximum = 1000000;
            _numDurationSec.DecimalPlaces = 3;
            _numDurationSec.Increment = 0.001M;

            _numInterruptMask = new NumericUpDown();
            _numInterruptMask.Dock = DockStyle.Fill;
            _numInterruptMask.Minimum = 0;
            _numInterruptMask.Maximum = uint.MaxValue;

            _chkCanMove = CreateCheckBox();
            _chkCanGuard = CreateCheckBox();
            _chkIsAttack = CreateCheckBox();

            _cmbAnimationId = new DefinitionReferenceComboBox<AnimationId>();
            _cmbAnimationId.Dock = DockStyle.Fill;

            _chkAnimationLoop = CreateCheckBox();

            BuildSegmentUi();

            _layout.AddRow("Character Id", _cmbCharacterId);
            _layout.AddRow("Action Local Id", _numActionDefId);
            _layout.AddRow("Action Id", _cmbActionId);
            _layout.AddRow("Name", _txtName);
            _layout.AddRow("Priority", _numPriority);
            _layout.AddRow("Duration Sec", _numDurationSec);
            _layout.AddRow("Interrupt Mask", _numInterruptMask);
            _layout.AddRow("Can Move", _chkCanMove);
            _layout.AddRow("Can Guard", _chkCanGuard);
            _layout.AddRow("Is Attack", _chkIsAttack);
            _layout.AddRow("Animation Id", _cmbAnimationId);
            _layout.AddRow("Animation Loop", _chkAnimationLoop);
            _layout.AddRow("Segments", _segmentLayout);

            Controls.Clear();
            Controls.Add(_layout);
        }

        private void BuildSegmentUi()
        {
            _segmentLayout = new TableLayoutPanel();
            _segmentLayout.Dock = DockStyle.Fill;
            _segmentLayout.Margin = new Padding(0);
            _segmentLayout.Padding = new Padding(0);
            _segmentLayout.ColumnCount = 2;
            _segmentLayout.RowCount = 1;
            _segmentLayout.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 220F));
            _segmentLayout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100F));
            _segmentLayout.RowStyles.Add(new RowStyle(SizeType.Percent, 100F));

            _segmentListPanel = new Panel();
            _segmentListPanel.Dock = DockStyle.Fill;
            _segmentListPanel.Margin = new Padding(0, 0, 6, 0);

            _lstSegments = new ListBox();
            _lstSegments.Dock = DockStyle.Fill;

            _segmentButtonPanel = new TableLayoutPanel();
            _segmentButtonPanel.Dock = DockStyle.Bottom;
            _segmentButtonPanel.Height = 34;
            _segmentButtonPanel.Margin = new Padding(0);
            _segmentButtonPanel.Padding = new Padding(0);
            _segmentButtonPanel.ColumnCount = 4;
            _segmentButtonPanel.RowCount = 1;
            _segmentButtonPanel.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 25F));
            _segmentButtonPanel.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 25F));
            _segmentButtonPanel.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 25F));
            _segmentButtonPanel.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 25F));
            _segmentButtonPanel.RowStyles.Add(new RowStyle(SizeType.Percent, 100F));

            _btnAddSegment = new Button();
            _btnAddSegment.Text = "Add";
            _btnAddSegment.Dock = DockStyle.Fill;
            _btnAddSegment.Margin = new Padding(0, 0, 3, 0);

            _btnRemoveSegment = new Button();
            _btnRemoveSegment.Text = "Remove";
            _btnRemoveSegment.Dock = DockStyle.Fill;
            _btnRemoveSegment.Margin = new Padding(0, 0, 3, 0);

            _btnMoveUpSegment = new Button();
            _btnMoveUpSegment.Text = "Up";
            _btnMoveUpSegment.Dock = DockStyle.Fill;
            _btnMoveUpSegment.Margin = new Padding(0, 0, 3, 0);

            _btnMoveDownSegment = new Button();
            _btnMoveDownSegment.Text = "Down";
            _btnMoveDownSegment.Dock = DockStyle.Fill;
            _btnMoveDownSegment.Margin = new Padding(0);

            _segmentButtonPanel.Controls.Add(_btnAddSegment, 0, 0);
            _segmentButtonPanel.Controls.Add(_btnRemoveSegment, 1, 0);
            _segmentButtonPanel.Controls.Add(_btnMoveUpSegment, 2, 0);
            _segmentButtonPanel.Controls.Add(_btnMoveDownSegment, 3, 0);

            _segmentListPanel.Controls.Add(_lstSegments);
            _segmentListPanel.Controls.Add(_segmentButtonPanel);

            _segmentEditor = new ActionSegmentDefEditor();
            _segmentEditor.Dock = DockStyle.Fill;
            _segmentEditor.Margin = new Padding(0);

            _segmentLayout.Controls.Add(_segmentListPanel, 0, 0);
            _segmentLayout.Controls.Add(_segmentEditor, 1, 0);
        }

        private static CheckBox CreateCheckBox()
        {
            CheckBox checkBox = new CheckBox();
            checkBox.Text = "Enabled";
            checkBox.Dock = DockStyle.Left;
            checkBox.AutoSize = true;
            return checkBox;
        }

        private void WireEvents()
        {
            _cmbCharacterId.SelectedIdChanged += OnAnyValueChanged;
            _numActionDefId.ValueChanged += OnAnyValueChanged;
            _cmbActionId.SelectedIdChanged += OnAnyValueChanged;
            _txtName.TextChanged += OnAnyValueChanged;
            _numPriority.ValueChanged += OnAnyValueChanged;
            _numDurationSec.ValueChanged += OnAnyValueChanged;
            _numInterruptMask.ValueChanged += OnAnyValueChanged;
            _chkCanMove.CheckedChanged += OnAnyValueChanged;
            _chkCanGuard.CheckedChanged += OnAnyValueChanged;
            _chkIsAttack.CheckedChanged += OnAnyValueChanged;
            _cmbAnimationId.SelectedIdChanged += OnAnyValueChanged;
            _chkAnimationLoop.CheckedChanged += OnAnyValueChanged;

            _btnAddSegment.Click += OnAddSegmentClicked;
            _btnRemoveSegment.Click += OnRemoveSegmentClicked;
            _btnMoveUpSegment.Click += OnMoveUpSegmentClicked;
            _btnMoveDownSegment.Click += OnMoveDownSegmentClicked;
            _lstSegments.SelectedIndexChanged += OnSegmentSelectionChanged;
            _segmentEditor.ValueChanged += OnSegmentEditorValueChanged;
        }

        private void OnAnyValueChanged(object sender, EventArgs e)
        {
            if (_suppressEvents)
                return;

            MarkDirty();
        }

        private void OnAddSegmentClicked(object sender, EventArgs e)
        {
            if (WorkingCopy == null)
                return;

            if (WorkingCopy.Segments == null)
                WorkingCopy.Segments = new List<ActionSegmentDef>();

            ActionSegmentDef seg = new ActionSegmentDef();
            WorkingCopy.Segments.Add(seg);

            RefreshSegmentList();
            _lstSegments.SelectedIndex = WorkingCopy.Segments.Count - 1;

            MarkDirty();
        }

        private void OnRemoveSegmentClicked(object sender, EventArgs e)
        {
            if (WorkingCopy == null || WorkingCopy.Segments == null)
                return;

            int index = _lstSegments.SelectedIndex;
            if (index < 0 || index >= WorkingCopy.Segments.Count)
                return;

            WorkingCopy.Segments.RemoveAt(index);

            RefreshSegmentList();

            if (WorkingCopy.Segments.Count > 0)
                _lstSegments.SelectedIndex = Math.Min(index, WorkingCopy.Segments.Count - 1);
            else
                _segmentEditor.SetValue(null);

            MarkDirty();
        }

        private void OnMoveUpSegmentClicked(object sender, EventArgs e)
        {
            if (WorkingCopy == null || WorkingCopy.Segments == null)
                return;

            int index = _lstSegments.SelectedIndex;
            if (index <= 0)
                return;

            ActionSegmentDef temp = WorkingCopy.Segments[index - 1];
            WorkingCopy.Segments[index - 1] = WorkingCopy.Segments[index];
            WorkingCopy.Segments[index] = temp;

            RefreshSegmentList();
            _lstSegments.SelectedIndex = index - 1;

            MarkDirty();
        }

        private void OnMoveDownSegmentClicked(object sender, EventArgs e)
        {
            if (WorkingCopy == null || WorkingCopy.Segments == null)
                return;

            int index = _lstSegments.SelectedIndex;
            if (index < 0 || index >= WorkingCopy.Segments.Count - 1)
                return;

            ActionSegmentDef temp = WorkingCopy.Segments[index + 1];
            WorkingCopy.Segments[index + 1] = WorkingCopy.Segments[index];
            WorkingCopy.Segments[index] = temp;

            RefreshSegmentList();
            _lstSegments.SelectedIndex = index + 1;

            MarkDirty();
        }

        private void OnSegmentSelectionChanged(object sender, EventArgs e)
        {
            if (WorkingCopy == null || WorkingCopy.Segments == null)
            {
                _segmentEditor.SetValue(null);
                return;
            }

            int index = _lstSegments.SelectedIndex;
            if (index < 0 || index >= WorkingCopy.Segments.Count)
            {
                _segmentEditor.SetValue(null);
                return;
            }

            _segmentEditor.SetValue(WorkingCopy.Segments[index]);
        }

        private void OnSegmentEditorValueChanged(object sender, EventArgs e)
        {
            int index = _lstSegments.SelectedIndex;
            if (index < 0)
                return;

            RefreshSegmentList(index);
            MarkDirty();
        }

        protected override void OnBeforeBind(
            DefinitionKind kind,
            ActionDef source,
            ActionDef workingCopy)
        {
            _cmbCharacterId.SetItems(_lookupProvider.GetCharacterItems());
            _cmbActionId.SetItems(_lookupProvider.GetActionItems());
            _cmbAnimationId.SetItems(_lookupProvider.GetAnimationItems());
        }

        protected override ActionDef CloneDefinition(ActionDef source)
        {
            ActionDef clone = new ActionDef();

            clone.Id = new ActionDefKey(source.Id.CharacterId, source.Id.ActionLocalId);
            clone.ActionId = source.ActionId;
            clone.Name = source.Name;
            clone.Priority = source.Priority;
            clone.DurationSec = source.DurationSec;
            clone.InterruptMask = source.InterruptMask;
            clone.CanMove = source.CanMove;
            clone.CanGuard = source.CanGuard;
            clone.IsAttack = source.IsAttack;
            clone.AnimationId = source.AnimationId;
            clone.AnimationLoop = source.AnimationLoop;

            clone.Segments = new List<ActionSegmentDef>();
            if (source.Segments != null)
            {
                for (int i = 0; i < source.Segments.Count; ++i)
                    clone.Segments.Add(CloneSegment(source.Segments[i]));
            }

            return clone;
        }

        protected override void CopyDefinition(ActionDef from, ActionDef to)
        {
            to.Id = new ActionDefKey(from.Id.CharacterId, from.Id.ActionLocalId);
            to.ActionId = from.ActionId;
            to.Name = from.Name;
            to.Priority = from.Priority;
            to.DurationSec = from.DurationSec;
            to.InterruptMask = from.InterruptMask;
            to.CanMove = from.CanMove;
            to.CanGuard = from.CanGuard;
            to.IsAttack = from.IsAttack;
            to.AnimationId = from.AnimationId;
            to.AnimationLoop = from.AnimationLoop;

            if (to.Segments == null)
                to.Segments = new List<ActionSegmentDef>();
            else
                to.Segments.Clear();

            if (from.Segments != null)
            {
                for (int i = 0; i < from.Segments.Count; ++i)
                    to.Segments.Add(CloneSegment(from.Segments[i]));
            }
        }

        protected override void LoadToControls(ActionDef value)
        {
            _suppressEvents = true;
            try
            {
                _cmbCharacterId.SetSelectedId(value.Id.CharacterId);
                _numActionDefId.Value = value.Id.ActionLocalId;

                _cmbActionId.SetSelectedId(value.ActionId);
                _txtName.Text = value.Name ?? string.Empty;

                _numPriority.Value = ClampDecimal(value.Priority, _numPriority);
                _numDurationSec.Value = ClampDecimal((decimal)value.DurationSec, _numDurationSec);
                _numInterruptMask.Value = ClampDecimal(value.InterruptMask, _numInterruptMask);

                _chkCanMove.Checked = value.CanMove;
                _chkCanGuard.Checked = value.CanGuard;
                _chkIsAttack.Checked = value.IsAttack;

                _cmbAnimationId.SetSelectedId(value.AnimationId);
                _chkAnimationLoop.Checked = value.AnimationLoop;

                RefreshSegmentList();

                if (value.Segments != null && value.Segments.Count > 0)
                    _lstSegments.SelectedIndex = 0;
                else
                    _segmentEditor.SetValue(null);
            }
            finally
            {
                _suppressEvents = false;
            }
        }

        protected override void SaveFromControls(ActionDef value)
        {
            value.Id = new ActionDefKey(
                _cmbCharacterId.GetSelectedId(),
                (byte)_numActionDefId.Value);

            value.ActionId = _cmbActionId.GetSelectedId();
            value.Name = _txtName.Text ?? string.Empty;
            value.Priority = Decimal.ToInt32(_numPriority.Value);
            value.DurationSec = (float)_numDurationSec.Value;
            value.InterruptMask = Decimal.ToUInt32(_numInterruptMask.Value);
            value.CanMove = _chkCanMove.Checked;
            value.CanGuard = _chkCanGuard.Checked;
            value.IsAttack = _chkIsAttack.Checked;
            value.AnimationId = _cmbAnimationId.GetSelectedId();
            value.AnimationLoop = _chkAnimationLoop.Checked;
        }

        protected override void ClearControls()
        {
            _suppressEvents = true;
            try
            {
                _cmbCharacterId.SelectedIndex = -1;
                _numActionDefId.Value = 0;
                _cmbActionId.SelectedIndex = -1;
                _txtName.Text = string.Empty;
                _numPriority.Value = 0;
                _numDurationSec.Value = 0;
                _numInterruptMask.Value = 0;
                _chkCanMove.Checked = false;
                _chkCanGuard.Checked = false;
                _chkIsAttack.Checked = false;
                _cmbAnimationId.SelectedIndex = -1;
                _chkAnimationLoop.Checked = false;
                _lstSegments.Items.Clear();
                _segmentEditor.SetValue(null);
            }
            finally
            {
                _suppressEvents = false;
            }
        }

        private void RefreshSegmentList()
        {
            int selectedIndex = _lstSegments.SelectedIndex;
            RefreshSegmentList(selectedIndex);
        }

        private void RefreshSegmentList(int preferredIndex)
        {
            _lstSegments.BeginUpdate();
            try
            {
                _lstSegments.Items.Clear();

                if (WorkingCopy == null || WorkingCopy.Segments == null)
                {
                    _segmentEditor.SetValue(null);
                    return;
                }

                for (int i = 0; i < WorkingCopy.Segments.Count; ++i)
                    _lstSegments.Items.Add(BuildSegmentDisplayText(i, WorkingCopy.Segments[i]));

                if (WorkingCopy.Segments.Count == 0)
                {
                    _segmentEditor.SetValue(null);
                    return;
                }

                if (preferredIndex < 0)
                    preferredIndex = 0;
                if (preferredIndex >= WorkingCopy.Segments.Count)
                    preferredIndex = WorkingCopy.Segments.Count - 1;

                _lstSegments.SelectedIndex = preferredIndex;
            }
            finally
            {
                _lstSegments.EndUpdate();
            }
        }

        private static string BuildSegmentDisplayText(int index, ActionSegmentDef seg)
        {
            if (seg == null)
                return index.ToString() + " : <null>";

            return string.Format(
                "#{0} [{1:0.###} ~ {2:0.###}] M={3}, Y={4}, V={5}",
                index,
                seg.StartNormalizedTime,
                seg.EndNormalizedTime,
                seg.MoveMode,
                seg.YawMode,
                seg.VerticalMode);
        }

        private static ActionSegmentDef CloneSegment(ActionSegmentDef source)
        {
            ActionSegmentDef clone = new ActionSegmentDef();

            if (source == null)
                return clone;

            clone.StartNormalizedTime = source.StartNormalizedTime;
            clone.EndNormalizedTime = source.EndNormalizedTime;

            clone.MoveMode = source.MoveMode;
            clone.YawMode = source.YawMode;
            clone.VerticalMode = source.VerticalMode;

            clone.MoveParams = new MoveParamsDef();
            if (source.MoveParams != null)
            {
                clone.MoveParams.Distance = source.MoveParams.Distance;
                clone.MoveParams.MaxSpeed = source.MoveParams.MaxSpeed;
                clone.MoveParams.MaxTravel = source.MoveParams.MaxTravel;
                clone.MoveParams.StopRange = source.MoveParams.StopRange;
                clone.MoveParams.LockDir = source.MoveParams.LockDir;
            }

            clone.YawParams = new YawParamsDef();
            if (source.YawParams != null)
            {
                clone.YawParams.TurnSpeedRad = source.YawParams.TurnSpeedRad;
                clone.YawParams.YawEpsRad = source.YawParams.YawEpsRad;
            }

            clone.VerticalParams = new VerticalParamsDef();
            if (source.VerticalParams != null)
            {
                clone.VerticalParams.DeltaY = source.VerticalParams.DeltaY;
            }

            return clone;
        }

        private static decimal ClampDecimal(int value, NumericUpDown input)
        {
            return ClampDecimal((decimal)value, input);
        }

        private static decimal ClampDecimal(uint value, NumericUpDown input)
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