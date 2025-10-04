#include "ClientManager.h"

#include <chrono>
#include <numeric>
#include <ranges>

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

		_clients.push_back(std::move(client));
		lastConnectTime = high_resolution_clock::now();
	}
}

void ClientManager::OnTick()
{
	using namespace std::chrono;

	for (auto& client : _clients) {
		if (client and client->GetState() == ClientState::ST_INGAME) {
			// TEMP : 시나리오 실행
			MoveScenario::Instance().OnTick(client.get());
		}
	}
}

void ClientManager::DisconnectClient(int id)
{
	for (auto& client : _clients) {
		if (client->GetId() == id) {
			client->Disconnect();
		}
	}
}

std::vector<Client*> ClientManager::GetClientList()
{
	std::vector<Client*> result;
	for (auto& client : _clients) {
		if (client and client->IsConnected()) {
			result.push_back(client.get());
		}
	}

	return result;
}
