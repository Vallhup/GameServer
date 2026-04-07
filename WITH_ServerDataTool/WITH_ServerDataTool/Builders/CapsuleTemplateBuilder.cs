using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Numerics;
using WITH_ServerDataTool.Models;
using WITH_ServerDataTool.Readers;

namespace WITH_ServerDataTool.Builders
{
	internal sealed class CapsuleTemplateBuilder
	{
		private readonly MeshReader _meshReader = new MeshReader();

		public CapsuleTemplateDocument Build(string objectName, string meshDirectory)
		{
			if (!Directory.Exists(meshDirectory))
			{
				throw new DirectoryNotFoundException("Mesh directory was not found: " + meshDirectory);
			}

			var groupedByBone = new Dictionary<int, List<Vector3>>();
			foreach (string meshPath in Directory.GetFiles(meshDirectory, "*.mesh"))
			{
				foreach (var vertex in _meshReader.ReadVertices(meshPath))
				{
					if (vertex.BoneIndices.Length == 0 || vertex.BoneWeights.Length == 0)
					{
						continue;
					}

					int dominantBone = SelectDominantBone(vertex.BoneIndices, vertex.BoneWeights);
					if (!groupedByBone.TryGetValue(dominantBone, out var points))
					{
						points = new List<Vector3>();
						groupedByBone.Add(dominantBone, points);
					}

					points.Add(vertex.Position);
				}
			}

			var entries = new List<CapsuleTemplateEntry>();
			foreach (var pair in groupedByBone.OrderBy(pair => pair.Key))
			{
				if (pair.Value.Count < 5)
				{
					continue;
				}

				ComputePrincipalAxis(pair.Value, out Vector3 mean, out Vector3 axis);
				ComputeEndpoints(pair.Value, mean, axis, out Vector3 p0, out Vector3 p1);

				float radius = ComputeRadius(pair.Value, p0, p1);
				Vector3 center = (p0 + p1) * 0.5f;
				Vector3 direction = Vector3.Normalize(p1 - p0);
				float halfHeight = Vector3.Distance(p0, p1) * 0.5f;

				entries.Add(new CapsuleTemplateEntry
				{
					Bone = pair.Key,
					Radius = radius,
					HalfHeight = halfHeight,
					LocalCenter = center,
					LocalDirection = direction
				});
			}

			return new CapsuleTemplateDocument
			{
				ObjectName = objectName,
				Capsules = entries
			};
		}

		private static int SelectDominantBone(int[] boneIndices, float[] boneWeights)
		{
			int bestIndex = 0;
			float bestWeight = float.MinValue;
			int length = Math.Min(boneIndices.Length, boneWeights.Length);
			for (int index = 0; index < length; index++)
			{
				if (boneWeights[index] > bestWeight)
				{
					bestWeight = boneWeights[index];
					bestIndex = index;
				}
			}

			return boneIndices[bestIndex];
		}

		private static void ComputePrincipalAxis(IReadOnlyList<Vector3> points, out Vector3 mean, out Vector3 axis)
		{
			mean = Vector3.Zero;
			foreach (var point in points)
			{
				mean += point;
			}

			mean /= points.Count;

			double c00 = 0.0;
			double c01 = 0.0;
			double c02 = 0.0;
			double c11 = 0.0;
			double c12 = 0.0;
			double c22 = 0.0;

			foreach (var point in points)
			{
				Vector3 delta = point - mean;
				c00 += delta.X * delta.X;
				c01 += delta.X * delta.Y;
				c02 += delta.X * delta.Z;
				c11 += delta.Y * delta.Y;
				c12 += delta.Y * delta.Z;
				c22 += delta.Z * delta.Z;
			}

			Vector3 vector = Vector3.Normalize(new Vector3(1.0f, 1.0f, 1.0f));
			for (int iteration = 0; iteration < 16; iteration++)
			{
				Vector3 next = new Vector3(
					(float)(c00 * vector.X + c01 * vector.Y + c02 * vector.Z),
					(float)(c01 * vector.X + c11 * vector.Y + c12 * vector.Z),
					(float)(c02 * vector.X + c12 * vector.Y + c22 * vector.Z));

				if (next.LengthSquared() <= float.Epsilon)
				{
					break;
				}

				vector = Vector3.Normalize(next);
			}

			axis = vector;
		}

		private static void ComputeEndpoints(
			IReadOnlyList<Vector3> points,
			Vector3 mean,
			Vector3 axis,
			out Vector3 p0,
			out Vector3 p1)
		{
			float minProjection = float.MaxValue;
			float maxProjection = float.MinValue;

			foreach (var point in points)
			{
				float projection = Vector3.Dot(point - mean, axis);
				minProjection = Math.Min(minProjection, projection);
				maxProjection = Math.Max(maxProjection, projection);
			}

			p0 = mean + axis * minProjection;
			p1 = mean + axis * maxProjection;
		}

		private static float ComputeRadius(IReadOnlyList<Vector3> points, Vector3 p0, Vector3 p1)
		{
			float radius = 0.0f;
			foreach (var point in points)
			{
				radius = Math.Max(radius, PointSegmentDistance(point, p0, p1));
			}

			return radius;
		}

		private static float PointSegmentDistance(Vector3 point, Vector3 p0, Vector3 p1)
		{
			Vector3 segment = p1 - p0;
			float denominator = Vector3.Dot(segment, segment);
			if (denominator <= float.Epsilon)
			{
				return Vector3.Distance(point, p0);
			}

			float t = Vector3.Dot(point - p0, segment) / denominator;
			t = Math.Max(0.0f, Math.Min(1.0f, t));
			Vector3 closest = p0 + segment * t;
			return Vector3.Distance(point, closest);
		}
	}
}
