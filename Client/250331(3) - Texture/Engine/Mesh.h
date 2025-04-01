#pragma once

class Texture;

class Mesh
{
public:
	void Init();
	void Render();

	void MakeTriangle();
	void MakeRectangle();
	void SetRectangleIdx();

	void SetTransform(const Transform& t) { _transform = t; }
	void SetTexture(shared_ptr<Texture> tex) { _tex = tex; }

private:
	void CreateVertexBuffer();
	void CreateIndexBuffer();

private:
	ComPtr<ID3D12Resource>		_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW	_vertexBufferView = {};
	uint32						_vertexCount = { 0 };

	ComPtr<ID3D12Resource>		_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW		_indexBufferView = {};
	uint32						_indexCount = { 0 };

	vector<Vertex> triangle;
	
	vector<Vertex> rectangle;
	vector<uint32> rectIdx;

	Transform _transform = {};
	shared_ptr<Texture> _tex = {};
};
