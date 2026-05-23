using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using WITH_ServerDataTool.Builders;
using WITH_ServerDataTool.Models;
using WITH_ServerDataTool.Presets;
using WITH_ServerDataTool.Readers;
using WITH_ServerDataTool.Writers;

namespace WITH_ServerDataTool.Cli
{
	internal static class AnimationPipelineCli
	{
		public static int Run(string[] args)
		{
			if (args.Length == 0 || HasFlag(args, "--help") || HasFlag(args, "-h"))
			{
				PrintHelp();
				return 0;
			}

			string command = args[0].Trim().ToLowerInvariant();
			var options = ParseOptions(args.Skip(1).ToArray());

			switch (command)
			{
				case "extract-capsules":
					ExecuteExtractCapsules(GetRequiredOption(options, "preset"));
					return 0;
				case "prebake-animation":
					ExecutePrebakeAnimation(
						GetRequiredOption(options, "preset"),
						GetRequiredOption(options, "clip"));
					return 0;
				case "build-all":
					ExecuteBuildAll(GetRequiredOption(options, "preset"));
					return 0;
				default:
					Console.Error.WriteLine("Unknown command: " + command);
					PrintHelp();
					return 1;
			}
		}

		private static void ExecuteExtractCapsules(string presetArgument)
		{
			var preset = new PresetLoader().Load(presetArgument);
			var builder = new CapsuleTemplateBuilder();
			var writer = new CapsuleTemplateJsonStore();

			CapsuleTemplateDocument template = builder.Build(
				preset.ObjectName,
				preset.MeshDirectory,
				preset.ExtremeTrimFraction);
			writer.Save(preset.CapsuleTemplateOutputPath, template);

			Console.WriteLine("Capsule template written: " + preset.CapsuleTemplateOutputPath);
			Console.WriteLine("Capsules: " + template.Capsules.Count);
		}

		private static void ExecutePrebakeAnimation(string presetArgument, string clipName)
		{
			var preset = new PresetLoader().Load(presetArgument);
			var clip = ResolveClip(preset, clipName);

			EnsureCapsuleTemplateExists(preset);

			var templateStore = new CapsuleTemplateJsonStore();
			var template = templateStore.Load(preset.CapsuleTemplateOutputPath);

			var bonePath = Path.Combine(preset.AnimationDirectory, clip.Source);
			var animation = new BoneAnimationReader().Read(bonePath);
			var document = new AnimationClipBuilder().Build(
				animation,
				template,
				preset.WeaponBones,
				preset.ExcludedBones,
				preset.DefaultRoles,
				preset.WeaponRoles,
				preset.ObjectName,
				BuildClipId(preset.ObjectName, clip.Name),
				clip.Source,
				clip.Loop);

			string outputPath = Path.Combine(preset.AnimationOutputDirectory, clip.Output);
			new AnimationClipJsonWriter().Save(outputPath, document);

			Console.WriteLine("Animation clip written: " + outputPath);
			Console.WriteLine("Frames: " + document.NumFrames);
			Console.WriteLine("Capsules: " + document.Capsules.Count);
		}

		private static void ExecuteBuildAll(string presetArgument)
		{
			var preset = new PresetLoader().Load(presetArgument);
			EnsureCapsuleTemplateExists(preset);

			foreach (var clip in EnumerateClips(preset))
			{
				ExecutePrebakeAnimation(presetArgument, clip.Name);
			}
		}

		private static void EnsureCapsuleTemplateExists(AnimationPipelinePreset preset)
		{
			var store = new CapsuleTemplateJsonStore();
			string expectedSignature = CapsuleTemplateBuilder.ComputeBuildSignature(
				preset.MeshDirectory,
				preset.ExtremeTrimFraction);

			if (File.Exists(preset.CapsuleTemplateOutputPath) && !string.IsNullOrEmpty(expectedSignature))
			{
				try
				{
					var cached = store.Load(preset.CapsuleTemplateOutputPath);
					if (string.Equals(cached.BuildSignature, expectedSignature, StringComparison.Ordinal))
					{
						return;
					}

					Console.WriteLine("Capsule template signature mismatch - rebuilding: " + preset.CapsuleTemplateOutputPath);
				}
				catch (Exception ex)
				{
					Console.WriteLine("Failed to load cached capsule template, rebuilding (" + ex.GetType().Name + ": " + ex.Message + ")");
				}
			}

			var builder = new CapsuleTemplateBuilder();
			CapsuleTemplateDocument template = builder.Build(
				preset.ObjectName,
				preset.MeshDirectory,
				preset.ExtremeTrimFraction);
			store.Save(preset.CapsuleTemplateOutputPath, template);
		}

		private static AnimationClipPreset ResolveClip(AnimationPipelinePreset preset, string clipName)
		{
			var clip = EnumerateClips(preset).FirstOrDefault(
				entry =>
					string.Equals(entry.Name, clipName, StringComparison.OrdinalIgnoreCase) ||
					string.Equals(Path.GetFileNameWithoutExtension(entry.Source), clipName, StringComparison.OrdinalIgnoreCase) ||
					string.Equals(entry.Source, clipName, StringComparison.OrdinalIgnoreCase));

			if (clip == null)
			{
				throw new InvalidOperationException("Clip was not found in preset: " + clipName);
			}

			return clip;
		}

		private static Dictionary<string, string> ParseOptions(string[] args)
		{
			var options = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
			for (int i = 0; i < args.Length; i++)
			{
				string arg = args[i];
				if (!arg.StartsWith("--"))
				{
					continue;
				}

				string key = arg.Substring(2);
				string value = (i + 1 < args.Length && !args[i + 1].StartsWith("--"))
					? args[++i]
					: "true";

				options[key] = value;
			}

			return options;
		}

		private static bool HasFlag(string[] args, string flag)
		{
			return args.Any(arg => string.Equals(arg, flag, StringComparison.OrdinalIgnoreCase));
		}

		private static string GetRequiredOption(Dictionary<string, string> options, string key)
		{
			if (!options.TryGetValue(key, out string value) || string.IsNullOrWhiteSpace(value))
			{
				throw new InvalidOperationException("Missing required option: --" + key);
			}

			return value;
		}

		private static void PrintHelp()
		{
			Console.WriteLine("WITH_ServerDataTool Animation Pipeline");
			Console.WriteLine("Commands:");
			Console.WriteLine("  extract-capsules --preset <preset-name-or-path>");
			Console.WriteLine("  prebake-animation --preset <preset-name-or-path> --clip <clip-name>");
			Console.WriteLine("  build-all --preset <preset-name-or-path>");
		}

		private static string BuildClipId(string objectName, string clipName)
		{
			return objectName + "_" + clipName;
		}

		private static List<AnimationClipPreset> EnumerateClips(AnimationPipelinePreset preset)
		{
			if (!Directory.Exists(preset.AnimationDirectory))
			{
				throw new DirectoryNotFoundException("Animation directory was not found: " + preset.AnimationDirectory);
			}

			var overridesBySource = preset.Clips
				.Where(entry => !string.IsNullOrWhiteSpace(entry.Source))
				.GroupBy(entry => entry.Source, StringComparer.OrdinalIgnoreCase)
				.ToDictionary(group => group.Key, group => group.Last(), StringComparer.OrdinalIgnoreCase);

			var clips = new List<AnimationClipPreset>();
			foreach (var path in Directory.GetFiles(preset.AnimationDirectory, "*.bone").OrderBy(path => path, StringComparer.OrdinalIgnoreCase))
			{
				string sourceFileName = Path.GetFileName(path);
				if (overridesBySource.TryGetValue(sourceFileName, out var overrideClip))
				{
					clips.Add(new AnimationClipPreset
					{
						Name = string.IsNullOrWhiteSpace(overrideClip.Name)
							? DeriveClipName(preset.ObjectName, sourceFileName)
							: overrideClip.Name,
						Source = sourceFileName,
						Output = string.IsNullOrWhiteSpace(overrideClip.Output)
							? DeriveOutputFileName(preset.ObjectName, sourceFileName)
							: overrideClip.Output,
						Loop = overrideClip.Loop
					});
					continue;
				}

				clips.Add(new AnimationClipPreset
				{
					Name = DeriveClipName(preset.ObjectName, sourceFileName),
					Source = sourceFileName,
					Output = DeriveOutputFileName(preset.ObjectName, sourceFileName),
					Loop = false
				});
			}

			return clips;
		}

		private static string DeriveClipName(string objectName, string sourceFileName)
		{
			string clipBaseName = Path.GetFileNameWithoutExtension(sourceFileName)
				.Replace("_baked", string.Empty);

			string normalizedObjectName = NormalizeObjectName(objectName);
			string[] removablePrefixes =
			{
				"monster_" + normalizedObjectName + "_",
				normalizedObjectName + "_animation_",
				normalizedObjectName + "_",
				"boss_animation_",
				"boss_",
				"knight_animation_",
				"knight_"
			};

			foreach (var prefix in removablePrefixes)
			{
				if (clipBaseName.StartsWith(prefix, StringComparison.OrdinalIgnoreCase))
				{
					return clipBaseName.Substring(prefix.Length);
				}
			}

			return clipBaseName;
		}

		private static string DeriveOutputFileName(string objectName, string sourceFileName)
		{
			return NormalizeObjectName(objectName) + "_animation_" + DeriveClipName(objectName, sourceFileName) + ".json";
		}

		private static string NormalizeObjectName(string objectName)
		{
			if (string.IsNullOrWhiteSpace(objectName))
			{
				return "unknown";
			}

			var buffer = new System.Text.StringBuilder(objectName.Length + 8);
			for (int i = 0; i < objectName.Length; i++)
			{
				char ch = objectName[i];
				if (char.IsUpper(ch) && i > 0 && objectName[i - 1] != '_')
				{
					buffer.Append('_');
				}

				buffer.Append(char.ToLowerInvariant(ch));
			}

			return buffer.ToString();
		}
	}
}
