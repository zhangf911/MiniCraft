#include "stdafx.h"
#include "GameDefine.h"
#include <SdkCameraMan.h>
#include "BattleState.h"
#include "LoginState.h"
#include "OgreManager.h"
#include "Unit.h"
#include "Command.h"
#include "World.h"
#include "Building.h"
#include "Resource.h"
#include "AIComponent.h"
#include "GUIManager.h"

std::string CBattleState::StateName = "BattleState";

CBattleState::CBattleState()
:m_bCamMoveLeft(false)
,m_bCamMoveRight(false)
,m_bCamMoveUp(false)
,m_bCamMoveDown(false)
,m_bLBDown(false)
,m_LBDownPos(-1, -1, -1)
,m_LBDownScreenPos(-1, -1)
,m_pSelectionQuad(nullptr)
,m_pQuadNode(nullptr)
{
}

CBattleState::~CBattleState()
{
}

void CBattleState::enter()
{
	using namespace Ogre;

	World& world = World::GetSingleton();
	world.Init();

	m_pSelectionQuad = new Ogre::Rectangle2D(true);
	m_pSelectionQuad->setMaterial("SelectionQuad");
	m_pSelectionQuad->setRenderQueueGroup(RENDER_QUEUE_WORLD_GEOMETRY_2);
	(const_cast<Ogre::AxisAlignedBox&>(m_pSelectionQuad->getBoundingBox())).setInfinite();
	m_pQuadNode = RenderManager.m_pSceneMgr->getRootSceneNode()->createChildSceneNode("SelectionQuadNode");
	m_pQuadNode->attachObject(m_pSelectionQuad);
	m_pQuadNode->setVisible(false);
	
	//绑定输入事件
	Kratos::CInputManager& inputMgr = INPUTMANAGER;
	inputMgr.BindKeyPressed(boost::bind(&CBattleState::OnInputSys_KeyPressed, this, _1));
	inputMgr.BindKeyReleased(boost::bind(&CBattleState::OnInputSys_KeyReleased, this, _1));
	inputMgr.BindMousePressed(boost::bind(&CBattleState::OnInputSys_MousePressed, this, _1, _2));
	inputMgr.BindMouseRelease(boost::bind(&CBattleState::OnInputSys_MouseReleased, this, _1, _2));
	inputMgr.BindMouseMove(boost::bind(&CBattleState::OnInputSys_MouseMove, this, _1));

	GUIMANAGER.SetCursorMode(Kratos::eCursorMode_Normal);

	m_bQuit = false;
}

bool CBattleState::pause()
{
    return true;
}

void CBattleState::resume()
{
	
}

void CBattleState::exit()
{
	Kratos::CInputManager& InputMgr = INPUTMANAGER;
	InputMgr.UnbindMouseMove(Kratos::eInputEventPriority_default);
	InputMgr.UnbindMousePressed(Kratos::eInputEventPriority_default);
	InputMgr.UnbindMouseRelease(Kratos::eInputEventPriority_default);

	World::GetSingleton().Shutdown();
	SAFE_DELETE(m_pSelectionQuad);
	m_pQuadNode = nullptr;
}

void CBattleState::update(float timeSinceLastFrame)
{
	if(m_bQuit)
	{
		changeAppState(findByName(LoginState::StateName));
		return;
	}

	World& world = World::GetSingleton();
	world.Update(timeSinceLastFrame);

	if(world.IsFreeCameraEnabled())
	{
		Ogre::FrameEvent evt;
		evt.timeSinceLastFrame = timeSinceLastFrame;
		world.GetCameraMan()->frameRenderingQueued(evt);
	}
	else
	{
		Ogre::Camera* pCam = RenderManager.m_pMainCamera;
		//更新摄像机运动
		if(m_bCamMoveLeft)	pCam->move(Ogre::Vector3(timeSinceLastFrame*40, 0, 0));
		if(m_bCamMoveRight) pCam->move(Ogre::Vector3(-timeSinceLastFrame*40, 0, 0));
		if(m_bCamMoveUp)	pCam->move(Ogre::Vector3(0, 0, timeSinceLastFrame*40));
		if(m_bCamMoveDown)	pCam->move(Ogre::Vector3(0, 0, -timeSinceLastFrame*40));
	}

	world.UpdateConsoleUITransform(timeSinceLastFrame);
}

bool CBattleState::OnInputSys_MousePressed( const OIS::MouseEvent& arg, OIS::MouseButtonID id )
{
	World& world = World::GetSingleton();

	float screenX = arg.state.X.abs / (float)arg.state.width;
	float screenY = arg.state.Y.abs / (float)arg.state.height;

	POS intersectPos;
	if(!world.GetTerrainIntersectPos(FLOAT2(screenX, screenY), intersectPos))
		return true;

	if(id == OIS::MB_Left)
	{
		//坐标空间转换
		float x = screenX * 2 - 1;
		float y = 1 - 2 * screenY;

		m_pSelectionQuad->setCorners(x, y, x, y, false);
		m_pQuadNode->setVisible(true);

		m_LBDownScreenPos = Ogre::Vector2(screenX, screenY);
		m_LBDownPos = intersectPos;
		m_bLBDown = true;
	}
	else if (id == OIS::MB_Right)
	{
		const SelectedContainer& selectedUnits = world.GetSelectedObjects();
		if(!selectedUnits.empty())
			selectedUnits[0]->GetAi()->GiveCommand(arg, id);
	}        

	return true;
}

bool CBattleState::OnInputSys_MouseReleased( const OIS::MouseEvent& arg, OIS::MouseButtonID id )
{
	if (id == OIS::MB_Left)
	{
		m_bLBDown = false;

		World& world = World::GetSingleton();
		world.ClearSelectedState();
		m_pQuadNode->setVisible(false);

		float screenX = arg.state.X.abs / (float)arg.state.width;
		float screenY = arg.state.Y.abs / (float)arg.state.height;
		
		std::vector<Ogre::MovableObject*> vecResult;
		//鼠标拖动小于阕值时,进行射线查询,使得结果更准确
		float dist = m_LBDownScreenPos.squaredDistance(FLOAT2(screenX, screenY));
		if(dist <= 0.001f)
		{
			Ogre::MovableObject* pObject = world.GetRaySceneQueryResult(arg, QueryTypeSelectableObject);
			if (pObject)
				vecResult.push_back(pObject);
		}
		else
		{
			Ogre::Ray ray;
			RenderManager.m_pMainCamera->getCameraToViewportRay(screenX, screenY, &ray);

			POS intersectPos;
			if(!world.GetTerrainIntersectPos(FLOAT2(screenX, screenY), intersectPos))
				return true;

			//建立查询包围盒
			Ogre::AxisAlignedBox aabb;
			aabb.setMinimum(std::min(m_LBDownPos.x, intersectPos.x), std::min(m_LBDownPos.y, intersectPos.y), 
				std::min(m_LBDownPos.z, intersectPos.z));
			aabb.setMaximum(std::max(m_LBDownPos.x, intersectPos.x), std::max(m_LBDownPos.y + 100, intersectPos.y + 100),
				std::max(m_LBDownPos.z, intersectPos.z));

			world.GetAABBSceneQueryResult(aabb, vecResult, QueryTypeSelectableObject);
		}

		//单位设置为选中状态
		//TODO: 目前只支持单个选中
// 		for(size_t i=0; i<vecResult.size(); ++i)
// 		{
// 			Object* pObj = Ogre::any_cast<Object*>(vecResult[i]->getUserAny());
// 			world.SetObjectSelected(pObj->GetID());
// 		}
		if (!vecResult.empty())
		{
			SelectableObject* pObj = Ogre::any_cast<SelectableObject*>(vecResult[0]->getUserAny());
			world.SetObjectSelected(pObj->GetID());
		}
	}

	return true;
}

bool CBattleState::OnInputSys_MouseMove( const OIS::MouseEvent& arg )
{

	if (World::GetSingleton().IsFreeCameraEnabled())
		World::GetSingleton().GetCameraMan()->injectMouseMove(arg);

	if(m_bLBDown)
	{
		Ogre::Vector2 newScreenPos((float)arg.state.X.abs/arg.state.width, (float)arg.state.Y.abs/arg.state.height);
		float left		= std::min(m_LBDownScreenPos.x, newScreenPos.x);
		float bottom	= std::max(m_LBDownScreenPos.y, newScreenPos.y);
		float right		= std::max(m_LBDownScreenPos.x, newScreenPos.x);
		float top		= std::min(m_LBDownScreenPos.y, newScreenPos.y);

		left = 2 * left - 1;
		right = 2 * right - 1;
		bottom = 1 - 2 * bottom;
		top	= 1 - 2 * top;

		m_pSelectionQuad->setCorners(left, top, right, bottom, false);
	}

	//获取屏幕尺寸
	Ogre::RenderWindow* pRenderWnd = RenderManager.mWindow;
	int w = (int)pRenderWnd->getWidth(), h = (int)pRenderWnd->getHeight();
	//获取当前鼠标绝对位置
	int absX = arg.state.X.abs, absY = arg.state.Y.abs;

	if		(absX == w	&& arg.state.X.rel	> 0 ) m_bCamMoveRight = true;
	else if	(absX == 0	&& arg.state.X.rel < 0)	m_bCamMoveLeft = true;
	if		(absY == h	&& arg.state.Y.rel > 0) m_bCamMoveDown = true;
	else if	(absY == 0	&& arg.state.Y.rel < 0) m_bCamMoveUp = true;

	if (absX > 0 && absX < w)
		m_bCamMoveRight = m_bCamMoveLeft = false;
	if (absY > 0 && absY < h)
		m_bCamMoveUp = m_bCamMoveDown = false;

	return true;
}

bool CBattleState::OnInputSys_KeyPressed( const OIS::KeyEvent& arg )
{
	if (World::GetSingleton().IsFreeCameraEnabled())
		World::GetSingleton().GetCameraMan()->injectKeyDown(arg);

	return true;
}

bool CBattleState::OnInputSys_KeyReleased( const OIS::KeyEvent& arg )
{
	Kratos::COgreManager& renderMgr = RenderManager;

	switch (arg.key)
	{
	case OIS::KC_ESCAPE: m_bQuit = true; break;
	case OIS::KC_F1: World::GetSingleton().EnableFreeCamera(!World::GetSingleton().IsFreeCameraEnabled()); break;
	case OIS::KC_F2: renderMgr.EnableShadow(!renderMgr.GetEffectConfig().bShadow); break;
	case OIS::KC_F3: renderMgr.EnableSSAO(!renderMgr.GetEffectConfig().bSSAO); break;
	case OIS::KC_F4: renderMgr.EnableFXAA(!renderMgr.GetEffectConfig().bFXAA); break;
	case OIS::KC_F5: renderMgr.EnableSharpen(!renderMgr.GetEffectConfig().bSharpen); break;
	case OIS::KC_F6: Ogre::Profiler::getSingleton().setEnabled(!Ogre::Profiler::getSingleton().getEnabled()); break;
	case OIS::KC_F7: Ogre::Profiler::getSingleton().setDisplayMode((Ogre::Profiler::DisplayMode)(int)!Ogre::Profiler::getSingleton().getDisplayMode()); break;
	case OIS::KC_H: renderMgr.ToggleShowFrameStats(); break;
	}

	if (World::GetSingleton().IsFreeCameraEnabled())
		World::GetSingleton().GetCameraMan()->injectKeyUp(arg);

	return true;
}
