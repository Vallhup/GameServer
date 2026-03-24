using System.Collections.Generic;
using WITH_ServerDataTool.ID;
using WITH_ServerDataTool.Utility;

namespace WITH_ServerDataTool.Definition
{
    public sealed class SpawnSetDef : IDefinition<SpawnSetId>
    {
        public SpawnSetId Id { get; set; }
        public string Name { get; set; }

        public List<SpawnEntryDef> Entries { get; set; }

        public SpawnSetDef()
        {
            Name = string.Empty;
            Entries = new List<SpawnEntryDef>();
        }
    }

    public sealed class SpawnEntryDef
    {
        public CharacterId CharacterId { get; set; }

        public float X { get; set; }
        public float Y { get; set; }
        public float Z { get; set; }

        public float YawRad { get; set; }

        public bool IsPlayerSpawn { get; set; }
    }
}
