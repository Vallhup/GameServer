using System;
using System.Collections.Generic;
using WITH_ServerDataTool.ID;
using WITH_ServerDataTool.Utility;

namespace WITH_ServerDataTool.Definition
{  
    public struct ActionDefKey : IEquatable<ActionDefKey>
    {
        public CharacterId CharacterId { get; set; }
        public byte ActionLocalId { get; set; }

        public ActionDefKey(CharacterId characterId, byte actionLocalId)
        {
            CharacterId = characterId;
            ActionLocalId = actionLocalId;
        }

        public bool Equals(ActionDefKey other)
        {
            return
                CharacterId.Equals(other.CharacterId) &&
                ActionLocalId.Equals(other.ActionLocalId);
        }

        public override bool Equals(object obj)
        {
            if (!(obj is ActionDefKey)) return false;
            return Equals((ActionDefKey)obj);
        }

        public override int GetHashCode()
        {
            unchecked
            {
                return (CharacterId.GetHashCode() * 397) ^
                    ActionLocalId.GetHashCode();
            }
        }

        public override string ToString()
        {
            return CharacterId.ToString() + " : " + ActionLocalId.ToString();
        }
    }

    public sealed class ActionDef : IDefinition<ActionDefKey>
    {
        public ActionDefKey Id { get; set; }

        public ActionId ActionId { get; set; }
        public string Name { get; set; }

        public int Priority { get; set; }
        public float DurationSec { get; set; }
        public uint InterruptMask { get; set; }

        public bool CanMove { get; set; }
        public bool CanGuard { get; set; }
        public bool IsAttack { get; set; }

        public AnimationId AnimationId { get; set; }
        public bool AnimationLoop { get; set; }

        public List<ActionSegmentDef> Segments { get; set; }

        public ActionDef()
        {
            Name = string.Empty;
            Segments = new List<ActionSegmentDef>();
        }
    }

    public enum MoveModeId : byte
    {
        None = 0,
        FixedDistance = 1,
        DashToTarget = 2,
    }

    public enum YawModeId : byte
    {
        None = 0,
        FaceMoveDir = 1,
        FaceTarget = 2,
    }

    public enum VerticalModeId : byte
    {
        None = 0,
        FixedDeltaY = 1,
    }

    public sealed class MoveParamsDef
    {
        public float Distance { get; set; }
        public float MaxSpeed { get; set; }
        public float MaxTravel { get; set; }
        public float StopRange { get; set; }
        public bool LockDir { get; set; }
    }

    public sealed class YawParamsDef
    {
        public float TurnSpeedRad { get; set; }
        public float YawEpsRad { get; set; }
    }

    public sealed class VerticalParamsDef
    {
        public float DeltaY { get; set; }
    }


    public sealed class ActionSegmentDef
    {
        public float StartNormalizedTime { get; set; }
        public float EndNormalizedTime { get; set; }

        public MoveModeId MoveMode { get; set; }
        public MoveParamsDef MoveParams { get; set; }

        public YawModeId YawMode { get; set; }
        public YawParamsDef YawParams { get; set; }

        public VerticalModeId VerticalMode { get; set; }
        public VerticalParamsDef VerticalParams { get; set; }

        public ActionSegmentDef()
        {
            MoveMode = MoveModeId.None;
            MoveParams = new MoveParamsDef();

            YawMode = YawModeId.None;
            YawParams = new YawParamsDef();

            VerticalMode = VerticalModeId.None;
            VerticalParams = new VerticalParamsDef();
        }
    }
}
