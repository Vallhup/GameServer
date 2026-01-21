#include "pch.h"
#include "ClientManager.h"

#include "Scenario.h"
#include "Service.h"

void ClientManager::AdjustClients()
{
	using namespace std::chrono;

	static bool increasing{ true };
	static int clientToClose{ 0 };
	static int maxLimit{ (std::numeric_limits<int>::max)() };
	static auto lastConnectTime = high_resolution_clock::now();

	/*int activeCount{ 0 };
	for (auto& client : _clients) {
		if (client and client->IsConnected()) {
			activeCount++;
		}
	}

	if (activeCount >= _maxClients) return;

	auto duration = high_resolution_clock::now() - lastConnectTime;
	if (duration_cast<milliseconds>(duration).count() < 100) return;

	if (_globalDelay > 150) {
		if (increasing) {
			maxLimit = activeCount;
			increasing = false;
		}

		if (activeCount > 100) {
			if (clientToClose < _clients.size() and _clients[clientToClose]) {
				DisconnectClient(clientToClose);
				clientToClose++;
			}
		}

		return;
	}*/


	auto client = std::make_shared<Client>();
	if (client->Connect()) {
		_service.RegisterClient(client);
		ConnectScenario::Instance().OnStart(client.get());

		{
			std::lock_guard lock(_clientMutex);
			_clients.push_back(std::move(client));
		}
		lastConnectTime = high_resolution_clock::now();
	}
}

void ClientManager::OnTick(float deltaTime)
{
	using namespace std::chrono;

	std::lock_guard lock(_clientMutex);
	for (auto& client : _clients) {
		if (client and client->GetState() == ClientState::ST_INGAME) {
			client->Update(deltaTime);

			// TEMP : 시나리오 실행
			MoveScenario::Instance().OnTick(client.get());
		}
	}
}

void ClientManager::DisconnectClient(int id)
{
	std::lock_guard lock(_clientMutex);
	for (auto& client : _clients) {
		if (client->GetId() == id) {
			client->Disconnect();
		}
	}
}

const std::vector<std::shared_ptr<Client>>& ClientManager::GetClientList()
{
	std::lock_guard lock(_clientMutex);
	return _clients;
}
