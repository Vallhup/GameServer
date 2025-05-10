#include "pch.h"
#include "AStar.h"

int Heuristic(const APos& a, const APos& b)
{
	return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

// node의 parent를 따라가면서 전체 경로를 path에 넣고 return
std::vector<APos> ReconstructPath(NodePtr node)
{
	std::vector<APos> path;
	while (nullptr != node) {
		path.push_back(node->pos);
		node = node->parent;
	}

	std::reverse(path.begin(), path.end());

	return path;
}

std::vector<APos> AStar(bool map[MAP_SIZE][MAP_SIZE], APos start, APos goal)
{
	// priority_queue는 우선순위 큐 (우선순위가 높은 원소부터 pop)
	std::priority_queue<NodePtr, std::vector<NodePtr>, Compare> openList;

	// 이미 탐색 완료된 노드를 true로 표시하여 중복 방문, 무한 루프 방지
	std::unordered_set<APos> closed;

	// gCost를 저장해놓는 container
	//std::unordered_map<APos, int> gScore;
	std::array<std::array<int, MAP_SIZE>, MAP_SIZE> gScore;
	for (auto& row : gScore) {
		row.fill(INT_MAX);
	}

	// 시작 노드 설정
	auto startNode = std::make_shared<Node>();
	startNode->pos = start;
	startNode->gCost = 0;
	startNode->hCost = Heuristic(start, goal);
	openList.push(startNode);
	gScore[start.y][start.x] = 0;


	const std::vector<APos> directions = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };

	while (not openList.empty()) {
		NodePtr current = openList.top();
		openList.pop();

		// 이미 처리된 노드는 건너뛰기
		if (closed.count(current->pos)) {
			continue;
		}

		// goal에 도착했으면
		if (current->pos == goal) {
			// path return
			return ReconstructPath(current);
		}

		// 현재 노드를 true로 설정
		closed.insert(current->pos);

		// 상하좌우 모두 검사
		for (const APos& dir : directions) {
			APos next = { current->pos.x + dir.x, current->pos.y + dir.y };
			if ((next.x < 0) or (next.x >= MAP_SIZE) or (next.y < 0) or (next.y >= MAP_SIZE)) {
				continue;
			}

			if ((not map[next.y][next.x]) or closed.count(next)) {
				continue;
			}

			// 이미 더 나은 경로가 기록되어 있으면 건너뛰기
			int tentativeG = current->gCost + 1;
			if (tentativeG >= gScore[next.y][next.x]) {
				continue;
			}

			// 새로운 경로 발견 시 노드 업데이트
			auto neighbor = std::make_shared<Node>();
			neighbor->pos = next;
			neighbor->gCost = current->gCost + 1;
			neighbor->hCost = Heuristic(next, goal);
			neighbor->parent = current;

			gScore[next.y][next.x] = tentativeG;
			openList.push(neighbor);
		}
	}

	// 경로가 없으면 빈 벡터 반환
	return {};
}