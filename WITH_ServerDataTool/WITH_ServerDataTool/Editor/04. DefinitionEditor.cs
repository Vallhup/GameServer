using System;
using System.Windows.Forms;

namespace WITH_ServerDataTool.Editor
{
    public interface IDefinitionEditor
    {
        Control EditorControl { get; }
        event EventHandler ValueChanged;

        void Bind(DefinitionKind kind, object value);
        void Clear();
        void Commit();
    }

    public abstract class DefinitionEditorBase<TDef> : UserControl, IDefinitionEditor
        where TDef : class
    {
        private DefinitionKind _kind;
        private TDef _source;
        private TDef _workingCopy;
        private bool _isDirty;
        private bool _suspendEvents;

        public event EventHandler ValueChanged;

        public Control EditorControl
        {
            get { return this; }
        }

        protected DefinitionKind BoundKind
        {
            get { return _kind; }
        }

        protected TDef SourceValue
        {
            get { return _source; }
        }

        protected TDef WorkingCopy
        {
            get { return _workingCopy; }
        }

        public bool IsDirty
        {
            get { return _isDirty; }
        }

        protected bool IsBinding
        {
            get { return _suspendEvents; }
        }

        public void Bind(DefinitionKind kind, object value)
        {
            if (value == null)
                throw new ArgumentNullException("value");

            TDef typed = value as TDef;
            if (typed == null)
            {
                throw new ArgumentException(
                    "Invalid definition type. Expected " + typeof(TDef).Name,
                    "value");
            }

            _kind = kind;
            _source = typed;
            _workingCopy = CloneDefinition(_source);
            _isDirty = false;

            SuspendEditorEvents();
            try
            {
                OnBeforeBind(kind, _source, _workingCopy);
                LoadToControls(_workingCopy);
                OnAfterBind(kind, _source, _workingCopy);
            }
            finally
            {
                ResumeEditorEvents();
            }

            UpdateControlState(true);
        }

        public void Clear()
        {
            _source = null;
            _workingCopy = null;
            _isDirty = false;

            SuspendEditorEvents();
            try
            {
                ClearControls();
            }
            finally
            {
                ResumeEditorEvents();
            }

            UpdateControlState(false);
        }

        public void Commit()
        {
            if (_source == null || _workingCopy == null)
                return;

            SuspendEditorEvents();
            try
            {
                SaveFromControls(_workingCopy);
                CopyDefinition(_workingCopy, _source);
                _workingCopy = CloneDefinition(_source);
                _isDirty = false;
                OnAfterCommit(_source);
            }
            finally
            {
                ResumeEditorEvents();
            }
        }

        protected void MarkDirty()
        {
            if (_suspendEvents)
                return;

            if (_workingCopy == null)
                return;

            _isDirty = true;

            EventHandler handler = ValueChanged;
            if (handler != null)
                handler(this, EventArgs.Empty);
        }

        protected void RefreshWorkingCopyFromControls()
        {
            if (_workingCopy == null)
                return;

            SaveFromControls(_workingCopy);
        }

        protected void ReloadControlsFromWorkingCopy()
        {
            SuspendEditorEvents();
            try
            {
                if (_workingCopy != null)
                    LoadToControls(_workingCopy);
                else
                    ClearControls();
            }
            finally
            {
                ResumeEditorEvents();
            }
        }

        protected void SuspendEditorEvents()
        {
            _suspendEvents = true;
        }

        protected void ResumeEditorEvents()
        {
            _suspendEvents = false;
        }

        protected virtual void OnBeforeBind(
            DefinitionKind kind,
            TDef source,
            TDef workingCopy)
        {
        }

        protected virtual void OnAfterBind(
            DefinitionKind kind,
            TDef source,
            TDef workingCopy)
        {
        }

        protected virtual void OnAfterCommit(TDef source)
        {
        }

        protected virtual void UpdateControlState(bool hasValue)
        {
            this.Enabled = hasValue;
        }

        protected abstract TDef CloneDefinition(TDef source);
        protected abstract void CopyDefinition(TDef from, TDef to);
        protected abstract void LoadToControls(TDef value);
        protected abstract void SaveFromControls(TDef value);
        protected abstract void ClearControls();
    }
}
