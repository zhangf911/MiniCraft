#include "stdafx.h"
#include "BehaviorTreeEditorDlg.h"
#include "BehaviorTreeEditorView.h"
#include "BehaviorTreeEditorExplorer.h"
#include "BehaviorTreeEditorProperty.h"
#include "Manipulator/ManipulatorScene.h"

BEGIN_MESSAGE_MAP(DialogBehaviorTreeEditor, CXTPDialog)
	ON_WM_SIZE()
	ON_MESSAGE(XTPWM_DOCKINGPANE_NOTIFY, _AttachDockPane)
	ON_COMMAND(IDC_BTEditor_Arrange, OnBtnArrange)
	ON_UPDATE_COMMAND_UI(IDC_BTEditor_Arrange, OnUpdateUI_Btn)
	ON_COMMAND(IDC_BTEditor_Validate, OnBtnValidate)
	ON_UPDATE_COMMAND_UI(IDC_BTEditor_Validate, OnUpdateUI_Btn)
	ON_COMMAND(IDC_BTEditor_Save, OnBtnSave)
	ON_UPDATE_COMMAND_UI(IDC_BTEditor_Save, OnUpdateUI_Btn)
	ON_COMMAND(IDC_BTEditor_AddOwnParam, OnBtnAddOwnParam)
	ON_UPDATE_COMMAND_UI(IDC_BTEditor_AddOwnParam, OnUpdateUI_Btn)
	ON_COMMAND(IDC_BTEditor_AddRaceParam, OnBtnAddRaceParam)
	ON_UPDATE_COMMAND_UI(IDC_BTEditor_AddRaceParam, OnUpdateUI_Btn)
	ON_COMMAND(IDC_BTEditor_AddRaceParam, OnBtnAddRaceParam)
	ON_UPDATE_COMMAND_UI(IDC_BTEditor_AddSequenceNode, OnUpdateUI_Btn)
	ON_UPDATE_COMMAND_UI(IDC_BTEditor_AddConditionNode, OnUpdateUI_Btn)
	ON_UPDATE_COMMAND_UI(IDC_BTEditor_AddActionNode, OnUpdateUI_Btn)
	ON_COMMAND(IDC_BTEditor_AddSequenceNode, OnBtnAddNode<ManipulatorGameData::eBTNodeType_Sequence>)
	ON_COMMAND(IDC_BTEditor_AddConditionNode, OnBtnAddNode<ManipulatorGameData::eBTNodeType_Condition>)
	ON_COMMAND(IDC_BTEditor_AddActionNode, OnBtnAddNode<ManipulatorGameData::eBTNodeType_Action>)
END_MESSAGE_MAP()

DialogBehaviorTreeEditor::DialogBehaviorTreeEditor( CWnd* pParent /*= NULL*/ )
:CXTPDialog()
,m_pView(new BehaviorTreeEditorView(this))
,m_pExplorer(new BehaviorTreeEditorExplorer(this))
,m_property(new BehaviorTreeEditorProperty(this))
,m_paneExplorer(NULL)
,m_paneProperty(NULL)
{
	Create(IDD_DlgChildEmpty, pParent);
}

DialogBehaviorTreeEditor::~DialogBehaviorTreeEditor()
{
	SAFE_DELETE(m_pView);
	SAFE_DELETE(m_pExplorer);
	SAFE_DELETE(m_property);
}

BOOL DialogBehaviorTreeEditor::OnInitDialog()
{
	if (!m_pView->Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0), this, AFX_IDW_PANE_FIRST, NULL))
		return FALSE;

	if (!InitCommandBars())
		return FALSE;

	_CreateRibbon();
	_LoadIcon();
	_CreateDockPane();

	m_pView->SetPropertyDlg(m_property);
	m_pExplorer->SetView(m_pView);
	m_property->SetView(m_pView);

	return TRUE;
}

void DialogBehaviorTreeEditor::OnSize( UINT nType, int cx, int cy )
{
	CRect rcClient;
	GetClientRect(rcClient);

	RepositionBars(0, 0xffff, AFX_IDW_PANE_FIRST, reposDefault, 0, rcClient);
	RepositionBars(0, 0xffff, AFX_IDW_PANE_FIRST, reposQuery, rcClient, rcClient);

	HDWP hdwp = BeginDeferWindowPos(1);
	DeferWindowPos(hdwp, m_pView->GetSafeHwnd(), nullptr, rcClient.left, rcClient.top, rcClient.Width(), rcClient.Height(), SWP_NOZORDER);
	EndDeferWindowPos(hdwp);
}

void DialogBehaviorTreeEditor::_CreateRibbon()
{
	CXTPPaintManager::SetTheme(xtpThemeRibbon);
	 
	///Ribbon
	CXTPRibbonBar* pRibbonBar = (CXTPRibbonBar*)GetCommandBars()->Add(_T("Ribbon"), xtpBarTop, RUNTIME_CLASS(CXTPRibbonBar));
	pRibbonBar->ShowCaptionAlways(FALSE);
	pRibbonBar->ShowQuickAccess(FALSE);

	///RibbonHome
	CXTPRibbonTab* pTab = pRibbonBar->AddTab(L"Home");
	//RibbonHome - GroupHome
	CXTPRibbonGroup* pGroup = pTab->AddGroup(L"Home");
	//RibbonHome - GroupHome - Arrange
	pGroup->Add(xtpControlButton, IDC_BTEditor_Arrange);
	//RibbonHome - GroupHome - Validate
	pGroup->Add(xtpControlButton, IDC_BTEditor_Validate);
	//RibbonHome - GroupHome - Save
	pGroup->Add(xtpControlButton, IDC_BTEditor_Save);

	//RibbonHome - GroupBlackboard
	pGroup = pTab->AddGroup(L"Blackboard");
	//RibbonHome - GroupBlackboard - AddOwn
	pGroup->Add(xtpControlButton, IDC_BTEditor_AddOwnParam);
	//RibbonHome - GroupBlackboard - AddGlobal
	pGroup->Add(xtpControlButton, IDC_BTEditor_AddRaceParam);

	//RibbonHome - GroupAddNode
	pGroup = pTab->AddGroup(L"AddNode");
	//RibbonHome - GroupAddNode - Sequence
	pGroup->Add(xtpControlButton, IDC_BTEditor_AddSequenceNode);
	//RibbonHome - GroupAddNode - Condition
	pGroup->Add(xtpControlButton, IDC_BTEditor_AddConditionNode);
	//RibbonHome - GroupAddNode - Action
	pGroup->Add(xtpControlButton, IDC_BTEditor_AddActionNode);
	
}

void DialogBehaviorTreeEditor::_LoadIcon()
{
	CXTPImageManager* imgMgr = GetCommandBars()->GetImageManager();
	UINT icon[1] = { IDC_BTEditor_Arrange };	imgMgr->SetIcons(IDB_Button, icon, _countof(icon), CSize(32, 32));
	icon[0] = IDC_BTEditor_Validate;			imgMgr->SetIcons(IDB_Button, icon, _countof(icon), CSize(32, 32));
	icon[0] = IDC_BTEditor_Save;				imgMgr->SetIcons(IDB_Button, icon, _countof(icon), CSize(32, 32));
	icon[0] = IDC_BTEditor_AddOwnParam;			imgMgr->SetIcons(IDB_Button, icon, _countof(icon), CSize(32, 32));
	icon[0] = IDC_BTEditor_AddRaceParam;		imgMgr->SetIcons(IDB_Button, icon, _countof(icon), CSize(32, 32));
	icon[0] = IDC_BTEditor_AddSequenceNode;		imgMgr->SetIcons(IDB_Button, icon, _countof(icon), CSize(32, 32));
	icon[0] = IDC_BTEditor_AddConditionNode;	imgMgr->SetIcons(IDB_Button, icon, _countof(icon), CSize(32, 32));
	icon[0] = IDC_BTEditor_AddActionNode;		imgMgr->SetIcons(IDB_Button, icon, _countof(icon), CSize(32, 32));
}

void DialogBehaviorTreeEditor::_CreateDockPane()
{
	m_dockMgr.InstallDockingPanes(this);
	m_dockMgr.SetTheme(xtpPaneThemeVisualStudio2010);
	m_dockMgr.SetClientMargin(6);
	m_dockMgr.SetThemedFloatingFrames(TRUE);
	m_dockMgr.SetAlphaDockingContext(TRUE);
	m_dockMgr.SetShowDockingContextStickers(TRUE);
	m_dockMgr.SetShowContentsWhileDragging(TRUE);
	m_dockMgr.SetDefaultPaneOptions(xtpPaneNoHideable);

	m_paneExplorer = m_dockMgr.CreatePane(IDR_Pane_BT_Explorer, CRect(0, 0, 150, 0), xtpPaneDockLeft);
	m_paneExplorer->SetMinTrackSize(100);
	m_paneExplorer->Attach(m_pExplorer);

	m_paneProperty = m_dockMgr.CreatePane(IDR_Pane_BT_Property, CRect(0, 0, 250, 0), xtpPaneDockRight);
	m_paneProperty->SetMinTrackSize(100);
	m_paneProperty->Attach(m_property);
}

LRESULT DialogBehaviorTreeEditor::_AttachDockPane( WPARAM wParam, LPARAM lParam )
{
	if (wParam == XTP_DPN_SHOWWINDOW)
	{
		CXTPDockingPane* pPane = (CXTPDockingPane*)lParam;

		if (!pPane->IsValid())
		{
			switch (pPane->GetID())
			{
			case IDR_Pane_BT_Explorer: pPane->Attach(m_pExplorer); break;
			case IDR_Pane_BT_Property: pPane->Attach(m_property); break;
			default: assert(0);
			}
		}
		return 1;
	}

	return 0;
}

void DialogBehaviorTreeEditor::OnUpdateUI_Btn( CCmdUI* pCmdUI )
{
	pCmdUI->Enable(m_pView->GetActiveTemplate()?TRUE:FALSE);
}

void DialogBehaviorTreeEditor::OnBtnArrange()
{
	m_pView->Arrange();
}

void DialogBehaviorTreeEditor::OnBtnValidate()
{
	ManipulatorSystem.GetGameData().ValidateBehaviorTemplate(*m_pView->GetActiveTemplate());
}

void DialogBehaviorTreeEditor::OnBtnSave()
{
	ManipulatorSystem.GetGameData().SaveAllBehaviorTreeTemplates();
}

void DialogBehaviorTreeEditor::OnBtnAddOwnParam()
{
	auto curTmpl = m_pView->GetActiveTemplate();
	ManipulatorSystem.GetGameData().DefineBlackboardParam(true, *curTmpl);
	m_pView->Refresh();
}

void DialogBehaviorTreeEditor::OnBtnAddRaceParam()
{
	auto curTmpl = m_pView->GetActiveTemplate();
	ManipulatorSystem.GetGameData().DefineBlackboardParam(false, *curTmpl);
	m_pView->Refresh();
}

template<int type>
void DialogBehaviorTreeEditor::OnBtnAddNode()
{
	m_pView->AddNewNode((ManipulatorGameData::eBTNodeType)type);
}
