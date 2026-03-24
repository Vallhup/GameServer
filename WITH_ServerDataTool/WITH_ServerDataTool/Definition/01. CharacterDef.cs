using System.Collections.Generic;
using WITH_ServerDataTool.ID;
using WITH_ServerDataTool.Utility;

namespace WITH_ServerDataTool.Definition
{
    public sealed class CharacterDef : IDefinition<CharacterId>
    {
        public CharacterId Id { get; set; }
        public string Name { get; set; }

        public EntityCategoryId CategoryId { get; set; }
        public FactionId DefaultFactionId { get; set; }
        public AIArchetypeId AIArchetypeId { get; set; }

        public List<BuffId> DefaultBuffIds { get; set; }

        public CharacterDef()
        {
            Name = string.Empty;
            DefaultBuffIds = new List<BuffId>();
        }
    }
}
