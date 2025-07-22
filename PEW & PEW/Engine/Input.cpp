#include "pch.h"
#include "Input.h"
#include "Camera.h"
#include "NetworkManager.h"
#include "PacketFactory.h"
#include "GraphicsManager.h"
#include "MainCharacter.h"
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

	switch (key) {
	case GLFW_KEY_P:
		if (action == GLFW_PRESS)
			input->camera->SetStart(true);
		break;
	case GLFW_KEY_Q:
		if ((!(input->camera->Get_start_pos() == 0) && !input->mainCat->GetDead())/* || finish*/)
		{
			if (action == GLFW_PRESS)
				glfwSetWindowShouldClose(window, GL_TRUE);
		}
		break;
	case GLFW_KEY_LEFT_SHIFT:
		if ((input->camera->Get_start_pos() == 0 && !input->mainCat->GetDead())/* || finish*/)
		{
			if (action == GLFW_PRESS)
			{
				input->mainCat->SetShift(true);
				if (input->sceneType == SceneType::Scene2)
					input->SendMovePacket();
			}
			else if (action == GLFW_RELEASE)
			{
				input->mainCat->SetShift(false);
				if (input->sceneType == SceneType::Scene2)
					input->SendMovePacket();
			}
		}
		break;
	case GLFW_KEY_C:
		if (action == GLFW_PRESS)
		{
			input->graphics->DebugAllCharacterPositions();
			input->mainCat->GoToEndPosition();
		}
		break;
	case GLFW_KEY_D:
		if ((input->camera->Get_start_pos() == 0 && !input->mainCat->GetDead()))
		{
			if (action == GLFW_PRESS)
			{
				input->mainCat->SetRight(true);
				if (input->sceneType == SceneType::Scene2)
					input->SendMovePacket();
			}
			else if (action == GLFW_RELEASE)
			{
				input->mainCat->SetRight(false);
				if (input->sceneType == SceneType::Scene2)
					input->SendMovePacket();
			}
		}
		break;
	case GLFW_KEY_A:
		if ((input->camera->Get_start_pos() == 0 && !input->mainCat->GetDead())/* || finish*/)
		{
			if (action == GLFW_PRESS)
			{
				input->mainCat->SetLeft(true);
				if (input->sceneType == SceneType::Scene2)
					input->SendMovePacket();
			}
			else if (action == GLFW_RELEASE)
			{
				input->mainCat->SetLeft(false);
				if (input->sceneType == SceneType::Scene2)
					input->SendMovePacket();
			}
		}
		break;
	case GLFW_KEY_W:
		if ((input->camera->Get_start_pos() == 0 && !input->mainCat->GetDead())/* || finish*/)
		{
			if (action == GLFW_PRESS)
			{
				input->mainCat->SetTop(true);
				if (input->sceneType == SceneType::Scene2)
					input->SendMovePacket();
			}
			else if (action == GLFW_RELEASE)
			{
				input->mainCat->SetTop(false);
				if (input->sceneType == SceneType::Scene2)
					input->SendMovePacket();
			}
		}
		break;
	case GLFW_KEY_S:
		if ((input->camera->Get_start_pos() == 0 && !input->mainCat->GetDead())/* || finish*/)
		{
			if (action == GLFW_PRESS)
			{
				input->mainCat->SetBottom(true);
				if (input->sceneType == SceneType::Scene2)
					input->SendMovePacket();
			}
			else if (action == GLFW_RELEASE)
			{
				input->mainCat->SetBottom(false);
				if (input->sceneType == SceneType::Scene2)
					input->SendMovePacket();
			}
		}
		break;
	case GLFW_KEY_H:
		if ((input->camera->Get_start_pos() == 0 && !input->mainCat->GetDead())/* || finish*/)
		{
			if (action == GLFW_PRESS)
			{
				if (input->mainCat->GetHitBox())
					input->mainCat->SetHitBox(false);
				else
					input->mainCat->SetHitBox(true);
			}
		}
		break;
	case GLFW_KEY_V:
		if ((input->camera->Get_start_pos() == 0 && !input->mainCat->GetDead())/* || finish*/)
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
		case GLFW_KEY_0:
			if (input->camera->Get_start_pos() == 0 && !input->mainCat->GetDead()/* && !finish*/)
			{
				if (action == GLFW_PRESS)
				{
					cout << "x: " << input->mainCat->GetPosition().x << endl;
					cout << "z: " << input->mainCat->GetPosition().z << endl;
				}
			}
			break;
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
		if ((input->camera->Get_start_pos() == 0 && !input->mainCat->GetDead())/* || finish*/)
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
}

void Input::Scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	Input* input = static_cast<Input*>(glfwGetWindowUserPointer(window));

	if (input->camera->Get_start_pos() == 0)
		input->camera->HandleScroll(yoffset);
}

void Input::MouseMoveFunc(GLFWwindow* window, double xpos, double ypos)
{
	Input* input = static_cast<Input*>(glfwGetWindowUserPointer(window));

	if (!input->mainCat) return;

	if (input->camera->Get_start_pos() != 0 || input->mainCat->GetDead()) {
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
		if (input->sceneType == SceneType::Scene2)
		{
			input->lastMouseAngle = angleRad;
			input->SendMovePacket();
		}
	}
}

void Input::Update(GLFWwindow* window)
{
	CheckContinuousAttack(window);
}

void Input::CheckContinuousAttack(GLFWwindow* window)
{
	if (!mainCat || mainCat->GetDead()) return;

	bool isMousePressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

	std::string currentAnim = mainCat->GetAnimLibrary()->GetCurrentAnimation();
	bool isFireAnim = (currentAnim == "Fire" || currentAnim == "FireWalk" || currentAnim == "FireRun");

	if (isMousePressed && camera->Get_start_pos() == 0)
	{
		if (!isAttacking)
		{
			if (!isFireAnim)
			{
				// 첫 공격 시작
				isAttacking = true;
				if (sceneType == SceneType::Scene2)
					SendAttackPacket();
				else
					mainCat->SetFiring(true);
				//cout << "first attack packet has send" << '\n';
			}
			else
			{
				AnimInfo* currentAnimInfo = mainCat->GetCurrentAnim();

				if (currentAnimInfo->CurrentTime + 50.0f >= currentAnimInfo->Duration)
				{
					// 첫 공격 시작
					isAttacking = true;
					if (sceneType == SceneType::Scene2)
						SendAttackPacket();
					else
						mainCat->SetFiring(true);
					//cout << "first attack packet has send" << '\n';
				}
			}
		}
		else
		{
			if (isFireAnim)
			{
				AnimInfo* currentAnimInfo = mainCat->GetCurrentAnim();

				if (currentAnimInfo->CurrentTime + 50.0f >= currentAnimInfo->Duration)
				{
					if (!wasFireAnimation)
					{
						if (sceneType == SceneType::Scene2)
							SendAttackPacket();
						else
							mainCat->SetFiring(true);
						wasFireAnimation = true;
						//cout << "continuous attack packet has send" << '\n';
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
			// attack end 패킷 전송
			if (sceneType == SceneType::Scene2)
				SendAttackEndPacket();
			else
				mainCat->SetFiring(false);
			//cout << "attack end packet has send" << '\n';

			// 상태 초기화
			isAttacking = false;
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
	bool isRunning = mainCat->GetShift();

	vector<char> packet = PacketFactory::CSMovePacket(lastMouseAngle, direction, isRunning);
	network->Send(packet);
}

void Input::SendAttackPacket()
{
	if (!network) return;

	glm::vec3 position = mainCat->GetPosition();
	position.y = 0.45f;

	// 과거 방식처럼 mousePick 사용
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIN_W / (float)WIN_H, 0.1f, 1000.0f);
	glm::mat4 view = camera->GetViewMatrix(mainCat->GetPosition());

	glm::vec3 mousePick = camera->GetMousePicking(cur_x, cur_y, projection, view);
	glm::vec3 targetPos = mousePick;
	targetPos.y = 0.45f;

	// 시작 위치 조정 (과거 방식과 동일)
	float tempAngle = atan2(mouseDir.x, mouseDir.z);
	position.x += cos(tempAngle) * 0.2f;
	position.z -= sin(tempAngle) * 0.2f;

	// 실제 목표점으로 방향 계산
	glm::vec3 direction = glm::normalize(targetPos - position);

	vector<char> packet = PacketFactory::CSAttackPacket(direction);
	network->Send(packet);
}

void Input::SendAttackEndPacket()
{
	if (!network) return;

	vector<char> packet = PacketFactory::CSAttackEndPacket();
	network->Send(packet);
}