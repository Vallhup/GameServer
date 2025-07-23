#include "pch.h"
#include "GameLogic.h"
#include "CharacterManager.h"

GameLogic::GameLogic(IGameContext& gameCtx) : _gameCtx(gameCtx) 
{
	RegisterHandlers();
}

void GameLogic::LogicUpdate(float deltaTime)
{
	UpdateCharacters(deltaTime);
	UpdateProjectiles(deltaTime);
	CheckCollisions();
}

void GameLogic::NetworkUpdate()
{
	for (auto& character : _gameCtx.GetCharacterManager().GetCharacterList()) {
		if (character->VersionCheckAndChange()) {
			_gameCtx.BroadCast(PacketFactory::SCMovePacket(*character));
		}
	}

	for (auto& projectile : _gameCtx.GetProjectileManager().GetProjectileList()) {
		if (projectile->VersionCheckAndChange()) {
			_gameCtx.BroadCast(PacketFactory::SCMovePacket(*projectile));
		}
	}
}

void GameLogic::OnPlayerAction(int sessionId, const std::vector<char>& packet)
{
	const unsigned char packetType = packet[1];
	auto it = _packetHandlers.find(packetType);

	if (it != _packetHandlers.end()) {
		it->second(sessionId, packet);
	}

	else {
		LOG_ERR("Unknown Packet Type");
	}
}

void GameLogic::UpdateCharacters(float deltaTime)
{
	static int projectileId{ 64 };

	// 1. Character Update (이동)
	_gameCtx.GetCharacterManager().Update(deltaTime);

	// 2. 공격
	auto projectiles = _gameCtx.GetCharacterManager().GetPendingProjectiles(_gameCtx.GetNowTime());
	for (auto& projectile : projectiles) {
		auto projectilePtr = std::make_shared<Projectile>(projectileId++, projectile.characterId, projectile.position, projectile.direction, 10);
		_gameCtx.GetProjectileManager().AddProjectile(projectilePtr);
		_gameCtx.BroadCast(PacketFactory::SCAddPacket(*projectilePtr));
	}
}

void GameLogic::UpdateProjectiles(float deltaTime)
{
	// 1. Projectile Update (이동)
	_gameCtx.GetProjectileManager().Update(deltaTime);

	// 2. 시간 초과 (삭제)
	for (auto& projectile : _gameCtx.GetProjectileManager().GetProjectileList()) {
		if (not projectile->IsActive()) {
			_gameCtx.GetProjectileManager().RemoveProjectile(projectile->GetId());
			_gameCtx.BroadCast(PacketFactory::SCRemovePacket(*projectile));
		}
	}
}

void GameLogic::CheckCollisions()
{
	auto charList = _gameCtx.GetCharacterManager().GetCharacterList();
	auto projList = _gameCtx.GetProjectileManager().GetProjectileList();

	auto collisionList = _gameCtx.GetCollisionManager().GetCollisionList(projList, charList);

	for (auto& [projectile, character] : collisionList) {
		character->TakeDamage(projectile->GetDamage());
		projectile->SetInactive();
		_gameCtx.GetProjectileManager().RemoveProjectile(projectile->GetId());
		_gameCtx.BroadCast(PacketFactory::SCRemovePacket(*projectile));
		_gameCtx.BroadCast(PacketFactory::SCStatUpdatePacket(*character));
	}

	// TODO : 여기에 넣으니까 죽어있는 동안 계속 돌아감 죽었을 때 1번만 실행하도록 만들어야 됨
	/*auto deathList = _gameCtx.GetCharacterManager().GetDeathCharacterList();
	for (auto& deathCharacter : deathList) {
		_gameCtx.BroadCast(PacketFactory::SCDeadPacket(*deathCharacter));
		_gameCtx.GetTimerManager().AddOneTimeTask(
			[deathCharacter, this]()
			{
				if (deathCharacter) {
					deathCharacter->Revive();
					_gameCtx.BroadCast(PacketFactory::SCRevivePacket(*deathCharacter));
				}
			}, 5000.0f);
	}*/
}

void GameLogic::RegisterHandlers()
{
	_packetHandlers[CS_MOVE] =
		[this](int sessionId, const std::vector<char>& packet)
		{
			OnPlayerMove(sessionId, packet);
		};

	_packetHandlers[CS_ATTACK] =
		[this](int sessionId, const std::vector<char>& packet)
		{
			OnPlayerAttack(sessionId, packet);
		};

	_packetHandlers[CS_ATTACK_END] =
		[this](int sessionId, const std::vector<char>& packet)
		{
			OnPlayerAttackEnd(sessionId, packet);
		};
}

void GameLogic::OnPlayerMove(int sessionId, const std::vector<char>& packet)
{
	auto move = PacketFactory::Deserialize<CS_MOVE_PACKET>(packet);
	auto session = _gameCtx.GetSessionManager().GetSession(sessionId);
	auto character = session->GetCharacter();

	if (character) {
		character->SetInput(move.angle, move.direction, move.isRun);

		if (move.direction < 0 or move.direction >= 8) {
			_gameCtx.BroadCast(PacketFactory::SCMovePacket(*character));
		}
	}
}

void GameLogic::OnPlayerAttack(int sessionId, const std::vector<char>& packet)
{
	auto attack = PacketFactory::Deserialize<CS_ATTACK_PACKET>(packet);
	auto session = _gameCtx.GetSessionManager().GetSession(sessionId);
	auto character = session->GetCharacter();

	if (character) {
		vec3 attackDir{ attack.x, attack.y, attack.z };
		character->SetAttackSequence(_gameCtx.GetNowTime(), attackDir);
		_gameCtx.BroadCast(PacketFactory::SCAttackPacket(*session));
	}
}

void GameLogic::OnPlayerAttackEnd(int sessionId, const std::vector<char>& packet)
{
	auto end = PacketFactory::Deserialize<CS_ATTACK_END_PACKET>(packet);
	auto session = _gameCtx.GetSessionManager().GetSession(sessionId);
	auto character = session->GetCharacter();

	if (character) {
		_gameCtx.BroadCast(PacketFactory::SCAttackEndPacket(*session));
	}
}
