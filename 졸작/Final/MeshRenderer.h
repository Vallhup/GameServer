#pragma once
#include "Component.h"

class VertexIndexBuffer;
class Material;
class UploadBuffer;

class MeshRenderer : public Component
{
public:
	MeshRenderer();
	~MeshRenderer();

	void Update(float deltaTime) override;
	void Render();
	void RenderInstanced(UINT instanceCount, UploadBuffer* instancedBuffer);
	void SetMesh(const wstring& path);
private:
	unique_ptr<VertexIndexBuffer> vertexIndexBuffer;  
	shared_ptr<Material> material;
	bool visible = true;

	UINT myID;
	static UINT idCounter;
};

