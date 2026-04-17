#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ICharacterAspect.h"

// 등록된 모든 aspect 를 정해진 순서대로 보유한다.
// 순서는 곧 attach 순서이며, 등록 순서가 의존성 순서를 표현한다.
class CharacterAspectRegistry final {
public:
	CharacterAspectRegistry() = default;
	~CharacterAspectRegistry() = default;

	CharacterAspectRegistry(const CharacterAspectRegistry&) = delete;
	CharacterAspectRegistry& operator=(const CharacterAspectRegistry&) = delete;

	CharacterAspectRegistry(CharacterAspectRegistry&&) = default;
	CharacterAspectRegistry& operator=(CharacterAspectRegistry&&) = default;

	// 기본 9개 aspect 를 정해진 순서로 채워서 반환.
	static CharacterAspectRegistry BuildDefault();

	void Add(std::unique_ptr<ICharacterAspect> aspect);

	// 등록된 모든 aspect 의 storage 를 등록.
	void RegisterStoragesAll(WorldRuntime& runtime) const;

	// def 에 매칭되는 aspect 들만 골라 attach.
	void Assemble(
		WorldRuntime& runtime,
		Entity entity,
		const CharacterDef& def,
		const AssembleParams& params) const;

	// def 에 매칭되는 aspect 들만 골라 검증. 첫 실패에서 멈추고 false 반환.
	bool ValidateAll(
		const CharacterDef& def,
		std::string& outError) const;

	size_t Size() const noexcept { return _aspects.size(); }

private:
	std::vector<std::unique_ptr<ICharacterAspect>> _aspects;
};

// 프로세스 전체가 공유하는 기본 aspect registry.
// 여러 월드/서비스가 모두 같은 구성을 쓰므로 single shared instance 로 충분.
// 필요 시 framework 레벨로 이관 가능.
const CharacterAspectRegistry& GetGlobalCharacterAspectRegistry();
