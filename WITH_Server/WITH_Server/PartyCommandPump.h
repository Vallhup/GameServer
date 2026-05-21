#pragma once

#include <vector>

#include "FrameworkRuntime.h"
#include "PartyCommandQueue.h"
#include "PartyService.h"

class PartyCommandPump final
{
public:
	PartyCommandPump(
		PartyCommandQueue& queue,
		PartyService& partyService,
		FrameworkRuntime& framework);

	void Pump(double nowSec);

private:
	void ApplyCommand(const PartyCommand& command, double nowSec);
	void BeginWorldEntry(const PartyCommand& command, double nowSec);

private:
	PartyCommandQueue& _queue;
	PartyService& _partyService;
	FrameworkRuntime& _framework;
	std::vector<PartyCommand> _scratch;
};
