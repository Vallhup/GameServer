using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using WITH_ServerDataTool.Models;

namespace WITH_ServerDataTool.Builders
{
	internal sealed class AnimationClipBuilder
	{
		public AnimationClipDocumentV3 Build(
			BoneAnimationClipSource animation,
			CapsuleTemplateDocument template,
			IReadOnlyCollection<int> weaponBones,
			IReadOnlyCollection<int> excludedBones,
			IReadOnlyList<CapsuleRole> defaultRoles,
			IReadOnlyList<CapsuleRole> weaponRoles,
			string skeleton,
			string clipId,
			string source,
			bool loop)
		{
			var clip = new AnimationClipDocumentV3
			{
				ClipId = clipId,
				Skeleton = skeleton,
				Fps = animation.Fps,
				NumFrames = animation.FrameCount,
				DurationSec = animation.Fps > float.Epsilon
					? animation.FrameCount / animation.Fps
					: 0.0f,
				Loop = loop,
				Source = source
			};

			var excludedBoneSet = new HashSet<int>(excludedBones ?? Enumerable.Empty<int>());
			var sortedCapsules = template.Capsules
				.Where(entry => !excludedBoneSet.Contains(entry.Bone))
				.OrderBy(entry => entry.Bone)
				.ToList();

			foreach (var entry in sortedCapsules)
			{
				clip.Capsules.Add(new AnimationCapsuleDefinition
				{
					Bone = entry.Bone,
					Radius = entry.Radius,
					Roles = weaponBones.Contains(entry.Bone)
						? new List<CapsuleRole>(weaponRoles)
						: new List<CapsuleRole>(defaultRoles)
				});
			}

			foreach (var frame in animation.Frames)
			{
				var frameCapsules = new List<AnimationFrameCapsulePose>(sortedCapsules.Count);
				foreach (var entry in sortedCapsules)
				{
					Matrix4x4 matrix = frame[entry.Bone];
					Vector3 position = new Vector3(matrix.M14, matrix.M24, matrix.M34);
					Vector3 worldOffset = TransformDirection(matrix, entry.LocalCenter);
					Vector3 worldDirection = TransformDirection(matrix, entry.LocalDirection);

					if (worldDirection.LengthSquared() > float.Epsilon)
					{
						worldDirection = Vector3.Normalize(worldDirection);
					}

					Vector3 centerWorld = position + worldOffset;
					Vector3 delta = worldDirection * entry.HalfHeight;

					frameCapsules.Add(new AnimationFrameCapsulePose
					{
						P0 = centerWorld + delta,
						P1 = centerWorld - delta
					});
				}

				clip.Frames.Add(frameCapsules);
			}

			return clip;
		}

		private static Vector3 TransformDirection(Matrix4x4 matrix, Vector3 vector)
		{
			return new Vector3(
				matrix.M11 * vector.X + matrix.M12 * vector.Y + matrix.M13 * vector.Z,
				matrix.M21 * vector.X + matrix.M22 * vector.Y + matrix.M23 * vector.Z,
				matrix.M31 * vector.X + matrix.M32 * vector.Y + matrix.M33 * vector.Z);
		}
	}
}
