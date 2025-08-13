#include "pch.h"
#include "IComponent.h"

void IComponent::SetEnable(bool e)
{
	if (_enable != e) {
		_enable = e;
		e ? OnActivate() : OnDeactivate();
	}
}