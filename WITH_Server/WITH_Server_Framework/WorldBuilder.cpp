#include "pch.h"
#include "WorldBuilder.h"

Entity WorldBuilder::SpawnEmpty()
{
	RequireStage(BuildStage::SpawnInitial);
	return _rt.GetECS().CreateEntityImmediate();
}

void WorldBuilder::BeginInitialSpawns()
{
	RequireStage(BuildStage::Register);
	_rt.GetECS().FixStorages();
	_stage = BuildStage::SpawnInitial;
}

void WorldBuilder::Commit()
{
	if (_stage == BuildStage::Register)
		_stage = BuildStage::SpawnInitial;

	RequireStage(BuildStage::SpawnInitial);
	_stage = BuildStage::Finalized;
}

void WorldBuilder::RequireStage(BuildStage expected) const
{
	if (_stage != expected)
		std::runtime_error("BuildStage 맞지 않음");
}