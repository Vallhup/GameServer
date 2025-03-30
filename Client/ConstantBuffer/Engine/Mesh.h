#pragma once

class Mesh
{
public:
	void Init(vector<Vertex>& vec);
	void Render();

	void MakeTriangles(vector<Vertex>& v);

private:
	ComPtr<ID3D12Resource>		_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW	_vertexBufferView = {};
	uint32						_vertexCount = { 0 };
};
