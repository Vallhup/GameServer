#include "pch.h"
#include "StaticObjectManager.h"
#include "StaticObject.h"

void StaticObjectManager::Init()
{
	AddStaticObject("StaticGlb/ground.glb", "Texture/map.png");
	AddStaticObject("StaticGlb/title.glb", "Texture/title.png");
	AddStaticObject("StaticGlb/fence.glb", "Texture/fence.png");
	AddStaticObject("StaticGlb/tree1.glb", "Texture/tree1.png");
	AddStaticObject("StaticGlb/tree2.glb", "Texture/tree2.png");
	AddStaticObject("StaticGlb/tree3.glb", "Texture/tree3.png");
	AddStaticObject("StaticGlb/tree4.glb", "Texture/tree4.png");
	AddStaticObject("StaticGlb/bridge.glb", "Texture/bridge.png");
	AddStaticObject("StaticGlb/box.glb", "Texture/box.png");
	AddStaticObject("StaticGlb/cart.glb", "Texture/cart.png");
	AddStaticObject("StaticGlb/housemain.glb", "Texture/housemain.png");
	AddStaticObject("StaticGlb/house1.glb", "Texture/house1.png");
	AddStaticObject("StaticGlb/house2.glb", "Texture/house2.png");
	AddStaticObject("StaticGlb/house3.glb", "Texture/house3.png");
	AddStaticObject("StaticGlb/house4.glb", "Texture/house4.png");
	AddStaticObject("StaticGlb/house5.glb", "Texture/house5.png");
	AddStaticObject("StaticGlb/house6.glb", "Texture/house6.png");
	AddStaticObject("StaticGlb/house7.glb", "Texture/house7.png");
	AddStaticObject("StaticGlb/house8.glb", "Texture/house8.png");
	AddStaticObject("StaticGlb/house9.glb", "Texture/house9.png");
	AddStaticObject("StaticGlb/rock1.glb", "Texture/rock1.png");
	AddStaticObject("StaticGlb/rock2.glb", "Texture/rock2.png");
	AddStaticObject("StaticGlb/waterwheel.glb", "Texture/waterwheel.png");
	AddStaticObject("StaticGlb/windmill.glb", "Texture/windmill.png");
	AddStaticObject("StaticGlb/cloud.glb", "Texture/cloud.png");
	AddStaticObject("StaticGlb/cave.glb", "Texture/cave.png");
	AddStaticObject("StaticGlb/startlogo.glb", "Texture/startlogo.png");
	AddStaticObject("StaticGlb/gameclear.glb", "Texture/gameclear.png");
}

void StaticObjectManager::Release()
{
	for (auto& obj : StaticObjects)
	{
		delete obj;
	}
	StaticObjects.clear();
}

void StaticObjectManager::Update()
{
	// 구름 같은 움직이는 오브젝트들 업데이트 필요
}

void StaticObjectManager::Draw(const glm::mat4& orgview, const glm::mat4& orgproj, glm::vec3 viewPos,
	glm::mat4 lightSpaceMatrix, GLuint shadowMap)
{
	for (auto& obj : StaticObjects)
	{
		obj->drawStaticobject(orgview, orgproj, viewPos, lightSpaceMatrix, shadowMap);
	}
}

void StaticObjectManager::DrawShadow(const glm::mat4& lightSpaceMatrix, GLuint depthShader)
{
	for (auto& obj : StaticObjects)
	{
		obj->drawStaticobjectShadow(lightSpaceMatrix, depthShader);
	}
}

StaticObject* StaticObjectManager::AddStaticObject(const char* glb, const char* png)
{
	StaticObject* obj = new StaticObject(glb, png);
	StaticObjects.push_back(obj);
	return obj;
}
