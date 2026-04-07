using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Newtonsoft.Json;
using WITH_ServerDataTool.Models;

namespace WITH_ServerDataTool.Presets
{
	internal sealed class PresetLoader
	{
		public AnimationPipelinePreset Load(string presetArgument)
		{
			string path = ResolvePresetPath(presetArgument);
			string baseDirectory = Path.GetDirectoryName(path);

			var dto = JsonConvert.DeserializeObject<AnimationPipelinePresetDto>(File.ReadAllText(path));
			if (dto == null)
			{
				throw new InvalidDataException("Failed to load animation pipeline preset.");
			}

			if (dto.SchemaVersion < 1 || dto.SchemaVersion > 2)
			{
				throw new InvalidDataException("Unsupported preset schemaVersion: " + dto.SchemaVersion);
			}

			return new AnimationPipelinePreset
			{
				SchemaVersion = dto.SchemaVersion,
				ObjectName = dto.ObjectName,
				MeshDirectory = ResolvePath(baseDirectory, dto.Inputs.MeshDirectory),
				AnimationDirectory = ResolvePath(baseDirectory, dto.Inputs.AnimationDirectory),
				CapsuleTemplateOutputPath = ResolvePath(baseDirectory, dto.Outputs.CapsuleTemplatePath),
				AnimationOutputDirectory = ResolvePath(baseDirectory, dto.Outputs.AnimationDirectory),
				DefaultRoles = dto.CapsuleRules.DefaultRoles.Select(ParseRole).ToList(),
				WeaponRoles = dto.CapsuleRules.WeaponRoles.Select(ParseRole).ToList(),
				WeaponBones = dto.CapsuleRules.WeaponBones ?? new List<int>(),
				Clips = dto.Clips ?? new List<AnimationClipPreset>()
			};
		}

		private static string ResolvePresetPath(string presetArgument)
		{
			if (File.Exists(presetArgument))
			{
				return Path.GetFullPath(presetArgument);
			}

			string localPresetPath = Path.GetFullPath(
				Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "..", "..", "Presets", presetArgument + ".json"));
			if (File.Exists(localPresetPath))
			{
				return localPresetPath;
			}

			throw new FileNotFoundException("Preset file was not found: " + presetArgument);
		}

		private static string ResolvePath(string baseDirectory, string path)
		{
			if (Path.IsPathRooted(path))
			{
				return path;
			}

			return Path.GetFullPath(Path.Combine(baseDirectory, path));
		}

		private static CapsuleRole ParseRole(string role)
		{
			switch ((role ?? string.Empty).Trim().ToLowerInvariant())
			{
				case "hurt": return CapsuleRole.Hurt;
				case "hit": return CapsuleRole.Hit;
				case "guard": return CapsuleRole.Guard;
				case "parry": return CapsuleRole.Parry;
				default:
					throw new InvalidDataException("Unsupported capsule role: " + role);
			}
		}
	}
}
