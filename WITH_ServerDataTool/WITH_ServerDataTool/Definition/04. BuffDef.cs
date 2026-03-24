using System.Collections.Generic;
using WITH_ServerDataTool.ID;
using WITH_ServerDataTool.Utility;

namespace WITH_ServerDataTool.Definition
{
    public sealed class BuffDef : IDefinition<BuffId>
    {
        public BuffId Id { get; set; }
        public string Name { get; set; }

        public BuffPolicyId Policy { get; set; }
        public List<BuffModifierDef> Modifiers { get; set; }

        public BuffDef()
        {
            Name = string.Empty;
            Policy = BuffPolicyId.None;
            Modifiers = new List<BuffModifierDef>();
        }
    }

    public sealed class BuffModifierDef
    {
        public BuffEffectId Effect { get; set; }
        public StatId StatId { get; set; }
        public float Value { get; set; }
    }

    public enum BuffPolicyId : byte
    {
        None = 0,
        Duration = 1,
        Stacks = 2,
    }

    public enum BuffEffectId : byte
    {
        None = 0,
        AddStat = 1,
        MulStat = 2,
    }

    public enum StatId : byte
    {
        None = 0,
        MaxHp = 1,
        MaxStamina = 2,
        Attack = 3,
        AttackSpeed = 4,
        Defence = 5,
        MoveSpeed = 6,
    }
}
