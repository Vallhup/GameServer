#pragma once

struct FbxMaterialInfo
{
	Vec4			diffuse;
	Vec4			ambient;
	Vec4			specular;
	wstring			name;

	wstring diffuseTexName;     // 또는 baseColorTexName
	wstring normalTexName;
	wstring specularTexName;    // 또는 roughnessTexName

	wstring baseColorTexName;   // BaseColor
	wstring metallicTexName;    // Metallic
	wstring roughnessTexName;   // Roughness
	wstring heightTexName;      // Height
	wstring alphaTexName;       // Alpha (투명도)
	wstring emissionTexName;    // Emission (발광)
	wstring aoTexName;          // Ambient Occlusion
};

struct VertexKey {
	Vec3 pos;
	Vec2 uv;
	Vec3 normal;

	bool operator==(const VertexKey& other) const {
		const float eps = 1e-5f;
		return fabsf(pos.x - other.pos.x) < eps &&
			fabsf(pos.y - other.pos.y) < eps &&
			fabsf(pos.z - other.pos.z) < eps &&
			fabsf(uv.x - other.uv.x) < eps &&
			fabsf(uv.y - other.uv.y) < eps &&
			fabsf(normal.x - other.normal.x) < eps &&
			fabsf(normal.y - other.normal.y) < eps &&
			fabsf(normal.z - other.normal.z) < eps;
	}
};

struct VertexKeyHash {
	size_t operator()(const VertexKey& k) const {
		auto h1 = hash<int>{}(static_cast<int>(k.pos.x * 10000));
		auto h2 = hash<int>{}(static_cast<int>(k.pos.y * 10000));
		auto h3 = hash<int>{}(static_cast<int>(k.pos.z * 10000));
		auto h4 = hash<int>{}(static_cast<int>(k.uv.x * 10000));
		auto h5 = hash<int>{}(static_cast<int>(k.uv.y * 10000));
		return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4);
	}
};

struct BoneWeight
{
	using Pair = pair<int32, double>;
	vector<Pair> boneWeights;

	void AddWeights(uint32 index, double weight)
	{
		if (weight <= 0.f)
			return;

		auto findIt = std::find_if(boneWeights.begin(), boneWeights.end(),
			[=](const Pair& p) { return p.second < weight; });

		if (findIt != boneWeights.end())
			boneWeights.insert(findIt, Pair(index, weight));
		else
			boneWeights.push_back(Pair(index, weight));

		// 가중치는 최대 4개
		if (boneWeights.size() > 4)
			boneWeights.pop_back();
	}

	void Normalize()
	{
		double sum = 0.f;
		std::for_each(boneWeights.begin(), boneWeights.end(), [&](Pair& p) { sum += p.second; });
		std::for_each(boneWeights.begin(), boneWeights.end(), [=](Pair& p) { p.second = p.second / sum; });
	}
};

struct FbxMeshInfo
{
	wstring								name;
	vector<Vertex>						vertices;
	vector<vector<uint32>>				indices;
	vector<FbxMaterialInfo>				materials;
	vector<BoneWeight>					boneWeights; // 뼈 가중치
	bool								hasAnimation;
};

struct FbxKeyFrameInfo
{
	FbxAMatrix  matTransform;
	double		time;
};

struct FbxBoneInfo
{
	wstring					boneName;
	int32					parentIndex;
	FbxAMatrix				matOffset;
};

struct FbxAnimClipInfo
{
	wstring			name;
	FbxTime			startTime;
	FbxTime			endTime;
	FbxTime::EMode	mode;
	vector<vector<FbxKeyFrameInfo>>	keyFrames;
};

class FBXLoader
{
public:
	FBXLoader();
	~FBXLoader();

public:
	bool LoadFbx(const wstring& path);

public:
	int32 GetMeshCount() { return static_cast<int32>(_meshes.size()); }
	const FbxMeshInfo& GetMesh(int32 idx) { return _meshes[idx]; }
	vector<shared_ptr<FbxBoneInfo>>& GetBones() { return _bones; }
	vector<shared_ptr<FbxAnimClipInfo>>& GetAnimClip() { return _animClips; }
private:
	bool Import(const wstring& path);

	void ParseNode(FbxNode* root);
	void LoadMesh(FbxMesh* mesh);
	void LoadCollisionMesh(FbxMesh* mesh);
	void LoadMaterial(FbxSurfaceMaterial* surfaceMaterial);
	void LoadAllTextures(FbxSurfaceMaterial* surfaceMaterial, FbxMaterialInfo& material);
	bool ContainsKeyword(const string& propName, const vector<string>& keywords);

	void		GetNormal(FbxMesh* mesh, FbxMeshInfo* container, int32 idx, int32 vertexCounter);
	void		GetTangent(FbxMesh* mesh, FbxMeshInfo* container, int32 idx, int32 vertexCounter);
	void		GetUV(FbxMesh* mesh, FbxMeshInfo* container, int32 idx, int32 vertexCounter);
	Vec4		GetMaterialData(FbxSurfaceMaterial* surface, const char* materialName, const char* factorName);
	wstring		GetTextureRelativeName(FbxSurfaceMaterial* surface, const char* materialProperty);

	// Animation
	void LoadBones(FbxNode* node) { LoadBones(node, 0, -1); }
	void LoadBones(FbxNode* node, int32 idx, int32 parentIdx);
	void LoadAnimationInfo();
	void LoadStandaloneAnimations();
	void LoadBoneKeyframes(int32 animIndex, FbxNode* boneNode, int32 boneIdx);

	void LoadAnimationData(FbxMesh* mesh, FbxMeshInfo* meshInfo);
	void LoadBoneWeight(FbxCluster* cluster, int32 boneIdx, FbxMeshInfo* meshInfo);
	void LoadOffsetMatrix(FbxCluster* cluster, const FbxAMatrix& matNodeTransform, int32 boneIdx, FbxMeshInfo* meshInfo);
	void LoadKeyframe(int32 animIndex, FbxNode* node, FbxCluster* cluster, const FbxAMatrix& matNodeTransform, int32 boneIdx, FbxMeshInfo* container);

	int32 FindBoneIndex(string name);
	FbxAMatrix GetTransform(FbxNode* node);

	void FillBoneWeight(FbxMesh* mesh, FbxMeshInfo* meshInfo);
	void RemapBoneWeights(FbxMesh* mesh, const vector<int32>& controlPointMapping, FbxMeshInfo& meshInfo);

private:
	FbxManager* _manager = nullptr;
	FbxScene* _scene = nullptr;
	FbxImporter* _importer = nullptr;
	wstring			_resourceDirectory;

	vector<FbxMeshInfo>					_meshes;
	vector<shared_ptr<FbxBoneInfo>>		_bones;
	vector<shared_ptr<FbxAnimClipInfo>>	_animClips;
	FbxArray<FbxString*>				_animNames;
};