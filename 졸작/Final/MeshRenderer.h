#pragma once
#include "Component.h"

class VertexIndexBuffer;

class MeshRenderer : public Component
{
public:
	MeshRenderer();
	~MeshRenderer();

	void Update(float deltaTime) override;
	void Render();
	void SetMesh(const wstring& path);
private:
	unique_ptr<VertexIndexBuffer> vertexIndexBuffer;  
	//MaterialProperties material;
	bool visible = true;
};

