using System.Collections.Generic;
using WITH_ServerDataTool.Definition;

namespace WITH_ServerDataTool.Utility
{
    public sealed class GameDataSet
    {
        public List<CharacterDef> Characters { get; set; }
        public List<ActionDef> Actions { get; set; }
        public List<AnimationResourceDef> Animations { get; set; }
        public List<BuffDef> Buffs { get; set; }
        public List<WorldDef> Worlds { get; set; }
        public List<SpawnSetDef> SpawnSets { get; set; }

        public GameDataSet()
        {
            Characters = new List<CharacterDef>();
            Actions = new List<ActionDef>();
            Animations = new List<AnimationResourceDef>();
            Buffs = new List<BuffDef>();
            Worlds = new List<WorldDef>();
            SpawnSets = new List<SpawnSetDef>();
        }
    }
}
