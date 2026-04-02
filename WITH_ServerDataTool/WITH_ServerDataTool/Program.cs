using System;
using WITH_ServerDataTool.Cli;

namespace WITH_ServerDataTool
{
	internal static class Program
	{
		[STAThread]
		private static int Main(string[] args)
		{
			try
			{
				return AnimationPipelineCli.Run(args);
			}
			catch (Exception ex)
			{
				Console.Error.WriteLine(ex.Message);
				return 1;
			}
		}
	}
}
