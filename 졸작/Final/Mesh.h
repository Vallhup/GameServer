#pragma once
#include "Component.h"

class DX12Core;
class VertexIndexBuffer;
class Material;
struct MaterialData;
struct SubMeshInfo;
struct MeshData;
struct CachedMeshData;

class Mesh : public Component
{
public:
	void SetMesh(DX12Core& core, const wstring& path);
	void SetMesh2(DX12Core& core, const wstring& path);
	void SetCollisionMesh(DX12Core& core, const wstring& path);
	void SetProceduralMesh(DX12Core& core, const vector<Vertex>& vertices, const vector<UINT>& indices);
	void ReleaseUploadBuffers();

	void SetUnlit(bool in) { unlit = in; }
	bool IsUnlit() const { return unlit; }

	void SetBrightness(float in) { brightness = in; }
	float GetBrightness() const { return brightness; }

	VertexIndexBuffer* GetVertexIndexBuffer() const { return vertexIndexBuffer.get(); }
	const vector<SubMeshInfo>& GetSubMeshes() const { return subMeshes; }
	Material* GetMaterial() const { return material.get(); }
	const vector<shared_ptr<Material>>& GetMaterials() const { return materials; }
	const vector<MaterialData>& GetOriginalMaterialData() const { return originalMaterialData; }

	bool HasMultiMaterial() const { return !materials.empty(); }

	VertexIndexBuffer* GetCollisionMeshBuffer() const { return collisionMeshBuffer.get(); }
	void ToggleCollisionMesh() { showCollisionMesh = !showCollisionMesh; }
	bool IsCollisionMeshVisible() const { return showCollisionMesh && collisionMeshBuffer; }

	void SetTwoSided(bool in) { twoSided = in; }
	bool IsTwoSided() const { return twoSided; }

	bool HasCollisionData() const;
	const vector<XMFLOAT3>& GetCollisionPositions() const;
	const vector<UINT>& GetCollisionIndices() const;

private:
	void SetSingleMaterial(DX12Core& core, const vector<MaterialData>& mats, const wstring& texBasePath = L"../Assets/FBXModel/");
	void SetMultiMaterials(DX12Core& core, const vector<MaterialData>& mats, const wstring& texBasePath = L"../Assets/FBXModel/");
	void DebugMaterialInfo(const MeshData& mesh, const vector<MaterialData>& mats);
	void StoreCollisionTriangles(const MeshData& mesh);

private:
	shared_ptr<VertexIndexBuffer> vertexIndexBuffer;
	vector<SubMeshInfo> subMeshes;

	shared_ptr<Material> material;
	vector<shared_ptr<Material>> materials;
	vector<MaterialData> originalMaterialData;

	shared_ptr<VertexIndexBuffer> collisionMeshBuffer;
	bool showCollisionMesh = false;

	bool unlit = false;
	float brightness = 1.0f;

	shared_ptr<CachedMeshData> cachedRef;   

	bool twoSided = false;	
};