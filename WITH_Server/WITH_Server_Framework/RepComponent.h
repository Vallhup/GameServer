#pragma once

#include "NetId.h"
#include "Component.h"
#include "EntityType.h"

struct NetIdComp : Component
{
	NetId id;
};

struct WorldIdComp : Component
{
	WorldId id;
};

struct SpawnTypeComp : Component
{
	EntityType type;
};

struct ReplicatedTag : TagComponent { };