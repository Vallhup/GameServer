#include "pch.h"
#include "AnimatedModel.h"

AnimatedModel::AnimatedModel()
{
	m_GlobalInverseTransform.resize(10, glm::mat4(1.0f));
}

void AnimatedModel::LoadGLBFile(int j, vector<BoneInfo>& BoneInfoName, const std::string& filename, GLuint& VAO, GLuint& VBO, GLuint& VBO2, GLuint& EBO, vector<unsigned int>& Indices)
{
	
	const aiScene* scene = characterImporter.ReadFile(filename,
		aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices |
		aiProcess_CalcTangentSpace);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		std::cout << "Failed to load GLB file: " << characterImporter.GetErrorString() << std::endl;
		return;
	}

	BoneInfoName.clear();
	BoneInfoName.resize(MAX_BONES);
	m_BoneNameToIndexMap.clear();

	aiMatrix4x4 GlobalTransform = scene->mRootNode->mTransformation;
	m_GlobalInverseTransform[j] = glm::inverse(glm::mat4(
		GlobalTransform.a1, GlobalTransform.b1, GlobalTransform.c1, GlobalTransform.d1,
		GlobalTransform.a2, GlobalTransform.b2, GlobalTransform.c2, GlobalTransform.d2,
		GlobalTransform.a3, GlobalTransform.b3, GlobalTransform.c3, GlobalTransform.d3,
		GlobalTransform.a4, GlobalTransform.b4, GlobalTransform.c4, GlobalTransform.d4
	));

	for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
		const aiMesh* mesh = scene->mMeshes[i];
		if (mesh->HasBones()) {
			for (unsigned int j = 0; j < mesh->mNumBones; j++) {
				const aiBone* bone = mesh->mBones[j];
				std::string boneName(bone->mName.C_Str());

				if (m_BoneNameToIndexMap.find(boneName) == m_BoneNameToIndexMap.end()) {
					unsigned int boneIndex = m_BoneNameToIndexMap.size();
					m_BoneNameToIndexMap[boneName] = boneIndex;

					BoneInfo bi;
					bi.BoneOffset = glm::mat4(
						bone->mOffsetMatrix.a1, bone->mOffsetMatrix.b1, bone->mOffsetMatrix.c1, bone->mOffsetMatrix.d1,
						bone->mOffsetMatrix.a2, bone->mOffsetMatrix.b2, bone->mOffsetMatrix.c2, bone->mOffsetMatrix.d2,
						bone->mOffsetMatrix.a3, bone->mOffsetMatrix.b3, bone->mOffsetMatrix.c3, bone->mOffsetMatrix.d3,
						bone->mOffsetMatrix.a4, bone->mOffsetMatrix.b4, bone->mOffsetMatrix.c4, bone->mOffsetMatrix.d4
					);

					if (boneIndex < MAX_BONES) {
						BoneInfoName[boneIndex] = bi;
					}
				}
			}
		}
	}

	Parse_Scene(scene);
	//CheckBoneHierarchy(scene->mRootNode);		// 본 계층구조 확인함수
	//PrintBoneHierarchy(scene->mRootNode);
	//checkFileAnimation(scene);		// T-pose 캐릭터 파일에 애니메이션 있는지 확인

	for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
		const aiMesh* mesh = scene->mMeshes[i];
		LoadBones(mesh, BoneInfoName);
	}

	std::vector<float> vertexData;
	std::vector<int> boneData;
	unsigned int totalVertices = 0;

	Indices.clear();

	for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++) {
		aiMesh* mesh = scene->mMeshes[meshIndex];

		// 텍스처 좌표 추출 (동일)
		std::vector<float> texCoords;
		if (mesh->HasTextureCoords(0)) {
			for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
				aiVector3D texCoord = mesh->mTextureCoords[0][i];
				texCoords.push_back(texCoord.x);
				texCoords.push_back(texCoord.y);
			}
		}
		else {
			for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
				texCoords.push_back(0.0f);
				texCoords.push_back(0.0f);
			}
		}

		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			aiVector3D vertex = mesh->mVertices[i];
			vertexData.push_back(vertex.x);
			vertexData.push_back(vertex.y);
			vertexData.push_back(vertex.z);

			if (mesh->HasNormals()) {
				aiVector3D normal = mesh->mNormals[i];
				vertexData.push_back(normal.x);
				vertexData.push_back(normal.y);
				vertexData.push_back(normal.z);
			}
			else {
				vertexData.push_back(0.0f);
				vertexData.push_back(1.0f);
				vertexData.push_back(0.0f);
			}

			if (mesh->HasTextureCoords(0)) {
				vertexData.push_back(texCoords[i * 2]);
				vertexData.push_back(texCoords[i * 2 + 1]);
			}
			else {
				vertexData.push_back(0.0f);
				vertexData.push_back(0.0f);
			}

			for (int j = 0; j < MAX_NUM_BONES_PER_VERTEX; j++) {
				vertexData.push_back(vertex_to_bones[totalVertices + i].Weights[j]);
			}

			for (int j = 0; j < MAX_NUM_BONES_PER_VERTEX; j++) {
				boneData.push_back(vertex_to_bones[totalVertices + i].BoneIDs[j]);
			}
		}

		for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++) {
				Indices.push_back(face.mIndices[j] + totalVertices);
			}
		}

		totalVertices += mesh->mNumVertices;
	}

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &VBO2);
	glBindBuffer(GL_ARRAY_BUFFER, VBO2);
	glBufferData(GL_ARRAY_BUFFER, boneData.size() * sizeof(int), boneData.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, Indices.size() * sizeof(unsigned int), Indices.data(), GL_STATIC_DRAW);

	const GLsizei vertexStride = (3 + 3 + 2 + 4) * sizeof(float);  // position + normal + texcoord + weights

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexStride, (void*)0);  // position
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertexStride, (void*)(3 * sizeof(float)));  // normal
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, vertexStride, (void*)(6 * sizeof(float)));  // texcoord
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, vertexStride, (void*)(8 * sizeof(float)));  // weights

	glBindBuffer(GL_ARRAY_BUFFER, VBO2);
	glBufferData(GL_ARRAY_BUFFER, boneData.size() * sizeof(int), boneData.data(), GL_STATIC_DRAW);
	glVertexAttribIPointer(3, 4, GL_INT, 0, (void*)0);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glEnableVertexAttribArray(4);

	glBindVertexArray(0);
}

void AnimatedModel::NormalizeBoneWeights()
{
	for (auto& vertex : vertex_to_bones) {
		float total = 0.0f;
		for (int i = 0; i < MAX_NUM_BONES_PER_VERTEX; i++) {
			if (vertex.Weights[i] < 0.01f) {
				vertex.Weights[i] = 0.0f;
			}
			total += vertex.Weights[i];
		}

		if (total > 0.0f) {
			for (int i = 0; i < MAX_NUM_BONES_PER_VERTEX; i++) {
				vertex.Weights[i] /= total;
			}
		}
		else {
			vertex.BoneIDs[0] = 0;
			vertex.Weights[0] = 1.0f;
		}
	}
}

void AnimatedModel::CheckWeights()
{
	for (size_t i = 0; i < vertex_to_bones.size(); i++) {
		float total = 0.0f;
		for (int j = 0; j < MAX_NUM_BONES_PER_VERTEX; j++) {
			total += vertex_to_bones[i].Weights[j];
		}
		if (total < 0.99f || total > 1.01f) {
			std::cout << "Vertex " << i << " has invalid weight total: " << total << std::endl;
		}
	}
}

void AnimatedModel::CalculateNodeTransform(int j, vector<BoneInfo>& BoneInfoName, const aiNode* pNode, const glm::mat4& ParentTransform, const AnimInfo& currentAnim)
{
	std::string NodeName(pNode->mName.data);
	glm::mat4 NodeTransformation = AiMatrix4x4ToGlm(pNode->mTransformation);
	const aiNodeAnim* pNodeAnim = FindNodeAnim(currentAnim.animation, NodeName);

	if (pNodeAnim) {
		glm::vec3 Scaling = InterpolateScale(currentAnim.CurrentTime, pNodeAnim);
		glm::mat4 ScalingM = glm::scale(glm::mat4(1.0f), Scaling);

		glm::quat RotationQ = InterpolateRotation(currentAnim.CurrentTime, pNodeAnim);
		glm::mat4 RotationM = glm::mat4_cast(RotationQ);

		glm::vec3 Translation = InterpolatePosition(currentAnim.CurrentTime, pNodeAnim);
		glm::mat4 TranslationM = glm::translate(glm::mat4(1.0f), Translation);

		NodeTransformation = TranslationM * RotationM * ScalingM;
	}

	glm::mat4 GlobalTransformation = ParentTransform * NodeTransformation;

	if (m_BoneNameToIndexMap.find(NodeName) != m_BoneNameToIndexMap.end()) {
		unsigned int BoneIndex = m_BoneNameToIndexMap[NodeName];
		BoneInfoName[BoneIndex].LocalTransform = NodeTransformation;
		BoneInfoName[BoneIndex].FinalTransformation = m_GlobalInverseTransform[j] * GlobalTransformation * BoneInfoName[BoneIndex].BoneOffset;
	}

	for (unsigned int i = 0; i < pNode->mNumChildren; i++) {
		CalculateNodeTransform(j, BoneInfoName, pNode->mChildren[i], GlobalTransformation, currentAnim);
	}
}

void AnimatedModel::LoadBones(const aiMesh* mesh, vector<BoneInfo>& BoneInfoName)
{
	for (unsigned int i = 0; i < mesh->mNumBones; i++) {
		string BoneName(mesh->mBones[i]->mName.data);
		if (m_BoneNameToIndexMap.find(BoneName) != m_BoneNameToIndexMap.end()) {
			unsigned int BoneIndex = m_BoneNameToIndexMap[BoneName];

			aiMatrix4x4& offset = mesh->mBones[i]->mOffsetMatrix;

			BoneInfoName[BoneIndex].BoneOffset = glm::mat4(
				offset.a1, offset.b1, offset.c1, offset.d1,
				offset.a2, offset.b2, offset.c2, offset.d2,
				offset.a3, offset.b3, offset.c3, offset.d3,
				offset.a4, offset.b4, offset.c4, offset.d4
			);
		}
	}
}

void AnimatedModel::SetupBoneTransforms(const vector<BoneInfo>& BoneInfoName, GLuint shadername)
{
	for (unsigned int i = 0; i < BoneInfoName.size(); i++) {
		std::string uniformName = "gBones[" + std::to_string(i) + "]";
		GLint boneTransformLoc = glGetUniformLocation(shadername, uniformName.c_str());
		if (boneTransformLoc != -1) {
			glUniformMatrix4fv(boneTransformLoc, 1, GL_FALSE,
				glm::value_ptr(BoneInfoName[i].FinalTransformation));
		}
	}
}

void AnimatedModel::UpdateAnimation(int j, vector<BoneInfo>& BoneInfoName, float deltaTime, AnimInfo& currentAnim)
{
	if (!currentAnim.animation || !currentAnim.isPlaying) {
		return;
	}
	currentAnim.CurrentTime = fmod(currentAnim.CurrentTime + deltaTime * currentAnim.TicksPerSecond, currentAnim.Duration);
	CalculateNodeTransform(j, BoneInfoName, currentAnim.rootNode, glm::mat4(1.0f), currentAnim);
}

int AnimatedModel::Get_Bone_Id(const aiBone* pBone)
{
	int bone_id = 0;
	std::string bone_name(pBone->mName.C_Str());

	if (m_BoneNameToIndexMap.find(bone_name) == m_BoneNameToIndexMap.end())
	{
		if (m_BoneNameToIndexMap.size() >= MAX_BONES) {
			std::cerr << "Error: Exceeded maximum number of bones (" << MAX_BONES << ")" << std::endl;
			return 0;
		}

		bone_id = static_cast<int>(m_BoneNameToIndexMap.size());
		m_BoneNameToIndexMap[bone_name] = bone_id;

		std::cout << "Adding new bone: " << bone_name << " with ID: " << bone_id << std::endl;
	}
	else
	{
		bone_id = m_BoneNameToIndexMap[bone_name];
	}

	if (bone_id >= MAX_BONES) {
		std::cerr << "Warning: Bone ID " << bone_id << " exceeds MAX_BONES (" << MAX_BONES << ")" << std::endl;
		return 0;
	}

	return bone_id;
}

void AnimatedModel::Parse_Single_Bone(int mesh_index, const aiBone* pBone)
{
	if (!pBone) return;

	int bone_id = Get_Bone_Id(pBone);
	std::string bone_name(pBone->mName.C_Str());

	for (unsigned int i = 0; i < pBone->mNumWeights; ++i) {
		const aiVertexWeight& vw = pBone->mWeights[i];
		unsigned int global_vertex_id = mesh_base_vertex[mesh_index] + vw.mVertexId;

		vertex_to_bones[global_vertex_id].AddBoneData(bone_id, vw.mWeight);
	}
}

void AnimatedModel::Parsh_Mesh_Bones(int mesh_index, const aiMesh* pMesh)
{
	for (int i = 0; i < pMesh->mNumBones; ++i)
	{
		Parse_Single_Bone(mesh_index, pMesh->mBones[i]);
	}
}

void AnimatedModel::Parse_Meshes(const aiScene* pScene)
{
	int total_vertices = 0;
	int total_indices = 0;
	int total_bones = 0;

	mesh_base_vertex.resize(pScene->mNumMeshes);

	for (int i = 0; i < pScene->mNumMeshes; ++i)
	{
		const aiMesh* pMesh = pScene->mMeshes[i];
		int num_vertices = pMesh->mNumVertices;
		int num_indices = pMesh->mNumFaces * 3;
		int num_bones = pMesh->mNumBones;
		mesh_base_vertex[i] = total_vertices;

		total_vertices += num_vertices;
		total_indices += num_indices;
		total_bones += num_bones;

		vertex_to_bones.resize(total_vertices);

		if (pMesh->HasBones()) {
			Parsh_Mesh_Bones(i, pMesh);
		}
	}
	NormalizeBoneWeights();
	CheckWeights();
}

void AnimatedModel::Parse_Scene(const aiScene* pScene)
{
	Parse_Meshes(pScene);
}

glm::mat4 AnimatedModel::AiMatrix4x4ToGlm(const aiMatrix4x4& from)
{
	glm::mat4 to;
	to[0][0] = from.a1; to[0][1] = from.b1; to[0][2] = from.c1; to[0][3] = from.d1;
	to[1][0] = from.a2; to[1][1] = from.b2; to[1][2] = from.c2; to[1][3] = from.d2;
	to[2][0] = from.a3; to[2][1] = from.b3; to[2][2] = from.c3; to[2][3] = from.d3;
	to[3][0] = from.a4; to[3][1] = from.b4; to[3][2] = from.c4; to[3][3] = from.d4;
	return to;
}

const aiNodeAnim* AnimatedModel::FindNodeAnim(const aiAnimation* animation, const std::string& nodeName)
{
	if (!animation) {
		return nullptr;
	}

	for (unsigned int i = 0; i < animation->mNumChannels; i++) {
		const aiNodeAnim* nodeAnim = animation->mChannels[i];
		if (std::string(nodeAnim->mNodeName.data) == nodeName) {
			return nodeAnim;
		}
	}
	return nullptr;
}

glm::vec3 AnimatedModel::InterpolateScale(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	if (pNodeAnim->mNumScalingKeys == 1) {
		return glm::vec3(pNodeAnim->mScalingKeys[0].mValue.x,
			pNodeAnim->mScalingKeys[0].mValue.y,
			pNodeAnim->mScalingKeys[0].mValue.z);
	}

	unsigned int ScalingIndex = 0;
	for (unsigned int i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++) {
		if (AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime) {
			ScalingIndex = i;
			break;
		}
	}

	unsigned int NextScalingIndex = (ScalingIndex + 1);

	float DeltaTime = (float)(pNodeAnim->mScalingKeys[NextScalingIndex].mTime -
		pNodeAnim->mScalingKeys[ScalingIndex].mTime);
	float Factor = (AnimationTime - (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime) / DeltaTime;


	const aiVector3D& Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
	const aiVector3D& End = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
	aiVector3D Delta = End - Start;

	return glm::vec3(Start.x + Factor * Delta.x,
		Start.y + Factor * Delta.y,
		Start.z + Factor * Delta.z);
}

glm::quat AnimatedModel::InterpolateRotation(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	if (pNodeAnim->mNumRotationKeys == 1) {
		const aiQuaternion& rot = pNodeAnim->mRotationKeys[0].mValue;
		return glm::quat(rot.w, rot.x, rot.y, rot.z);
	}

	unsigned int currentFrame = 0;
	for (unsigned int i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++) {
		if (AnimationTime < pNodeAnim->mRotationKeys[i + 1].mTime) {
			currentFrame = i;
			break;
		}
	}

	unsigned int nextFrame = (currentFrame + 1) % pNodeAnim->mNumRotationKeys;

	float frameStart = pNodeAnim->mRotationKeys[currentFrame].mTime;
	float frameEnd = pNodeAnim->mRotationKeys[nextFrame].mTime;
	float factor = (AnimationTime - frameStart) / (frameEnd - frameStart);

	const aiQuaternion& start = pNodeAnim->mRotationKeys[currentFrame].mValue;
	const aiQuaternion& end = pNodeAnim->mRotationKeys[nextFrame].mValue;

	float dot = start.x * end.x + start.y * end.y + start.z * end.z + start.w * end.w;

	aiQuaternion correctedEnd = dot < 0 ? aiQuaternion(-end.w, -end.x, -end.y, -end.z) : end;


	aiQuaternion out;
	aiQuaternion::Interpolate(out, start, end, factor);
	return glm::quat(out.w, out.x, out.y, out.z);
}

glm::vec3 AnimatedModel::InterpolatePosition(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	if (pNodeAnim->mNumPositionKeys == 1) {
		return glm::vec3(pNodeAnim->mPositionKeys[0].mValue.x,
			pNodeAnim->mPositionKeys[0].mValue.y,
			pNodeAnim->mPositionKeys[0].mValue.z);
	}

	const aiVector3D& basePos = pNodeAnim->mPositionKeys[0].mValue;
	glm::vec3 basePosition(basePos.x, basePos.y, basePos.z);
	float baseLength = glm::length(basePosition);

	unsigned int PositionIndex = 0;
	for (unsigned int i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++) {
		if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime) {
			PositionIndex = i;
			break;
		}
	}

	unsigned int NextPositionIndex = (PositionIndex + 1);
	float DeltaTime = (float)(pNodeAnim->mPositionKeys[NextPositionIndex].mTime -
		pNodeAnim->mPositionKeys[PositionIndex].mTime);
	float Factor = (AnimationTime - (float)pNodeAnim->mPositionKeys[PositionIndex].mTime) / DeltaTime;

	const aiVector3D& Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
	const aiVector3D& End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
	glm::vec3 currentPos = glm::vec3(
		Start.x + Factor * (End.x - Start.x),
		Start.y + Factor * (End.y - Start.y),
		Start.z + Factor * (End.z - Start.z)
	);

	float currentLength = glm::length(currentPos);
	if (currentLength > 0.0f) {
		currentPos = glm::normalize(currentPos) * baseLength;
	}

	return currentPos;
}
