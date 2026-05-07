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
	void HandleLogin(const Protocol::SC_LOGIN_SUCCESS_PACKET& login) override;

	void InitializeLogic() override;					
};
