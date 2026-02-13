#pragma once
#include "Component.h"

class DX12Core;
class VertexIndexBuffer;
class Material;
struct MaterialData;
struct SubMeshInfo;
struct MeshData;

class Mesh : public Component
{
public:
	void SetMesh(DX12Core& core, const wstring& path);
	void SetMesh2(DX12Core& core, const wstring& path);
	void SetCollisionMesh(DX12Core& core, const wstring& path);
	void ReleaseUploadBuffers();

	VertexIndexBuffer* GetVertexIndexBuffer() const { return vertexIndexBuffer.get(); }
	const vector<SubMeshInfo>& GetSubMeshes() const { return subMeshes; }
	Material* GetMaterial() const { return material.get(); }
	const vector<shared_ptr<Material>>& GetMaterials() const { return materials; }
	const vector<MaterialData>& GetOriginalMaterialData() const { return originalMaterialData; }

	bool HasMultiMaterial() const { return !materials.empty(); }

	// Collision Mesh
	VertexIndexBuffer* GetCollisionMeshBuffer() const { return collisionMeshBuffer.get(); }
	void ToggleCollisionMesh() { showCollisionMesh = !showCollisionMesh; }
	bool IsCollisionMeshVisible() const { return showCollisionMesh && collisionMeshBuffer; }

private:
	void SetSingleMaterial(DX12Core& core, const vector<MaterialData>& mats);
	void SetMultiMaterials(DX12Core& core, const vector<MaterialData>& mats);
	void DebugMaterialInfo(const MeshData& mesh, const vector<MaterialData>& mats);

private:
	shared_ptr<VertexIndexBuffer> vertexIndexBuffer;
	vector<SubMeshInfo> subMeshes;

	shared_ptr<Material> material;
	vector<shared_ptr<Material>> materials;
	vector<MaterialData> originalMaterialData;

	// Collision Mesh
	shared_ptr<VertexIndexBuffer> collisionMeshBuffer;
	bool showCollisionMesh = false;
};