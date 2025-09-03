#include "pch.h"
#include "IComponent.h"

IComponent::IComponent(GameObject& owner, Instance* instance)
	: _owner(owner), _instance(instance), _version(0), _lastSentVersion(0)
{
}

bool IComponent::VersionCheckAndChange()
{
	if (_version != _lastSentVersion) {
		_lastSentVersion = _version;
		return true;
	}

	return false;
}