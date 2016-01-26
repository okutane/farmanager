﻿/*
filepanels.cpp

Файловые панели
*/
/*
Copyright © 1996 Eugene Roshal
Copyright © 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "headers.hpp"
#pragma hdrstop

#include "filepanels.hpp"
#include "keys.hpp"
#include "macroopcode.hpp"
#include "filelist.hpp"
#include "cmdline.hpp"
#include "treelist.hpp"
#include "qview.hpp"
#include "infolist.hpp"
#include "help.hpp"
#include "filefilter.hpp"
#include "findfile.hpp"
#include "savescr.hpp"
#include "manager.hpp"
#include "syslog.hpp"
#include "pathmix.hpp"
#include "dirmix.hpp"
#include "interf.hpp"
#include "language.hpp"
#include "config.hpp"
#include "keybar.hpp"
#include "menubar.hpp"
#include "strmix.hpp"
#include "colormix.hpp"
#include "diskmenu.hpp"

FilePanels::FilePanels(private_tag):
	m_ActivePanelIndex(panel_left)
{
}

filepanels_ptr FilePanels::create(bool CreateRealPanels, int DirCount)
{
	const auto FilePanelsPtr = std::make_shared<FilePanels>(private_tag());

	FilePanelsPtr->m_windowKeyBar = std::make_unique<KeyBar>(FilePanelsPtr);
	FilePanelsPtr->SetMacroMode(MACROAREA_SHELL);
	FilePanelsPtr->m_KeyBarVisible = Global->Opt->ShowKeyBar;

	if (CreateRealPanels)
	{
		FilePanelsPtr->m_Panels[panel_left].m_Panel = FilePanelsPtr->CreatePanel(panel_type::value_type(Global->Opt->LeftPanel.m_Type.Get()));
		FilePanelsPtr->m_Panels[panel_right].m_Panel = FilePanelsPtr->CreatePanel(panel_type::value_type(Global->Opt->RightPanel.m_Type.Get()));
		FilePanelsPtr->Init(DirCount);
	}
	else
	{
		FilePanelsPtr->m_Panels[panel_left].m_Panel.reset(new dummy_panel(FilePanelsPtr));
		FilePanelsPtr->m_Panels[panel_right].m_Panel.reset(new dummy_panel(FilePanelsPtr));
	}
	return FilePanelsPtr;
}

static void PrepareOptFolder(string &strSrc, int IsLocalPath_FarPath)
{
	if (strSrc.empty())
	{
		strSrc = Global->g_strFarPath;
		DeleteEndSlash(strSrc);
	}
	else
	{
		strSrc = os::env::expand_strings(strSrc);
	}

	if (strSrc == L"/")
	{
		strSrc = Global->g_strFarPath;

		if (IsLocalPath_FarPath)
		{
			strSrc.resize(2);
			strSrc += L"\\";
		}
	}
	else
	{
		CheckShortcutFolder(strSrc, true, true);
	}

	//ConvertNameToFull(strSrc,strSrc);
}

void FilePanels::Init(int DirCount)
{
	CmdLine = std::make_unique<CommandLine>(shared_from_this());
	TopMenuBar = std::make_unique<MenuBar>(shared_from_this());

	m_ActivePanelIndex = Global->Opt->LeftFocus? panel_left : panel_right;

	const auto Left = std::pair<panel_ptr, Options::PanelOptions&>(LeftPanel(), Global->Opt->LeftPanel);
	const auto Right = std::pair<panel_ptr, Options::PanelOptions&>(RightPanel(), Global->Opt->RightPanel);

	SetPanelPositions(FileList::IsModeFullScreen(Left.second.ViewMode),
	                  FileList::IsModeFullScreen(Right.second.ViewMode));

	const auto InitPanel = [](const std::pair<panel_ptr, const Options::PanelOptions&>& Params)
	{
		Params.first->SetViewMode(Params.second.ViewMode);

		if (Params.second.SortMode < panel_sort::COUNT)
			Params.first->SetSortMode(panel_sort::value_type(Params.second.SortMode.Get()));

		Params.first->SetNumericSort(Params.second.NumericSort);
		Params.first->SetCaseSensitiveSort(Params.second.CaseSensitiveSort);
		Params.first->SetSortOrder(Params.second.ReverseSortOrder);
		Params.first->SetSortGroups(Params.second.SortGroups);
		Params.first->SetShowShortNamesMode(Params.second.ShowShortNames);
		Params.first->SetSelectedFirstMode(Params.second.SelectedFirst);
		Params.first->SetDirectoriesFirst(Params.second.DirectoriesFirst);
	};

	InitPanel(Left);
	InitPanel(Right);

	SetCanLoseFocus(TRUE);

	SetActivePanelInternal(ActivePanel());

	// пытаемся избавится от зависания при запуске
	int IsLocalPath_FarPath = ParsePath(Global->g_strFarPath)==PATH_DRIVELETTER;

	const auto SetFolder = [&](const std::pair<panel_ptr, Options::PanelOptions&>& Params)
	{
		auto Folder = Params.second.Folder.Get();
		PrepareOptFolder(Folder, IsLocalPath_FarPath);
		Params.second.Folder = Folder;
	};
	SetFolder(Left);
	SetFolder(Right);

	const auto InitCurDir_checked = [&](const std::pair<panel_ptr, const Options::PanelOptions&>& Params)
	{
		Params.first->InitCurDir(os::fs::exists(Params.second.Folder.Get())? Params.second.Folder.Get() : Global->g_strFarPath);
	};

	if (Global->Opt->AutoSaveSetup || !DirCount)
	{
		InitCurDir_checked(Left);
		InitCurDir_checked(Right);
	}

	if (!Global->Opt->AutoSaveSetup)
	{
		if (DirCount >= 1)
		{
			InitCurDir_checked(IsRightActive()? Right : Left);

			if (DirCount == 2)
			{
				InitCurDir_checked(IsLeftActive()? Right : Left);
			}
		}

		const string& PassiveFolder = m_ActivePanelIndex == panel_right? Left.second.Folder : Right.second.Folder;

		if (DirCount < 2 && !PassiveFolder.empty() && os::fs::exists(PassiveFolder))
		{
			PassivePanel()->InitCurDir(PassiveFolder);
		}
	}

#if 1

	//! Вначале "показываем" пассивную панель
	if (m_ActivePanelIndex == panel_right)
	{
		if (Left.second.Visible)
		{
			Left.first->Show();
		}

		if (Right.second.Visible)
		{
			Right.first->Show();
		}
	}
	else
	{
		if (Right.second.Visible)
		{
			Right.first->Show();
		}

		if (Left.second.Visible)
		{
			Left.first->Show();
		}
	}

#endif

	// при погашенных панелях не забыть бы выставить корректно каталог в CmdLine
	if (!Right.second.Visible && !Left.second.Visible)
	{
		CmdLine->SetCurDir(m_ActivePanelIndex == panel_right? Right.second.Folder : Left.second.Folder);
	}

	SetScreenPosition();
}

FilePanels::~FilePanels()
{
}

void FilePanels::SetPanelPositions(bool LeftFullScreen, bool RightFullScreen)
{
	if (Global->Opt->WidthDecrement < -(ScrX/2-10))
		Global->Opt->WidthDecrement=-(ScrX/2-10);

	if (Global->Opt->WidthDecrement > (ScrX/2-10))
		Global->Opt->WidthDecrement=(ScrX/2-10);

	Global->Opt->LeftHeightDecrement=std::max(0ll, std::min(Global->Opt->LeftHeightDecrement.Get(), ScrY-7ll));
	Global->Opt->RightHeightDecrement=std::max(0ll, std::min(Global->Opt->RightHeightDecrement.Get(), ScrY-7ll));

	const auto Left = LeftPanel();
	const auto Right = RightPanel();

	if (LeftFullScreen)
	{
		Left->SetPosition(0,Global->Opt->ShowMenuBar?1:0,ScrX,ScrY-1-(Global->Opt->ShowKeyBar)-Global->Opt->LeftHeightDecrement);
		Left->SetFullScreen();
	}
	else
	{
		Left->SetPosition(0,Global->Opt->ShowMenuBar?1:0,ScrX/2-Global->Opt->WidthDecrement,ScrY-1-(Global->Opt->ShowKeyBar)-Global->Opt->LeftHeightDecrement);
	}

	if (RightFullScreen)
	{
		Right->SetPosition(0,Global->Opt->ShowMenuBar?1:0,ScrX,ScrY-1-(Global->Opt->ShowKeyBar)-Global->Opt->RightHeightDecrement);
		Right->SetFullScreen();
	}
	else
	{
		Right->SetPosition(ScrX/2+1-Global->Opt->WidthDecrement,Global->Opt->ShowMenuBar?1:0,ScrX,ScrY-1-(Global->Opt->ShowKeyBar)-Global->Opt->RightHeightDecrement);
	}
}

void FilePanels::SetScreenPosition()
{
	_OT(SysLog(L"[%p] FilePanels::SetScreenPosition() {%d, %d - %d, %d}", this,m_X1,m_Y1,m_X2,m_Y2));
	CmdLine->SetPosition(0,ScrY-(Global->Opt->ShowKeyBar),ScrX-1,ScrY-(Global->Opt->ShowKeyBar));
	TopMenuBar->SetPosition(0, 0, ScrX, 0);
	m_windowKeyBar->SetPosition(0, ScrY, ScrX, ScrY);
	SetPanelPositions(LeftPanel()->IsFullScreen(), RightPanel()->IsFullScreen());
	SetPosition(0,0,ScrX,ScrY);
}

void FilePanels::RedrawKeyBar()
{
	ActivePanel()->UpdateKeyBar();
	m_windowKeyBar->Redraw();
}


panel_ptr FilePanels::CreatePanel(panel_type Type)
{
	switch (Type.value())
	{
	default:
	case panel_type::FILE_PANEL:
		return FileList::create(shared_from_this());
	case panel_type::TREE_PANEL:
		return TreeList::create(shared_from_this());
	case panel_type::QVIEW_PANEL:
		return QuickView::create(shared_from_this());
	case panel_type::INFO_PANEL:
		return InfoList::create(shared_from_this());
	}
}

int FilePanels::SetAnhoterPanelFocus()
{
	bool Result = false;

	if (IsLeftActive())
	{
		const auto Right = RightPanel();
		if (Right->IsVisible())
		{
			SetActivePanel(Right);
			Result = true;
		}
	}
	else
	{
		const auto Left = LeftPanel();
		if (Left->IsVisible())
		{
			SetActivePanel(Left);
			Result = true;
		}
	}

	return Result;
}


int FilePanels::SwapPanels()
{
	int Ret=FALSE; // это значит ни одна из панелей не видна

	if (LeftPanel()->IsVisible() || RightPanel()->IsVisible())
	{
		int XL1,YL1,XL2,YL2;
		int XR1,YR1,XR2,YR2;
		LeftPanel()->GetPosition(XL1,YL1,XL2,YL2);
		RightPanel()->GetPosition(XR1,YR1,XR2,YR2);

		if (!LeftPanel()->IsFullScreen() || !RightPanel()->IsFullScreen())
		{
			Global->Opt->WidthDecrement=-Global->Opt->WidthDecrement;

			Global->Opt->LeftHeightDecrement^=Global->Opt->RightHeightDecrement;
			Global->Opt->RightHeightDecrement=Global->Opt->LeftHeightDecrement^Global->Opt->RightHeightDecrement;
			Global->Opt->LeftHeightDecrement^=Global->Opt->RightHeightDecrement;

		}

		using std::swap;
		swap(m_Panels[panel_left].m_Panel, m_Panels[panel_right].m_Panel);
		swap(m_Panels[panel_left].m_LastFilePanel, m_Panels[panel_right].m_LastFilePanel);
		swap(m_Panels[panel_left].m_LastType, m_Panels[panel_right].m_LastType);
		swap(m_Panels[panel_left].m_StateBeforeHide, m_Panels[panel_right].m_StateBeforeHide);
		FileFilter::SwapFilter();
		m_ActivePanelIndex = IsLeftActive()? panel_right : panel_left;
		Ret=TRUE;
	}
	SetScreenPosition();
	Global->WindowManager->RefreshWindow();
	return Ret;
}

__int64 FilePanels::VMProcess(int OpCode,void *vParam,__int64 iParam)
{
	if (OpCode == MCODE_F_KEYBAR_SHOW)
	{
		int PrevMode=Global->Opt->ShowKeyBar?2:1;
		switch (iParam)
		{
			case 0:
				break;
			case 1:
				Global->Opt->ShowKeyBar = true;
				m_windowKeyBar->Show();
				m_KeyBarVisible = Global->Opt->ShowKeyBar;
				SetScreenPosition();
				Global->WindowManager->RefreshWindow();
				break;
			case 2:
				Global->Opt->ShowKeyBar = false;
				m_windowKeyBar->Hide();
				m_KeyBarVisible = Global->Opt->ShowKeyBar;
				SetScreenPosition();
				Global->WindowManager->RefreshWindow();
				break;
			case 3:
				ProcessKey(Manager::Key(KEY_CTRLB));
				break;
			default:
				PrevMode=0;
				break;
		}
		return PrevMode;
	}
	return ActivePanel()->VMProcess(OpCode, vParam, iParam);
}

int FilePanels::ProcessKey(const Manager::Key& Key)
{
	const auto LocalKey = Key();
	if (!LocalKey)
		return TRUE;

	if ((LocalKey==KEY_CTRLLEFT || LocalKey==KEY_CTRLRIGHT || LocalKey==KEY_CTRLNUMPAD4 || LocalKey==KEY_CTRLNUMPAD6
		|| LocalKey==KEY_RCTRLLEFT || LocalKey==KEY_RCTRLRIGHT || LocalKey==KEY_RCTRLNUMPAD4 || LocalKey==KEY_RCTRLNUMPAD6
	        /* || LocalKey==KEY_CTRLUP   || LocalKey==KEY_CTRLDOWN || LocalKey==KEY_CTRLNUMPAD8 || LocalKey==KEY_CTRLNUMPAD2 */) &&
	        (CmdLine->GetLength()>0 ||
			(!LeftPanel()->IsVisible() && !RightPanel()->IsVisible())))
	{
		CmdLine->ProcessKey(Key);
		return TRUE;
	}

	switch (LocalKey)
	{
		case KEY_F1:
		{
			if (!ActivePanel()->ProcessKey(Manager::Key(KEY_F1)))
			{
				Help::create(L"Contents");
			}

			return TRUE;
		}
		case KEY_TAB:
		{
			SetAnhoterPanelFocus();
			break;
		}
		case KEY_CTRLF1:
		case KEY_RCTRLF1:
		{
			if (LeftPanel()->IsVisible())
			{
				LeftPanel()->Hide();

				if (RightPanel()->IsVisible())
					SetActivePanel(RightPanel());
			}
			else
			{
				if (!RightPanel()->IsVisible())
					SetActivePanel(LeftPanel());

				LeftPanel()->Show();
			}

			Redraw();
			break;
		}
		case KEY_CTRLF2:
		case KEY_RCTRLF2:
		{
			if (RightPanel()->IsVisible())
			{
				RightPanel()->Hide();

				if (LeftPanel()->IsVisible())
					SetActivePanel(LeftPanel());
			}
			else
			{
				if (!LeftPanel()->IsVisible())
					SetActivePanel(RightPanel());

				RightPanel()->Show();
			}

			Redraw();
			break;
		}
		case KEY_CTRLB:
		case KEY_RCTRLB:
		{
			Global->Opt->ShowKeyBar=!Global->Opt->ShowKeyBar;
			m_KeyBarVisible = Global->Opt->ShowKeyBar;

			if (!m_KeyBarVisible)
				m_windowKeyBar->Hide();

			SetScreenPosition();
			Global->WindowManager->RefreshWindow();
			break;
		}
		case KEY_CTRLT: case KEY_RCTRLT:
			if (Global->Opt->Tree.TurnOffCompletely)
				break;
		case KEY_CTRLL: case KEY_RCTRLL:
		case KEY_CTRLQ: case KEY_RCTRLQ:
		{
			if (ActivePanel()->IsVisible())
			{
				auto AnotherPanel = PassivePanel();
				panel_type NewType = panel_type::FILE_PANEL;

				if (LocalKey==KEY_CTRLL || LocalKey==KEY_RCTRLL)
					NewType = panel_type::INFO_PANEL;
				else if (LocalKey==KEY_CTRLQ || LocalKey==KEY_RCTRLQ)
					NewType = panel_type::QVIEW_PANEL;
				else
					NewType = panel_type::TREE_PANEL;

				if (ActivePanel()->GetType() == NewType)
					AnotherPanel = ActivePanel();

				if (!AnotherPanel->ProcessPluginEvent(FE_CLOSE,nullptr))
				{
					if (AnotherPanel->GetType()==NewType)
						/* $ 19.09.2000 IS
						  Повторное нажатие на ctrl-l|q|t всегда включает файловую панель
						*/
						AnotherPanel = ChangePanel(AnotherPanel, panel_type::FILE_PANEL, FALSE, FALSE);
					else
						AnotherPanel=ChangePanel(AnotherPanel,NewType,FALSE,FALSE);

					/* $ 07.09.2001 VVM
					  ! При возврате из CTRL+Q, CTRL+L восстановим каталог, если активная панель - дерево. */
					if (ActivePanel()->GetType() == panel_type::TREE_PANEL)
					{
						string strCurDir(ActivePanel()->GetCurDir());
						AnotherPanel->SetCurDir(strCurDir, true);
						AnotherPanel->Update(0);
					}
					else
						AnotherPanel->Update(UPDATE_KEEP_SELECTION);

					AnotherPanel->Show();
				}
			}

			break;
		}
		case KEY_CTRLO:
		case KEY_RCTRLO:
		{
			{
				int LeftVisible = LeftPanel()->IsVisible();
				int RightVisible = RightPanel()->IsVisible();
				int HideState=!LeftVisible && !RightVisible;

				if (!HideState)
				{
					m_Panels[panel_left].m_StateBeforeHide = LeftVisible;
					m_Panels[panel_right].m_StateBeforeHide = RightVisible;
					LeftPanel()->Hide();
					RightPanel()->Hide();
					Global->WindowManager->RefreshWindow();
				}
				else
				{
					if (!m_Panels[panel_left].m_StateBeforeHide && !m_Panels[panel_right].m_StateBeforeHide)
						m_Panels[panel_left].m_StateBeforeHide = m_Panels[panel_right].m_StateBeforeHide = TRUE;

					if (m_Panels[panel_left].m_StateBeforeHide)
						LeftPanel()->Show();

					if (m_Panels[panel_right].m_StateBeforeHide)
						RightPanel()->Show();

					if (!ActivePanel()->IsVisible())
					{
						SetActivePanel(IsRightActive()? LeftPanel() : RightPanel());
					}
				}
			}
			break;
		}
		case KEY_CTRLP:
		case KEY_RCTRLP:
		{
			if (ActivePanel()->IsVisible())
			{
				const auto AnotherPanel = PassivePanel();

				if (AnotherPanel->IsVisible())
					AnotherPanel->Hide();
				else
					AnotherPanel->Show();

				CmdLine->Redraw();
			}

			Global->WindowManager->RefreshWindow();
			break;
		}
		case KEY_CTRLI:
		case KEY_RCTRLI:
		{
			ActivePanel()->EditFilter();
			return TRUE;
		}
		case KEY_CTRLU:
		case KEY_RCTRLU:
		{
			if (!LeftPanel()->IsVisible() && !RightPanel()->IsVisible())
				CmdLine->ProcessKey(Key);
			else
				SwapPanels();

			break;
		}
		/* $ 08.04.2002 IS
		   При смене диска установим принудительно текущий каталог на активной
		   панели, т.к. система не знает ничего о том, что у Фара две панели, и
		   текущим для системы после смены диска может быть каталог и на пассивной
		   панели
		*/
		case KEY_ALTF1:
		case KEY_RALTF1:
		{
			ChangeDisk(LeftPanel());

			if (!IsLeftActive())
				ActivePanel()->SetCurPath();

			break;
		}
		case KEY_ALTF2:
		case KEY_RALTF2:
		{
			ChangeDisk(RightPanel());

			if (!IsRightActive())
				ActivePanel()->SetCurPath();

			break;
		}
		case KEY_ALTF7:
		case KEY_RALTF7:
		{
			FindFiles();
			break;
		}
		case KEY_CTRLUP:  case KEY_CTRLNUMPAD8:
		case KEY_RCTRLUP: case KEY_RCTRLNUMPAD8:
		{
			bool Set=false;
			if (Global->Opt->LeftHeightDecrement<ScrY-7)
			{
				++Global->Opt->LeftHeightDecrement;
				Set=true;
			}
			if (Global->Opt->RightHeightDecrement<ScrY-7)
			{
				++Global->Opt->RightHeightDecrement;
				Set=true;
			}
			if(Set)
			{
				SetScreenPosition();
				Global->WindowManager->RefreshWindow();
			}

			break;
		}
		case KEY_CTRLDOWN:  case KEY_CTRLNUMPAD2:
		case KEY_RCTRLDOWN: case KEY_RCTRLNUMPAD2:
		{
			bool Set=false;
			if (Global->Opt->LeftHeightDecrement>0)
			{
				--Global->Opt->LeftHeightDecrement;
				Set=true;
			}
			if (Global->Opt->RightHeightDecrement>0)
			{
				--Global->Opt->RightHeightDecrement;
				Set=true;
			}
			if(Set)
			{
				SetScreenPosition();
				Global->WindowManager->RefreshWindow();
			}

			break;
		}

		case KEY_CTRLSHIFTUP:  case KEY_CTRLSHIFTNUMPAD8:
		case KEY_RCTRLSHIFTUP: case KEY_RCTRLSHIFTNUMPAD8:
		{
			IntOption& HeightDecrement = IsLeftActive()? Global->Opt->LeftHeightDecrement : Global->Opt->RightHeightDecrement;
			if (HeightDecrement<ScrY-7)
			{
				++HeightDecrement;
				SetScreenPosition();
				Global->WindowManager->RefreshWindow();
			}
			break;
		}

		case KEY_CTRLSHIFTDOWN:  case KEY_CTRLSHIFTNUMPAD2:
		case KEY_RCTRLSHIFTDOWN: case KEY_RCTRLSHIFTNUMPAD2:
		{
			IntOption& HeightDecrement = IsLeftActive()? Global->Opt->LeftHeightDecrement : Global->Opt->RightHeightDecrement;
			if (HeightDecrement>0)
			{
				--HeightDecrement;
				SetScreenPosition();
				Global->WindowManager->RefreshWindow();
			}
			break;
		}

		case KEY_CTRLLEFT:  case KEY_CTRLNUMPAD4:
		case KEY_RCTRLLEFT: case KEY_RCTRLNUMPAD4:
		{
			if (Global->Opt->WidthDecrement<ScrX/2-10)
			{
				++Global->Opt->WidthDecrement;
				SetScreenPosition();
				Global->WindowManager->RefreshWindow();
			}

			break;
		}
		case KEY_CTRLRIGHT:  case KEY_CTRLNUMPAD6:
		case KEY_RCTRLRIGHT: case KEY_RCTRLNUMPAD6:
		{
			if (Global->Opt->WidthDecrement>-(ScrX/2-10))
			{
				--Global->Opt->WidthDecrement;
				SetScreenPosition();
				Global->WindowManager->RefreshWindow();
			}

			break;
		}
		case KEY_CTRLCLEAR:
		case KEY_RCTRLCLEAR:
		{
			if (Global->Opt->WidthDecrement)
			{
				Global->Opt->WidthDecrement=0;
				SetScreenPosition();
				Global->WindowManager->RefreshWindow();
			}

			break;
		}
		case KEY_CTRLALTCLEAR:
		case KEY_RCTRLRALTCLEAR:
		case KEY_CTRLRALTCLEAR:
		case KEY_RCTRLALTCLEAR:
		{
			bool Set=false;
			if (Global->Opt->LeftHeightDecrement)
			{
				Global->Opt->LeftHeightDecrement=0;
				Set=true;
			}
			if (Global->Opt->RightHeightDecrement)
			{
				Global->Opt->RightHeightDecrement=0;
				Set=true;
			}
			if(Set)
			{
				SetScreenPosition();
				Global->WindowManager->RefreshWindow();
			}

			break;
		}
		case KEY_F9:
		{
			Global->Opt->ShellOptions(false,nullptr);
			return TRUE;
		}
		case KEY_SHIFTF10:
		{
			Global->Opt->ShellOptions(true,nullptr);
			return TRUE;
		}
		case KEY_F11:
		{
			// We will never get here in normal flow since F11 is processed by manager itself,
			// but auto-completion menu can forward keys back to the owner
			return Global->WindowManager->ProcessKey(Key);
		}
		default:
		{
			if (LocalKey >= KEY_CTRL0 && LocalKey <= KEY_CTRL9)
				ChangePanelViewMode(ActivePanel(), LocalKey - KEY_CTRL0, TRUE);
			if (!ActivePanel()->ProcessKey(Key))
				CmdLine->ProcessKey(Key);

			break;
		}
	}

	return TRUE;
}

int FilePanels::ChangePanelViewMode(panel_ptr Current, int Mode, BOOL RefreshWindow)
{
	if (Current && Mode >= VIEW_0 && Mode < (int)Global->Opt->ViewSettings.size())
	{
		Current->SetViewMode(Mode);
		Current = ChangePanelToFilled(Current, panel_type::FILE_PANEL);
		Current->SetViewMode(Mode);
		// ВНИМАНИЕ! Костыль! Но Работает!
		SetScreenPosition();

		if (RefreshWindow)
			Global->WindowManager->RefreshWindow();

		return TRUE;
	}

	return FALSE;
}

void FilePanels::SetActivePanel(Panel* ToBeActive)
{
	if (ActivePanel().get() != ToBeActive)
		SetActivePanelInternal(ToBeActive->shared_from_this());
}

void FilePanels::SetActivePanelInternal(panel_ptr ToBeActive)
{
	const auto ToBePassive = ActivePanel();
	m_ActivePanelIndex = IsLeft(ToBeActive)? panel_left : panel_right;

	Global->WindowManager->UpdateMacroArea();

	ToBePassive->OnFocusChange(false);
	ToBeActive->OnFocusChange(true);

	FarChDir(ToBeActive->GetCurDir());
	RedrawKeyBar();

	ToBeActive->SetTitle();
}

panel_ptr FilePanels::ChangePanelToFilled(panel_ptr Current, panel_type NewType)
{
	if (Current->GetType()!=NewType && !Current->ProcessPluginEvent(FE_CLOSE,nullptr))
	{
		Current->Hide();
		Current=ChangePanel(Current,NewType,FALSE,FALSE);
		Current->Update(0);
		Current->Show();
	}

	return Current;
}

panel_ptr FilePanels::GetAnotherPanel(const Panel* Current)
{
	if (Current==LeftPanel().get())
		return RightPanel();
	else
		return LeftPanel();
}


panel_ptr FilePanels::ChangePanel(panel_ptr Current, panel_type NewType, int CreateNew, int Force)
{
	assert(Current == m_Panels[panel_left].m_Panel || Current == m_Panels[panel_right].m_Panel);

	std::unique_ptr<SaveScreen> TemporarySaveScr;
	// OldType не инициализировался...
	const auto OldType = Current->GetType();
	int X1, Y1, X2, Y2;
	const auto OldPanelMode = Current->GetMode();

	if (!Force && NewType == OldType && OldPanelMode == panel_mode::NORMAL_PANEL)
		return Current;

	bool UsedLastPanel = false;

	int OldViewMode=Current->GetPrevViewMode();
	bool OldFullScreen=Current->IsFullScreen();
	const auto OldSortMode=Current->GetPrevSortMode();
	bool OldSortOrder=Current->GetPrevSortOrder();
	bool OldNumericSort=Current->GetPrevNumericSort();
	bool OldCaseSensitiveSort=Current->GetPrevCaseSensitiveSort();
	bool OldSortGroups=Current->GetSortGroups();
	bool OldShowShortNames=Current->GetShowShortNamesMode();
	bool OldFocus = Current->IsFocused();
	bool OldSelectedFirst=Current->GetSelectedFirstMode();
	bool OldDirectoriesFirst=Current->GetPrevDirectoriesFirst();
	const auto LeftPosition = (Current == LeftPanel());

	auto& LastFilePanel = m_Panels[LeftPosition? panel_left : panel_right].m_LastFilePanel;
	Current->GetPosition(X1,Y1,X2,Y2);
	int ChangePosition = ((OldType == panel_type::FILE_PANEL && NewType != panel_type::FILE_PANEL &&
	                    OldFullScreen) || (NewType==panel_type::FILE_PANEL &&
	                                    ((OldFullScreen && !FileList::IsModeFullScreen(OldViewMode)) ||
	                                     (!OldFullScreen && FileList::IsModeFullScreen(OldViewMode)))));

	if (!ChangePosition)
	{
		TemporarySaveScr = std::move(Current->SaveScr);
	}

	if (OldType == panel_type::FILE_PANEL && NewType != panel_type::FILE_PANEL)
	{
		Current->SaveScr.reset();
		Current->Hide();

		LastFilePanel = std::move(Current);

		if (LastFilePanel->SaveScr)
		{
			LastFilePanel->SaveScr->Discard();
			LastFilePanel->SaveScr.reset();
		}
	}
	else
	{
		Current->Hide();
		Current.reset();

		if (OldType == panel_type::FILE_PANEL && NewType == panel_type::FILE_PANEL)
		{
			LastFilePanel.reset();
		}
	}

	auto& NewPanel = m_Panels[LeftPosition? panel_left : panel_right].m_Panel;

	m_Panels[LeftPosition? panel_left : panel_right].m_LastType = OldType;

	if (!CreateNew && NewType == panel_type::FILE_PANEL && LastFilePanel)
	{
		int LastX1,LastY1,LastX2,LastY2;
		LastFilePanel->GetPosition(LastX1,LastY1,LastX2,LastY2);

		if (LastFilePanel->IsFullScreen())
			LastFilePanel->SetPosition(LastX1,Y1,LastX2,Y2);
		else
			LastFilePanel->SetPosition(X1,Y1,X2,Y2);

		NewPanel = std::move(LastFilePanel);

		if (!ChangePosition)
		{
			if ((NewPanel->IsFullScreen() && !OldFullScreen) ||
			        (!NewPanel->IsFullScreen() && OldFullScreen))
			{
				const auto AnotherPanel = GetAnotherPanel(Current);

				if (TemporarySaveScr && AnotherPanel->IsVisible() &&
					AnotherPanel->GetType() == panel_type::FILE_PANEL && AnotherPanel->IsFullScreen())
					TemporarySaveScr->Discard();
			}
			else
				NewPanel->SaveScr = std::move(TemporarySaveScr);
		}

		if (!OldFocus && NewPanel->IsFocused())
			SetActivePanel(PassivePanel());

		UsedLastPanel = true;
	}
	else
	{
		NewPanel=CreatePanel(NewType);
	}

	if (!UsedLastPanel)
	{
		if (ChangePosition)
		{
			if (LeftPosition)
			{
				NewPanel->SetPosition(0,Y1,ScrX/2-Global->Opt->WidthDecrement,Y2);
				RightPanel()->Redraw();
			}
			else
			{
				NewPanel->SetPosition(ScrX/2+1-Global->Opt->WidthDecrement,Y1,ScrX,Y2);
				LeftPanel()->Redraw();
			}
		}
		else
		{
			NewPanel->SaveScr = std::move(SaveScr);
			NewPanel->SetPosition(X1,Y1,X2,Y2);
		}

		NewPanel->SetSortMode(OldSortMode);
		NewPanel->SetSortOrder(OldSortOrder);
		NewPanel->SetNumericSort(OldNumericSort);
		NewPanel->SetCaseSensitiveSort(OldCaseSensitiveSort);
		NewPanel->SetSortGroups(OldSortGroups);
		NewPanel->SetShowShortNamesMode(OldShowShortNames);
		NewPanel->SetPrevViewMode(OldViewMode);
		NewPanel->SetViewMode(OldViewMode);
		NewPanel->SetSelectedFirstMode(OldSelectedFirst);
		NewPanel->SetDirectoriesFirst(OldDirectoriesFirst);
	}

	return NewPanel;
}

int  FilePanels::GetTypeAndName(string &strType, string &strName)
{
	strType = MSG(MScreensPanels);
	string strFullName, strShortName;

	switch (ActivePanel()->GetType().value())
	{
		case panel_type::TREE_PANEL:
		case panel_type::QVIEW_PANEL:
		case panel_type::FILE_PANEL:
		case panel_type::INFO_PANEL:
			ActivePanel()->GetCurName(strFullName, strShortName);
			ConvertNameToFull(strFullName, strFullName);
			break;
	}

	strName = strFullName;
	return windowtype_panels;
}

void FilePanels::DisplayObject()
{
//  if ( !Focus )
//      return;
	_OT(SysLog(L"[%p] FilePanels::Redraw() {%d, %d - %d, %d}", this,m_X1,m_Y1,m_X2,m_Y2));
	Global->WindowManager->ShowBackground();

	if (Global->Opt->ShowMenuBar)
		TopMenuBar->Show();

	CmdLine->Show();

	if (Global->Opt->ShowKeyBar)
		m_windowKeyBar->Show();
	else if (m_windowKeyBar->IsVisible())
		m_windowKeyBar->Hide();

	m_KeyBarVisible=Global->Opt->ShowKeyBar;
#if 1

	if (LeftPanel()->IsVisible())
		LeftPanel()->Show();

	if (RightPanel()->IsVisible())
		RightPanel()->Show();

#else
	Panel *PassivePanel=nullptr;
	int PassiveIsLeftFlag=TRUE;

	if (Global->Opt->LeftPanel.Focus)
	{
		ActivePanel=LeftPanel;
		PassivePanel=RightPanel;
		PassiveIsLeftFlag=FALSE;
	}
	else
	{
		ActivePanel=RightPanel;
		PassivePanel=LeftPanel;
		PassiveIsLeftFlag=TRUE;
	}

	//! Вначале "показываем" пассивную панель
	if (PassiveIsLeftFlag)
	{
		if (Global->Opt->LeftPanel.Visible)
		{
			LeftPanel->Show();
		}

		if (Global->Opt->RightPanel.Visible)
		{
			RightPanel->Show();
		}
	}
	else
	{
		if (Global->Opt->RightPanel.Visible)
		{
			RightPanel->Show();
		}

		if (Global->Opt->LeftPanel.Visible)
		{
			LeftPanel->Show();
		}
	}

#endif
}

int  FilePanels::ProcessMouse(const MOUSE_EVENT_RECORD *MouseEvent)
{
	if (!MouseEvent->dwMousePosition.Y)
	{
		if ((MouseEvent->dwButtonState & 3) && !MouseEvent->dwEventFlags)
		{
			if (!MouseEvent->dwMousePosition.X)
				ProcessKey(Manager::Key(KEY_CTRLO));
			else
				Global->Opt->ShellOptions(false, MouseEvent);

			return TRUE;
		}
	}

	if (!ActivePanel()->ProcessMouse(MouseEvent))
		if (!PassivePanel()->ProcessMouse(MouseEvent))
			if (!m_windowKeyBar->ProcessMouse(MouseEvent))
				CmdLine->ProcessMouse(MouseEvent);

	return TRUE;
}

void FilePanels::ShowConsoleTitle()
{
	ActivePanel()->SetTitle();
}

void FilePanels::ResizeConsole()
{
	window::ResizeConsole();
	CmdLine->ResizeConsole();
	m_windowKeyBar->ResizeConsole();
	TopMenuBar->ResizeConsole();
	SetScreenPosition();
	_OT(SysLog(L"[%p] FilePanels::ResizeConsole() {%d, %d - %d, %d}", this,m_X1,m_Y1,m_X2,m_Y2));
}

bool FilePanels::CanFastHide() const
{
	return (Global->Opt->AllCtrlAltShiftRule & CASR_PANEL) != 0;
}

void FilePanels::Refresh()
{
	window::Refresh();
	PassivePanel()->UpdateIfChanged(false);
	ActivePanel()->UpdateIfChanged(false);
	ActivePanel()->SetCurPath();
}

void FilePanels::GoToFile(const string& FileName)
{
	if (FindSlash(FileName) != string::npos)
	{
		string ADir,PDir;

		const auto PassiveMode = PassivePanel()->GetMode();

		if (PassiveMode == panel_mode::NORMAL_PANEL)
		{
			PDir = PassivePanel()->GetCurDir();
			AddEndSlash(PDir);
		}

		const auto ActiveMode = ActivePanel()->GetMode();

		if (ActiveMode == panel_mode::NORMAL_PANEL)
		{
			ADir = ActivePanel()->GetCurDir();
			AddEndSlash(ADir);
		}

		string strNameFile = PointToName(FileName);
		string strNameDir = FileName;
		CutToSlash(strNameDir);
		/* $ 10.04.2001 IS
		     Не делаем SetCurDir, если нужный путь уже есть на открытых
		     панелях, тем самым добиваемся того, что выделение с элементов
		     панелей не сбрасывается.
		*/
		BOOL AExist = (ActiveMode == panel_mode::NORMAL_PANEL) && !StrCmpI(ADir, strNameDir);
		BOOL PExist = (PassiveMode == panel_mode::NORMAL_PANEL) && !StrCmpI(PDir, strNameDir);

		// если нужный путь есть на пассивной панели
		if (!AExist && PExist)
			ProcessKey(Manager::Key(KEY_TAB));

		if (!AExist && !PExist)
			ActivePanel()->SetCurDir(strNameDir, true);

		ActivePanel()->GoToFile(strNameFile);
		// всегда обновим заголовок панели, чтобы дать обратную связь, что
		// Ctrl-F10 обработан
		ActivePanel()->SetTitle();
	}
}


FARMACROAREA FilePanels::GetMacroArea() const
{
	switch (ActivePanel()->GetType().value())
	{
	case panel_type::FILE_PANEL:
		return MACROAREA_SHELL;
	case panel_type::TREE_PANEL:
		return MACROAREA_TREEPANEL;
	case panel_type::QVIEW_PANEL:
		return MACROAREA_QVIEWPANEL;
	case panel_type::INFO_PANEL:
		return MACROAREA_INFOPANEL;
	}
	return MACROAREA_INVALID;
}

Viewer* FilePanels::GetViewer(void)
{
	auto result=ActivePanel()->GetViewer();
	if (!result) result=PassivePanel()->GetViewer();
	return result;
}

Viewer* FilePanels::GetById(int ID)
{
	auto result=LeftPanel()->GetById(ID);
	if (!result) result=RightPanel()->GetById(ID);
	return result;
}

CommandLine* FilePanels::GetCmdLine(void)
{
	return CmdLine.get();
}

void FilePanels::Show()
{
	window::Show();

	if (Global->Opt->ShowMenuBar)
	{
		TopMenuBar->Show();
	}
}