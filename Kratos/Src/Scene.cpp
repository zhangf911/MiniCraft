#include "Scene.h"
#include <OgreResourceGroupManager.h>
#include <OgreSceneManager.h>
#include <Terrain/OgreTerrain.h>
#include <Terrain/OgreTerrainGroup.h>
#include <Terrain/OgreTerrainQuadTreeNode.h>
#include <Terrain/OgreTerrainMaterialGeneratorA.h>
#include "SceneSerializer.h"
#include "OgreManager.h"

using namespace Ogre;

Scene::Scene()
:m_pTerrain(nullptr)
,m_terrainGroup(nullptr)
,m_terrainOption(nullptr)
,m_sunLightDir(Ogre::Vector3(0.55f, -0.3f, 0.75f))
,m_sunLightDiffuse(Ogre::ColourValue::White)
{
	
}

void Scene::Load( const std::string& sceneName, const std::string& sceneGroup, SceneSerializer* pHandler )
{
	pHandler->LoadScene(sceneName, sceneGroup, this);
}

void Scene::Load( const std::string& fullPath, SceneSerializer* pHandler )
{
	String pathname, basename, extname;
	StringUtil::splitFullFilename(fullPath, basename, extname, pathname);

	const String tmpGroup("#Temp");
	ResourceGroupManager::getSingleton().addResourceLocation(pathname, "FileSystem", tmpGroup);

	String sceneName = basename + "." + extname;
	Load(sceneName, tmpGroup, pHandler);
}

void Scene::Save(const std::string& fullPath, SceneSerializer* pHandler)
{
	pHandler->SaveScene(fullPath, this);
}

void Scene::Reset()
{
	if(m_terrainOption)
		RenderManager.EnableDeferredShading(false);

	if(m_pTerrain)
	{
		m_terrainGroup->removeTerrain(0, 0);
		m_pTerrain = nullptr;
	}
	if(m_terrainGroup)
	{
		delete m_terrainGroup;
		m_terrainGroup = nullptr;
	}
	if(m_terrainOption)
	{
		delete m_terrainOption;
		m_terrainOption = nullptr;
	}

	RenderManager.m_pSceneMgr->clearScene();
}

void Scene::New()
{
	//������
	RenderManager.m_pSceneMgr->setAmbientLight(Ogre::ColourValue::White);

	//ȫ�ֹ�
	Ogre::Light* pSunLight = RenderManager.m_pSceneMgr->createLight("SunLight");
	pSunLight->setType(Ogre::Light::LT_DIRECTIONAL);
	pSunLight->setDirection(m_sunLightDir);
	pSunLight->setDiffuseColour(m_sunLightDiffuse);

	float WORLD_SIZE = 128;
	Ogre::uint16 MAP_SIZE = 129;
	Ogre::Vector3 ORIGIN = Ogre::Vector3::ZERO;

	m_terrainOption = new TerrainGlobalOptions;
	m_terrainGroup = new TerrainGroup(RenderManager.m_pSceneMgr, Terrain::ALIGN_X_Z, MAP_SIZE, WORLD_SIZE);
	m_terrainGroup->setOrigin(ORIGIN);

	// 	MaterialManager::getSingleton().setDefaultTextureFiltering(TFO_ANISOTROPIC);
	// 	MaterialManager::getSingleton().setDefaultAnisotropy(7);

	//ȫ�ֹ�
	m_terrainOption->setMaxPixelError(8);
	m_terrainOption->setCompositeMapDistance(3000);
	//m_terrainOption->setUseRayBoxDistanceCalculation(true);
	//m_terrainOption->getDefaultMaterialGenerator()->setDebugLevel(1);
	//m_terrainOption->setLightMapSize(512);

	Ogre::TerrainMaterialGeneratorA::SM2Profile* matProfile = 
		static_cast<Ogre::TerrainMaterialGeneratorA::SM2Profile*>(m_terrainOption->getDefaultMaterialGenerator()->getActiveProfile());
	matProfile->setLightmapEnabled(false);
	// Important to set these so that the terrain knows what to use for derived (non-realtime) data
	// 	m_terrainOption->setLightMapDirection(pSunLight->getDirection().normalisedCopy());
	// 	m_terrainOption->setCompositeMapAmbient(RenderManager.m_pSceneMgr->getAmbientLight());
	// 	m_terrainOption->setCompositeMapDiffuse(pSunLight->getDiffuseColour());

	// Configure default import settings for if we use imported image
	Terrain::ImportData& defaultimp = m_terrainGroup->getDefaultImportSettings();
	defaultimp.terrainSize = MAP_SIZE;
	defaultimp.worldSize = WORLD_SIZE;
	defaultimp.inputScale = 1.0f;
	defaultimp.minBatchSize = 17;
	defaultimp.maxBatchSize = 65;

	size_t maxLayer = GetTerrainMaxLayer();
	defaultimp.layerList.resize(maxLayer);
	for (size_t iLayer=0; iLayer<maxLayer; ++iLayer)
	{
		defaultimp.layerList[iLayer].worldSize = 128;
		defaultimp.layerList[iLayer].textureNames.clear();
		defaultimp.layerList[iLayer].textureNames.clear();
	}

	//����ʼ������������1��
	defaultimp.layerList[0].worldSize = 128;
	defaultimp.layerList[0].textureNames.push_back("dirt_grayrocky_diffusespecular.dds");
	defaultimp.layerList[0].textureNames.push_back("dirt_grayrocky_normalheight.dds");

	//��ʼ��ƽ̹����
	m_terrainGroup->defineTerrain(0, 0, 0.0f);

	// sync load since we want everything in place when we start
	m_terrainGroup->loadTerrain(0, 0);

	m_terrainGroup->freeTemporaryResources();
	m_pTerrain = m_terrainGroup->getTerrain(0, 0);
}