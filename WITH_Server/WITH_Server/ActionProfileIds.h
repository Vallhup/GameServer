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
	inline constexpr CharacterActionProfileId DemonStriker = 1004;
	inline constexpr CharacterActionProfileId DemonExecutioner = 1005;
}

namespace ActionInputBindingProfileIds
{
	inline constexpr ActionInputBindingProfileId Knight = 2001;
	inline constexpr ActionInputBindingProfileId Imp = 2002;
	inline constexpr ActionInputBindingProfileId FinalBoss = 2003;
	inline constexpr ActionInputBindingProfileId DemonStriker = 2004;
	inline constexpr ActionInputBindingProfileId DemonExecutioner = 2005;
}

namespace ActionFallbackReactionProfileIds
{
	inline constexpr ActionFallbackReactionProfileId Knight = 3001;
	inline constexpr ActionFallbackReactionProfileId Imp = 3002;
	inline constexpr ActionFallbackReactionProfileId FinalBoss = 3003;
	inline constexpr ActionFallbackReactionProfileId DemonStriker = 3004;
	inline constexpr ActionFallbackReactionProfileId DemonExecutioner = 3005;
}

namespace AnimationBindingProfileIds
{
	inline constexpr AnimationBindingProfileId Knight = 4001;
	inline constexpr AnimationBindingProfileId Imp = 4002;
	inline constexpr AnimationBindingProfileId FinalBoss = 4003;
	inline constexpr AnimationBindingProfileId DemonStriker = 4004;
	inline constexpr AnimationBindingProfileId DemonExecutioner = 4005;
}
