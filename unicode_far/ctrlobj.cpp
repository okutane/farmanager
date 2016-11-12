﻿/*
ctrlobj.cpp

Управление остальными объектами, раздача сообщений клавиатуры и мыши
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

#include "ctrlobj.hpp"
#include "manager.hpp"
#include "cmdline.hpp"
#include "hilight.hpp"
#include "history.hpp"
#include "filefilter.hpp"
#include "filepanels.hpp"
#include "syslog.hpp"
#include "interf.hpp"
#include "config.hpp"
#include "dirmix.hpp"
#include "console.hpp"
#include "poscache.hpp"
#include "plugins.hpp"
#include "desktop.hpp"
#include "colormix.hpp"
#include "scrbuf.hpp"

ControlObject::ControlObject()
{
	_OT(SysLog(L"[%p] ControlObject::ControlObject()", this));

	SetColor(COL_COMMANDLINEUSERSCREEN);
	GotoXY(0, ScrY - 3);
	ShowCopyright();
	GotoXY(0, ScrY - 2);
	MoveCursor(0, ScrY - 1);

	Desktop = desktop::create();
	Global->WindowManager->InsertWindow(Desktop);
	Desktop->TakeSnapshot();

	HiFiles = std::make_unique<HighlightFiles>();
	Plugins = std::make_unique<PluginManager>();

	CmdHistory = std::make_unique<History>(HISTORYTYPE_CMD, string{}, Global->Opt->SaveHistory);

	FolderHistory = std::make_unique<History>(HISTORYTYPE_FOLDER, string{}, Global->Opt->SaveFoldersHistory);
	FolderHistory->SetAddMode(true, 2, true);

	ViewHistory = std::make_unique<History>(HISTORYTYPE_VIEW, string{}, Global->Opt->SaveViewHistory);
	ViewHistory->SetAddMode(true,Global->Opt->FlagPosixSemantics?1:2,true);

	FileFilter::InitFilter();
}


void ControlObject::Init(int DirCount)
{
	FPanels = FilePanels::create(true, DirCount);

	Global->WindowManager->InsertWindow(FPanels); // before PluginCommit()

	const auto strOldTitle = Global->ScrBuf->GetTitle();
	Global->WindowManager->PluginCommit();
	Plugins->LoadPlugins();
	Global->ScrBuf->SetTitle(strOldTitle);

	FPanels->LeftPanel()->Update(0);
	FPanels->RightPanel()->Update(0);
	FPanels->LeftPanel()->GoToFile(Global->Opt->LeftPanel.CurFile);
	FPanels->RightPanel()->GoToFile(Global->Opt->RightPanel.CurFile);

	FarChDir(FPanels->ActivePanel()->GetCurDir());

	Macro.LoadMacros(true);
	FPanels->LeftPanel()->SetCustomSortMode(Global->Opt->LeftPanel.SortMode, SO_KEEPCURRENT);
	FPanels->RightPanel()->SetCustomSortMode(Global->Opt->RightPanel.SortMode, SO_KEEPCURRENT);
	Global->WindowManager->SwitchToPanels();  // otherwise panels are empty
}

void ControlObject::CreateDummyFilePanels()
{
	FPanels = FilePanels::create(false, 0);
}

ControlObject::~ControlObject()
{
	if (Global->CriticalInternalError)
	{
		Global->WindowManager->CloseAll();
		return;
	}

	_OT(SysLog(L"[%p] ControlObject::~ControlObject()", this));

	// dummy_panel indicates /v or /e mode
	if (FPanels && FPanels->ActivePanel() && !std::dynamic_pointer_cast<dummy_panel>(FPanels->ActivePanel()))
	{
		if (Global->Opt->AutoSaveSetup)
			Global->Opt->Save(false);

		if (FPanels->ActivePanel()->GetMode() != panel_mode::PLUGIN_PANEL)
		{
			FolderHistory->AddToHistory(FPanels->ActivePanel()->GetCurDir());
		}
	}

	Global->WindowManager->CloseAll();
	FileFilter::CloseFilter();
	History::CompactHistory();
	FilePositionCache::CompactHistory();
}


void ControlObject::ShowCopyright(DWORD Flags)
{
	if (Flags&1)
	{
		string strOut = concat(Global->Version(), EOL_STR, Global->Copyright(), EOL_STR);
		Console().Write(strOut);
		Console().Commit();
	}
	else
	{
		COORD Size, CursorPosition;
		Console().GetSize(Size);
		Console().GetCursorPosition(CursorPosition);
		int FreeSpace=Size.Y-CursorPosition.Y-1;

		if (FreeSpace<5)
			ScrollScreen(5-FreeSpace);

		GotoXY(0,ScrY-4);
		Text(Global->Version());
		GotoXY(0,ScrY-3);
		Text(Global->Copyright());
	}
}

FilePanels* ControlObject::Cp() const
{
	return FPanels.get();
}

window_ptr ControlObject::Panels() const
{
	return FPanels;
}

CommandLine* ControlObject::CmdLine() const
{
	return FPanels->GetCmdLine();
}
