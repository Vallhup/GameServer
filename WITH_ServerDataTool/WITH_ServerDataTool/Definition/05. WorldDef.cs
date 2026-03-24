using System.Collections.Generic;
using WITH_ServerDataTool.ID;
using WITH_ServerDataTool.Utility;

namespace WITH_ServerDataTool.Definition
{
    public sealed class WorldDef : IDefinition<WorldId>
    {
        public WorldId Id { get; set; }
        public string Name { get; set; }

        public MapDef Map { get; set; }
        public SpawnSetId SpawnSetId { get; set; }
        
        public WorldDef()
        {
            Name = string.Empty;
            Map = new MapDef();
        }
    }

    public sealed class MapDef
    {
        public string CollisionMapPath { get; set; }
        public string HeightMapPath { get; set; }

        public int CollisionWidth { get; set; }
        public int CollisionHeight { get; set; }

        public float WorldSize { get; set; }
        public float HeightScale { get; set; }

        public MapDef()
        {
            CollisionMapPath = string.Empty;
            HeightMapPath = string.Empty;
        }
    }
}
