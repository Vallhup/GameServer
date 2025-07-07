#include "pch.h"
#include "Input.h"
#include "Camera.h"
#include "MainCharacter.h"

void Input::KeyBoardInput(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Input* input = static_cast<Input*>(glfwGetWindowUserPointer(window));

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
				}
				else if (action == GLFW_RELEASE)
				{
					input->mainCat->Shift_on(false);
				}
			}
			break;
		//case GLFW_KEY_C:
		//	if (action == GLFW_PRESS)
		//	{
		//		mainCat->setPosition();
		//	}
		//	break;
		case GLFW_KEY_D:
			if ((input->camera->Get_start_pos() == 0 && !input->mainCat->GetDying())/* || finish*/)
			{
				if (action == GLFW_PRESS)
				{
					input->mainCat->SetRight_on(true);
				}
				else if (action == GLFW_RELEASE)
				{
					input->mainCat->SetRight_on(false);
				}
			}
			break;
		case GLFW_KEY_A:
			if ((input->camera->Get_start_pos() == 0 && !input->mainCat->GetDying())/* || finish*/)
			{
				if (action == GLFW_PRESS)
				{
					input->mainCat->SetLeft_on(true);
				}
				else if (action == GLFW_RELEASE)
				{
					input->mainCat->SetLeft_on(false);
				}
			}
			break;
		case GLFW_KEY_W:
			if ((input->camera->Get_start_pos() == 0 && !input->mainCat->GetDying())/* || finish*/)
			{
				if (action == GLFW_PRESS)
				{
					input->mainCat->SetTop_on(true);
				}
				else if (action == GLFW_RELEASE)
				{
					input->mainCat->SetTop_on(false);
				}
			}
			break;
		case GLFW_KEY_S:
			if ((input->camera->Get_start_pos() == 0 && !input->mainCat->GetDying())/* || finish*/)
			{
				if (action == GLFW_PRESS)
				{
					input->mainCat->SetBottom_on(true);
				}
				else if (action == GLFW_RELEASE)
				{
					input->mainCat->SetBottom_on(false);
				}
			}
			break;
		//case GLFW_KEY_H:
		//	if ((input->camera->Get_start_pos() == 0 && !input->mainCat->GetDying())/* || finish*/)
		//	{
		//		if (action == GLFW_PRESS)
		//		{
		//			if (mainCat->hitbox_ison())
		//				mainCat->hitboxOnOff(false);
		//			else
		//				mainCat->hitboxOnOff(true);
		//		}
		//	}
		//	break;
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
