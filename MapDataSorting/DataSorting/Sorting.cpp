#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <DirectXMath.h>
using namespace std;
using namespace DirectX;

struct SRTData {
	XMFLOAT3 position;
	XMFLOAT3 rotation;
	XMFLOAT3 scale;
};

int main()
{
	map<string, vector<SRTData>> m;

	ifstream in{ "MapInstanceData.txt" };
	string s;

	while (in >> s)
	{
		SRTData data;
		if (in >> data.position.x >> data.position.y >> data.position.z >>
			data.rotation.x >> data.rotation.y >> data.rotation.z >>
			data.scale.x >> data.scale.y >> data.scale.z)
			m[s].push_back(data);
	}

	ofstream out{ "SortedMapData.txt" };
	int cnt = 0;
	for (const auto& [name, SRT] : m)
	{
		for (const auto& data : SRT)
		{
			out << name << " " <<
				data.position.x << " " << data.position.y << " " << data.position.z << " " <<
				data.rotation.x << " " << data.rotation.y << " " << data.rotation.z << " " <<
				data.scale.x << " " << data.scale.y << " " << data.scale.z << '\n';
		}
		cnt++;
	}

	cout << cnt << "Á¾·ù" << '\n';
}