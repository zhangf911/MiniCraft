#include "SceneSerializer.h"
#include "Scene.h"
#include <Ogre.h>
#include <Terrain/OgreTerrain.h>
#include <Terrain/OgreTerrainGroup.h>
#include "OgreManager.h"


using namespace Ogre;

const Ogre::String SCENE_VERSION = "0.3";

SceneSerializer::SceneSerializer()
:m_pOwner(nullptr)
,m_sceneGroup(StringUtil::BLANK)
{

}

void SceneSerializer::LoadScene( const std::string& sceneName, const std::string& sceneGroup, Scene* pOwner )
{
	DataStreamPtr stream = ResourceGroupManager::getSingleton().openResource(sceneName, sceneGroup);
	char* szData = strdup(stream->getAsString().c_str());

	m_pOwner = pOwner;
	m_sceneGroup = sceneGroup;

	//parse
	rapidxml::xml_document<> XMLDoc;
	rapidxml::xml_node<>* XMLRoot;
	XMLDoc.parse<0>(szData);

	// Grab the scene node
	XMLRoot = XMLDoc.first_node("scene");

	// Validate the File
	const String strVer = XMLRoot->first_attribute("formatVersion")->value();
	if(strVer != SCENE_VERSION)
	{
		assert(0);
		return;
	}

	Ogre::SceneManager* sm = RenderManager.m_pSceneMgr;
	sm->setShadowTechnique(/*SHADOWTYPE_TEXTURE_ADDITIVE_INTEGRATED*/SHADOWTYPE_NONE);
	//sm->setShadowFarDistance(3000);

	// 3 textures per directional light (PSSM)
	//sm->setShadowTextureCountPerLightType(Ogre::Light::LT_DIRECTIONAL, 3);

	// shadow camera setup
// 	PSSMShadowCameraSetup* pssmSetup = new PSSMShadowCameraSetup();
// 	pssmSetup->setSplitPadding(/*m_pOwner->->getNearClipDistance()*/0.1f);
// 	pssmSetup->calculateSplitPoints(3, /*mCamera->getNearClipDistance()*/0.1f, sm->getShadowFarDistance());
// 	pssmSetup->setOptimalAdjustFactor(0, 2);
// 	pssmSetup->setOptimalAdjustFactor(1, 1);
// 	pssmSetup->setOptimalAdjustFactor(2, 0.5);
// 
// 	sm->setShadowCameraSetup(ShadowCameraSetupPtr(pssmSetup));
// 	sm->setShadowTextureCount(3);
// 	sm->setShadowTextureConfig(0, 2048, 2048, PF_X8B8G8R8);
// 	sm->setShadowTextureConfig(1, 1024, 1024, PF_X8B8G8R8);
// 	sm->setShadowTextureConfig(2, 1024, 1024, PF_X8B8G8R8);
// 	sm->setShadowTextureSelfShadow(false);
// 	sm->setShadowCasterRenderBackFaces(false);
// 	sm->setShadowTextureCasterMaterial(StringUtil::BLANK);

	//������
	const String strAmbient = XMLRoot->first_attribute("AmbientLight")->value();
	RenderManager.m_pSceneMgr->setAmbientLight(Ogre::StringConverter::parseColourValue(strAmbient));

	//ȫ�ֹ�
	Light* pSunLight = RenderManager.m_pSceneMgr->createLight("SunLight");
	pSunLight->setType(Light::LT_DIRECTIONAL);
	pSunLight->setDirection(pOwner->GetSunLightDirection().normalisedCopy());
	pSunLight->setDiffuseColour(ColourValue::White);

	rapidxml::xml_node<>* pElement = XMLRoot->first_node("terrain");
	assert(pElement);
	_LoadTerrain(pElement);

	pElement = XMLRoot->first_node("objects");
	assert(pElement);
	_LoadObjects(pElement);

	free(szData);
}

void SceneSerializer::SaveScene(const std::string& fullPath, Scene* pOwner)
{
	using namespace rapidxml;

	m_pOwner = pOwner;

	xml_document<> doc;  
	//�ļ�ͷ
	xml_node<>* rot = doc.allocate_node(rapidxml::node_pi, doc.allocate_string("xml version='1.0' encoding='utf-8'"));
	doc.append_node(rot);

	//scene��
	xml_node<>* sceneNode =   doc.allocate_node(node_element, "scene");
	const String strAmbient = Ogre::StringConverter::toString(RenderManager.m_pSceneMgr->getAmbientLight());
	sceneNode->append_attribute(doc.allocate_attribute("formatVersion", doc.allocate_string(SCENE_VERSION.c_str())));
	sceneNode->append_attribute(doc.allocate_attribute("AmbientLight", doc.allocate_string(strAmbient.c_str())));
	doc.append_node(sceneNode);

	//terrain��
	xml_node<>* terrainNode =   doc.allocate_node(node_element, "terrain");
	_SaveTerrain(&doc, terrainNode);
	sceneNode->append_node(terrainNode);

	//object��
	xml_node<>* objsNode =   doc.allocate_node(node_element, "objects");
	_SaveObjects(&doc, objsNode);
	sceneNode->append_node(objsNode);

	std::ofstream out(fullPath);
	out << doc;
}

void SceneSerializer::_LoadTerrain( rapidxml::xml_node<>* node )
{
	TerrainGlobalOptions* option = new TerrainGlobalOptions;

	float worldSize = StringConverter::parseReal(node->first_attribute("worldSize")->value());
	int mapSize = StringConverter::parseInt(node->first_attribute("mapSize")->value());
	//bool colourmapEnabled = DotSceneLoader::getAttribBool(XMLNode, "colourmapEnabled");
	//int colourMapTextureSize = Ogre::StringConverter::parseInt(XMLNode->first_attribute("colourMapTextureSize")->value());
	//int compositeMapDistance = Ogre::StringConverter::parseInt(XMLNode->first_attribute("tuningCompositeMapDistance")->value());
	int maxPixelError = StringConverter::parseInt(node->first_attribute("tuningMaxPixelError")->value());

	option->setMaxPixelError((Ogre::Real)maxPixelError);
	option->setCompositeMapDistance(3000);
// 	option->setUseRayBoxDistanceCalculation(true);
// 	option->setLightMapDirection(pSunLight->getDirection());
// 	option->setCompositeMapAmbient(m_pOwner->m_pSceneMgr->getAmbientLight());
// 	option->setCompositeMapDiffuse(pSunLight->getDiffuseColour());

	TerrainGroup* pTerrainGroup = new TerrainGroup(RenderManager.m_pSceneMgr, Terrain::ALIGN_X_Z, mapSize, worldSize);
	pTerrainGroup->setOrigin(Vector3::ZERO);

	//�ڵ��β�������ǰ����
	COgreManager::GetSingleton().EnableDeferredShading(true);

	//���ص�������
	pTerrainGroup->setResourceGroup(m_sceneGroup);
	pTerrainGroup->defineTerrain(0, 0, "terrain.dat");
	pTerrainGroup->loadTerrain(0, 0);
	pTerrainGroup->freeTemporaryResources();

	m_pOwner->m_pTerrain = pTerrainGroup->getTerrain(0, 0);
	m_pOwner->m_terrainGroup = pTerrainGroup;
	m_pOwner->m_terrainOption = option;
}