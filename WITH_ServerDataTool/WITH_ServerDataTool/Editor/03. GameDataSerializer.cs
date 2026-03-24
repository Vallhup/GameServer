using System;
using System.Collections.Generic;

using WITH_ServerDataTool.Definition;
using WITH_ServerDataTool.Utility;

using Newtonsoft.Json;
using Newtonsoft.Json.Converters;
using Newtonsoft.Json.Serialization;

namespace WITH_ServerDataTool.Editor
{
    public interface IGameDataSerializer
    {
        string Serialize(GameDataSet data);
        GameDataSet Deserialize(string text);
    }

    public sealed class GameDataJsonSerializer : IGameDataSerializer
    {
        private readonly JsonSerializerSettings _settings;

        public GameDataJsonSerializer()
        {
            _settings = new JsonSerializerSettings
            {
                Formatting = Formatting.Indented,
                NullValueHandling = NullValueHandling.Ignore,
                ContractResolver = new CamelCasePropertyNamesContractResolver()
            };

            _settings.Converters.Add(new StringEnumConverter());
        }

        public string Serialize(GameDataSet data)
        {
            if (data == null)
                throw new ArgumentNullException("data");

            return JsonConvert.SerializeObject(data, _settings);
        }

        public GameDataSet Deserialize(string text)
        {
            if (string.IsNullOrEmpty(text))
                throw new ArgumentException("text");

            GameDataSet data =
                JsonConvert.DeserializeObject<GameDataSet>(text, _settings);

            if (data == null)
                throw new InvalidOperationException("GameDataSet 역직렬화 결과가 null 입니다.");

            EnsureLists(data);
            return data;
        }

        private static void EnsureLists(GameDataSet data)
        {
            if (data.Characters == null) data.Characters = new List<CharacterDef>();
            if (data.Actions == null) data.Actions = new List<ActionDef>();
            if (data.Animations == null) data.Animations = new List<AnimationResourceDef>();
            if (data.Buffs == null) data.Buffs = new List<BuffDef>();
            if (data.Worlds == null) data.Worlds = new List<WorldDef>();
            if (data.SpawnSets == null) data.SpawnSets = new List<SpawnSetDef>();
        }
    }
}
