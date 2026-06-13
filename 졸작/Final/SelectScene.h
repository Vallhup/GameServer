#pragma once
#include "Scene.h"

class SelectScene final : public Scene
{
public:
	SelectScene() = default;
	SelectScene(const SelectScene&) = delete;
	SelectScene& operator=(const SelectScene&) = delete;
	~SelectScene() = default;

	void Release() override;

protected:
	const char* GetBGMPath() const override;
	float GetBGMFadeInSeconds() const override;

	void InitializeLogic() override;
};
