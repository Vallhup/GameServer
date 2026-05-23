#include "pch.h"
#include "GameplayContentCatalog.h"

const AbilityDef* GameplayContentCatalogSnapshot::FindAbilityByKey(
	std::string_view key) const noexcept
{
	for (const AbilityDef& ability : _abilities.GetAll())
	{
		if (ability.key == key)
			return &ability;
	}

	return nullptr;
}

const AttributeDef* GameplayContentCatalogSnapshot::FindAttributeByKey(
	std::string_view key) const noexcept
{
	for (const AttributeDef& attribute : _attributes.GetAll())
	{
		if (attribute.key == key)
			return &attribute;
	}

	return nullptr;
}

const AbilitySetDef* GameplayContentCatalogSnapshot::FindAbilitySetByKey(
	std::string_view key) const noexcept
{
	for (const AbilitySetDef& set : _abilitySets.abilitySets.GetAll())
	{
		if (set.key == key)
			return &set;
	}

	return nullptr;
}

const GameplayEffectDef* GameplayContentCatalogSnapshot::FindEffectByKey(
	std::string_view key) const noexcept
{
	for (const GameplayEffectDef& effect : _effects.GetAll())
	{
		if (effect.key == key)
			return &effect;
	}

	return nullptr;
}

const ProjectileDef* GameplayContentCatalogSnapshot::FindProjectileByKey(
	std::string_view key) const noexcept
{
	for (const ProjectileDef& projectile : _projectiles.GetAll())
	{
		if (projectile.key == key)
			return &projectile;
	}

	return nullptr;
}

const AreaHitDef* GameplayContentCatalogSnapshot::FindAreaHitByKey(
	std::string_view key) const noexcept
{
	for (const AreaHitDef& areaHit : _areaHits.GetAll())
	{
		if (areaHit.key == key)
			return &areaHit;
	}

	return nullptr;
}

DefLoadResult GameplayContentCatalogSnapshot::LoadFromRoots(const GameplayContentCatalogRoots& roots)
{
	DefLoadResult result =
		LoadAttributeDefsFromJsonDirectory(roots.attributeRoot, _attributes);
	if (!result.succeeded)
	{
		result.error = "Attribute catalog load failed: " + result.error;
		return result;
	}

	result = LoadGameplayTagDefsFromJsonDirectory(roots.tagRoot, _tags);
	if (!result.succeeded)
	{
		result.error = "Gameplay tag catalog load failed: " + result.error;
		return result;
	}

	result = LoadGameplayEffectDefsFromJsonDirectory(
		roots.effectRoot, _attributes.GetAll(), _tags.GetAll(), _effects);
	if (!result.succeeded)
	{
		result.error = "Gameplay effect catalog load failed: " + result.error;
		return result;
	}

	result = LoadAreaHitDefsFromJsonDirectory(roots.areaHitRoot, _areaHits);
	if (!result.succeeded)
	{
		result.error = "Area hit catalog load failed: " + result.error;
		return result;
	}

	result = LoadProjectileDefsFromJsonDirectory(
		roots.projectileRoot,
		_areaHits.GetAll(),
		_projectiles);
	if (!result.succeeded)
	{
		result.error = "Projectile catalog load failed: " + result.error;
		return result;
	}

	result = LoadAbilityDefsFromJsonDirectory(
		roots.abilityRoot, _attributes.GetAll(), 
		_effects.GetAll(), _tags.GetAll(), _abilities);
	if (!result.succeeded)
	{
		result.error = "Ability catalog load failed: " + result.error;
		return result;
	}

	result = LoadAbilitySetDefsFromJsonDirectory(
		roots.abilitySetRoot,
		_abilities.GetAll(),
		_abilitySets);
	if (!result.succeeded)
	{
		result.error = "Ability set catalog load failed: " + result.error;
		return result;
	}

	result.succeeded = true;
	result.loadedCount = Size();
	result.error.clear();
	return result;
}

void GameplayContentCatalogSnapshot::Clear() noexcept
{
	_attributes.Clear();
	_tags.Clear();
	_effects.Clear();
	_projectiles.Clear();
	_areaHits.Clear();
	_abilities.Clear();
	_abilitySets.abilitySets.Clear();
	_abilitySets.inputBindingProfiles.Clear();
	_abilitySets.animationBindingProfiles.Clear();
	_abilitySets.fallbackReactionProfiles.Clear();
}

size_t GameplayContentCatalogSnapshot::Size() const noexcept
{
	return
		_attributes.Size() +
		_tags.Size() +
		_effects.Size() +
		_projectiles.Size() +
		_areaHits.Size() +
		_abilities.Size() +
		_abilitySets.abilitySets.Size() +
		_abilitySets.inputBindingProfiles.Size() +
		_abilitySets.animationBindingProfiles.Size() +
		_abilitySets.fallbackReactionProfiles.Size();
}
