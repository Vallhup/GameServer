using System.Collections.Generic;
using System.Numerics;

namespace WITH_ServerDataTool.Models
{
	internal enum CapsuleRole
	{
		Hurt,
		Hit,
		Guard,
		Parry
	}

	internal sealed class MeshVertex
	{
		public Vector3 Position { get; set; }
		public int[] BoneIndices { get; set; } = new int[0];
		public float[] BoneWeights { get; set; } = new float[0];
	}

	internal sealed class CapsuleTemplateEntry
	{
		public int Bone { get; set; }
		public float Radius { get; set; }
		public float HalfHeight { get; set; }
		public Vector3 LocalCenter { get; set; }
		public Vector3 LocalDirection { get; set; }
	}

	internal sealed class CapsuleTemplateDocument
	{
		public int Version { get; set; } = 1;
		public string ObjectName { get; set; } = string.Empty;
		public string Units { get; set; } = "cm";
		public string BuildSignature { get; set; } = string.Empty;
		public List<CapsuleTemplateEntry> Capsules { get; set; } = new List<CapsuleTemplateEntry>();
	}

	internal sealed class BoneAnimationClipSource
	{
		public string Name { get; set; } = string.Empty;
		public float Fps { get; set; }
		public int BoneCount { get; set; }
		public int FrameCount { get; set; }
		public List<Matrix4x4[]> Frames { get; set; } = new List<Matrix4x4[]>();
	}

	internal sealed class AnimationCapsuleDefinition
	{
		public int Bone { get; set; }
		public float Radius { get; set; }
		public List<CapsuleRole> Roles { get; set; } = new List<CapsuleRole>();
	}

	internal sealed class AnimationFrameCapsulePose
	{
		public Vector3 P0 { get; set; }
		public Vector3 P1 { get; set; }
	}

	internal sealed class AnimationClipDocumentV2
	{
		public int Version { get; set; } = 2;
		public float Fps { get; set; }
		public int NumFrames { get; set; }
		public List<AnimationCapsuleDefinition> Capsules { get; set; } = new List<AnimationCapsuleDefinition>();
		public List<List<AnimationFrameCapsulePose>> Frames { get; set; } = new List<List<AnimationFrameCapsulePose>>();
	}

	internal sealed class AnimationClipDocumentV3
	{
		public int Version { get; set; } = 3;
		public string ClipId { get; set; } = string.Empty;
		public string Skeleton { get; set; } = string.Empty;
		public float Fps { get; set; }
		public int NumFrames { get; set; }
		public float DurationSec { get; set; }
		public string Units { get; set; } = "cm";
		public bool Loop { get; set; }
		public string Source { get; set; } = string.Empty;
		public List<AnimationCapsuleDefinition> Capsules { get; set; } = new List<AnimationCapsuleDefinition>();
		public List<List<AnimationFrameCapsulePose>> Frames { get; set; } = new List<List<AnimationFrameCapsulePose>>();
	}
}
