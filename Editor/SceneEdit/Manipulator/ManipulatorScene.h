/********************************************************************
	created:	14:12:2012   1:24
	filename: 	F:\MiniCraft\Editor\SceneEdit\Manipulator\ManipulatorScene.h
	author:		maval
	
	purpose:	场景加载,保存
*********************************************************************/

#ifndef ManipulatorScene_h__
#define ManipulatorScene_h__

#include "../Singleton.h"
#include "ManipulatorObject.h"
#include "ManipulatorTerrain.h"
#include "ManipulatorEventCallback.h"
#include "ManipulatorNavMesh.h"

namespace Ogre
{
	class SceneManager;
}

class DotSceneSerialezer;
class DotSceneLoader;


class ManipulatorScene : public CSingleton<ManipulatorScene>, public ManipulatorCallbackManager<ManipulatorSceneEventCallback>
{
	ManipulatorScene();
	~ManipulatorScene();
	DECLEAR_SINGLETON(ManipulatorScene);

public:
	void	SceneNew(const std::wstring& sceneName);
	void	SceneOpen(const std::wstring& filepath);
	void	SceneSave();
	void	SceneClose();

	bool	GetIsSceneReady()	{ return m_bIsSceneReay; }
	//重新绑定SceneNode(比如切换了场景)
	void	OnGizmoNodeReset();

	void	Shutdown();
	const std::wstring&	GetScenePath()	{ return m_scenePath; }
	const std::wstring&	GetSceneName()	{ return m_sceneName; }
	//包括场景文件夹的名字(带\\)
	const std::wstring	GenerateSceneFullPath();

	ManipulatorTerrain&	GetTerrain()	{ return *m_manipulatorTerrain; }
	ManipulatorObject&	GetObject()		{ return *m_manipulatorObject; }
	ManipulatorNavMesh&	GetNavMesh()	{ return *m_manipulatorNavMesh; }

private:
	std::wstring		m_scenePath;
	std::wstring		m_sceneName;
	bool				m_bIsSceneReay;
	DotSceneSerialezer*	m_sceneSerializer;
	DotSceneLoader*		m_sceneLoader;
	ManipulatorTerrain*	m_manipulatorTerrain;
	ManipulatorObject*	m_manipulatorObject;
	ManipulatorNavMesh*	m_manipulatorNavMesh;
};

#define	ManipulatorSystem	ManipulatorScene::GetSingleton()

#endif // ManipulatorScene_h__







