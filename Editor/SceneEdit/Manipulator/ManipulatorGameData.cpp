#include "stdafx.h"
#include "ManipulatorGameData.h"
#include "Utility.h"
#include "AI/BehaviorTree/BehaviorTreeTemplateManager.h"
#include "AI/BehaviorTree/BehaviorTreeNode.h"
#include "AI/BehaviorTree/BehaviorTreeTemplate.h"
#include "AI/BehaviorTree/BehaviorTree.h"
#include "AI/BehaviorTree/BlackBoard.h"
#include "AI/BehaviorTree/Behavior.h"
#include "ScriptSystem.h"

ManipulatorGameData::ManipulatorGameData()
{
	m_dataMgr = GameDataDefManager::GetSingletonPtr();
	SCRIPTNAMAGER.Init();
}

ManipulatorGameData::~ManipulatorGameData()
{
	SCRIPTNAMAGER.Shutdown();

	for (size_t i=0; i<m_btTemplates.size(); ++i)
		std::for_each(m_btTemplates[i].m_nodeList.begin(), m_btTemplates[i].m_nodeList.end(), std::default_delete<BTTemplate::SBTNode>());
	m_btTemplates.clear();
}

void ManipulatorGameData::LoadAllXml()
{
	m_dataMgr->LoadAllData();
	_ParseAllBTTemplates();
}

void ManipulatorGameData::SaveAllXml()
{
	m_dataMgr->SaveAllData();
}

std::vector<std::wstring> ManipulatorGameData::GetRaceBuildingNames( eGameRace race ) const
{
	std::vector<std::wstring> retNames;
	for (auto iter=m_dataMgr->m_buildingData.begin(); iter!=m_dataMgr->m_buildingData.end(); ++iter)
	{
		SBuildingData& data = iter->second;
		int iRace = (eGameRace)Ogre::StringConverter::parseInt(data.params["race"]);
		if((eGameRace)iRace == race)
			retNames.push_back(Utility::EngineToUnicode(iter->first));
	}

	return std::move(retNames);
}

SBuildingData* ManipulatorGameData::GetBuildingData( const std::wstring& name )
{
	auto iter = m_dataMgr->m_buildingData.find(Utility::UnicodeToEngine(name));
	assert(iter != m_dataMgr->m_buildingData.end());
	return &(iter->second);
}

std::vector<std::wstring> ManipulatorGameData::GetAbilityNames() const
{
	std::vector<std::wstring> retNames;
	retNames.resize(m_dataMgr->m_abilityData.size());

	int i = 0;
	for (auto iter=m_dataMgr->m_abilityData.begin(); iter!=m_dataMgr->m_abilityData.end(); ++iter)
		retNames[i++] = Utility::EngineToUnicode(iter->first);

	return std::move(retNames);
}

const SAbilityData* ManipulatorGameData::GetAbilityData( const std::wstring& name )
{
	auto iter = m_dataMgr->m_abilityData.find(Utility::UnicodeToEngine(name));
	if(iter != m_dataMgr->m_abilityData.end())
		return &iter->second;
	else
		return nullptr;
}

void ManipulatorGameData::SetBuildingAbility( const std::wstring& buildingName, int slotIndex, const std::wstring& abilName )
{
	assert(slotIndex >= 0 && slotIndex < 15);
	(const_cast<SBuildingData*>(GetBuildingData(buildingName)))->m_vecAbilities[slotIndex] = Utility::UnicodeToEngine(abilName);
}

SUnitData* ManipulatorGameData::GetUnitData( const std::string& meshname )
{
	UnitTable& units = GameDataDefManager::GetSingleton().m_unitData;

	for(auto iter=units.begin(); iter!=units.end(); ++iter)
	{
		SUnitData& unitData = iter->second;
		if(unitData.params["meshname"] == meshname)
			return &unitData;
	}
	
	return nullptr;
}

std::vector<std::wstring> ManipulatorGameData::GetAllBehaviorTreeTemplateNames() const
{
	auto names = Kratos::aiBehaviorTreeTemplateManager::GetSingleton().GetAllTemplateNames();
	std::vector<std::wstring> ret(names.size());
	for(size_t i=0; i<names.size(); ++i)
		ret[i] = Utility::EngineToUnicode(names[i]);

	return std::move(ret);
}

std::vector<std::wstring> ManipulatorGameData::GetAllBehaviorNames() const
{
	auto names = Kratos::aiBehaviorTreeTemplateManager::GetSingleton().GetAllBehaviorNames();
	std::vector<std::wstring> ret(names.size());
	for(size_t i=0; i<names.size(); ++i)
		ret[i] = Utility::EngineToUnicode(names[i]);

	return std::move(ret);
}

void ManipulatorGameData::_ParseAllBTTemplates()
{
	m_btTemplates.clear();
	Kratos::aiBehaviorTreeTemplateManager& btMgr = Kratos::aiBehaviorTreeTemplateManager::GetSingleton();
	auto names = btMgr.GetAllTemplateNames();
	m_btTemplates.resize(names.size());

	for (size_t i=0; i<names.size(); ++i)
	{
		BTTemplate& tmpl = m_btTemplates.at(i);
		Kratos::aiBehaviorTreeTemplate* pEngineTmpl = btMgr.GetTemplate(names[i]);

		tmpl.m_name = Utility::EngineToUnicode(names[i]);
		tmpl.race = (int)pEngineTmpl->GetBT()->GetRace();

		Kratos::aiBehaviorTreeNode* pNode = pEngineTmpl->GetBT()->GetRootNode();
		BTTemplate::SBTNode* root = new BTTemplate::SBTNode;
		_ParseBTNode(pNode, root, nullptr, tmpl);

		//黑板
		_ParseBlackboard(tmpl.m_ownBB, pEngineTmpl->GetBB());
		tmpl.m_raceBB = &m_raceBlackboards[pEngineTmpl->GetBT()->GetRace()];
	}

	//所有全局黑板
	_ParseBlackboard(m_raceBlackboards[eGameRace_Terran], btMgr.GetGlobalBB(eGameRace_Terran));
	_ParseBlackboard(m_raceBlackboards[eGameRace_Zerg], btMgr.GetGlobalBB(eGameRace_Zerg));

	//hack a little..
	class FakeBehavior : public Kratos::aiBehavior
	{
	public:
		virtual	void	Execute(Ogre::Any& owner) {}
	};

	btMgr.AddBehavior("Idle", new FakeBehavior);
	btMgr.AddBehavior("MoveToEnemyBase", new FakeBehavior);
}

void ManipulatorGameData::_ParseBlackboard( Blackboard& bb, Kratos::aiBlackBoard* pEngineBB )
{
	const Kratos::aiBlackBoard::ParamMap& params = pEngineBB->GetParams();
	for(auto iter=params.begin(); iter!=params.end(); ++iter)
	{
		if (iter->second.m_bSave)
			bb.push_back(Utility::EngineToUnicode(iter->first));
	}
}

void ManipulatorGameData::_ParseBTNode( const Kratos::aiBehaviorTreeNode* pEngineNode, BTTemplate::SBTNode* pNode, 
	BTTemplate::SBTNode* parent, BTTemplate& tmpl )
{
	pNode->parent = parent;
	const STRING type = Kratos::aiBehaviorTreeNode::GetNodeTypeToStr(pEngineNode->GetType());
	pNode->type = Utility::EngineToUnicode(type);
	pNode->color = _GetBTNodeColor((eBTNodeType)pEngineNode->GetType());

	switch (pEngineNode->GetType())
	{
	case Kratos::eNodeType_Sequence: 
		{
			pNode->txtProperty = L"";
		}
		break;

	case Kratos::eNodeType_Condition: 
		{
			const Kratos::aiBTConditionNode* conditionNode = dynamic_cast<const Kratos::aiBTConditionNode*>(pEngineNode);
			pNode->txtProperty = Utility::EngineToUnicode(conditionNode->GetConditions());
		}
		break;

	case Kratos::eNodeType_Action: 
		{ 
			const Kratos::aiBTActionNode* actionNode = dynamic_cast<const Kratos::aiBTActionNode*>(pEngineNode);
			pNode->txtProperty = Utility::EngineToUnicode(actionNode->GetBehaviorName());
		} 
		break;
	}

	auto& childs = pEngineNode->GetChilds();
	for(auto iter=childs.begin(); iter!=childs.end(); ++iter)
	{
		BTTemplate::SBTNode* childNode = new BTTemplate::SBTNode;
		_ParseBTNode(*iter, childNode, pNode, tmpl);
		pNode->childs.push_back(childNode);
	}

	tmpl.m_nodeList.push_back(pNode);
}

ManipulatorGameData::BTTemplate& ManipulatorGameData::GetBTTemplate( const std::wstring& name )
{
	auto itTmpl= std::find_if(m_btTemplates.begin(), m_btTemplates.end(), [&](const ManipulatorGameData::BTTemplate& tmpl) { return tmpl.m_name == name; });
	assert(itTmpl != m_btTemplates.end());
	return *itTmpl;
}

void ManipulatorGameData::SaveAllBehaviorTreeTemplates()
{
	using namespace rapidxml;

	Ogre::StringVectorPtr loc = Ogre::ResourceGroupManager::getSingleton().findResourceLocation("BehaviorTemplate", "*Behaviors");

	for (size_t i=0; i<m_btTemplates.size(); ++i)
	{
		STRING filepath = loc->at(0) + "\\" + Utility::UnicodeToEngine(m_btTemplates[i].m_name) + ".xml";
		_SaveBTTemplate(m_btTemplates.at(i), filepath);
	}

	//保存种族全局黑板
	STRING filepath = loc->at(0) + "\\RaceGlobal.xml";
	xml_document<> doc;  
	xml_node<>* header = doc.allocate_node(rapidxml::node_pi, "xml version='1.0' encoding='utf-8'");
	doc.append_node(header);
	xml_node<>* root = doc.allocate_node(node_element, "Root");
	doc.append_node(root);

	//Terran
	xml_node<>* bbNode = doc.allocate_node(node_element, "BlackBoard");
	root->append_node(bbNode);
	bbNode->append_attribute(doc.allocate_attribute("race", "Terran"));
	Kratos::aiBlackBoard* pBB = Kratos::aiBehaviorTreeTemplateManager::GetSingleton().GetGlobalBB(eGameRace_Terran);
	_SaveBlackboard(&doc, pBB, bbNode);

	//Zerg
	bbNode = doc.allocate_node(node_element, "BlackBoard");
	root->append_node(bbNode);
	bbNode->append_attribute(doc.allocate_attribute("race", "Zerg"));
	pBB = Kratos::aiBehaviorTreeTemplateManager::GetSingleton().GetGlobalBB(eGameRace_Zerg);
	_SaveBlackboard(&doc, pBB, bbNode);

	//脚本信息
	xml_node<>* scriptNode = doc.allocate_node(node_element, "Script");
	root->append_node(scriptNode);
	scriptNode->append_attribute(doc.allocate_attribute("filename", "GlobalBlackboard.lua"));

	std::ofstream out(filepath);
	out << doc;
}

void ManipulatorGameData::_SaveBTTemplate( const BTTemplate& tmpl, const STRING& filepath )
{
	using namespace rapidxml;

	xml_document<> doc;  
	xml_node<>* header = doc.allocate_node(rapidxml::node_pi, "xml version='1.0' encoding='utf-8'");
	doc.append_node(header);
	xml_node<>* root = doc.allocate_node(node_element, "Root");
	doc.append_node(root);
	xml_node<>* tmplNode = doc.allocate_node(node_element, "BehaviorTemplate");
	root->append_node(tmplNode);

	STRING race;
	eGameRace eRace = (eGameRace)tmpl.race;
	if(eRace == eGameRace_Terran) race = "Terran";
	else if(eRace == eGameRace_Zerg) race = "Zerg";
	else assert(0);
	tmplNode->append_attribute(doc.allocate_attribute("name", doc.allocate_string(Utility::UnicodeToEngine(tmpl.m_name).c_str())));
	tmplNode->append_attribute(doc.allocate_attribute("race", doc.allocate_string(race.c_str())));
	xml_node<>* btreeNode = doc.allocate_node(node_element, "BehaviorTree");
	tmplNode->append_node(btreeNode);

	//寻找根节点
	BTTemplate::SBTNode* node = tmpl.m_nodeList.front();
	while(node->parent)
		node = node->parent;

	//保存树结构
	_SaveBTNode(&doc, node->childs[0], btreeNode);

	//保存黑板
	Kratos::aiBehaviorTreeTemplate* engineTmpl = Kratos::aiBehaviorTreeTemplateManager::GetSingleton().GetTemplate(
		Utility::UnicodeToEngine(tmpl.m_name));
	xml_node<>* bbNode = doc.allocate_node(node_element, "BlackBoard");
	tmplNode->append_node(bbNode);

	_SaveBlackboard(&doc, engineTmpl->GetBB(), bbNode);

	//保存脚本信息
	const STRING& scriptName = engineTmpl->GetBBScriptName();
	const STRING& scriptEntry = engineTmpl->GetBBScriptEntry();
	xml_node<>* scriptNode = doc.allocate_node(node_element, "Script");
	tmplNode->append_node(scriptNode);
	scriptNode->append_attribute(doc.allocate_attribute("filename", scriptName.c_str()));
	scriptNode->append_attribute(doc.allocate_attribute("entry", scriptEntry.c_str()));

	std::ofstream out(filepath);
	out << doc;
}

void ManipulatorGameData::_SaveBTNode( rapidxml::xml_document<>* doc, BTTemplate::SBTNode* node, rapidxml::xml_node<>* xmlNode )
{
	//自己
	rapidxml::xml_node<>* treeNode = nullptr;
	if(node->type == L"Sequence") 
	{
		treeNode = doc->allocate_node(rapidxml::node_element, "SequenceNode");
	}
	else if(node->type == L"Condition")
	{
		treeNode = doc->allocate_node(rapidxml::node_element, "ConditionNode");
		const STRING condition = Utility::UnicodeToEngine(node->txtProperty);
		treeNode->append_attribute(doc->allocate_attribute("expression", doc->allocate_string(condition.c_str())));
	}
	else if(node->type == L"Action")
	{
		treeNode = doc->allocate_node(rapidxml::node_element, "ActionNode");
		const STRING action = Utility::UnicodeToEngine(node->txtProperty);
		treeNode->append_attribute(doc->allocate_attribute("behavior", doc->allocate_string(action.c_str())));
	}
	else 
	{
		assert(0);
	}
	xmlNode->append_node(treeNode);

	//子节点
	for(size_t i=0; i<node->childs.size(); ++i)
		_SaveBTNode(doc, node->childs[i], treeNode);
}

void ManipulatorGameData::_SaveBlackboard( rapidxml::xml_document<>* doc, Kratos::aiBlackBoard* pBB, rapidxml::xml_node<>* xmlNode)
{
	const Kratos::aiBlackBoard::ParamMap& params = pBB->GetParams();
	for (auto iter=params.begin(); iter!=params.end(); ++iter)
	{
		const Kratos::aiBlackBoard::SValue& val = iter->second;
		if(!val.m_bSave)
			continue;

		rapidxml::xml_node<>* varNode = doc->allocate_node(rapidxml::node_element, "Variable");
		xmlNode->append_node(varNode);
		varNode->append_attribute(doc->allocate_attribute("name", iter->first.c_str()));
		varNode->append_attribute(doc->allocate_attribute("value", val.m_value.c_str()));

		STRING type;
		switch (val.m_type)
		{
		case Kratos::aiBlackBoard::eVarType_Int: type = "int"; break;
		case Kratos::aiBlackBoard::eVarType_Float: type = "float"; break;
		case Kratos::aiBlackBoard::eVarType_Bool: type = "bool"; break;
		default: assert(0);
		}
		varNode->append_attribute(doc->allocate_attribute("type", doc->allocate_string(type.c_str())));
	}
}

void ManipulatorGameData::ValidateBehaviorTemplate( const BTTemplate& tmpl )
{
	//先保存到临时xml中
	Ogre::StringVectorPtr loc = Ogre::ResourceGroupManager::getSingleton().findResourceLocation("BehaviorTemplate", "*Behaviors");
	const STRING filepath = loc->at(0) + "\\TempBehaviorTreeTemplate.xml";
	_SaveBTTemplate(tmpl, filepath);

	//然后加载模板,进行校验
	Kratos::aiBehaviorTreeTemplate temp;
	temp.Load("TempBehaviorTreeTemplate.xml");

	try
	{
		temp.GetBT()->ValidateTree();
	} 
	catch(Ogre::Exception& e)
	{
		MessageBoxW(nullptr, Utility::EngineToUnicode(e.getDescription()).c_str(), L"Warning!", MB_ICONWARNING);
	}
	
	//最后删除临时xml
	Ogre::ResourceGroupManager::getSingleton().deleteResource("TempBehaviorTreeTemplate.xml", "BehaviorTemplate");
}

void ManipulatorGameData::DefineBlackboardParam( bool bOwnBB, BTTemplate& tmpl )
{
	if (bOwnBB)
	{
		STRING name = "Param_";
		name += Ogre::StringConverter::toString(tmpl.m_ownBB.size());
		
		Kratos::aiBlackBoard* pBB = Kratos::aiBehaviorTreeTemplateManager::GetSingleton().GetTemplate(
			Utility::UnicodeToEngine(tmpl.m_name))->GetBB();
		pBB->DefineParam(name, "0", Kratos::aiBlackBoard::eVarType_Int);

		tmpl.m_ownBB.push_back(Utility::EngineToUnicode(name));
	}
	else
	{
		STRING name = "Param_";
		name += Ogre::StringConverter::toString(tmpl.m_raceBB->size());

		Kratos::aiBlackBoard* pBB = Kratos::aiBehaviorTreeTemplateManager::GetSingleton().GetGlobalBB((eGameRace)tmpl.race);
		pBB->DefineParam(name, "0", Kratos::aiBlackBoard::eVarType_Int);

		tmpl.m_raceBB->push_back(Utility::EngineToUnicode(name));
	}
}

const ManipulatorGameData::SBBParam ManipulatorGameData::GetBlackboardParam(const std::wstring& name, const BTTemplate& tmpl, bool bOwnBB) const
{
	Kratos::aiBlackBoard* pBB = nullptr;
	if (bOwnBB)
		pBB = Kratos::aiBehaviorTreeTemplateManager::GetSingleton().GetTemplate(Utility::UnicodeToEngine(tmpl.m_name))->GetBB();
	else
		pBB = Kratos::aiBehaviorTreeTemplateManager::GetSingleton().GetGlobalBB((eGameRace)tmpl.race);

	auto& param = pBB->GetParam(Utility::UnicodeToEngine(name));
	SBBParam p;
	p.value = Utility::EngineToUnicode(param.m_value);

	if(param.m_type == Kratos::aiBlackBoard::eVarType_Int) p.type = L"Int";
	else if(param.m_type == Kratos::aiBlackBoard::eVarType_Float) p.type = L"Float";
	else if(param.m_type == Kratos::aiBlackBoard::eVarType_Bool) p.type = L"Bool";
	else assert(0);

	return p;
}

void ManipulatorGameData::RenameBlackboardParam( const std::wstring& oldName, const std::wstring& newName, BTTemplate& tmpl, bool bOwnBB )
{
	Kratos::aiBlackBoard* pEngineBB = nullptr;
	Blackboard* pBB = nullptr;
	if (bOwnBB)
	{
		pBB = &tmpl.m_ownBB;
		pEngineBB = Kratos::aiBehaviorTreeTemplateManager::GetSingleton().GetTemplate(Utility::UnicodeToEngine(tmpl.m_name))->GetBB();
	}
	else
	{
		pBB = tmpl.m_raceBB;
		pEngineBB = Kratos::aiBehaviorTreeTemplateManager::GetSingleton().GetGlobalBB((eGameRace)tmpl.race);
	}

	auto iter = std::find(pBB->begin(), pBB->end(), oldName);
	assert(iter != pBB->end());
	*iter = newName;

	auto param = pEngineBB->GetParam(Utility::UnicodeToEngine(oldName));
	auto& paramMap = const_cast<Kratos::aiBlackBoard::ParamMap&>(pEngineBB->GetParams());
	paramMap.erase(Utility::UnicodeToEngine(oldName));
	paramMap[Utility::UnicodeToEngine(newName)] = param;
}

void ManipulatorGameData::SetBlackboardParam( const std::wstring& name, const SBBParam& param, const BTTemplate& tmpl, bool bOwnBB )
{
	Kratos::aiBlackBoard* pBB = nullptr;
	if (bOwnBB)
		pBB = Kratos::aiBehaviorTreeTemplateManager::GetSingleton().GetTemplate(Utility::UnicodeToEngine(tmpl.m_name))->GetBB();
	else
		pBB = Kratos::aiBehaviorTreeTemplateManager::GetSingleton().GetGlobalBB((eGameRace)tmpl.race);

	auto& paramEngine = pBB->GetParam(Utility::UnicodeToEngine(name));
	paramEngine.m_value = Utility::UnicodeToEngine(param.value);

	if(param.type == L"Int") paramEngine.m_type = Kratos::aiBlackBoard::eVarType_Int;
	else if(param.type == L"Float") paramEngine.m_type = Kratos::aiBlackBoard::eVarType_Float;
	else if(param.type == L"Bool") paramEngine.m_type = Kratos::aiBlackBoard::eVarType_Bool;
	else assert(0);
}

ManipulatorGameData::BTTemplate::SBTNode* ManipulatorGameData::AddBTNode( BTTemplate& tmpl, eBTNodeType type )
{
	BTTemplate::SBTNode* newNode = new BTTemplate::SBTNode;

	newNode->type = Utility::EngineToUnicode(Kratos::aiBTActionNode::GetNodeTypeToStr((Kratos::eNodeType)type));
	newNode->color = _GetBTNodeColor(type);
	tmpl.m_nodeList.push_back(newNode);

	return newNode;
}

DWORD ManipulatorGameData::_GetBTNodeColor( eBTNodeType type )
{
	switch (type)
	{
	case eBTNodeType_Sequence:	return 0xffc080ff;
	case eBTNodeType_Condition:	return 0xff008888;
	case eBTNodeType_Action:	return 0xff008800; 
	default:					return 0;
	}
}
