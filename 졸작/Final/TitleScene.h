#pragma once
#include "Scene.h"

class TitleScene final : public Scene
{
public:
	TitleScene() = default;
	TitleScene(const TitleScene&) = delete;
	TitleScene& operator=(const TitleScene&) = delete;
	~TitleScene() = default;

	void Release() override;

protected:
	void InitializeLogic() override;

	const char* GetBGMPath() const override;
};
