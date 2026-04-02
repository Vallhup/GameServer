using System.Collections.Generic;

namespace WITH_ServerDataTool.Models
{
	internal sealed class AnimationClipPreset
	{
		public string Name { get; set; } = string.Empty;
		public string Source { get; set; } = string.Empty;
		public string Output { get; set; } = string.Empty;
		public bool Loop { get; set; }
	}

	internal sealed class AnimationPipelineInputPathsDto
	{
		public string MeshDirectory { get; set; } = string.Empty;
		public string AnimationDirectory { get; set; } = string.Empty;
	}

	internal sealed class AnimationPipelineOutputPathsDto
	{
		public string CapsuleTemplatePath { get; set; } = string.Empty;
		public string AnimationDirectory { get; set; } = string.Empty;
	}

	internal sealed class AnimationPipelineCapsuleRulesDto
	{
		public List<string> DefaultRoles { get; set; } = new List<string>();
		public List<string> WeaponRoles { get; set; } = new List<string>();
		public List<int> WeaponBones { get; set; } = new List<int>();
	}

	internal sealed class AnimationPipelinePreset
	{
		public int SchemaVersion { get; set; } = 2;
		public string ObjectName { get; set; } = string.Empty;
		public string MeshDirectory { get; set; } = string.Empty;
		public string AnimationDirectory { get; set; } = string.Empty;
		public string CapsuleTemplateOutputPath { get; set; } = string.Empty;
		public string AnimationOutputDirectory { get; set; } = string.Empty;
		public List<CapsuleRole> DefaultRoles { get; set; } = new List<CapsuleRole>();
		public List<CapsuleRole> WeaponRoles { get; set; } = new List<CapsuleRole>();
		public List<int> WeaponBones { get; set; } = new List<int>();
		public List<AnimationClipPreset> Clips { get; set; } = new List<AnimationClipPreset>();
	}

	internal sealed class AnimationPipelinePresetDto
	{
		public int SchemaVersion { get; set; } = 2;
		public string ObjectName { get; set; } = string.Empty;
		public AnimationPipelineInputPathsDto Inputs { get; set; } = new AnimationPipelineInputPathsDto();
		public AnimationPipelineOutputPathsDto Outputs { get; set; } = new AnimationPipelineOutputPathsDto();
		public AnimationPipelineCapsuleRulesDto CapsuleRules { get; set; } = new AnimationPipelineCapsuleRulesDto();
		public List<AnimationClipPreset> Clips { get; set; } = new List<AnimationClipPreset>();
	}
}
