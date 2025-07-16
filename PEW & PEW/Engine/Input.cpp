#include "pch.h"
#include "Input.h"
#include "Camera.h"
#include "NetworkManager.h"
#include "PacketFactory.h"
#include "GraphicsManager.h"
#include "Character.h"
#include "WindowInfo.h"

void Input::KeyBoardInput(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Input* input = static_cast<Input*>(glfwGetWindowUserPointer(window));

	// mainCat이 없으면 graphics에서 동적으로 가져오기
	if (!input->mainCat && input->graphics) {
		input->mainCat = input->graphics->GetLocalCharacter();
	}

	// 로컬 캐릭터가 아직 생성되지 않았으면 아무것도 하지 않음
	if (!input->mainCat) {
		return;
	}

	bool wasMoving = input->mainCat->IsMoving();
	bool wasRunning = input->mainCat->Shift_value();

	switch (key) {
	case GLFW_KEY_P:
		if (action == GLFW_PRESS)
			input->camera->SetStart(true);
		break;
	case GLFW_KEY_Q:
		if ((!(input->camera->Get_start_pos() == 0) && !input->mainCat->GetDying())/* || finish*/)
		{
			if (action == GLFW_PRESS)
				glfwSetWindowShouldClose(window, GL_TRUE);
		}
		break;
	case GLFW_KEY_LEFT_SHIFT:
		if ((input->camera->Get_start_pos() == 0 && !input->mainCat->GetDying())/* || finish*/)
		{
			if (action == GLFW_PRESS)
			{
				input->mainCat->Shift_on(true);
				input->SendMovePacket();
			}
			else if (action == GLFW_RELEASE)
			{
				input->mainCat->Shift_on(false);
				input->SendMovePacket();
			}
		}
		break;
	case GLFW_KEY_C:
		if (action == GLFW_PRESS)
		{
			input->graphics->DebugAllCharacterPositions();
		}
		break;
	case GLFW_KEY_D:
		if ((input->camera->Get_start_pos() == 0 && !input->mainCat->GetDying()))
		{
			if (action == GLFW_PRESS)
			{
				input->mainCat->SetRight_on(true);
				input->SendMovePacket();
			}
			else if (action == GLFW_RELEASE)
			{
				input->mainCat->SetRight_on(false);
				input->SendMovePacket();
			}
		}
		break;
	case GLFW_KEY_A:
		if ((input->camera->Get_start_pos() == 0 && !input->mainCat->GetDying())/* || finish*/)
		{
			if (action == GLFW_PRESS)
			{
				input->mainCat->SetLeft_on(true);
				input->SendMovePacket();
			}
			else if (action == GLFW_RELEASE)
			{
				input->mainCat->SetLeft_on(false);
				input->SendMovePacket();
			}
		}
		break;
	case GLFW_KEY_W:
		if ((input->camera->Get_start_pos() == 0 && !input->mainCat->GetDying())/* || finish*/)
		{
			if (action == GLFW_PRESS)
			{
				input->mainCat->SetTop_on(true);
				input->SendMovePacket();
			}
			else if (action == GLFW_RELEASE)
			{
				input->mainCat->SetTop_on(false);
				input->SendMovePacket();
			}
		}
		break;
	case GLFW_KEY_S:
		if ((input->camera->Get_start_pos() == 0 && !input->mainCat->GetDying())/* || finish*/)
		{
			if (action == GLFW_PRESS)
			{
				input->mainCat->SetBottom_on(true);
				input->SendMovePacket();
			}
			else if (action == GLFW_RELEASE)
			{
				input->mainCat->SetBottom_on(false);
				input->SendMovePacket();
			}
		}
		break;
	case GLFW_KEY_H:
		if ((input->camera->Get_start_pos() == 0 && !input->mainCat->GetDying())/* || finish*/)
		{
			if (action == GLFW_PRESS)
			{
				if (input->mainCat->hitbox_ison())
					input->mainCat->hitboxOnOff(false);
				else
					input->mainCat->hitboxOnOff(true);
			}
		}
		break;
	case GLFW_KEY_V:
		if ((input->camera->Get_start_pos() == 0 && !input->mainCat->GetDying())/* || finish*/)
		{
			if (action == GLFW_PRESS)
			{
				input->camera->ChangeViewType();

				if (input->camera->GetViewType()) {
					input->camera->SetInitialDirection(mouseDir);
				}
			}
		}
		break;
		//case GLFW_KEY_0:
		//	if (camera.Get_start_pos() == 0 && !mainCat->getdying() && !finish)
		//	{
		//		if (action == GLFW_PRESS)
		//		{
		//			cout << "x: " << mainCat->getPosition().x << endl;
		//			cout << "z: " << mainCat->getPosition().z << endl;
		//		}
		//	}
		//	break;
		//case GLFW_KEY_1:
		//	if (camera.Get_start_pos() == 0 && !mainCat->getdying() && !finish)
		//	{
		//		if (action == GLFW_PRESS)
		//		{
		//			playeranimLib.changeAnimation("Dance", player_CurrentAnim);
		//		}
		//	}
		//	break;
	case GLFW_KEY_LEFT_ALT:
		if ((input->camera->Get_start_pos() == 0 && !input->mainCat->GetDying())/* || finish*/)
		{
			if (action == GLFW_PRESS)
				input->camera->HandleAltKey(true);
			else if (action == GLFW_RELEASE)
				input->camera->HandleAltKey(false);
		}
		break;
		//case GLFW_KEY_EQUAL:
		//	if (action == GLFW_PRESS)
		//	{
		//		if (mods == GLFW_MOD_SHIFT)
		//		{
		//			if (soundVol < 1.0f)
		//			{
		//				soundVol += 0.05f;
		//				std::cout << "Sound: " << soundVol << std::endl;
		//			}
		//		}
		//	}
		//	break;
		//case GLFW_KEY_MINUS:
		//	if (action == GLFW_PRESS)
		//	{
		//		if (soundVol > 0.0f)
		//		{
		//			soundVol -= 0.05f;
		//			std::cout << "Sound: " << soundVol << std::endl;
		//		}
		//	}
		//	break;
	}

	bool isMovingNow = input->mainCat->IsMoving();
	bool isRunningNow = input->mainCat->Shift_value();

	if (wasMoving != isMovingNow || (isMovingNow && wasRunning != isRunningNow)) {
		if (!input->mainCat->GetFiringInduration())
		{
			if (isMovingNow) {
				if (isRunningNow)
				{
					input->mainCat->GetAnimLibrary()->ChangeAnimation("Run", *input->mainCat->GetCurrentAnim());
				}
				else
				{
					input->mainCat->GetAnimLibrary()->ChangeAnimation("Walk", *input->mainCat->GetCurrentAnim());
				}
			}
			else
			{
				input->mainCat->GetAnimLibrary()->ChangeAnimation("Idle", *input->mainCat->GetCurrentAnim());
			}
		}
	}
}

void Input::Scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	Input* input = static_cast<Input*>(glfwGetWindowUserPointer(window));

	if (input->camera->Get_start_pos() == 0)
		input->camera->HandleScroll(yoffset);
}

void Input::MouseFunc(GLFWwindow* window, int button, int action, int mods)
{
	Input* input = static_cast<Input*>(glfwGetWindowUserPointer(window));

	// mainCat이 없으면 graphics에서 동적으로 가져오기
	if (!input->mainCat && input->graphics) {
		input->mainCat = input->graphics->GetLocalCharacter();
	}

	// 로컬 캐릭터가 아직 생성되지 않았으면 아무것도 하지 않음
	if (!input->mainCat) {
		return;
	}

	switch (button)
	{
	case GLFW_MOUSE_BUTTON_LEFT:
		if (input->camera->Get_start_pos() == 0 && !input->mainCat->GetDying() /*&& !finish*/)
		{
			if (action == GLFW_PRESS)
			{
				input->mainCat->SetFiring(true);
			}
			else if (action == GLFW_RELEASE)
			{
				input->mainCat->SetFiring(false);
			}
		}
		break;
	}

	// 수정필요
	if (input->mainCat->GetFiring() && input->mainCat->GetAnimLibrary()->GetCurrentAnimation() == "Run")
		input->mainCat->GetAnimLibrary()->ChangeAnimation("FireRun", *input->mainCat->GetCurrentAnim());
	else if (input->mainCat->GetFiring() && input->mainCat->GetAnimLibrary()->GetCurrentAnimation() == "Walk")
		input->mainCat->GetAnimLibrary()->ChangeAnimation("FireWalk", *input->mainCat->GetCurrentAnim());
	else if (input->mainCat->GetFiring() && input->mainCat->GetAnimLibrary()->GetCurrentAnimation() == "Idle")
		input->mainCat->GetAnimLibrary()->ChangeAnimation("Fire", *input->mainCat->GetCurrentAnim());
}

void Input::MouseMoveFunc(GLFWwindow* window, double xpos, double ypos)
{
	Input* input = static_cast<Input*>(glfwGetWindowUserPointer(window));

	if (!input->mainCat) return;

	if (input->camera->Get_start_pos() != 0 || input->mainCat->GetDying()) {
		return;
	}

	cur_x = xpos;
	cur_y = ypos;

	// Camera 클래스의 기존 시스템 활용
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIN_W / (float)WIN_H, 0.1f, 1000.0f);
	glm::mat4 view = input->camera->GetViewMatrix(input->mainCat->GetPosition());

	// Camera의 SetMouseWorldDirection 사용
	if (!input->camera->IsAltPressed())
		mouseDir = input->camera->SetMouseWorldDirection(xpos, ypos, projection, view, input->mainCat->GetPosition());

	// 3D 방향벡터를 2D 각도로 변환
	float angleRad = (float)PI + atan2(mouseDir.x, mouseDir.z);

	// 이전 각도와 비교해서 1도 이상 차이나면 갱신하고 패킷 전송
	if (abs(angleRad - input->mainCat->GetAngle()) >= 0.1f) {
		input->mainCat->SetAngle(angleRad);
		input->lastMouseAngle = angleRad;
		input->SendMovePacket();
	}
}

void Input::Update()
{
	CheckContinuousAttack();
}

void Input::CheckContinuousAttack()
{
	if (!mainCat) return;

	// 직접 마우스 상태 체크
	GLFWwindow* window = GET_SINGLE(WindowInfo)->GetWindow();
	bool isMousePressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

	if (isMousePressed && camera->Get_start_pos() == 0 && !mainCat->GetDying())
	{
		if (!isAttacking)
		{
			// 첫 공격 시작
			mainCat->SetFiring(true);
			isAttacking = true;
			SendAttackPacket();
			firstAttackSent = true;
		}
		else
		{
			// 연속 공격 체크 로직
			std::string currentAnim = mainCat->GetAnimLibrary()->GetCurrentAnimation();
			bool isFireAnim = (currentAnim == "Fire" || currentAnim == "FireWalk" || currentAnim == "FireRun");

			if (isFireAnim)
			{
				AnimInfo* currentAnimInfo = mainCat->GetCurrentAnim();

				if (currentAnimInfo->CurrentTime + 10.0f >= currentAnimInfo->Duration)
				{
					if (!wasFireAnimation)
					{
						SendAttackPacket();
						wasFireAnimation = true;
					}
				}
				else
				{
					wasFireAnimation = false;
				}
			}
		}
	}
	else
	{
		// 마우스를 떼었을 때
		if (isAttacking)
		{
			mainCat->SetFiring(false);
			isAttacking = false;
			firstAttackSent = false;
			wasFireAnimation = false;
		}
	}
}

char Input::GetCurrentDirection()
{
	if (!mainCat) return -1;

	bool up = mainCat->GetTop();
	bool down = mainCat->GetBottom();
	bool right = mainCat->GetRight();
	bool left = mainCat->GetLeft();

	if (up && right) return UPRIGHT;
	if (up && left) return UPLEFT;
	if (down && right) return DOWNRIGHT;
	if (down && left) return DOWNLEFT;

	if (up) return UP;
	if (down) return DOWN;
	if (right) return RIGHT;
	if (left) return LEFT;

	return -1;
}

void Input::SendMovePacket()
{
	if (!network) return;

	char direction = GetCurrentDirection();
	bool isRunning = mainCat->Shift_value();

	vector<char> packet = PacketFactory::CSMovePacket(lastMouseAngle, direction, isRunning);
	network->Send(packet);
}

void Input::SendAttackPacket()
{
	if (!network) return;

	glm::vec3 position = mainCat->GetPosition();
	position.y = 0.45f;

	float angle = atan2(mouseDir.x, mouseDir.z);

	position.x += cos(angle) * 0.2f;
	position.z -= sin(angle) * 0.2f;

	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIN_W / (float)WIN_H, 0.1f, 1000.0f);
	glm::mat4 view = camera->GetViewMatrix(mainCat->GetPosition());

	glm::vec3 mousePick = camera->GetMousePicking(cur_x, cur_y, projection, view);

	glm::vec3 targetPos = mousePick;
	targetPos.y = 0.45f;

	glm::vec3 direction = glm::normalize(targetPos - position);

	vector<char> packet = PacketFactory::CSAttackPacket(direction);
	network->Send(packet);
}