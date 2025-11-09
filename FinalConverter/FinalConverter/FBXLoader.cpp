#include "pch.h"
#include "FBXLoader.h"

FBXLoader::FBXLoader()
{

}

FBXLoader::~FBXLoader()
{
	if (_scene)
		_scene->Destroy();
	if (_manager)
		_manager->Destroy();
}

bool FBXLoader::LoadFbx(const wstring& path)
{
	if (!Import(path)) return false;

	LoadBones(_scene->GetRootNode());
	LoadAnimationInfo();
	ParseNode(_scene->GetRootNode());

	if (_meshes.empty() && !_animClips.empty() && !_bones.empty())
	{
		LoadStandaloneAnimations();
	}

	return true;  // 성공
}

bool FBXLoader::Import(const wstring& path)
{
	// FBX SDK 관리자 객체 생성
	_manager = FbxManager::Create();
	if (!_manager) {
		wcout << L"FbxManager 생성 실패" << endl;
		return false;
	}

	// IOSettings 객체 생성 및 설정
	FbxIOSettings* settings = FbxIOSettings::Create(_manager, IOSROOT);
	_manager->SetIOSettings(settings);

	// FbxScene 객체 생성
	_scene = FbxScene::Create(_manager, "");
	if (!_scene) {
		wcout << L"FbxScene 생성 실패" << endl;
		return false;
	}

	// FbxImporter 객체 생성
	_importer = FbxImporter::Create(_manager, "");
	if (!_importer) {
		wcout << L"FbxImporter 생성 실패" << endl;
		return false;
	}

	// 파일 경로 변환
	string strPath = ws2s(path);

	// FBX 파일 초기화
	if (!_importer->Initialize(strPath.c_str(), -1, _manager->GetIOSettings())) {
		wcout << L"FBX 파일 초기화 실패: " << path << endl;
		wcout << L"에러: " << s2ws(_importer->GetStatus().GetErrorString()) << endl;
		return false;
	}

	// FBX 파일 임포트
	if (!_importer->Import(_scene)) {
		wcout << L"FBX 파일 임포트 실패: " << path << endl;
		wcout << L"에러: " << s2ws(_importer->GetStatus().GetErrorString()) << endl;
		return false;
	}

	// DirectX 좌표계로 변환
	_scene->GetGlobalSettings().SetAxisSystem(FbxAxisSystem::DirectX);

	// 삼각형화
	FbxGeometryConverter geometryConverter(_manager);
	geometryConverter.Triangulate(_scene, true);

	// 임포터 정리
	_importer->Destroy();
	_importer = nullptr;

	wcout << L"FBX 파일 로딩 성공: " << path << endl;
	return true;
}

void FBXLoader::ParseNode(FbxNode* node)
{
	FbxNodeAttribute* attribute = node->GetNodeAttribute();

	if (attribute)
	{
		switch (attribute->GetAttributeType())
		{
		case FbxNodeAttribute::eMesh:
		{
			//const char* meshName = node->GetMesh()->GetName();	이렇게 접근하면 안됨 다찬이가 노드 이름으로 해놧음 mesh 이름이 아니라 mesh이름은 빈 깡통임
			string nodeNameStr = node->GetName();
			if (nodeNameStr.find("collision") != string::npos) {
				LoadCollisionMesh(node->GetMesh());
			}
			else {
				LoadMesh(node->GetMesh());
			}
		}
			break;
		}
	}

	// Material 로드
	const uint32 materialCount = node->GetMaterialCount();
	for (uint32 i = 0; i < materialCount; ++i)
	{
		FbxSurfaceMaterial* surfaceMaterial = node->GetMaterial(i);
		LoadMaterial(surfaceMaterial);
	}

	// Tree 구조 재귀 호출
	const int32 childCount = node->GetChildCount();
	for (int32 i = 0; i < childCount; ++i)
		ParseNode(node->GetChild(i));
}

void FBXLoader::LoadMesh(FbxMesh* mesh)
{
	_meshes.push_back(FbxMeshInfo());
	FbxMeshInfo& meshInfo = _meshes.back();
	meshInfo.name = s2ws(mesh->GetName());

	// 정점 확장
	vector<Vertex> expandedVertices;
	vector<int32> controlPointMapping;
	FbxVector4* controlPoints = mesh->GetControlPoints();

	const int32 materialCount = mesh->GetNode()->GetMaterialCount();
	meshInfo.indices.resize(materialCount);
	FbxGeometryElementMaterial* geometryElementMaterial = mesh->GetElementMaterial();

	uint32 currentVertexIndex = 0;
	const int32 triCount = mesh->GetPolygonCount();

	for (int32 i = 0; i < triCount; i++) {
		uint32 triangleIndices[3];

		for (int32 j = 0; j < 3; j++) {
			int32 controlPointIndex = mesh->GetPolygonVertex(i, j);
			Vertex newVertex = {};

			// Position
			newVertex.pos.x = static_cast<float>(controlPoints[controlPointIndex].mData[0]);
			newVertex.pos.y = static_cast<float>(controlPoints[controlPointIndex].mData[2]);
			newVertex.pos.z = static_cast<float>(controlPoints[controlPointIndex].mData[1]);

			// UV
			FbxVector2 uv = mesh->GetElementUV()->GetDirectArray().GetAt(mesh->GetTextureUVIndex(i, j));
			newVertex.uv.x = static_cast<float>(uv.mData[0]);
			newVertex.uv.y = 1.f - static_cast<float>(uv.mData[1]);

			// Normal - 직접 계산
			if (mesh->GetElementNormalCount() > 0) {
				FbxGeometryElementNormal* normal = mesh->GetElementNormal();
				uint32 normalIdx = currentVertexIndex;
				if (normal->GetMappingMode() == FbxGeometryElement::eByPolygonVertex) {
					if (normal->GetReferenceMode() == FbxGeometryElement::eDirect)
						normalIdx = currentVertexIndex;
					else
						normalIdx = normal->GetIndexArray().GetAt(currentVertexIndex);
				}
				FbxVector4 vec = normal->GetDirectArray().GetAt(normalIdx);
				newVertex.normal.x = static_cast<float>(vec.mData[0]);
				newVertex.normal.y = static_cast<float>(vec.mData[2]);
				newVertex.normal.z = static_cast<float>(vec.mData[1]);
			}

			// Tangent - 기본값
			newVertex.tangent = { 1.0f, 0.0f, 0.0f };

			expandedVertices.push_back(newVertex);
			controlPointMapping.push_back(controlPointIndex);
			triangleIndices[j] = currentVertexIndex;
			currentVertexIndex++;
		}

		const uint32 subsetIdx = geometryElementMaterial->GetIndexArray().GetAt(i);
		meshInfo.indices[subsetIdx].push_back(triangleIndices[0]);
		meshInfo.indices[subsetIdx].push_back(triangleIndices[2]);
		meshInfo.indices[subsetIdx].push_back(triangleIndices[1]);
	}

	meshInfo.vertices = expandedVertices;

	const int32 originalVertexCount = mesh->GetControlPointsCount();
	meshInfo.boneWeights.resize(originalVertexCount);

	// 애니메이션 로드
	LoadAnimationData(mesh, &meshInfo);

	vector<BoneWeight> originalWeights = meshInfo.boneWeights;
	meshInfo.boneWeights.resize(expandedVertices.size());

	for (size_t i = 0; i < controlPointMapping.size(); ++i) {
		int32 originalIndex = controlPointMapping[i];
		if (originalIndex < originalWeights.size()) {
			meshInfo.boneWeights[i] = originalWeights[originalIndex];
		}
	}

	FillBoneWeight(mesh, &meshInfo);
}

void FBXLoader::LoadCollisionMesh(FbxMesh* mesh)
{
	_meshes.push_back(FbxMeshInfo());
	FbxMeshInfo& meshInfo = _meshes.back();
	meshInfo.name = s2ws(mesh->GetName());

	FbxVector4* controlPoints = mesh->GetControlPoints();
	const int32 vertexCount = mesh->GetControlPointsCount();

	meshInfo.vertices.resize(vertexCount);
	for (int32 i = 0; i < vertexCount; ++i) {
		Vertex& vertex = meshInfo.vertices[i];

		vertex.pos.x = static_cast<float>(controlPoints[i].mData[0]);
		vertex.pos.y = static_cast<float>(controlPoints[i].mData[2]);
		vertex.pos.z = static_cast<float>(controlPoints[i].mData[1]);

		vertex.uv = { 0.0f, 0.0f };
		vertex.normal = { 0.0f, 1.0f, 0.0f };
		vertex.tangent = { 1.0f, 0.0f, 0.0f };
	}

	meshInfo.indices.resize(1);
	const int32 triCount = mesh->GetPolygonCount();

	for (int32 i = 0; i < triCount; i++) {
		int32 idx0 = mesh->GetPolygonVertex(i, 0);
		int32 idx1 = mesh->GetPolygonVertex(i, 1);
		int32 idx2 = mesh->GetPolygonVertex(i, 2);

		meshInfo.indices[0].push_back(idx0);
		meshInfo.indices[0].push_back(idx2);  
		meshInfo.indices[0].push_back(idx1);
	}

	meshInfo.boneWeights.resize(vertexCount);
	LoadAnimationData(mesh, &meshInfo);
	FillBoneWeight(mesh, &meshInfo);
}

void FBXLoader::LoadMaterial(FbxSurfaceMaterial* surfaceMaterial)
{
	FbxMaterialInfo material{};
	material.name = s2ws(surfaceMaterial->GetName());

	// 기본 색상 정보
	material.diffuse = GetMaterialData(surfaceMaterial, FbxSurfaceMaterial::sDiffuse, FbxSurfaceMaterial::sDiffuseFactor);
	material.ambient = GetMaterialData(surfaceMaterial, FbxSurfaceMaterial::sAmbient, FbxSurfaceMaterial::sAmbientFactor);
	material.specular = GetMaterialData(surfaceMaterial, FbxSurfaceMaterial::sSpecular, FbxSurfaceMaterial::sSpecularFactor);

	//// 모든 텍스처 타입 추출
	//material.baseColorTexName = GetTextureRelativeName(surfaceMaterial, FbxSurfaceMaterial::sDiffuse);
	//material.diffuseTexName = material.baseColorTexName;  // 호환성을 위해
	//
	//material.normalTexName = GetTextureRelativeName(surfaceMaterial, FbxSurfaceMaterial::sNormalMap);
	//material.roughnessTexName = GetTextureRelativeName(surfaceMaterial, "Roughness");
	//material.specularTexName = material.roughnessTexName;  // 호환성을 위해
	//
	//material.metallicTexName = GetTextureRelativeName(surfaceMaterial, "Metallic");
	//material.heightTexName = GetTextureRelativeName(surfaceMaterial, "Height");
	//material.alphaTexName = GetTextureRelativeName(surfaceMaterial, "Opacity");
	//material.emissionTexName = GetTextureRelativeName(surfaceMaterial, "Emission");
	//material.aoTexName = GetTextureRelativeName(surfaceMaterial, "AmbientOcclusion");

	LoadAllTextures(surfaceMaterial, material);

	_meshes.back().materials.push_back(material);
}

void FBXLoader::LoadAllTextures(FbxSurfaceMaterial* surfaceMaterial, FbxMaterialInfo& material)
{
	FbxProperty prop = surfaceMaterial->GetFirstProperty();

	while (prop.IsValid())
	{
		if (prop.GetSrcObjectCount<FbxFileTexture>() > 0)
		{
			FbxFileTexture* texture = prop.GetSrcObject<FbxFileTexture>(0);
			if (texture)
			{
				string propName = prop.GetName().Buffer();
				wstring textureName = s2ws(texture->GetRelativeFileName());

				if (ContainsKeyword(propName, { "diffusecolor", "base_color_map", "basecolor", "albedo" })) {
					material.baseColorTexName = textureName;
				}
				else if (ContainsKeyword(propName, { "norm", "normal", "bump" })) {
					material.normalTexName = textureName;
				}
				else if (ContainsKeyword(propName, { "roughness", "rough", "specular", "shininess" })) {
					material.roughnessTexName = textureName;
				}
				else if (ContainsKeyword(propName, { "reflection", "metallic", "metal" })) {
					material.metallicTexName = textureName;
				}
				else if (ContainsKeyword(propName, { "height", "displacement" })) {
					material.heightTexName = textureName;
				}
				else if (ContainsKeyword(propName, { "transparency", "alpha", "opacity" })) {
					material.alphaTexName = textureName;
				}
				else if (ContainsKeyword(propName, { "emit", "emission", "emissive" })) {
					material.emissionTexName = textureName;
				}
				else if (ContainsKeyword(propName, { "diffusefactor", "ao", "ambient", "occlusion" })) {
					material.aoTexName = textureName;
				}

				wcout << L"Found texture: " << s2ws(propName) << L" -> " << textureName << endl;
			}
		}
		prop = surfaceMaterial->GetNextProperty(prop);
	}
}

bool FBXLoader::ContainsKeyword(const string& propName, const vector<string>& keywords)
{
	string lowerProp = propName;
	transform(lowerProp.begin(), lowerProp.end(), lowerProp.begin(), ::tolower);

	for (const auto& keyword : keywords) {
		if (lowerProp.find(keyword) != string::npos) {
			return true;
		}
	}
	return false;
}

void FBXLoader::GetNormal(FbxMesh* mesh, FbxMeshInfo* container, int32 idx, int32 vertexCounter)
{
	if (mesh->GetElementNormalCount() == 0)
		return;

	FbxGeometryElementNormal* normal = mesh->GetElementNormal();
	uint32 normalIdx = 0;

	if (normal->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
	{
		if (normal->GetReferenceMode() == FbxGeometryElement::eDirect)
			normalIdx = vertexCounter;
		else
			normalIdx = normal->GetIndexArray().GetAt(vertexCounter);
	}
	else if (normal->GetMappingMode() == FbxGeometryElement::eByControlPoint)
	{
		if (normal->GetReferenceMode() == FbxGeometryElement::eDirect)
			normalIdx = idx;
		else
			normalIdx = normal->GetIndexArray().GetAt(idx);
	}

	FbxVector4 vec = normal->GetDirectArray().GetAt(normalIdx);
	container->vertices[idx].normal.x = static_cast<float>(vec.mData[0]);
	container->vertices[idx].normal.y = static_cast<float>(vec.mData[2]);
	container->vertices[idx].normal.z = static_cast<float>(vec.mData[1]);
}

void FBXLoader::GetTangent(FbxMesh* mesh, FbxMeshInfo* meshInfo, int32 idx, int32 vertexCounter)
{
	if (mesh->GetElementTangentCount() == 0)
	{
		// TEMP : 원래는 이런 저런 알고리즘으로 Tangent 만들어줘야 함
		meshInfo->vertices[idx].tangent.x = 1.f;
		meshInfo->vertices[idx].tangent.y = 0.f;
		meshInfo->vertices[idx].tangent.z = 0.f;
		return;
	}

	FbxGeometryElementTangent* tangent = mesh->GetElementTangent();
	uint32 tangentIdx = 0;

	if (tangent->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
	{
		if (tangent->GetReferenceMode() == FbxGeometryElement::eDirect)
			tangentIdx = vertexCounter;
		else
			tangentIdx = tangent->GetIndexArray().GetAt(vertexCounter);
	}
	else if (tangent->GetMappingMode() == FbxGeometryElement::eByControlPoint)
	{
		if (tangent->GetReferenceMode() == FbxGeometryElement::eDirect)
			tangentIdx = idx;
		else
			tangentIdx = tangent->GetIndexArray().GetAt(idx);
	}

	FbxVector4 vec = tangent->GetDirectArray().GetAt(tangentIdx);
	meshInfo->vertices[idx].tangent.x = static_cast<float>(vec.mData[0]);
	meshInfo->vertices[idx].tangent.y = static_cast<float>(vec.mData[2]);
	meshInfo->vertices[idx].tangent.z = static_cast<float>(vec.mData[1]);
}

void FBXLoader::GetUV(FbxMesh* mesh, FbxMeshInfo* meshInfo, int32 idx, int32 uvIndex)
{
	FbxVector2 uv = mesh->GetElementUV()->GetDirectArray().GetAt(uvIndex);
	meshInfo->vertices[idx].uv.x = static_cast<float>(uv.mData[0]);
	meshInfo->vertices[idx].uv.y = 1.f - static_cast<float>(uv.mData[1]);
}

Vec4 FBXLoader::GetMaterialData(FbxSurfaceMaterial* surface, const char* materialName, const char* factorName)
{
	FbxDouble3  material;
	FbxDouble	factor = 0.f;

	FbxProperty materialProperty = surface->FindProperty(materialName);
	FbxProperty factorProperty = surface->FindProperty(factorName);

	if (materialProperty.IsValid() && factorProperty.IsValid())
	{
		material = materialProperty.Get<FbxDouble3>();
		factor = factorProperty.Get<FbxDouble>();
	}

	Vec4 ret = Vec4(
		static_cast<float>(material.mData[0] * factor),
		static_cast<float>(material.mData[1] * factor),
		static_cast<float>(material.mData[2] * factor),
		static_cast<float>(factor));

	return ret;
}

wstring FBXLoader::GetTextureRelativeName(FbxSurfaceMaterial* surface, const char* materialProperty)
{
	string name;

	FbxProperty textureProperty = surface->FindProperty(materialProperty);
	if (textureProperty.IsValid())
	{
		uint32 count = textureProperty.GetSrcObjectCount();

		if (1 <= count)
		{
			FbxFileTexture* texture = textureProperty.GetSrcObject<FbxFileTexture>(0);
			if (texture)
				name = texture->GetRelativeFileName();
		}
	}

	return s2ws(name);
}

void FBXLoader::LoadBones(FbxNode* node, int32 idx, int32 parentIdx)
{
	FbxNodeAttribute* attribute = node->GetNodeAttribute();

	if (attribute && attribute->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	{
		shared_ptr<FbxBoneInfo> bone = make_shared<FbxBoneInfo>();
		bone->boneName = s2ws(node->GetName());
		bone->parentIndex = parentIdx;
		_bones.push_back(bone);
	}

	const int32 childCount = node->GetChildCount();
	for (int32 i = 0; i < childCount; i++)
		LoadBones(node->GetChild(i), static_cast<int32>(_bones.size()), idx);
}

void FBXLoader::LoadAnimationInfo()
{
	_scene->FillAnimStackNameArray(OUT _animNames);

	const int32 animCount = _animNames.GetCount();
	for (int32 i = 0; i < animCount; i++)
	{
		FbxAnimStack* animStack = _scene->FindMember<FbxAnimStack>(_animNames[i]->Buffer());
		if (animStack == nullptr)
			continue;

		shared_ptr<FbxAnimClipInfo> animClip = make_shared<FbxAnimClipInfo>();
		animClip->name = s2ws(animStack->GetName());
		animClip->keyFrames.resize(_bones.size()); // 키프레임은 본의 개수만큼

		FbxTakeInfo* takeInfo = _scene->GetTakeInfo(animStack->GetName());
		animClip->startTime = takeInfo->mLocalTimeSpan.GetStart();
		animClip->endTime = takeInfo->mLocalTimeSpan.GetStop();
		animClip->mode = _scene->GetGlobalSettings().GetTimeMode();

		_animClips.push_back(animClip);
	}
}

void FBXLoader::LoadStandaloneAnimations()
{
	if (_animClips.empty() || _bones.empty()) return;

	// 스켈레톤 노드들을 찾아서 애니메이션 키프레임 로드
	for (size_t animIdx = 0; animIdx < _animClips.size(); ++animIdx)
	{
		FbxAnimStack* animStack = _scene->FindMember<FbxAnimStack>(_animNames[animIdx]->Buffer());
		if (!animStack) continue;

		_scene->SetCurrentAnimationStack(animStack);

		for (size_t boneIdx = 0; boneIdx < _bones.size(); ++boneIdx)
		{
			// 본 이름으로 노드 찾기
			string boneName = ws2s(_bones[boneIdx]->boneName);
			FbxNode* boneNode = _scene->FindNodeByName(boneName.c_str());

			if (boneNode)
			{
				LoadBoneKeyframes(animIdx, boneNode, boneIdx);
			}
		}
	}
}

void FBXLoader::LoadBoneKeyframes(int32 animIndex, FbxNode* boneNode, int32 boneIdx)
{
	if (!boneNode || animIndex >= _animClips.size()) return;

	FbxTime::EMode timeMode = _scene->GetGlobalSettings().GetTimeMode();
	FbxLongLong startFrame = _animClips[animIndex]->startTime.GetFrameCount(timeMode);
	FbxLongLong endFrame = _animClips[animIndex]->endTime.GetFrameCount(timeMode);

	// Reflection 행렬 (좌표계 변환용)
	FbxVector4 v1 = { 1, 0, 0, 0 };
	FbxVector4 v2 = { 0, 0, 1, 0 };
	FbxVector4 v3 = { 0, 1, 0, 0 };
	FbxVector4 v4 = { 0, 0, 0, 1 };
	FbxAMatrix matReflect;
	matReflect.mData[0] = v1;
	matReflect.mData[1] = v2;
	matReflect.mData[2] = v3;
	matReflect.mData[3] = v4;

	for (FbxLongLong frame = startFrame; frame < endFrame; frame++)
	{
		FbxKeyFrameInfo keyFrameInfo = {};
		FbxTime fbxTime;
		fbxTime.SetFrame(frame, timeMode);

		FbxNode* rootNode = _scene->GetRootNode();
		FbxAMatrix matFromRoot = rootNode->EvaluateGlobalTransform(fbxTime);
		FbxAMatrix matTransform = matFromRoot.Inverse() * boneNode->EvaluateGlobalTransform(fbxTime);
		FbxAMatrix finalTransform = matReflect * matTransform * matReflect;

		keyFrameInfo.time = fbxTime.GetSecondDouble();
		keyFrameInfo.matTransform = finalTransform;

		_animClips[animIndex]->keyFrames[boneIdx].push_back(keyFrameInfo);
	}
}

void FBXLoader::LoadAnimationData(FbxMesh* mesh, FbxMeshInfo* meshInfo)
{
	const int32 skinCount = mesh->GetDeformerCount(FbxDeformer::eSkin);
	if (skinCount <= 0)
		return;

	meshInfo->hasAnimation = true;

	for (int32 i = 0; i < skinCount; i++)
	{
		FbxSkin* fbxSkin = static_cast<FbxSkin*>(mesh->GetDeformer(i, FbxDeformer::eSkin));

		if (fbxSkin)
		{
			FbxSkin::EType type = fbxSkin->GetSkinningType();
			if (FbxSkin::eRigid == type || FbxSkin::eLinear)
			{
				const int32 clusterCount = fbxSkin->GetClusterCount();
				for (int32 j = 0; j < clusterCount; j++)
				{
					FbxCluster* cluster = fbxSkin->GetCluster(j);
					if (cluster->GetLink() == nullptr)
						continue;

					int32 boneIdx = FindBoneIndex(cluster->GetLink()->GetName());
					assert(boneIdx >= 0);

					FbxAMatrix matNodeTransform = GetTransform(mesh->GetNode());
					LoadBoneWeight(cluster, boneIdx, meshInfo);
					LoadOffsetMatrix(cluster, matNodeTransform, boneIdx, meshInfo);

					if (!_animClips.empty()) {
						const int32 animCount = _animNames.Size();
						for (int32 k = 0; k < animCount; k++)
							LoadKeyframe(k, mesh->GetNode(), cluster, matNodeTransform, boneIdx, meshInfo);
					}
				}
			}
		}
	}
}

void FBXLoader::FillBoneWeight(FbxMesh* mesh, FbxMeshInfo* meshInfo)
{
	const int32 size = static_cast<int32>(meshInfo->boneWeights.size());
	for (int32 v = 0; v < size; v++)
	{
		BoneWeight& boneWeight = meshInfo->boneWeights[v];
		boneWeight.Normalize();

		float animBoneIndex[4] = {};
		float animBoneWeight[4] = {};

		const int32 weightCount = static_cast<int32>(boneWeight.boneWeights.size());
		for (int32 w = 0; w < weightCount; w++)
		{
			animBoneIndex[w] = static_cast<float>(boneWeight.boneWeights[w].first);
			animBoneWeight[w] = static_cast<float>(boneWeight.boneWeights[w].second);
		}

		memcpy(&meshInfo->vertices[v].indices, animBoneIndex, sizeof(Vec4));
		memcpy(&meshInfo->vertices[v].weights, animBoneWeight, sizeof(Vec4));
	}
}

void FBXLoader::RemapBoneWeights(FbxMesh* mesh, const vector<int32>& controlPointMapping, FbxMeshInfo& meshInfo)
{
	const int32 originalVertexCount = mesh->GetControlPointsCount();
	vector<BoneWeight> originalWeights(originalVertexCount);

	// 기존 애니메이션 로직이 원본 크기로 로드했다면 백업
	for (int32 i = 0; i < originalVertexCount && i < meshInfo.boneWeights.size(); ++i) {
		originalWeights[i] = meshInfo.boneWeights[i];
	}

	// 확장된 정점에 매핑
	for (size_t i = 0; i < controlPointMapping.size(); ++i) {
		int32 originalIndex = controlPointMapping[i];
		if (originalIndex < originalWeights.size()) {
			meshInfo.boneWeights[i] = originalWeights[originalIndex];
		}
	}
}

void FBXLoader::LoadBoneWeight(FbxCluster* cluster, int32 boneIdx, FbxMeshInfo* meshInfo)
{
	const int32 indicesCount = cluster->GetControlPointIndicesCount();
	for (int32 i = 0; i < indicesCount; i++)
	{
		double weight = cluster->GetControlPointWeights()[i];
		int32 vtxIdx = cluster->GetControlPointIndices()[i];
		meshInfo->boneWeights[vtxIdx].AddWeights(boneIdx, weight);
	}
}

void FBXLoader::LoadOffsetMatrix(FbxCluster* cluster, const FbxAMatrix& matNodeTransform, int32 boneIdx, FbxMeshInfo* meshInfo)
{
	FbxAMatrix matClusterTrans;
	FbxAMatrix matClusterLinkTrans;
	// The transformation of the mesh at binding time 
	cluster->GetTransformMatrix(matClusterTrans);
	// The transformation of the cluster(joint) at binding time from joint space to world space 
	cluster->GetTransformLinkMatrix(matClusterLinkTrans);

	FbxVector4 V0 = { 1, 0, 0, 0 };
	FbxVector4 V1 = { 0, 0, 1, 0 };
	FbxVector4 V2 = { 0, 1, 0, 0 };
	FbxVector4 V3 = { 0, 0, 0, 1 };

	FbxAMatrix matReflect;
	matReflect[0] = V0;
	matReflect[1] = V1;
	matReflect[2] = V2;
	matReflect[3] = V3;

	FbxAMatrix matOffset;
	matOffset = matClusterLinkTrans.Inverse() * matClusterTrans;
	matOffset = matReflect * matOffset * matReflect;

	_bones[boneIdx]->matOffset = matOffset.Transpose();
}

void FBXLoader::LoadKeyframe(int32 animIndex, FbxNode* node, FbxCluster* cluster, const FbxAMatrix& matNodeTransform, int32 boneIdx, FbxMeshInfo* meshInfo)
{
	if (_animClips.empty())
		return;

	FbxVector4	v1 = { 1, 0, 0, 0 };
	FbxVector4	v2 = { 0, 0, 1, 0 };
	FbxVector4	v3 = { 0, 1, 0, 0 };
	FbxVector4	v4 = { 0, 0, 0, 1 };
	FbxAMatrix	matReflect;
	matReflect.mData[0] = v1;
	matReflect.mData[1] = v2;
	matReflect.mData[2] = v3;
	matReflect.mData[3] = v4;

	FbxTime::EMode timeMode = _scene->GetGlobalSettings().GetTimeMode();

	// 애니메이션 골라줌
	FbxAnimStack* animStack = _scene->FindMember<FbxAnimStack>(_animNames[animIndex]->Buffer());
	_scene->SetCurrentAnimationStack(OUT animStack);

	FbxLongLong startFrame = _animClips[animIndex]->startTime.GetFrameCount(timeMode);
	FbxLongLong endFrame = _animClips[animIndex]->endTime.GetFrameCount(timeMode);

	for (FbxLongLong frame = startFrame; frame < endFrame; frame++)
	{
		FbxKeyFrameInfo keyFrameInfo = {};
		FbxTime fbxTime = 0;

		fbxTime.SetFrame(frame, timeMode);

		FbxAMatrix matFromNode = node->EvaluateGlobalTransform(fbxTime);
		FbxAMatrix matTransform = matFromNode.Inverse() * cluster->GetLink()->EvaluateGlobalTransform(fbxTime);
		matTransform = matReflect * matTransform * matReflect;

		keyFrameInfo.time = fbxTime.GetSecondDouble();
		keyFrameInfo.matTransform = matTransform;

		_animClips[animIndex]->keyFrames[boneIdx].push_back(keyFrameInfo);
	}
}

int32 FBXLoader::FindBoneIndex(string name)
{
	wstring boneName = wstring(name.begin(), name.end());

	for (UINT i = 0; i < _bones.size(); ++i)
	{
		if (_bones[i]->boneName == boneName)
			return i;
	}

	return -1;
}

FbxAMatrix FBXLoader::GetTransform(FbxNode* node)
{
	const FbxVector4 translation = node->GetGeometricTranslation(FbxNode::eSourcePivot);
	const FbxVector4 rotation = node->GetGeometricRotation(FbxNode::eSourcePivot);
	const FbxVector4 scaling = node->GetGeometricScaling(FbxNode::eSourcePivot);
	return FbxAMatrix(translation, rotation, scaling);
}
