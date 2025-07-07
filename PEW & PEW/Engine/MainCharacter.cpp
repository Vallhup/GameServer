#include "pch.h"
#include "MainCharacter.h"
#include "ShadowMapping.h"
#include "Bullet.h"
#include "Camera.h"

MainCharacter::MainCharacter()
{
	player_BoneInfo = new vector<BoneInfo>();
	animModel = new AnimatedModel();
    player_CurrentAnim = new AnimInfo();
    animLibrary = new AnimatedModel::AnimationLibrary();

    SaveAnimations();

    animModel->LoadGLBFile(0, *player_BoneInfo, "Glb/cat_Tpose.glb", VAO, VBO, VBO2, EBO, Indices);
    Texture = LoadTexture("Texture/CatTexture.png");
    SetupShader("Shaders/CatVert.glsl", "Shaders/CatFrag.glsl", shaderprogram);
}

MainCharacter::~MainCharacter()
{
	// RELEASE(player_CurrentAnim);
    delete player_CurrentAnim;
    delete player_BoneInfo;
	delete animModel;

    if (animLibrary != nullptr) {
        delete animLibrary;
    }

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &VBO2);
	glDeleteBuffers(1, &EBO);
	glDeleteTextures(1, &Texture);
	glDeleteProgram(shaderprogram);
}

void MainCharacter::Update()
{
    if (IsMoving()) {
        if (Shift_value())
            Run();
        else
            Walk();
    }
}

void MainCharacter::Draw(glm::mat4 view, glm::mat4 projection, glm::vec3 viewPos, float deltaTime, float angle)
{
    //if (!dead)
    //{
        animModel->UpdateAnimation(0, *player_BoneInfo, deltaTime, *player_CurrentAnim);
        glUseProgram(shaderprogram);
        animModel->SetupBoneTransforms(*player_BoneInfo, shaderprogram);

        //if (!dying)
        //    lastangle = angle;

        model = glm::mat4(1.0f);
        model = glm::translate(model, characterPos);
        model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
        //if (!dying)
        //    model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
        //else
        //    model = glm::rotate(model, lastangle, glm::vec3(0.0f, 1.0f, 0.0f));

        //if (finish)
        //    model = glm::scale(model, glm::vec3(20.0f));

        ViewLoc = glGetUniformLocation(shaderprogram, "view");
        glUniformMatrix4fv(ViewLoc, 1, GL_FALSE, &view[0][0]);
        ProjLoc = glGetUniformLocation(shaderprogram, "projection");
        glUniformMatrix4fv(ProjLoc, 1, GL_FALSE, &projection[0][0]);
        ModelLoc = glGetUniformLocation(shaderprogram, "model");
        glUniformMatrix4fv(ModelLoc, 1, GL_FALSE, &model[0][0]);

        GLuint lightPosLoc = glGetUniformLocation(shaderprogram, "lightPos");
        GLuint viewPosLoc = glGetUniformLocation(shaderprogram, "viewPos");
        glm::vec3 lightPos{ -37.3051f - (1000.0f * cos(light_angle)), 0.0f + 1000.0f, 42.5001f + (1000.0f * sin(light_angle)) };
        glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
        glUniform3fv(viewPosLoc, 1, glm::value_ptr(viewPos));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Texture);
        TextureLoc = glGetUniformLocation(shaderprogram, "catTexture");
        glUniform1i(TextureLoc, 0);

        colorHitLoc = glGetUniformLocation(shaderprogram, "colorHit");
        glUniform4fv(colorHitLoc, 1, glm::value_ptr(hitcolor));

        UseTextureLoc = glGetUniformLocation(shaderprogram, "useTexture");
        glUniform1i(UseTextureLoc, 1);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, Indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        //if (hitcolor == glm::vec4(1.0f, 0.6f, 0.6f, 1.0f))
        //{
        //    hit_cnt -= 1;
        //}

        //if (hit_cnt == 0)
        //{
        //    hitcolor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        //    hit_cnt = 200;
        //}
    //}
    //else if (dead && revive_timer > 0)
    //{
    //    revive_timer -= 1.0f;
    //}
    //else if (dead && revive_timer == 0)
    //{
    //    life = { 6 };
    //    dead = { false };
    //    dying = { false };
    //    revive_timer = { 1200.0f };
    //    characterPos = { -37.3051f, 0.0f, 42.5001f };
    //    hit_cnt = { 200 };
    //    firing = false;
    //    firing_induration = false;
    //}
}

void MainCharacter::DrawMainCatShadow(float angle, GLuint depthShaderProgram, const glm::mat4& lightSpaceMatrix)
{
    //if (!dead)
    //{
        model = glm::mat4(1.0f);
        model = glm::translate(model, characterPos);
        if (!dying)
            model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
        else
            model = glm::rotate(model, lastangle, glm::vec3(0.0f, 1.0f, 0.0f));
        //if (finish)
        //    model = glm::scale(model, glm::vec3(20.0f));

        ModelLoc = glGetUniformLocation(depthShaderProgram, "model");
        glUniformMatrix4fv(ModelLoc, 1, GL_FALSE, glm::value_ptr(model));
        animModel->SetupBoneTransforms(*player_BoneInfo, depthShaderProgram);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, Indices.size(), GL_UNSIGNED_INT, 0);
    //}
}

void MainCharacter::DrawCatBulletShadow(const glm::mat4& lightSpaceMatrix, GLuint depthShader)
{
    for (auto& bullet : bullets)
    {
        bullet->RenderShadow(lightSpaceMatrix, depthShader);
    }
}

void MainCharacter::SetRight_on(bool in)
{
    _Right = in;
}

void MainCharacter::SetLeft_on(bool in)
{
    _Left = in;
}

void MainCharacter::SetTop_on(bool in)
{
    _Top = in;
}

void MainCharacter::SetBottom_on(bool in)
{
    _Bottom = in;
}

void MainCharacter::Shift_on(bool in)
{
    _Shift = in;
}

//void MainCharacter::hitboxOnOff(bool in)
//{
//    hitbox_on = in;
//}

void MainCharacter::Walk() {
    if (!camera->GetViewType()) {
        if (_Right/* && !wallcollapsed_d()*/)
            characterPos.x += 0.005f;
        if (_Left/* && !wallcollapsed_a()*/)
            characterPos.x -= 0.005f;
        if (_Top/* && !wallcollapsed_w()*/)
            characterPos.z -= 0.005f;
        if (_Bottom/* && !wallcollapsed_s()*/)
            characterPos.z += 0.005f;
    }
    else {
        glm::vec3 forward(
            sin(camera->GetHorizontalAngle()),
            0,
            cos(camera->GetHorizontalAngle())
        );
        glm::vec3 right = glm::cross(forward, glm::vec3(0, 1, 0));

        glm::vec3 moveDir(0.0f);
        if (_Top /*&& !wallcollapsed_forward(forward)*/) moveDir += forward;
        if (_Bottom /*&& !wallcollapsed_backward(forward)*/) moveDir -= forward;
        if (_Right /*&& !wallcollapsed_right(right)*/) moveDir += right;
        if (_Left /*&& !wallcollapsed_left(right)*/) moveDir -= right;

        if (glm::length(moveDir) > 0) {
            moveDir = glm::normalize(moveDir);
            glm::vec3 nextPos = characterPos + moveDir * 0.005f;
            characterPos = nextPos;
        }
    }
}

void MainCharacter::Run()
{
    if (!camera->GetViewType()) {
        if (_Right/* && !wallcollapsed_d()*/)
            characterPos.x += 0.012f;
        if (_Left/* && !wallcollapsed_a()*/)
            characterPos.x -= 0.012f;
        if (_Top/* && !wallcollapsed_w()*/)
            characterPos.z -= 0.012f;
        if (_Bottom/* && !wallcollapsed_s()*/)
            characterPos.z += 0.012f;
    }
    else {
        glm::vec3 forward(
            sin(camera->GetHorizontalAngle()),
            0,
            cos(camera->GetHorizontalAngle())
        );
        glm::vec3 right = glm::cross(forward, glm::vec3(0, 1, 0));
    
        glm::vec3 moveDir(0.0f);
        if (_Top /*&& !wallcollapsed_forward(forward)*/) moveDir += forward;
        if (_Bottom /*&& !wallcollapsed_backward(forward)*/) moveDir -= forward;
        if (_Right /*&& !wallcollapsed_right(right)*/) moveDir += right;
        if (_Left /*&& !wallcollapsed_left(right)*/) moveDir -= right;
    
        if (glm::length(moveDir) > 0) {
            moveDir = glm::normalize(moveDir);
            glm::vec3 nextPos = characterPos + moveDir * 0.01f;
            characterPos = nextPos;
        }
    }
    
}

//void MainCharacter::jump(bool in)
//{
//    /*if (in)
//        characterPos.y += 0.05;*/   // 점프는 나중에
//}

//void MainCharacter::stop()
//{
//    Right_on = { false };
//    Left_on = { false };
//    Top_on = { false };
//    Bottom_on = { false };
//    Shift_on = { false };
//    hitbox_on = { false };
//}

//void MainCharacter::changehitColor()
//{
//    if (hitcolor.g == 1.0f && hitcolor.b == 1.0f)
//    {
//        hitcolor.g = 0.6f;
//        hitcolor.b = 0.6f;
//    }
//    else if (hitcolor.g == 0.6f && hitcolor.b == 0.6f)
//    {
//        hitcolor.g = 1.0f;
//        hitcolor.b = 1.0f;
//    }
//}

void MainCharacter::Setlife()
{
    if (life > 1)
    {
        life -= 1;
        hitcolor = glm::vec4(1.0f, 0.6f, 0.6f, 1.0f);
    }
    else if (life > 0)
    {
        life -= 1;
        hit_cnt = 200;
        hitcolor = glm::vec4(1.0f, 0.6f, 0.6f, 1.0f);
        dying = true;
        _Right = { false };
        _Left = { false };
        _Top = { false };
        _Bottom = { false };
        _Shift = { false };
        hitbox_on = { false };
    }
}

void MainCharacter::ChangeCatAnimation(const glm::mat4& view, const glm::mat4& projection)
{
	float firetimer;
	glm::vec3 mousePick;

	if (!GetDying())
	{
		if (animLibrary->GetCurrentAnimation() == "FireRun")
		{
			firing_induration = true;
			firetimer = player_CurrentAnim->Duration * 0.56f;

			for (int i = 0; i < 3; ++i)
			{
				if (player_CurrentAnim->CurrentTime >= firetimer + (150.0f * i) && !Bullet_cnt[i])
				{
					mousePick = camera->GetMousePicking(cur_x, cur_y, projection, view);
					Bullet* newBullet = new Bullet(1, 0, 0);
					newBullet->BulletSetting(this, camera, mousePick);
					bullets.push_back(newBullet);
					Bullet_cnt[i] = true;
				}
			}

			if (player_CurrentAnim->CurrentTime + 10.0f >= player_CurrentAnim->Duration)
			{
				if (!firing)
				{
					if (IsMoving())
					{
						if (Shift_value())
							animLibrary->ChangeAnimation("Run", *player_CurrentAnim);
						else
							animLibrary->ChangeAnimation("Walk", *player_CurrentAnim);
					}
					else
						animLibrary->ChangeAnimation("Idle", *player_CurrentAnim);
					firing_induration = false;
				}
				else
				{
					if (IsMoving())
					{
						if (!Shift_value())
							animLibrary->ChangeAnimation("FireWalk", *player_CurrentAnim);
						else
							animLibrary->ChangeAnimation("FireRun", *player_CurrentAnim);
					}
					else
						animLibrary->ChangeAnimation("Fire", *player_CurrentAnim);
				}

				for (bool& bullet_nums : Bullet_cnt)
					bullet_nums = false;
			}
		}
		else if (animLibrary->GetCurrentAnimation() == "FireWalk")
		{
			firing_induration = true;
			firetimer = player_CurrentAnim->Duration * 0.56f;

			for (int i = 0; i < 3; ++i)
			{
				if (player_CurrentAnim->CurrentTime >= firetimer + (150.0f * i) && !Bullet_cnt[i])
				{
					mousePick = camera->GetMousePicking(cur_x, cur_y, projection, view);
					Bullet* newBullet = new Bullet(1, 0, 0);
					newBullet->BulletSetting(this, camera, mousePick);
					bullets.push_back(newBullet);
					Bullet_cnt[i] = true;
				}
			}

			if (player_CurrentAnim->CurrentTime + 10.0f >= player_CurrentAnim->Duration)
			{
				if (!firing)
				{
					if (IsMoving())
					{
						if (Shift_value())
							animLibrary->ChangeAnimation("Run", *player_CurrentAnim);
						else
							animLibrary->ChangeAnimation("Walk", *player_CurrentAnim);
					}
					else
						animLibrary->ChangeAnimation("Idle", *player_CurrentAnim);
					firing_induration = false;
				}
				else
				{
					if (IsMoving())
					{
						if (Shift_value())
							animLibrary->ChangeAnimation("FireRun", *player_CurrentAnim);
						else
							animLibrary->ChangeAnimation("FireWalk", *player_CurrentAnim);
					}
					else
						animLibrary->ChangeAnimation("Fire", *player_CurrentAnim);
				}

				for (bool& bullet_nums : Bullet_cnt)
					bullet_nums = false;
			}
		}
		else if (animLibrary->GetCurrentAnimation() == "Fire")
		{
			firing_induration = true;
			firetimer = player_CurrentAnim->Duration * 0.56f;

			for (int i = 0; i < 3; ++i)
			{
				if (player_CurrentAnim->CurrentTime >= firetimer + (150.0f * i) && !Bullet_cnt[i])
				{
					mousePick = camera->GetMousePicking(cur_x, cur_y, projection, view);
					Bullet* newBullet = new Bullet(1, 0, 0);
					newBullet->BulletSetting(this, camera, mousePick);
					bullets.push_back(newBullet);
					Bullet_cnt[i] = true;
				}
			}

			if (player_CurrentAnim->CurrentTime + 10.0f >= player_CurrentAnim->Duration)
			{
				if (!firing)
				{
					if (IsMoving())
					{
						if (Shift_value())
							animLibrary->ChangeAnimation("Run", *player_CurrentAnim);
						else
							animLibrary->ChangeAnimation("Walk", *player_CurrentAnim);
					}
					else
						animLibrary->ChangeAnimation("Idle", *player_CurrentAnim);
					firing_induration = false;
				}
				else
				{
					if (IsMoving())
					{
						if (Shift_value())
							animLibrary->ChangeAnimation("FireRun", *player_CurrentAnim);
						else
							animLibrary->ChangeAnimation("FireWalk", *player_CurrentAnim);
					}
					else
						animLibrary->ChangeAnimation("Fire", *player_CurrentAnim);
				}

				for (auto& bullet_nums : Bullet_cnt)
					bullet_nums = false;
			}
		}
		else if (animLibrary->GetCurrentAnimation() == "Die")
			animLibrary->ChangeAnimation("Idle", *player_CurrentAnim);
	}
	else
	{
		if (animLibrary->GetCurrentAnimation() != "Die")
			animLibrary->ChangeAnimation("Die", *player_CurrentAnim);
		else
		{
			if (player_CurrentAnim->CurrentTime + 10 >= player_CurrentAnim->Duration)
				SetDead(true);
		}
	}
}

void MainCharacter::ThrowBullets(const glm::mat4& orgview, const glm::mat4& orgproj, glm::vec3 viewPos, glm::mat4 lightSpaceMatrix, GLuint shadowMap)
{
	for (auto& bullet : bullets)
	{
		bullet->BulletUpdate();
		bullet->Render(orgview, orgproj, viewPos, lightSpaceMatrix, shadowMap);
	}
}

void MainCharacter::SaveAnimations()
{
//#define ANIMATION(x) "Animation"##x".glb");
//
//	Animation("cat_animation_idle");
	// Texture
	// sound

    animLibrary->LoadAnimation("Idle", "Animations/cat_animation_idle.glb", animationImporters, animModel);
    animLibrary->LoadAnimation("Die", "Animations/cat_animation_die.glb", animationImporters, animModel);
    animLibrary->LoadAnimation("Walk", "Animations/cat_animation_walking.glb", animationImporters, animModel);
    animLibrary->LoadAnimation("Run", "Animations/cat_animation_run.glb", animationImporters, animModel);
    animLibrary->LoadAnimation("Hit", "Animations/cat_animation_hit.glb", animationImporters, animModel);
    animLibrary->LoadAnimation("Jump", "Animations/cat_animation_jump.glb", animationImporters, animModel);
    animLibrary->LoadAnimation("AimIdle", "Animations/cat_animation_idle_aim.glb", animationImporters, animModel);
    animLibrary->LoadAnimation("AimWalk", "Animations/cat_animation_walking_aim.glb", animationImporters, animModel);
    animLibrary->LoadAnimation("Fire", "Animations/cat_animation_firing_aim.glb", animationImporters, animModel);
    animLibrary->LoadAnimation("FireWalk", "Animations/cat_animation_firing_walk.glb", animationImporters, animModel);
    animLibrary->LoadAnimation("FireRun", "Animations/cat_animation_firing_run.glb", animationImporters, animModel);
    animLibrary->LoadAnimation("Dance", "Animations/cat_animation_dance.glb", animationImporters, animModel);

    animLibrary->ChangeAnimation("Idle", *player_CurrentAnim);
}

void MainCharacter::SetAnimationType(const std::string& animName)
{
    if (animLibrary && player_CurrentAnim) {
        animLibrary->ChangeAnimation(animName, *player_CurrentAnim);
    }
}
