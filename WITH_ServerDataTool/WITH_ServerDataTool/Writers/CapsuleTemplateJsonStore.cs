using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Numerics;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using WITH_ServerDataTool.Models;

namespace WITH_ServerDataTool.Writers
{
	internal sealed class CapsuleTemplateJsonStore
	{
		public CapsuleTemplateDocument Load(string path)
		{
			var root = JObject.Parse(File.ReadAllText(path));
			var document = new CapsuleTemplateDocument
			{
				Version = root.Value<int?>("version") ?? 1,
				ObjectName = root.Value<string>("objectName") ?? string.Empty,
				Units = root.Value<string>("units") ?? "cm",
				BuildSignature = root.Value<string>("buildSignature") ?? string.Empty,
				Capsules = new List<CapsuleTemplateEntry>()
			};

			foreach (var item in root["capsules"] ?? new JArray())
			{
				document.Capsules.Add(new CapsuleTemplateEntry
				{
					Bone = item.Value<int>("bone"),
					Radius = item.Value<float>("radius"),
					HalfHeight = item.Value<float>("halfHeight"),
					LocalCenter = ReadVector3((JArray)item["center"]),
					LocalDirection = ReadVector3((JArray)item["direction"])
				});
			}

			return document;
		}

		public void Save(string path, CapsuleTemplateDocument document)
		{
			Directory.CreateDirectory(Path.GetDirectoryName(path));

			var root = new JObject
			{
				["version"] = document.Version,
				["objectName"] = document.ObjectName,
				["units"] = document.Units,
				["buildSignature"] = document.BuildSignature ?? string.Empty,
				["capsules"] = new JArray(
					document.Capsules.Select(entry => new JObject
					{
						["bone"] = entry.Bone,
						["radius"] = entry.Radius,
						["halfHeight"] = entry.HalfHeight,
						["center"] = WriteVector3(entry.LocalCenter),
						["direction"] = WriteVector3(entry.LocalDirection)
					}))
			};

			File.WriteAllText(path, root.ToString(Formatting.Indented));
		}

		private static Vector3 ReadVector3(JArray array)
		{
			return new Vector3(
				array[0].Value<float>(),
				array[1].Value<float>(),
				array[2].Value<float>());
		}

		private static JArray WriteVector3(Vector3 vector)
		{
			return new JArray(vector.X, vector.Y, vector.Z);
		}
	}
}
