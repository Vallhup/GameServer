#include "pch.h"
#include "WorldInstance.h"

WorldInstance::WorldInstance(WorldInstanceCreateParams params)
	: _identity(params.identity)
	, _def(params.def)
	, _executionModel(std::move(params.executionModel))
	, _runtime(WorldRuntimeCreateParams{
		_def,
		&_executionModel,
		params.transferProfile,
		params.transferBinding
		})
	, _impl(std::move(params.impl))
{
}

bool WorldInstance::Initialize()
{
	if (_initialized)
		return true;

	if (_shutdown)
		return false;

	if (!_runtime.Initialize())
		return false;

	if (_impl)
	{
		if (!_impl->OnCreate(_runtime))
			return false;

		_runtime.FixStorages();

		if (!_impl->OnStart(_runtime))
			return false;
	}
	else
	{
		_runtime.FixStorages();
	}

	if (!_runtime.BeginFrame(0, 0.0, 0.0))
		return false;

	if (!_runtime.FlushFrameCommands())
		return false;

	if (!_runtime.FlushLifecycleCommands())
		return false;

	_initialized = true;
	return true;
}

void WorldInstance::Shutdown()
{
	if (_shutdown)
		return;

	if (_impl && _initialized)
	{
		_impl->OnStop(_runtime);
	}

	_runtime.Shutdown();
	_shutdown = true;
}
