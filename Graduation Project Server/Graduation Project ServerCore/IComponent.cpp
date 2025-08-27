#include "pch.h"
#include "IComponent.h"

bool IComponent::VersionCheckAndChange()
{
	if (_version != _lastSentVersion) {
		_lastSentVersion = _version;
		return true;
	}

	return false;
}
void IComponent::SetEnable(bool e)
{
	if (_enable != e) {
		_enable = e;
		e ? OnActivate() : OnDeactivate();
	}
}