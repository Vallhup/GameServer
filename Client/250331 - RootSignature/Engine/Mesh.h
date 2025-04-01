#pragma once

class Mesh
{
public:
	void Init();
	void Render();

	void MakeTriangle();
	void MakeRectangle();

	void SetTransform(const Transform& t) { _transform = t; }

private:
	ComPtr<ID3D12Resource>		_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW	_vertexBufferView = {};
	uint32						_vertexCount = { 0 };

	vector<Vertex> triangle;
	vector<Vertex> rectangle;

	Transform _transform = {};
};
