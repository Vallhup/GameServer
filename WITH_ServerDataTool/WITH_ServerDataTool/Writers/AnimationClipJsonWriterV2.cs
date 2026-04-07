using System.IO;
using System.Linq;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using WITH_ServerDataTool.Models;

namespace WITH_ServerDataTool.Writers
{
	internal sealed class AnimationClipJsonWriter
	{
		public void Save(string path, AnimationClipDocumentV3 document)
		{
			Directory.CreateDirectory(Path.GetDirectoryName(path));

			var root = new JObject
			{
				["version"] = document.Version,
				["clipId"] = document.ClipId,
				["skeleton"] = document.Skeleton,
				["fps"] = document.Fps,
				["numFrames"] = document.NumFrames,
				["durationSec"] = document.DurationSec,
				["units"] = document.Units,
				["loop"] = document.Loop,
				["source"] = document.Source,
				["capsules"] = new JArray(
					document.Capsules.Select(capsule => new JObject
					{
						["bone"] = capsule.Bone,
						["radius"] = capsule.Radius,
						["roles"] = new JArray(capsule.Roles.Select(RoleToString))
					})),
				["frames"] = new JArray(
					document.Frames.Select(frame => new JArray(
						frame.Select(capsule => new JObject
						{
							["p0"] = new JArray(capsule.P0.X, capsule.P0.Y, capsule.P0.Z),
							["p1"] = new JArray(capsule.P1.X, capsule.P1.Y, capsule.P1.Z)
						}))))
			};

			File.WriteAllText(path, root.ToString(Formatting.Indented));
		}

		private static string RoleToString(CapsuleRole role)
		{
			switch (role)
			{
				case CapsuleRole.Hurt: return "hurt";
				case CapsuleRole.Hit: return "hit";
				case CapsuleRole.Guard: return "guard";
				case CapsuleRole.Parry: return "parry";
				default: return "hurt";
			}
		}
	}
}
