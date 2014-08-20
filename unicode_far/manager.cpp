/*
manager.cpp

������������ ����� ����������� file panels, viewers, editors, dialogs
*/
/*
Copyright � 1996 Eugene Roshal
Copyright � 2000 Far Group
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

#include "manager.hpp"
#include "keys.hpp"
#include "frame.hpp"
#include "vmenu2.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "savescr.hpp"
#include "cmdline.hpp"
#include "ctrlobj.hpp"
#include "syslog.hpp"
#include "interf.hpp"
#include "keyboard.hpp"
#include "grabber.hpp"
#include "message.hpp"
#include "config.hpp"
#include "plist.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "exitcode.hpp"
#include "scrbuf.hpp"
#include "console.hpp"
#include "configdb.hpp"
#include "DlgGuid.hpp"
#include "plugins.hpp"
#include "language.hpp"
#include "desktop.hpp"

long Manager::CurrentWindowType=-1;

// fixed indexes
enum
{
	DesktopIndex,
	FilePanelsIndex,
};

class Manager::MessageAbstract
{
public:
	virtual ~MessageAbstract() {}
	virtual bool Process(void) = 0;
};

class MessageCallback: public Manager::MessageAbstract
{
public:
	MessageCallback(const std::function<void(void)>& Callback): m_Callback(Callback) {}
	virtual bool Process(void) override { m_Callback(); return true; }

private:
	std::function<void(void)> m_Callback;
};

class MessageOneFrame: public Manager::MessageAbstract
{
public:
	MessageOneFrame(Frame* Param, const std::function<void(Frame*)>& Callback): m_Param(Param), m_Callback(Callback) {}
	virtual bool Process(void) override { m_Callback(m_Param); return true; }

private:
	Frame* m_Param;
	std::function<void(Frame*)> m_Callback;
};

class MessageTwoFrames: public Manager::MessageAbstract
{
public:
	MessageTwoFrames(Frame* Param1, Frame* Param2, const std::function<void(Frame*, Frame*)>& Callback): m_Param1(Param1), m_Param2(Param2), m_Callback(Callback) {}
	virtual bool Process(void) override { m_Callback(m_Param1, m_Param2); return true; }

private:
	Frame* m_Param1;
	Frame* m_Param2;
	std::function<void(Frame*, Frame*)> m_Callback;
};

class MessageStop: public Manager::MessageAbstract
{
public:
	MessageStop() {}
	virtual bool Process(void) override { return false; }
};

Manager::Manager():
	CurrentFrame(nullptr),
	FramePos(-1),
	ModalEVCount(0),
	EndLoop(false),
	ModalExitCode(-1),
	StartManager(false)
{
	Frames.reserve(1024);
	ModalFrames.reserve(1024);
}

Manager::~Manager()
{
}

/* $ 29.12.2000 IS
  ������ CloseAll, �� ��������� ����������� ����������� ������ � ����,
  ���� ������������ ��������� ������������� ����.
  ���������� TRUE, ���� ��� ������� � ����� �������� �� ����.
*/
BOOL Manager::ExitAll()
{
	_MANAGER(CleverSysLog clv(L"Manager::ExitAll()"));

	// BUGBUG don't use iterators here, may be invalidated by DeleteCommit()
	for(size_t i = ModalFrames.size(); i; --i)
	{
		if (i - 1 >= ModalFrames.size())
			continue;
		auto CurFrame = ModalFrames[i - 1];
		if (!CurFrame->GetCanLoseFocus(TRUE))
		{
			auto PrevFrameCount = ModalFrames.size();
			CurFrame->ProcessKey(Manager::Key(KEY_ESC));
			Commit();

			if (PrevFrameCount == ModalFrames.size())
			{
				return FALSE;
			}
		}
	}

	// BUGBUG don't use iterators here, may be invalidated by DeleteCommit()
	for(size_t i = Frames.size(); i; --i)
	{
		if (i - 1 >= Frames.size())
			continue;
		auto CurFrame = Frames[i - 1];
		if (!CurFrame->GetCanLoseFocus(TRUE))
		{
			ActivateFrame(CurFrame);
			Commit();
			auto PrevFrameCount = Frames.size();
			CurFrame->ProcessKey(Manager::Key(KEY_ESC));
			Commit();

			if (PrevFrameCount == Frames.size())
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

void Manager::CloseAll()
{
	_MANAGER(CleverSysLog clv(L"Manager::CloseAll()"));
	while(!ModalFrames.empty())
	{
		DeleteFrame(ModalFrames.back());
		Commit();
	}
	while(!Frames.empty())
	{
		DeleteFrame(Frames.back());
		Commit();
	}
	Frames.clear();
}

void Manager::PushFrame(Frame* Param, frame_callback Callback)
{
	m_Queue.push_back(std::make_unique<MessageOneFrame>(Param,[this,Callback](Frame* Param){(this->*Callback)(Param);}));
}

void Manager::CheckAndPushFrame(Frame* Param, frame_callback Callback)
{
	//assert(Param);
	if (Param&&!Param->IsDeleting()) PushFrame(Param,Callback);
}

void Manager::ProcessFrameByPos(int Index, frame_callback Callback)
{
	Frame* frame=GetFrame(Index);
	assert(frame); //e��� frame==nullptr -> ������������ ���������� ������.
	(this->*Callback)(frame);
}

void Manager::CallbackFrame(const std::function<void(void)>& Callback)
{
	m_Queue.push_back(std::make_unique<MessageCallback>(Callback));
}

void Manager::InsertFrame(Frame *Inserted)
{
	_MANAGER(CleverSysLog clv(L"Manager::InsertFrame(Frame *Inserted, int Index)"));
	_MANAGER(SysLog(L"Inserted=%p, Index=%i",Inserted, Index));

	CheckAndPushFrame(Inserted,&Manager::InsertCommit);
}

void Manager::DeleteFrame(Frame *Deleted)
{
	_MANAGER(CleverSysLog clv(L"Manager::DeleteFrame(Frame *Deleted)"));
	_MANAGER(SysLog(L"Deleted=%p",Deleted));

	Frame* frame=Deleted?Deleted:CurrentFrame;
	assert(frame);
	CheckAndPushFrame(frame,&Manager::DeleteCommit);
	if (frame->GetDynamicallyBorn())
		frame->SetDeleting();
}

void Manager::DeleteFrame(int Index)
{
	_MANAGER(CleverSysLog clv(L"Manager::DeleteFrame(int Index)"));
	_MANAGER(SysLog(L"Index=%i",Index));
	ProcessFrameByPos(Index,&Manager::DeleteFrame);
}

void Manager::RedeleteFrame(Frame *Deleted)
{
	m_Queue.push_back(std::make_unique<MessageStop>());
	PushFrame(Deleted,&Manager::DeleteCommit);
}

void Manager::ExecuteNonModal(const Frame *NonModal)
{
	_MANAGER(CleverSysLog clv(L"Manager::ExecuteNonModal ()"));
	if (!NonModal) return;
	for (;;)
	{
		Commit();

		if (CurrentFrame!=NonModal || EndLoop)
		{
			break;
		}

		ProcessMainLoop();
	}
}

void Manager::ExecuteModal(Frame *Executed)
{
	_MANAGER(CleverSysLog clv(L"Manager::ExecuteModal (Frame *Executed)"));
	_MANAGER(SysLog(L"Executed=%p",Executed));

	if (Executed)
	{
		CheckAndPushFrame(Executed,&Manager::ExecuteCommit);
	}

	auto ModalStartLevel=ModalFrames.size();
	auto OriginalStartManager = StartManager;
	StartManager = true;

	for (;;)
	{
		Commit();

		if (ModalFrames.size()<=ModalStartLevel)
		{
			break;
		}

		ProcessMainLoop();
	}

	StartManager = OriginalStartManager;
	return;// GetModalExitCode();
}

int Manager::GetModalExitCode() const
{
	return ModalExitCode;
}

/* $ 11.10.2001 IS
   ���������� ���������� ������� � ��������� ������.
*/
int Manager::CountFramesWithName(const string& Name, BOOL IgnoreCase)
{
	int Counter=0;
	typedef int (*CompareFunction)(const string&, const string&);
	CompareFunction CaseSenitive = StrCmp, CaseInsensitive = StrCmpI;
	CompareFunction CmpFunction = IgnoreCase? CaseInsensitive : CaseSenitive;

	string strType, strCurName;

	std::for_each(CONST_RANGE(Frames, i)
	{
		i->GetTypeAndName(strType, strCurName);
		if (!CmpFunction(Name, strCurName))
			++Counter;
	});

	return Counter;
}

/*!
  \return ���������� nullptr ���� ����� "�����" ��� ���� ����� ������� �����.
  ������� �������, ���� ����������� ����� �� ���������.
  ���� �� ����� ���������, �� ����� ������� ������ ����������
  ��������� �� ���������� �����.
*/
Frame *Manager::FrameMenu()
{
	/* $ 28.04.2002 KM
	    ���� ��� ����������� ����, ��� ���� ������������
	    ������� ��� ������������.
	*/
	static int AlreadyShown=FALSE;

	if (AlreadyShown)
		return nullptr;

	int ExitCode, CheckCanLoseFocus=CurrentFrame->GetCanLoseFocus();
	{
		VMenu2 ModalMenu(MSG(MScreensTitle),nullptr,0,ScrY-4);
		ModalMenu.SetHelp(L"ScrSwitch");
		ModalMenu.SetFlags(VMENU_WRAPMODE);
		ModalMenu.SetPosition(-1,-1,0,0);
		ModalMenu.SetId(ScreensSwitchId);

		size_t n = 0;
		std::for_each(CONST_RANGE(Frames, i)
		{
			string strType, strName, strNumText;
			i->GetTypeAndName(strType, strName);
			MenuItemEx ModalMenuItem;

			if (n < 10)
				strNumText = str_printf(L"&%d. ", n);
			else if (n < 36)
				strNumText = str_printf(L"&%c. ", n + 55);  // 55='A'-10
			else
				strNumText = L"&   ";

			//TruncPathStr(strName,ScrX-24);
			ReplaceStrings(strName, L"&", L"&&");
			/*  ����������� "*" ���� ���� ������� */
			ModalMenuItem.strName = str_printf(L"%s%-10.10s %c %s", strNumText.data(), strType.data(),(i->IsFileModified()?L'*':L' '), strName.data());
			ModalMenuItem.SetSelect(static_cast<int>(n) == FramePos);
			ModalMenu.AddItem(ModalMenuItem);
			++n;
		});

		AlreadyShown=TRUE;
		ExitCode=ModalMenu.Run();
		AlreadyShown=FALSE;
	}

	if (CheckCanLoseFocus)
	{
		if (ExitCode>=0)
		{
			Frame* ActivatedFrame=GetFrame(ExitCode);
			ActivateFrame(ActivatedFrame);
			return (ActivatedFrame == CurrentFrame || !CurrentFrame->GetCanLoseFocus())? nullptr : CurrentFrame;
		}
	}

	return nullptr;
}


int Manager::GetFrameCountByType(int Type)
{
	int ret=0;

	std::for_each(CONST_RANGE(Frames, i)
	{
		if (!i->IsDeleting() && i->GetExitCode() != XC_QUIT && i->GetType() == Type)
			ret++;
	});

	return ret;
}

/*$ 11.05.2001 OT ������ ����� ������ ���� �� ������ �� ������� �����, �� � �������� - ����, �������� ��� */
Frame* Manager::FindFrameByFile(int ModalType,const string& FileName, const wchar_t *Dir)
{
	string strBufFileName;
	string strFullFileName = FileName;

	if (Dir)
	{
		strBufFileName = Dir;
		AddEndSlash(strBufFileName);
		strBufFileName += FileName;
		strFullFileName = strBufFileName;
	}

	auto ItemIterator = std::find_if(CONST_RANGE(Frames, i) -> bool
	{
		string strType, strName;

		// Mantis#0000469 - �������� Name ����� ������ ��� ���������� ModalType
		if (!i->IsDeleting() && i->GetType() == ModalType)
		{
			i->GetTypeAndName(strType, strName);

			if (!StrCmpI(strName, strFullFileName))
				return true;
		}
		return false;
	});

	return ItemIterator == Frames.cend()? nullptr : *ItemIterator;
}

bool Manager::ShowBackground()
{
	Global->CtrlObject->Desktop->Show();
	return true;
}

void Manager::ActivateFrame(Frame *Activated)
{
	_MANAGER(CleverSysLog clv(L"Manager::ActivateFrame(Frame *Activated)"));
	_MANAGER(SysLog(L"Activated=%i",Activated));

	if (Activated) CheckAndPushFrame(Activated,&Manager::ActivateCommit);
}

void Manager::ActivateFrame(int Index)
{
	_MANAGER(CleverSysLog clv(L"Manager::ActivateFrame(int Index)"));
	_MANAGER(SysLog(L"Index=%i",Index));
	ProcessFrameByPos(Index,&Manager::ActivateFrame);
}

void Manager::DeactivateFrame(Frame *Deactivated,int Direction)
{
	_MANAGER(CleverSysLog clv(L"Manager::DeactivateFrame (Frame *Deactivated,int Direction)"));
	_MANAGER(SysLog(L"Deactivated=%p, Direction=%d",Deactivated,Direction));

	if (Direction)
	{

		FramePos += Direction;

		if (FramePos < 0)
			FramePos = static_cast<int>(Frames.size() - 1);
		else if (FramePos >= static_cast<int>(Frames.size()))
			FramePos = 0;

		// for now we don't want to switch the desktop window with Ctrl-[Shift-]Tab
		if (FramePos == DesktopIndex)
		{
			FramePos = FilePanelsIndex;
		}

		ActivateFrame(FramePos);
	}
	else
	{
		// Direction==0
		// Direct access from menu or (in future) from plugin
	}

	CheckAndPushFrame(Deactivated,&Manager::DeactivateCommit);
}

void Manager::RefreshFrame(Frame *Refreshed)
{
	_MANAGER(CleverSysLog clv(L"Manager::RefreshFrame(Frame *Refreshed)"));
	_MANAGER(SysLog(L"Refreshed=%p",Refreshed));

	CheckAndPushFrame(Refreshed?Refreshed:CurrentFrame,&Manager::RefreshCommit);
}

void Manager::RefreshFrame(int Index)
{
	_MANAGER(CleverSysLog clv(L"Manager::RefreshFrame(int Index)"));
	_MANAGER(SysLog(L"Index=%d",Index));
	ProcessFrameByPos(Index,&Manager::RefreshFrame);
}

void Manager::ExecuteFrame(Frame *Executed)
{
	_MANAGER(CleverSysLog clv(L"Manager::ExecuteFrame(Frame *Executed)"));
	_MANAGER(SysLog(L"Executed=%p",Executed));
	CheckAndPushFrame(Executed,&Manager::ExecuteCommit);
}

void Manager::UpdateFrame(Frame* Old,Frame* New)
{
	m_Queue.push_back(std::make_unique<MessageTwoFrames>(Old,New,[this](Frame* Param1,Frame* Param2){this->UpdateCommit(Param1,Param2);}));
}

void Manager::SwitchToPanels()
{
	_MANAGER(CleverSysLog clv(L"Manager::SwitchToPanels()"));
	if (!Global->OnlyEditorViewerUsed)
	{
		ActivateFrame(FilePanelsIndex);
	}
}


bool Manager::HaveAnyFrame() const
{
	return !Frames.empty() || !m_Queue.empty() || CurrentFrame;
}

bool Manager::OnlyDesktop() const
{
	return Frames.size() == 1 && m_Queue.empty();
}

void Manager::EnterMainLoop()
{
	Global->WaitInFastFind=0;
	StartManager = true;

	for (;;)
	{
		Commit();

		if (EndLoop || (!HaveAnyFrame() || OnlyDesktop()))
		{
			break;
		}

		ProcessMainLoop();
	}
}

void Manager::SetLastInputRecord(const INPUT_RECORD *Rec)
{
	if (&LastInputRecord != Rec)
		LastInputRecord=*Rec;
}


void Manager::ProcessMainLoop()
{
	if ( CurrentFrame && !CurrentFrame->ProcessEvents() )
	{
		ProcessKey(Manager::Key(KEY_IDLE));
	}
	else
	{
		// Mantis#0000073: �� �������� ������������ � QView
		Global->WaitInMainLoop=IsPanelsActive(true);
		//WaitInFastFind++;
		int Key=GetInputRecord(&LastInputRecord);
		//WaitInFastFind--;
		Global->WaitInMainLoop=FALSE;

		if (EndLoop)
			return;

		if (LastInputRecord.EventType==MOUSE_EVENT && !(Key==KEY_MSWHEEL_UP || Key==KEY_MSWHEEL_DOWN || Key==KEY_MSWHEEL_RIGHT || Key==KEY_MSWHEEL_LEFT))
		{
				// ���������� ����� ���������, �.�. LastInputRecord ����� �������� ���������� �� ����� ���������� ProcessMouse
				MOUSE_EVENT_RECORD mer=LastInputRecord.Event.MouseEvent;
				ProcessMouse(&mer);
		}
		else
			ProcessKey(Manager::Key(Key));
	}

	if(IsPanelsActive())
	{
		if(!Global->PluginPanelsCount)
		{
			Global->CtrlObject->Plugins->RefreshPluginsList();
		}
	}
}

void Manager::ExitMainLoop(int Ask)
{
	if (Global->CloseFAR)
	{
		Global->CloseFARMenu=TRUE;
	};

	const wchar_t* const Items[] = {MSG(MAskQuit),MSG(MYes),MSG(MNo)};

	if (!Ask || !Global->Opt->Confirm.Exit || !Message(0,2,MSG(MQuit),Items, ARRAYSIZE(Items), nullptr, nullptr, &FarAskQuitId))
	{
		/* $ 29.12.2000 IS
		   + ���������, ��������� �� ��� ���������� �����. ���� ���, �� �� �������
		     �� ����.
		*/
		if (ExitAll() || Global->CloseFAR)
		{
			FilePanels *cp;

			if (!(cp = Global->CtrlObject->Cp())
			        || (!cp->LeftPanel->ProcessPluginEvent(FE_CLOSE,nullptr) && !cp->RightPanel->ProcessPluginEvent(FE_CLOSE,nullptr)))
				EndLoop=true;
		}
		else
		{
			Global->CloseFARMenu=FALSE;
		}
	}
}

void Manager::AddGlobalKeyHandler(const std::function<int(Key)>& Handler)
{
	m_GlobalKeyHandlers.emplace_back(Handler);
}

int Manager::ProcessKey(Key key)
{
	if (CurrentFrame)
	{
		/*** ���� ����������������� ������ ! ***/
		/***   ������� ������ �����������    ***/

		switch (key.FarKey)
		{
			case KEY_ALT|KEY_NUMPAD0:
			case KEY_RALT|KEY_NUMPAD0:
			case KEY_ALTINS:
			case KEY_RALTINS:
			{
				RunGraber();
				return TRUE;
			}
			case KEY_CONSOLE_BUFFER_RESIZE:
				Sleep(1);
				ResizeAllFrame();
				return TRUE;
		}

		/*** � ��� ����� - ��� ���������! ***/
		if (!Global->IsProcessAssignMacroKey)
		{
			FOR(const auto& i, m_GlobalKeyHandlers)
			{
				if (i(key))
				{
					return TRUE;
				}
			}

			switch (key.FarKey)
			{
				case KEY_CTRLW:
				case KEY_RCTRLW:
					ShowProcessList();
					return TRUE;

				case KEY_F11:
				{
					int TypeFrame = Global->FrameManager->GetCurrentFrame()->GetType();
					static int reentry=0;
					if(!reentry && (TypeFrame == MODALTYPE_DIALOG || TypeFrame == MODALTYPE_VMENU))
					{
						++reentry;
						int r=CurrentFrame->ProcessKey(key);
						--reentry;
						return r;
					}

					PluginsMenu();
					Global->FrameManager->RefreshFrame();
					//_MANAGER(SysLog(-1));
					return TRUE;
				}
				case KEY_ALTF9:
				case KEY_RALTF9:
				{
					//_MANAGER(SysLog(1,"Manager::ProcessKey, KEY_ALTF9 pressed..."));
					Sleep(1);
					SetVideoMode();
					Sleep(1);

					/* � �������� ���������� Alt-F9 (� ���������� ������) � �������
					   ������� �������� WINDOW_BUFFER_SIZE_EVENT, ����������� �
					   ChangeVideoMode().
					   � ������ ���������� �������� ��� �� ���������� �� ������ ��������
					   ��������.
					*/
					if (Global->CtrlObject->Macro.IsExecuting())
					{
						int PScrX=ScrX;
						int PScrY=ScrY;
						Sleep(1);
						GetVideoMode(CurSize);

						if (PScrX+1 == CurSize.X && PScrY+1 == CurSize.Y)
						{
							//_MANAGER(SysLog(-1,"GetInputRecord(WINDOW_BUFFER_SIZE_EVENT); return KEY_NONE"));
							return TRUE;
						}
						else
						{
							PrevScrX=PScrX;
							PrevScrY=PScrY;
							//_MANAGER(SysLog(-1,"GetInputRecord(WINDOW_BUFFER_SIZE_EVENT); return KEY_CONSOLE_BUFFER_RESIZE"));
							Sleep(1);
							return ProcessKey(Manager::Key(KEY_CONSOLE_BUFFER_RESIZE));
						}
					}

					//_MANAGER(SysLog(-1));
					return TRUE;
				}
				case KEY_F12:
				{
					int TypeFrame = Global->FrameManager->GetCurrentFrame()->GetType();

					if (TypeFrame != MODALTYPE_HELP && TypeFrame != MODALTYPE_DIALOG && TypeFrame != MODALTYPE_VMENU && TypeFrame != MODALTYPE_GRABBER && TypeFrame != MODALTYPE_HMENU)
					{
						DeactivateFrame(FrameMenu(),0);
						//_MANAGER(SysLog(-1));
						return TRUE;
					}

					break; // ������� F12 ������ �� �������
				}

				case KEY_CTRLALTSHIFTPRESS:
				case KEY_RCTRLALTSHIFTPRESS:
				{
					if (!(Global->Opt->CASRule&1) && key.FarKey == KEY_CTRLALTSHIFTPRESS)
						break;

					if (!(Global->Opt->CASRule&2) && key.FarKey == KEY_RCTRLALTSHIFTPRESS)
						break;

						if (CurrentFrame->CanFastHide())
						{
							int isPanelFocus=CurrentFrame->GetType() == MODALTYPE_PANELS;

							if (isPanelFocus)
							{
								int LeftVisible=Global->CtrlObject->Cp()->LeftPanel->IsVisible();
								int RightVisible=Global->CtrlObject->Cp()->RightPanel->IsVisible();
								int CmdLineVisible=Global->CtrlObject->CmdLine->IsVisible();
								int KeyBarVisible=Global->CtrlObject->Cp()->MainKeyBar.IsVisible();
								ShowBackground();
								Global->CtrlObject->Cp()->LeftPanel->HideButKeepSaveScreen();
								Global->CtrlObject->Cp()->RightPanel->HideButKeepSaveScreen();

								switch (Global->Opt->PanelCtrlAltShiftRule)
								{
									case 0:
										if (CmdLineVisible)
											Global->CtrlObject->CmdLine->Show();
										if (KeyBarVisible)
											Global->CtrlObject->Cp()->MainKeyBar.Show();
										break;
									case 1:
										if (KeyBarVisible)
											Global->CtrlObject->Cp()->MainKeyBar.Show();
										break;
								}

								WaitKey(key.FarKey==KEY_CTRLALTSHIFTPRESS?KEY_CTRLALTSHIFTRELEASE:KEY_RCTRLALTSHIFTRELEASE);

								if (LeftVisible)      Global->CtrlObject->Cp()->LeftPanel->Show();

								if (RightVisible)     Global->CtrlObject->Cp()->RightPanel->Show();

								if (CmdLineVisible)   Global->CtrlObject->CmdLine->Show();

								if (KeyBarVisible)    Global->CtrlObject->Cp()->MainKeyBar.Show();
							}
							else
							{
								ImmediateHide();
								WaitKey(key.FarKey==KEY_CTRLALTSHIFTPRESS?KEY_CTRLALTSHIFTRELEASE:KEY_RCTRLALTSHIFTRELEASE);
							}

							Global->FrameManager->RefreshFrame();
						}

						return TRUE;

					break;
				}
				case KEY_CTRLTAB:
				case KEY_RCTRLTAB:
				case KEY_CTRLSHIFTTAB:
				case KEY_RCTRLSHIFTTAB:

					if (CurrentFrame->GetCanLoseFocus())
					{
						DeactivateFrame(CurrentFrame,(key.FarKey==KEY_CTRLTAB||key.FarKey==KEY_RCTRLTAB)?1:-1);
					}
					else
						break;

					_MANAGER(SysLog(-1));
					return TRUE;
			}
		}

		CurrentFrame->UpdateKeyBar();
		CurrentFrame->ProcessKey(key);
	}

	_MANAGER(SysLog(-1));
	return FALSE;
}

int Manager::ProcessMouse(const MOUSE_EVENT_RECORD *MouseEvent)
{
	int ret=FALSE;

//    _D(SysLog(1,"Manager::ProcessMouse()"));
	if (CurrentFrame)
		ret=CurrentFrame->ProcessMouse(MouseEvent);

//    _D(SysLog(L"Manager::ProcessMouse() ret=%i",ret));
	_MANAGER(SysLog(-1));
	return ret;
}

void Manager::PluginsMenu() const
{
	_MANAGER(SysLog(1));
	int curType = CurrentFrame->GetType();

	if (curType == MODALTYPE_PANELS || curType == MODALTYPE_EDITOR || curType == MODALTYPE_VIEWER || curType == MODALTYPE_DIALOG || curType == MODALTYPE_VMENU)
	{
		/* 02.01.2002 IS
		   ! ����� ���������� ������ �� Shift-F1 � ���� �������� � ���������/������/�������
		   ! ���� �� ������ QVIEW ��� INFO ������ ����, �� �������, ��� ���
		     ����������� ����� � ��������� � ��������������� ���������� �������
		*/
		if (curType==MODALTYPE_PANELS)
		{
			int pType=Global->CtrlObject->Cp()->ActivePanel()->GetType();

			if (pType==QVIEW_PANEL || pType==INFO_PANEL)
			{
				string strType, strCurFileName;
				Global->CtrlObject->Cp()->GetTypeAndName(strType, strCurFileName);

				if (!strCurFileName.empty())
				{
					DWORD Attr=api::GetFileAttributes(strCurFileName);

					// ���������� ������ ������� �����
					if (Attr!=INVALID_FILE_ATTRIBUTES && !(Attr&FILE_ATTRIBUTE_DIRECTORY))
						curType=MODALTYPE_VIEWER;
				}
			}
		}

		// � ���������, ������ ��� ������� ������� ���� ������ �� Shift-F1
		const wchar_t *Topic=curType==MODALTYPE_EDITOR?L"Editor":
		                     curType==MODALTYPE_VIEWER?L"Viewer":
		                     curType==MODALTYPE_DIALOG?L"Dialog":nullptr;
		Global->CtrlObject->Plugins->CommandsMenu(curType,0,Topic);
	}

	_MANAGER(SysLog(-1));
}

bool Manager::IsPanelsActive(bool and_not_qview) const
{
	if (FramePos>=0 && CurrentFrame)
	{
		auto fp = dynamic_cast<FilePanels*>(CurrentFrame);
		return fp && (!and_not_qview || fp->ActivePanel()->GetType() != QVIEW_PANEL);
	}
	else
	{
		return false;
	}
}

Frame* Manager::GetFrame(size_t Index) const
{
	if (Index >= Frames.size() || Frames.empty())
	{
		return nullptr;
	}

	return Frames[Index];
}

int Manager::IndexOfStack(Frame *Frame)
{
	auto ItemIterator = std::find(ALL_CONST_RANGE(ModalFrames), Frame);
	return ItemIterator != ModalFrames.cend()? ItemIterator - ModalFrames.cbegin() : -1;
}

int Manager::IndexOf(Frame *Frame)
{
	auto ItemIterator = std::find(ALL_CONST_RANGE(Frames), Frame);
	return ItemIterator != Frames.cend() ? ItemIterator - Frames.cbegin() : -1;
}

void Manager::Commit(void)
{
	_MANAGER(CleverSysLog clv(L"Manager::Commit()"));
	_MANAGER(ManagerClass_Dump(L"ManagerClass"));
	while (!m_Queue.empty())
	{
		auto message=std::move(m_Queue.front());
		m_Queue.pop_front();
		if (!message->Process()) break;
	}
}

void Manager::InsertCommit(Frame* Param)
{
	_MANAGER(CleverSysLog clv(L"Manager::InsertCommit()"));
	_MANAGER(SysLog(L"InsertedFrame=%p",Param));
	if (Param)
	{
		Param->FrameToBack=CurrentFrame;
		Frames.emplace_back(Param);
		ActivateCommit(Param);
	}
}

void Manager::DeleteCommit(Frame* Param)
{
	_MANAGER(CleverSysLog clv(L"Manager::DeleteCommit()"));
	_MANAGER(SysLog(L"DeletedFrame=%p",Param));

	if (!Param)
		return;

	if (Param->IsBlocked())
	{
		RedeleteFrame(Param);
		return;
	}

	Param->OnDestroy();

	int ModalIndex=IndexOfStack(Param);
	int FrameIndex=IndexOf(Param);
	assert(!(-1!=ModalIndex&&-1!=FrameIndex));

	std::for_each(CONST_RANGE(Frames, i)
	{
		if (i->FrameToBack==Param)
		{
			i->FrameToBack=Global->CtrlObject->Cp();
		}
	});

	auto ClearCurrentFrame=[this]
	{
		this->CurrentFrame=nullptr;
		InterlockedExchange(&CurrentWindowType,-1);
	};
	if (ModalIndex!=-1)
	{
		ModalFrames.erase(ModalFrames.begin() + ModalIndex);

		if (CurrentFrame==Param)
		{
			if (!ModalFrames.empty())
			{
				ActivateCommit(ModalFrames.back());
			}
			else if (!Frames.empty())
			{
				assert(FramePos < static_cast<int>(Frames.size()));
				assert(FramePos>=0);
				ActivateCommit(FramePos);
			}
			else ClearCurrentFrame();
		}
	}
	else if (-1!=FrameIndex)
	{
		Frames.erase(Frames.begin() + FrameIndex);

		if (FramePos >= static_cast<int>(Frames.size()))
		{
			FramePos = FilePanelsIndex;
			if (FramePos >= static_cast<int>(Frames.size()))
			{
				FramePos = DesktopIndex;
			}
		}

		if (CurrentFrame==Param)
		{
			if (!Frames.empty())
			{
				if (Param->FrameToBack == Global->CtrlObject->Desktop || Param->FrameToBack == Global->CtrlObject->Cp())
				{
					ActivateCommit(FramePos);
				}
				else
				{
					assert(Param->FrameToBack);
					ActivateCommit(Param->FrameToBack);
				}
			}
			else ClearCurrentFrame();
		}
		else
		{
			if (!Frames.empty())
			{
				RefreshFrame(FramePos);
				RefreshFrame(CurrentFrame);
			}
		}
	}

	assert(CurrentFrame!=Param);

	if (Param->GetDynamicallyBorn())
	{
		_MANAGER(SysLog(L"delete DeletedFrame %p", Param));
		delete Param;
	}
}

void Manager::ActivateCommit(Frame* Param)
{
	_MANAGER(CleverSysLog clv(L"Manager::ActivateCommit()"));
	_MANAGER(SysLog(L"ActivatedFrame=%p",Param));

	if (CurrentFrame==Param)
	{
		RefreshCommit(Param);
		return;
	}

	int FrameIndex=IndexOf(Param);

	if (-1!=FrameIndex)
	{
		FramePos=FrameIndex;
	}

	/* 14.05.2002 SKV
	  ���� �� �������� ������������ ������������� �����,
	  �� ���� ��� ������� �� ���� ����� �������.
	*/

	auto ItemIterator = std::find(ALL_RANGE(ModalFrames), Param);
	if (ItemIterator != ModalFrames.end())
	{
		std::swap(*ItemIterator, ModalFrames.back());
	}

	CurrentFrame=Param;
	InterlockedExchange(&CurrentWindowType,CurrentFrame->GetType());
	UpdateMacroArea();
	RefreshCommit(Param);
}

void Manager::ActivateCommit(int Index)
{
	ProcessFrameByPos(Index,&Manager::ActivateCommit);
}

void Manager::RefreshCommit(Frame* Param)
{
	_MANAGER(CleverSysLog clv(L"Manager::RefreshCommit()"));
	_MANAGER(SysLog(L"RefreshedFrame=%p",Param));

	if (!Param)
		return;

	if (IndexOf(Param)==-1 && IndexOfStack(Param)==-1)
		return;

	if (!Param->Locked())
	{
		if (!Global->IsRedrawFramesInProcess)
			Param->ShowConsoleTitle();

		Param->Refresh();
	}

	if
	(
		(Global->Opt->ViewerEditorClock && (Param->GetType() == MODALTYPE_EDITOR || Param->GetType() == MODALTYPE_VIEWER))
		||
		(Global->WaitInMainLoop && Global->Opt->Clock)
	)
		ShowTime(1);
}

void Manager::DeactivateCommit(Frame* Param)
{
	_MANAGER(CleverSysLog clv(L"Manager::DeactivateCommit()"));
	_MANAGER(SysLog(L"DeactivatedFrame=%p",Param));
	if (Param)
	{
		Param->OnChangeFocus(0);
	}
}

void Manager::ExecuteCommit(Frame* Param)
{
	_MANAGER(CleverSysLog clv(L"Manager::ExecuteCommit()"));
	_MANAGER(SysLog(L"ExecutedFrame=%p",Param));

	if (Param)
	{
		ModalFrames.emplace_back(Param);
		ActivateCommit(Param);
	}
}

void Manager::UpdateCommit(Frame* Old,Frame* New)
{
	int FrameIndex=IndexOf(Old);

	if (-1!=FrameIndex)
	{
		Frames[FrameIndex]=New;
		New->FrameToBack=CurrentFrame;
		ActivateCommit(New);
		DeleteFrame(Old);
	}
	else
	{
		_MANAGER(SysLog(L"ERROR! DeletedFrame not found"));
	}
}

/*$ 26.06.2001 SKV
  ��� ������ �� �������� ����������� ACTL_COMMIT
*/
void Manager::PluginCommit()
{
	Commit();
}

/* $ ������� ��� ���� CtrlAltShift OT */
void Manager::ImmediateHide()
{
	if (FramePos<0)
		return;

	// ������� ���������, ���� �� � ������������ ������ SaveScreen
	if (CurrentFrame->HasSaveScreen())
	{
		CurrentFrame->Hide();
		return;
	}

	// ������ ����������������, ������ ��� ������
	// �� ���������� ��������� �������, ����� �� �������.
	if (!ModalFrames.empty())
	{
		/* $ 28.04.2002 KM
		    ��������, � �� ��������� �� �������� ��� ������ �� �������
		    ���������� �����? � ���� ��, ������� User screen.
		*/
		if (ModalFrames.back()->GetType()==MODALTYPE_EDITOR || ModalFrames.back()->GetType()==MODALTYPE_VIEWER)
		{
			ShowBackground();
		}
		else
		{
			int UnlockCount=0;
			Global->IsRedrawFramesInProcess++;

			while (GetFrame(FramePos)->Locked())
			{
				GetFrame(FramePos)->Unlock();
				UnlockCount++;
			}

			RefreshFrame(GetFrame(FramePos));
			Commit();

			for (int i=0; i<UnlockCount; i++)
			{
				GetFrame(FramePos)->Lock();
			}

			if (ModalFrames.size() > 1)
			{
				FOR(const auto& i, make_range(ModalFrames.cbegin(), ModalFrames.cend() - 1))
				{
					RefreshFrame(i);
					Commit();
				}
			}

			/* $ 04.04.2002 KM
			   ���������� ��������� ������ � ��������� ������.
			   ���� �� ������������� ��������� ��������� �������
			   ��� ����������� ���� �������.
			*/
			Global->IsRedrawFramesInProcess--;
			CurrentFrame->ShowConsoleTitle();
		}
	}
	else
	{
		ShowBackground();
	}
}

void Manager::ResizeAllFrame()
{
	Global->ScrBuf->Lock();
	std::for_each(ALL_CONST_RANGE(Frames), std::mem_fn(&Frame::ResizeConsole));
	std::for_each(ALL_CONST_RANGE(ModalFrames), std::mem_fn(&Frame::ResizeConsole));

	ImmediateHide();
	RefreshFrame();
	Global->ScrBuf->Unlock();
}

void Manager::InitKeyBar()
{
	std::for_each(ALL_CONST_RANGE(Frames), std::mem_fn(&Frame::InitKeyBar));
}

void Manager::UpdateMacroArea(void)
{
	if (CurrentFrame) Global->CtrlObject->Macro.SetMode(CurrentFrame->GetMacroMode());
}
