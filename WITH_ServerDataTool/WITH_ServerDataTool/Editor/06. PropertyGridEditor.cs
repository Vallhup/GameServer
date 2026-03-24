using System;
using System.Windows.Forms;

namespace WITH_ServerDataTool.Editor
{
    public sealed class PropertyGridEditor : UserControl, IDefinitionEditor
    {
        private readonly PropertyGrid _grid;

        public event EventHandler ValueChanged;

        public Control EditorControl
        {
            get { return this; }
        }

        public PropertyGridEditor()
        {
            _grid = new PropertyGrid();
            _grid.Dock = DockStyle.Fill;
            _grid.PropertyValueChanged += OnPropertyValueChanged;
            Controls.Add(_grid);
        }

        public void Bind(DefinitionKind kind, object value)
        {
            _grid.SelectedObject = value;
        }

        public void Clear()
        {
            _grid.SelectedObject = null;
        }

        public void Commit()
        {
            _grid.Refresh();
        }

        private void OnPropertyValueChanged(object s, PropertyValueChangedEventArgs e)
        {
            EventHandler handler = ValueChanged;
            if (handler != null)
                handler(this, EventArgs.Empty);
        }
    }
}