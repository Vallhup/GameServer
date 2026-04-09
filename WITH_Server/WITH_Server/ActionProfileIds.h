#pragma once

#include <cstdint>

using CharacterActionProfileId = uint16_t;
using ActionInputBindingProfileId = uint16_t;
using ActionFallbackReactionProfileId = uint16_t;
using AnimationBindingProfileId = uint16_t;

struct CharacterActionDefRef
{
	CharacterActionProfileId actionProfileId{ 0 };
};

namespace CharacterActionProfileIds
{
	inline constexpr CharacterActionProfileId Knight = 1001;
	inline constexpr CharacterActionProfileId Imp = 1002;
	inline constexpr CharacterActionProfileId FinalBoss = 1003;
}

namespace ActionInputBindingProfileIds
{
	inline constexpr ActionInputBindingProfileId Knight = 2001;
	inline constexpr ActionInputBindingProfileId Imp = 2002;
	inline constexpr ActionInputBindingProfileId FinalBoss = 2003;
}

namespace ActionFallbackReactionProfileIds
{
	inline constexpr ActionFallbackReactionProfileId Knight = 3001;
	inline constexpr ActionFallbackReactionProfileId Imp = 3002;
	inline constexpr ActionFallbackReactionProfileId FinalBoss = 3003;
}

namespace AnimationBindingProfileIds
{
	inline constexpr AnimationBindingProfileId Knight = 4001;
	inline constexpr AnimationBindingProfileId Imp = 4002;
	inline constexpr AnimationBindingProfileId FinalBoss = 4003;
}
