#pragma once

struct AnimInfo {
	float Duration;
	float TicksPerSecond;
	float CurrentTime;
	const aiAnimation* animation;
	const aiNode* rootNode;
	bool isPlaying;

	AnimInfo() {
		Duration = 0.0f;
		TicksPerSecond = 0.0f;
		CurrentTime = 0.0f;
		animation = nullptr;
		rootNode = nullptr;
		isPlaying = false;
	}
};

struct BoneInfo {
	glm::mat4 BoneOffset;
	glm::mat4 FinalTransformation;
	glm::mat4 LocalTransform;
	aiNodeAnim* nodeAnim;

	BoneInfo() {
		BoneOffset = glm::mat4(1.0f);
		FinalTransformation = glm::mat4(1.0f);
		LocalTransform = glm::mat4(1.0f);
		nodeAnim = nullptr;
	}
};

struct VertexBoneData {
	unsigned int BoneIDs[MAX_NUM_BONES_PER_VERTEX];
	float Weights[MAX_NUM_BONES_PER_VERTEX];

	VertexBoneData() {
		for (int i = 0; i < MAX_NUM_BONES_PER_VERTEX; i++) {
			BoneIDs[i] = -1;
			Weights[i] = 0.0f;
		}
	}

	void AddBoneData(unsigned int BoneID, float Weight) {
		const float MIN_WEIGHT = 0.000001f;
		if (Weight < MIN_WEIGHT)
			return;

		int emptySlot = -1;
		for (int i = 0; i < MAX_NUM_BONES_PER_VERTEX; i++) {
			if (Weights[i] == 0.0f) {
				emptySlot = i;
				break;
			}
		}

		if (emptySlot != -1) {
			BoneIDs[emptySlot] = BoneID;
			Weights[emptySlot] = Weight;
		}
		else {
			float minWeight = Weights[0];
			int minWeightIndex = 0;
			for (int i = 1; i < MAX_NUM_BONES_PER_VERTEX; i++) {
				if (Weights[i] < minWeight) {
					minWeight = Weights[i];
					minWeightIndex = i;
				}
			}
			if (Weight > minWeight) {
				BoneIDs[minWeightIndex] = BoneID;
				Weights[minWeightIndex] = Weight;
			}
		}
	}

	void Normalize() {
		float total = 0.0f;
		int validBones = 0;

		for (int i = 0; i < MAX_NUM_BONES_PER_VERTEX; i++) {
			if (BoneIDs[i] != -1 && Weights[i] > 0.0f) {
				total += Weights[i];
				validBones++;
			}
		}

		if (total > 0.0f) {
			for (int i = 0; i < MAX_NUM_BONES_PER_VERTEX; i++) {
				if (BoneIDs[i] != -1) {
					Weights[i] /= total;
				}
			}
		}
		else {
			BoneIDs[0] = 0;
			Weights[0] = 1.0f;

			for (int i = 1; i < MAX_NUM_BONES_PER_VERTEX; i++) {
				BoneIDs[i] = -1;
				Weights[i] = 0.0f;
			}
		}

		float finalTotal = 0.0f;
		for (int i = 0; i < MAX_NUM_BONES_PER_VERTEX; i++) {
			if (BoneIDs[i] != -1) {
				finalTotal += Weights[i];
			}
		}

		if (abs(finalTotal - 1.0f) > 0.0001f) {
			for (int i = 0; i < MAX_NUM_BONES_PER_VERTEX; i++) {
				if (BoneIDs[i] != -1) {
					Weights[i] += (1.0f - finalTotal);
					break;
				}
			}
		}
	}
};

class AnimatedModel
{
	// public protected private
public:
	struct AnimationLibrary {
		unordered_map<std::string, AnimInfo> animations;
		std::string currentAnimationName;

		void LoadAnimation(const std::string& name, const std::string& filename,
			vector<unique_ptr<Assimp::Importer>>& importers, AnimatedModel* model) {
			importers.push_back(make_unique<Assimp::Importer>());
			const aiScene* scene = importers.back()->ReadFile(filename,
				aiProcess_Triangulate | aiProcess_FlipUVs);

			if (scene->mAnimations[0]->mNumChannels > 0) {
				aiNodeAnim* channel = scene->mAnimations[0]->mChannels[0];
			}

			if (scene && scene->HasAnimations()) {
				AnimInfo animInfo;
				animInfo.animation = scene->mAnimations[0];
				animInfo.Duration = scene->mAnimations[0]->mDuration;
				animInfo.TicksPerSecond = (scene->mAnimations[0]->mTicksPerSecond != 0) ?
					scene->mAnimations[0]->mTicksPerSecond : 24.0f;
				animInfo.rootNode = scene->mRootNode;

				for (unsigned int i = 0; i < scene->mAnimations[0]->mNumChannels; i++) {
					std::string animBoneName = scene->mAnimations[0]->mChannels[i]->mNodeName.data;
					if (model->m_BoneNameToIndexMap.find(animBoneName) == model->m_BoneNameToIndexMap.end()) {
					}
				}

				animations[name] = animInfo;
			}
			else
				cout << "Unloaded animation: " << name << endl;
		}

		void ChangeAnimation(const std::string& name, AnimInfo& currentAnim) {
			auto it = animations.find(name);
			if (it != animations.end()) {
				currentAnim = it->second;
				currentAnim.CurrentTime = 0.0f;
				currentAnim.isPlaying = true;
				currentAnimationName = name;
			}
		}

		const std::string& GetCurrentAnimation() const {
			return currentAnimationName;
		}
	};

public:
	AnimatedModel();
	void LoadGLBFile(int j, vector<BoneInfo>& BoneInfoName, const std::string& filename, GLuint& VAO, GLuint& VBO, GLuint& VBO2, GLuint& EBO, vector<unsigned int>& Indices); 
	void SetupBoneTransforms(const vector<BoneInfo>& BoneInfoName, GLuint shadername);
	void UpdateAnimation(int j, vector<BoneInfo>& BoneInfoName, float deltaTime, AnimInfo& currentAnim);

private:
	void NormalizeBoneWeights();
	void CheckWeights();
	void CalculateNodeTransform(int j, vector<BoneInfo>& BoneInfoName, const aiNode* pNode, const glm::mat4& ParentTransform, const AnimInfo& currentAnim);

	void LoadBones(const aiMesh* mesh, vector<BoneInfo>& BoneInfoName);
	
private:
	int Get_Bone_Id(const aiBone* pBone);
	void Parse_Single_Bone(int mesh_index, const aiBone* pBone);
	void Parsh_Mesh_Bones(int mesh_index, const aiMesh* pMesh);
	void Parse_Meshes(const aiScene* pScene);
	void Parse_Scene(const aiScene* pScene);

private:
	glm::mat4 AiMatrix4x4ToGlm(const aiMatrix4x4& from);
	const aiNodeAnim* FindNodeAnim(const aiAnimation* animation, const std::string& nodeName);

	glm::vec3 InterpolateScale(float AnimationTime, const aiNodeAnim* pNodeAnim);
	glm::quat InterpolateRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
	glm::vec3 InterpolatePosition(float AnimationTime, const aiNodeAnim* pNodeAnim);

private:
	std::vector<int> mesh_base_vertex;
	std::vector<VertexBoneData> vertex_to_bones;

private:
	std::map<std::string, unsigned int> m_BoneNameToIndexMap;
	std::vector<glm::mat4> m_GlobalInverseTransform;
	Assimp::Importer characterImporter;
};