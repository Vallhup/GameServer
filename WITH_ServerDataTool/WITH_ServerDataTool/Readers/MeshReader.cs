using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Numerics;
using System.Text.RegularExpressions;
using WITH_ServerDataTool.Models;

namespace WITH_ServerDataTool.Readers
{
	internal sealed class MeshReader
	{
		private static readonly Regex NumberRegex =
			new Regex(@"[-+]?(?:\d*\.\d+|\d+)(?:[eE][-+]?\d+)?", RegexOptions.Compiled);

		private static readonly Regex IntegerRegex =
			new Regex(@"\d+", RegexOptions.Compiled);

		public List<MeshVertex> ReadVertices(string path)
		{
			var vertices = new List<MeshVertex>();
			using (var reader = new StreamReader(path))
			{
				MeshVertex pending = null;
				string line;
				while ((line = reader.ReadLine()) != null)
				{
					line = line.Trim();
					if (line.StartsWith("Position:"))
					{
						var numbers = ExtractFloats(line);
						if (numbers.Length >= 3)
						{
							pending = pending ?? new MeshVertex();
							pending.Position = new Vector3(numbers[0], numbers[1], numbers[2]);
						}
					}
					else if (line.StartsWith("BoneIndices:"))
					{
						pending = pending ?? new MeshVertex();
						pending.BoneIndices = IntegerRegex.Matches(line)
							.Cast<Match>()
							.Select(match => int.Parse(match.Value, CultureInfo.InvariantCulture))
							.ToArray();
					}
					else if (line.StartsWith("BoneWeights:"))
					{
						pending = pending ?? new MeshVertex();
						pending.BoneWeights = ExtractFloats(line);
						vertices.Add(pending);
						pending = null;
					}
				}
			}

			return vertices;
		}

		private static float[] ExtractFloats(string line)
		{
			return NumberRegex.Matches(line)
				.Cast<Match>()
				.Select(match => float.Parse(match.Value, CultureInfo.InvariantCulture))
				.ToArray();
		}
	}
}
