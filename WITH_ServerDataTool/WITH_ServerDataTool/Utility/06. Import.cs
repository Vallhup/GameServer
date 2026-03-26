using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.IO;
using WITH_ServerDataTool.Definition;
using WITH_ServerDataTool.ID;

namespace WITH_ServerDataTool.Utility
{
    public sealed class ParsedAnimationDto
    {
        public int Version { get; set; }
        public float Fps { get; set; }
        public int NumFrames { get; set; }
        public List<ParsedCapsuleMetaDto> Capsules { get; set; }
        public List<List<ParsedCapsulePoseDto>> Frames { get; set; }

        public ParsedAnimationDto()
        {
            Capsules = new List<ParsedCapsuleMetaDto>();
            Frames = new List<List<ParsedCapsulePoseDto>>();
        }
    }

    public sealed class ParsedCapsuleMetaDto
    {
        public int Bone { get; set; }
        public float Radius { get; set; }
        public List<string> Roles { get; set; }

        public ParsedCapsuleMetaDto()
        {
            Roles = new List<string>();
        }
    }

    public sealed class ParsedCapsulePoseDto
    {
        public float[] P0 { get; set; }
        public float[] P1 { get; set; }

        public ParsedCapsulePoseDto()
        {
            P0 = new float[3];
            P1 = new float[3];
        }
    }

    public sealed class AnimationImportService
    {
        public AnimationResourceDef ImportFromParsedJson(
            string jsonPath,
            AnimationId id,
            string name,
            string sourcePath)
        {
            if (string.IsNullOrEmpty(jsonPath))
                throw new ArgumentException("jsonPath");

            if (!File.Exists(jsonPath))
                throw new FileNotFoundException("파서 결과 파일을 찾을 수 없습니다.", jsonPath);

            string json = File.ReadAllText(jsonPath);
            ParsedAnimationDto dto =
                JsonConvert.DeserializeObject<ParsedAnimationDto>(json);

            if (dto == null)
                throw new InvalidOperationException("애니메이션 JSON 역직렬화에 실패했습니다.");

            ValidateDto(dto, jsonPath);

            AnimationResourceDef def = new AnimationResourceDef();
            def.Id = id;
            def.Name = name ?? string.Empty;
            def.SourcePath = sourcePath ?? string.Empty;
            def.Version = dto.Version;
            def.Fps = dto.Fps;
            def.NumFrames = dto.NumFrames;

            for (int i = 0; i < dto.Capsules.Count; ++i)
            {
                ParsedCapsuleMetaDto src = dto.Capsules[i];

                StaticCapsuleDataDef cap = new StaticCapsuleDataDef();
                cap.Bone = checked((byte)src.Bone);

                // C++ 런타임 로더와 동일하게 0.01 스케일 적용
                cap.Radius = src.Radius * 0.01f;
                cap.TypeMask = ToHitboxMask(src.Roles);

                def.StaticCapsules.Add(cap);
            }

            for (int frameIndex = 0; frameIndex < dto.Frames.Count; ++frameIndex)
            {
                List<ParsedCapsulePoseDto> srcFrame = dto.Frames[frameIndex];

                if (srcFrame.Count != def.StaticCapsules.Count)
                    throw new InvalidOperationException(
                        "프레임 캡슐 수가 정적 캡슐 수와 다릅니다. frame=" + frameIndex);

                AnimationFrameDef frame = new AnimationFrameDef();

                for (int i = 0; i < srcFrame.Count; ++i)
                {
                    ParsedCapsulePoseDto srcPose = srcFrame[i];

                    DynamicCapsuleDataDef pose = new DynamicCapsuleDataDef();
                    pose.P0X = srcPose.P0[0] * 0.01f;
                    pose.P0Y = srcPose.P0[1] * 0.01f;
                    pose.P0Z = srcPose.P0[2] * 0.01f;

                    pose.P1X = srcPose.P1[0] * 0.01f;
                    pose.P1Y = srcPose.P1[1] * 0.01f;
                    pose.P1Z = srcPose.P1[2] * 0.01f;

                    frame.Capsules.Add(pose);
                }

                def.Frames.Add(frame);
            }

            return def;
        }

        private static void ValidateDto(ParsedAnimationDto dto, string jsonPath)
        {
            if (dto.Version != 2)
                throw new InvalidOperationException(
                    "지원하지 않는 애니메이션 버전입니다: " + dto.Version + ", file=" + jsonPath);

            if (dto.Fps <= 0.0f)
                throw new InvalidOperationException("FPS가 0 이하입니다.");

            if (dto.NumFrames <= 0 || dto.NumFrames > 255)
                throw new InvalidOperationException("잘못된 프레임 수입니다: " + dto.NumFrames);

            if (dto.Capsules == null)
                throw new InvalidOperationException("capsules가 없습니다.");

            if (dto.Frames == null)
                throw new InvalidOperationException("frames가 없습니다.");

            if (dto.Frames.Count != dto.NumFrames)
                throw new InvalidOperationException(
                    "NumFrames와 frames.Count가 일치하지 않습니다.");

            for (int i = 0; i < dto.Capsules.Count; ++i)
            {
                ParsedCapsuleMetaDto cap = dto.Capsules[i];

                if (cap.Bone < 0 || cap.Bone > 255)
                    throw new InvalidOperationException("잘못된 bone 인덱스입니다: " + cap.Bone);

                if (cap.Radius < 0.0f)
                    throw new InvalidOperationException("radius가 음수입니다.");

                if (cap.Roles == null)
                    throw new InvalidOperationException("roles가 null입니다.");
            }

            for (int frameIndex = 0; frameIndex < dto.Frames.Count; ++frameIndex)
            {
                List<ParsedCapsulePoseDto> frame = dto.Frames[frameIndex];

                if (frame == null)
                    throw new InvalidOperationException("frame이 null입니다: " + frameIndex);

                if (frame.Count != dto.Capsules.Count)
                    throw new InvalidOperationException(
                        "프레임 캡슐 수 불일치입니다. frame=" + frameIndex);

                for (int i = 0; i < frame.Count; ++i)
                {
                    ParsedCapsulePoseDto pose = frame[i];
                    if (pose.P0 == null || pose.P0.Length != 3)
                        throw new InvalidOperationException("p0 길이가 3이 아닙니다.");
                    if (pose.P1 == null || pose.P1.Length != 3)
                        throw new InvalidOperationException("p1 길이가 3이 아닙니다.");
                }
            }
        }

        private static HitboxType ToHitboxMask(List<string> roles)
        {
            HitboxType mask = HitboxType.None;

            for (int i = 0; i < roles.Count; ++i)
            {
                string role = roles[i];
                if (string.Equals(role, "hurt", StringComparison.OrdinalIgnoreCase))
                    mask |= HitboxType.Hurt;
                else if (string.Equals(role, "hit", StringComparison.OrdinalIgnoreCase))
                    mask |= HitboxType.Hit;
            }

            return mask;
        }
    }
}