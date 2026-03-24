using System;
using System.Windows.Forms;

namespace WITH_ServerDataTool.Editor
{
    internal static class Program
    {
        [STAThread]
        private static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            IGameDataSerializer serializer = new GameDataJsonSerializer();
            EditorController controller = new EditorController(serializer);

            Application.Run(new MainForm(controller));
        }
    }
}