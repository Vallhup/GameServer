using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Numerics;
using WITH_ServerDataTool.Models;

namespace WITH_ServerDataTool.Readers
{
	internal sealed class BoneAnimationReader
	{
		public BoneAnimationClipSource Read(string path)
		{
			var lines = File.ReadAllLines(path)
				.Select(line => line.Trim())
				.ToList();

			var index = 0;
			Require(lines[index].Contains("BAKED_ANIMATION"), "Invalid bone file header.");
			index++;

			string animationName = ReadValue(lines[index++]);
			int boneCount = int.Parse(ReadValue(lines[index++]), CultureInfo.InvariantCulture);
			int frameCount = int.Parse(ReadValue(lines[index++]), CultureInfo.InvariantCulture);
			index++; // Duration
			float fps = float.Parse(ReadValue(lines[index++]), CultureInfo.InvariantCulture);

			while (index < lines.Count && lines[index] != "---")
			{
				index++;
			}

			index++;

			var frames = new List<Matrix4x4[]>(frameCount);
			for (int frame = 0; frame < frameCount; frame++)
			{
				index = SkipBlank(lines, index);
				Require(lines[index].StartsWith("Frame["), "Missing frame marker.");
				index++;

				var boneMatrices = new Matrix4x4[boneCount];
				for (int bone = 0; bone < boneCount; bone++)
				{
					index = SkipBlank(lines, index);
					Require(lines[index].StartsWith("Bone["), "Missing bone marker.");
					index++;

					var r0 = ParseMatrixRow(lines[index++]);
					var r1 = ParseMatrixRow(lines[index++]);
					var r2 = ParseMatrixRow(lines[index++]);
					var r3 = ParseMatrixRow(lines[index++]);

					var matrix = new Matrix4x4(
						r0[0], r0[1], r0[2], r0[3],
						r1[0], r1[1], r1[2], r1[3],
						r2[0], r2[1], r2[2], r2[3],
						r3[0], r3[1], r3[2], r3[3]);

					boneMatrices[bone] = Matrix4x4.Transpose(matrix);
				}

				frames.Add(boneMatrices);
			}

			return new BoneAnimationClipSource
			{
				Name = animationName,
				Fps = fps,
				BoneCount = boneCount,
				FrameCount = frameCount,
				Frames = frames
			};
		}

		private static string ReadValue(string line)
		{
			int separator = line.IndexOf(':');
			if (separator < 0)
			{
				throw new InvalidDataException("Invalid key/value line.");
			}

			return line.Substring(separator + 1).Trim();
		}

		private static float[] ParseMatrixRow(string line)
		{
			return line.Split(new[] { ' ' }, StringSplitOptions.RemoveEmptyEntries)
				.Select(value => float.Parse(value, CultureInfo.InvariantCulture))
				.ToArray();
		}

		private static int SkipBlank(IReadOnlyList<string> lines, int index)
		{
			while (index < lines.Count && string.IsNullOrWhiteSpace(lines[index]))
			{
				index++;
			}

			return index;
		}

		private static void Require(bool condition, string message)
		{
			if (!condition)
			{
				throw new InvalidDataException(message);
			}
		}
	}
}
