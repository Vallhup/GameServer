using System.Collections.Generic;
using WITH_ServerDataTool.ID;
using WITH_ServerDataTool.Utility;

namespace WITH_ServerDataTool.Definition
{
    public enum HitboxType : byte
    {
        None = 0,
        Hurt = 1 << 0,
        Hit = 1 << 1,
    }

    public sealed class DynamicCapsuleDataDef
    {
        public float P0X { get; set; }
        public float P0Y { get; set; }
        public float P0Z { get; set; }

        public float P1X { get; set; }
        public float P1Y { get; set; }
        public float P1Z { get; set; }
    }

    public sealed class StaticCapsuleDataDef
    {
        public byte Bone { get; set; }
        public float Radius { get; set; }
        public HitboxType TypeMask { get; set; }
    }

    public sealed class AnimationFrameDef
    {
        public List<DynamicCapsuleDataDef> Capsules { get; set; }

        public AnimationFrameDef()
        {
            Capsules = new List<DynamicCapsuleDataDef>();
        }
    }

    public sealed class AnimationResourceDef : IDefinition<AnimationId>
    {
        public AnimationId Id { get; set; }
        public string Name { get; set; }

        public string SourcePath { get; set; }

        public int Version { get; set; }
        public float Fps { get; set; }
        public int NumFrames { get; set; }

        public List<StaticCapsuleDataDef> StaticCapsules { get; set; }
        public List<AnimationFrameDef> Frames { get; set; }

        public AnimationResourceDef()
        {
            Name = string.Empty;
            SourcePath = string.Empty;
            StaticCapsules = new List<StaticCapsuleDataDef>();
            Frames = new List<AnimationFrameDef>();
        }
    }
}
