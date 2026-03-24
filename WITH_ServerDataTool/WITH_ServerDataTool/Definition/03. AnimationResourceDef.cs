using WITH_ServerDataTool.ID;
using WITH_ServerDataTool.Utility;

namespace WITH_ServerDataTool.Definition
{
    public sealed class AnimationResourceDef : IDefinition<AnimationId>
    {
        public AnimationId Id { get; set; }
        public string Name { get; set; }

        public string SourcePath { get; set; }

        public int Version { get; set; }
        public float Fps { get; set; }
        public int NumFrames { get; set; }

        public AnimationResourceDef()
        {
            Name = string.Empty;
            SourcePath = string.Empty;
        }
    }
}
