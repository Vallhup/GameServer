#include "pch.h"
#include "AlienCharacter.h"
#include "MainCharacter.h"
#include "Bullet.h"
#include "ShadowMapping.h"
#include "CollisionManager.h"

AlienCharacter::AlienCharacter(int type, int location)
{
	alien_BoneInfo = new vector<BoneInfo>();
	animModel = new AnimatedModel();
	alien_CurrentAnim = new AnimInfo();
	animLibrary = new AnimatedModel::AnimationLibrary();

	alienType = type + 1;
	alienLocationSetter = location;

	SaveAnimations();
	LoadModel();
	SetSpawnPosition();
	SetSpawnAngle();
	SetupShaders();

	for (int i = 0; i < MAX_BULLETS; ++i)
		bullets[i].bullet = new Bullet(2, 0.02f);

	glGenVertexArrays(1, &lVAO);
	glGenBuffers(1, &lVBO);
}

AlienCharacter::~AlienCharacter()
{
	delete alien_CurrentAnim;
	delete alien_BoneInfo;
	delete animModel;
	delete animLibrary;

	glDeleteVertexArrays(1, &aVAO);
	glDeleteBuffers(1, &aVBO);
	glDeleteBuffers(1, &aVBO2);
	glDeleteBuffers(1, &aEBO);
	glDeleteTextures(1, &aTexture);
	glDeleteProgram(aShaderprogram);

	for (int i = 0; i < MAX_BULLETS; ++i) {
		delete bullets[i].bullet;
	}

	glDeleteVertexArrays(1, &lVAO);
	glDeleteBuffers(1, &lVBO);
	glDeleteProgram(lShaderprogram);
}

void AlienCharacter::Update(float deltaTime, MainCharacter* Cat)
{
	RotateAliens(Cat);
	ChangeAnimation();
	UpdateStateAndBehavior(Cat, deltaTime);
	UpdateBullets(Cat, deltaTime);
	UpdateHitDecision(deltaTime);
}

void AlienCharacter::Draw(glm::mat4 view, glm::mat4 projection, glm::vec3 viewPos, float deltaTime, glm::mat4 lightSpaceMatrix, GLuint depthMap)
{
	animModel->UpdateAnimation(alienType, *alien_BoneInfo, deltaTime, *alien_CurrentAnim);
	glUseProgram(aShaderprogram);
	animModel->SetupBoneTransforms(*alien_BoneInfo, aShaderprogram);

	ViewLoc = glGetUniformLocation(aShaderprogram, "view");
	glUniformMatrix4fv(ViewLoc, 1, GL_FALSE, &view[0][0]);
	ProjLoc = glGetUniformLocation(aShaderprogram, "projection");
	glUniformMatrix4fv(ProjLoc, 1, GL_FALSE, &projection[0][0]);
	ModelLoc = glGetUniformLocation(aShaderprogram, "model");
	glUniformMatrix4fv(ModelLoc, 1, GL_FALSE, &model[0][0]);

	GLuint lightSpaceMatrixLoc = glGetUniformLocation(aShaderprogram, "lightSpaceMatrix");
	glUniformMatrix4fv(lightSpaceMatrixLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	GLuint shadowMapLoc = glGetUniformLocation(aShaderprogram, "shadowMap");
	glUniform1i(shadowMapLoc, 1);

	GLuint lightPosLoc = glGetUniformLocation(aShaderprogram, "lightPos");
	GLuint viewPosLoc = glGetUniformLocation(aShaderprogram, "viewPos");
	glm::vec3 lightPos{ -37.3051f - (1000.0f * cos(light_angle)), 0.0f + 1000.0f, 42.5001f + (1000.0f * sin(light_angle)) };
	glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
	glUniform3fv(viewPosLoc, 1, glm::value_ptr(viewPos));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, aTexture);
	TextureLoc = glGetUniformLocation(aShaderprogram, "catTexture");
	glUniform1i(TextureLoc, 0);

	colorHitLoc = glGetUniformLocation(aShaderprogram, "colorHit");
	glUniform4fv(colorHitLoc, 1, glm::value_ptr(hitcolor));

	UseTextureLoc = glGetUniformLocation(aShaderprogram, "useTexture");
	glUniform1i(UseTextureLoc, 1);

	glBindVertexArray(aVAO);
	glDrawElements(GL_TRIANGLES, aIndices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	if (state == 2 && alien_CurrentAnim->CurrentTime < 750)
	{
		DrawAttackingLine(view, projection);
	}
}

void AlienCharacter::DrawShadow(ShadowMapping* shadowMap)
{
	ModelLoc = glGetUniformLocation(shadowMap->GetDepthShaderProgram(), "model");
	glUniformMatrix4fv(ModelLoc, 1, GL_FALSE, glm::value_ptr(model));
	animModel->SetupBoneTransforms(*alien_BoneInfo, shadowMap->GetDepthShaderProgram());
	glBindVertexArray(aVAO);
	glDrawElements(GL_TRIANGLES, aIndices.size(), GL_UNSIGNED_INT, 0);
}

void AlienCharacter::DrawBullets(const glm::mat4& view, const glm::mat4& projection, glm::vec3 viewPos, glm::mat4 lightSpaceMatrix, GLuint shadowMap)
{
	for (int i = 0; i < MAX_BULLETS; ++i)
	{
		if (bullets[i].isActive)
			bullets[i].bullet->Render(view, projection, viewPos, lightSpaceMatrix, shadowMap);
	}
}

void AlienCharacter::DrawBulletsShadow(const glm::mat4& lightSpaceMatrix, GLuint depthShader)
{
	for (int i = 0; i < MAX_BULLETS; ++i)
	{
		if (bullets[i].isActive)
			bullets[i].bullet->RenderShadow(lightSpaceMatrix, depthShader);
	}
}

void AlienCharacter::DrawAttackingLine(const glm::mat4& view, const glm::mat4& projection)
{
	glUseProgram(lShaderprogram);

	const int segments = 30;
	std::vector<glm::vec3> linePositions;

	glm::vec3 startPos = { alienPos.x, 0.45f, alienPos.z };
	glm::vec3 endPos = { targetPos.x - (alienPos.x - targetPos.x), 0.45f,
						targetPos.z - (alienPos.z - targetPos.z) };

	glm::vec3 direction = endPos - startPos;
	float totalLength = glm::length(direction);
	float segmentLength = totalLength / (segments * 2);

	direction = glm::normalize(direction);

	for (int i = 0; i < segments; i++) {
		float start = i * segmentLength * 2;
		linePositions.push_back(startPos + direction * start);
		linePositions.push_back(startPos + direction * (start + segmentLength));
	}

	glBindVertexArray(lVAO);
	glBindBuffer(GL_ARRAY_BUFFER, lVBO);
	glBufferData(GL_ARRAY_BUFFER, linePositions.size() * sizeof(glm::vec3),
		linePositions.data(), GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glUniformMatrix4fv(glGetUniformLocation(lShaderprogram, "view"), 1,
		GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(lShaderprogram, "projection"), 1,
		GL_FALSE, glm::value_ptr(projection));
	glUniform3f(glGetUniformLocation(lShaderprogram, "lineColor"), 1.0f, 0.2f, 0.2f);

	glLineWidth(2.0f);
	glDrawArrays(GL_LINES, 0, linePositions.size());
	glBindVertexArray(0);
}

void AlienCharacter::SaveAnimations()
{
	animLibrary->LoadAnimation("Idle", "Animations/alien_animation_idle.glb", animationImporters, animModel);
	animLibrary->LoadAnimation("Run", "Animations/alien_animation_run.glb", animationImporters, animModel);
	animLibrary->LoadAnimation("Attack", "Animations/alien_animation_attack.glb", animationImporters, animModel);
	animLibrary->LoadAnimation("Hit", "Animations/alien_animation_hit.glb", animationImporters, animModel);
	animLibrary->LoadAnimation("Death", "Animations/alien_animation_death.glb", animationImporters, animModel);
	animLibrary->LoadAnimation("Dance", "Animations/cat_animation_dance.glb", animationImporters, animModel);

	animLibrary->ChangeAnimation("Idle", *alien_CurrentAnim);
}

void AlienCharacter::LoadModel()
{
	if (alienType == 1)
	{
		animModel->LoadGLBFile(alienType, *alien_BoneInfo, "Glb/alien_1_Tpose.glb", aVAO, aVBO, aVBO2, aEBO, aIndices);
		aTexture = LoadTexture("Texture/alien_1_basecolor.png");
	}
	else if (alienType == 2)
	{
		animModel->LoadGLBFile(alienType, *alien_BoneInfo, "Glb/alien_2_Tpose.glb", aVAO, aVBO, aVBO2, aEBO, aIndices);
		aTexture = LoadTexture("Texture/alien_2_basecolor.png");
	}
	else if (alienType == 3)
	{
		animModel->LoadGLBFile(alienType, *alien_BoneInfo, "Glb/alien_3_Tpose.glb", aVAO, aVBO, aVBO2, aEBO, aIndices);
		aTexture = LoadTexture("Texture/alien_3_basecolor.png");
	}
}

void AlienCharacter::SetSpawnPosition()
{
	const std::vector<glm::vec3> ENEMY_SPAWN_POINTS = {
	glm::vec3(1.54972, 0.0f, 35.1811),
	glm::vec3(-2.91028, 0.0f, 19.5118),
	glm::vec3(9.50484, 0.0f, 10.4216),
	glm::vec3(26.1052, 0.0f, 21.8918),
	glm::vec3(36.5146, 0.0f, -33.659),
	glm::vec3(12.7399, 0.0f, -32.5541),
	glm::vec3(-7.1254, 0.0f, -45.7119),
	glm::vec3(-16.6856, 0.0f, -28.7741),
	glm::vec3(-48.3682, 0.0f, -19.8239),

	glm::vec3(0.0496913, 0.0f, 36.1712),
	glm::vec3(-1.64031, 0.0f, 18.1016),
	glm::vec3(10.5848, 0.0f, 11.7714),
	glm::vec3(24.5802, 0.0f, 23.1417),
	glm::vec3(37.8644, 0.0f, -32.0894),
	glm::vec3(14.025, 0.0f, -34.249),
	glm::vec3(-6.04527, 0.0f, -44.4673),
	glm::vec3(-15.4905, 0.0f, -30.1794),
	glm::vec3(-47.3833, 0.0f, -21.1092),

	glm::vec3(1.54463, 0.0f, 37.201),
	glm::vec3(-2.87536, 0.0f, 16.5065),
	glm::vec3(11.4648, 0.0f, 10.4414),
	glm::vec3(26.0501, 0.0f, 24.3717),
	glm::vec3(39.3223, 0.0f, -33.7221),
	glm::vec3(12.8531, 0.0f, -36.0768),
	glm::vec3(-4.94215, 0.0f, -45.7352),
	glm::vec3(-16.5574, 0.0f, -31.8925),
	glm::vec3(-48.2449, 0.0f, -22.5823),
	};

	alienPos = ENEMY_SPAWN_POINTS[(9 * (alienType - 1)) + alienLocationSetter];
}

void AlienCharacter::SetSpawnAngle()
{
	static const float angles[] = {
		-0.52f, 2.6f, 1.03f,
		-0.52f, 1.03f, 2.6f,
		1.03f, 2.6f, 2.6f
	};

	if (alienLocationSetter >= 0 && alienLocationSetter < 9)
		viewingAngle = angles[alienLocationSetter];
}

void AlienCharacter::SetupShaders()
{
	SetupShader("Shaders/EnemyVert.glsl", "Shaders/EnemyFrag.glsl", aShaderprogram);
	SetupShader("Shaders/EnemyLineVert.glsl", "Shaders/EnemyLineFrag.glsl", lShaderprogram);
}

void AlienCharacter::RotateAliens(MainCharacter* Cat)
{
	if (Cat->GetDead())
		return;

	model = glm::mat4(1.0f);
	model = glm::translate(model, GetPosition());
	glm::vec3 pos = Cat->GetPosition();
	targetPos = pos;

	float distance = glm::length(glm::vec2(pos.x - alienPos.x, pos.z - alienPos.z));
	float angle = atan2(pos.x - alienPos.x, pos.z - alienPos.z);

	if (distance < 13.0f)
	{
		if (!(state == 4))
		{
			viewingAngle = angle;
		}
	}
	model = glm::rotate(model, viewingAngle, glm::vec3(0.0f, 1.0f, 0.0f));
}

void AlienCharacter::ChangeAnimation()
{
	if (state == 0)
	{
		if (animLibrary->GetCurrentAnimation() != "Idle")
			animLibrary->ChangeAnimation("Idle", *alien_CurrentAnim);
	}
	else if (state == 1)
	{
		if (animLibrary->GetCurrentAnimation() != "Run")
			animLibrary->ChangeAnimation("Run", *alien_CurrentAnim);
	}
	else if (state == 2)
	{
		if (animLibrary->GetCurrentAnimation() != "Attack")
			animLibrary->ChangeAnimation("Attack", *alien_CurrentAnim);
	}
	else if (state == 3)
	{
		if (animLibrary->GetCurrentAnimation() == "Idle")
			animLibrary->ChangeAnimation("Hit", *alien_CurrentAnim);
	}
	else if (state == 4)
	{
		if (animLibrary->GetCurrentAnimation() != "Death")
			animLibrary->ChangeAnimation("Death", *alien_CurrentAnim);
	}
	else if (state == 5)
	{
		if (animLibrary->GetCurrentAnimation() != "Dance")
			animLibrary->ChangeAnimation("Dance", *alien_CurrentAnim);
	}
}

void AlienCharacter::UpdateStateAndBehavior(MainCharacter* Cat, const float deltaTime)
{
	glm::vec3 pos = Cat->GetPosition();
	glm::vec3 direction = glm::normalize(pos - alienPos);
	float distance = glm::length(glm::vec2(pos.x - alienPos.x, pos.z - alienPos.z));

	if (state == 0 && !Cat->GetDead())
	{
		if (distance > 4.0f && distance < 13.0f)
		{
			state = 1;
		}
	}
	else if (state == 1)
	{
		MoveToward(Cat, deltaTime);
	}
	else if (state == 2)
	{
		float currentTime = alien_CurrentAnim->CurrentTime;

		if (currentTime >= 800 && currentTime < 1550)
		{
			// 구간 인덱스 계산 (0~4)
			int interval = (int)((currentTime - 800.0f) / 75.0f);

			// 유효한 구간이고 아직 발사하지 않았다면
			if (interval >= 0 && interval < 10 && !shotFired[interval])
			{
				ActivateBullets();
				shotFired[interval] = true;  // 해당 구간 발사 완료 표시
			}
		}
		float progress = alien_CurrentAnim->CurrentTime / alien_CurrentAnim->Duration;
		if (progress >= 0.95f)
		{
			for (int i = 0; i < 10; ++i)
			{
				shotFired[i] = false;
			}

			if (distance > 4.0f && distance < 13.0f)
			{
				state = 1;
			}
			else if (distance >= 13.0f)
			{
				state = 0;
			}

			DeactivateBullets();

			if (Cat->GetDead())
			{
				state = 0;
			}
		}
	}
	else if (state == 3)
	{
		float progress = alien_CurrentAnim->CurrentTime / alien_CurrentAnim->Duration;
		if (progress >= 0.95f)
		{
			if (distance > 4.0f && distance < 13.0f)
			{
				state = 1;
			}
			else if (distance >= 13.0f)
			{
				state = 0;
			}
		}
	}
	else if (state == 4)
	{
		float progress = alien_CurrentAnim->CurrentTime / alien_CurrentAnim->Duration;
		if (progress >= 0.95f)
		{
			dead = true;
		}
	}
}

void AlienCharacter::MoveToward(MainCharacter* Cat, const float deltaTime)
{
	if (Cat->GetDead())
	{
		state = 0;
		return;
	}

	if (state == 4)
		return;

	glm::vec3 pos = Cat->GetPosition();
	float distance = glm::length(glm::vec2(pos.x - alienPos.x, pos.z - alienPos.z));
	glm::vec3 direction = glm::normalize(glm::vec3(pos.x - alienPos.x, 0.0f, pos.z - alienPos.z));

	float Move_SPEED = 2.5f;
	glm::vec3 movement = direction * Move_SPEED * deltaTime;

	if (distance > 0.1f) {
        glm::vec3 newPos = alienPos + movement;

        if (!GET_SINGLE(CollisionManager)->IsInsideCollisionBox(newPos.x, newPos.z))
            alienPos = newPos;
        else
        {
            auto* collisionManager = GET_SINGLE(CollisionManager);

            if (movement.x != 0 && !collisionManager->IsInsideCollisionBox(alienPos.x + movement.x, alienPos.z))
                alienPos.x += movement.x;

            if (movement.z != 0 && !collisionManager->IsInsideCollisionBox(alienPos.x, alienPos.z + movement.z))
                alienPos.z += movement.z;
        }
    }

	if (distance <= 4.0f)
	{
		state = 2;
	}
	else if (distance >= 13.0f)
	{
		state = 0;
	}
}

void AlienCharacter::ActivateBullets()
{
	for (int i = 0; i < MAX_BULLETS; ++i)
	{
		if (!bullets[i].isActive)
		{
			bullets[i].isActive = true;
			bullets[i].bullet->BulletSetting(alienPos, targetPos);
			return;
		}
	}
}

void AlienCharacter::DeactivateBullets()
{
	for (int i = 0; i < MAX_BULLETS; ++i)
	{
		bullets[i].isActive = false;
	}
}

void AlienCharacter::UpdateBullets(MainCharacter* Cat, const float deltaTime)
{
	for (int i = 0; i < MAX_BULLETS; ++i)
	{
		if (bullets[i].isActive)
		{
			bullets[i].bullet->BulletUpdate(deltaTime, 10.0f);

			if (bullets[i].bullet->IsCollapsed(Cat))
			{
				bullets[i].isActive = false;
				Cat->SetHit();
			}

			CheckBulletWallHit(i);
		}
	}
}

void AlienCharacter::CheckBulletWallHit(int bulletIndex)
{
	glm::vec3 bulletPos = bullets[bulletIndex].bullet->GetPosition();

	if (GET_SINGLE(CollisionManager)->IsInsideCollisionBox(bulletPos.x, bulletPos.z))
	{
		bullets[bulletIndex].isActive = false;
		cout << bulletIndex << "번째 총알 삭제!!" << '\n';
	}
}

void AlienCharacter::UpdateHitDecision(const float deltaTime)
{
	if (hit_cnt > 0)
		hit_cnt -= deltaTime;
	else
	{
		if (hitcolor != glm::vec4(1.0f, 1.0f, 1.0f, 1.0f))
			hitcolor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	}

	if (life == 0)
	{
		state = 4;
		dying = true;
	}
}

void AlienCharacter::SetHit()
{
	life -= 1;
	hit_cnt = 2.0f;
	hitcolor = glm::vec4(1.0f, 0.6f, 0.6f, 1.0f);
}