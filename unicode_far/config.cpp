/*
config.cpp

������������
*/
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
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

#include "config.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "colors.hpp"
#include "cmdline.hpp"
#include "ctrlobj.hpp"
#include "dialog.hpp"
#include "filepanels.hpp"
#include "filelist.hpp"
#include "panel.hpp"
#include "help.hpp"
#include "filefilter.hpp"
#include "poscache.hpp"
#include "findfile.hpp"
#include "hilight.hpp"
#include "registry.hpp"
#include "interf.hpp"
#include "keyboard.hpp"
#include "palette.hpp"
#include "message.hpp"
#include "stddlg.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "udlist.hpp"

Options Opt;// BUG !! ={0};

// ����������� ����� ������������
static const wchar_t *WordDiv0 = L"~!%^&*()+|{}:\"<>?`-=\\[];',./";

// ����������� ����� ������������ ��� ������� Xlat
static const wchar_t *WordDivForXlat0=L" \t!#$%^&*()+|=\\/@?";

string strKeyNameConsoleDetachKey;
static const wchar_t szCtrlShiftX[]=L"CtrlShiftX";
static const wchar_t szCtrlDot[]=L"Ctrl.";
static const wchar_t szCtrlShiftDot[]=L"CtrlShift.";

// KeyName
const wchar_t NKeyColors[]=L"Colors";
const wchar_t NKeyScreen[]=L"Screen";
const wchar_t NKeyCmdline[]=L"Cmdline";
const wchar_t NKeyInterface[]=L"Interface";
const wchar_t NKeyViewer[]=L"Viewer";
const wchar_t NKeyDialog[]=L"Dialog";
const wchar_t NKeyEditor[]=L"Editor";
const wchar_t NKeyXLat[]=L"XLat";
const wchar_t NKeySystem[]=L"System";
const wchar_t NKeySystemExecutor[]=L"System\\Executor";
const wchar_t NKeySystemNowell[]=L"System\\Nowell";
const wchar_t NKeyHelp[]=L"Help";
const wchar_t NKeyLanguage[]=L"Language";
const wchar_t NKeyConfirmations[]=L"Confirmations";
const wchar_t NKeyPluginConfirmations[]=L"PluginConfirmations";
const wchar_t NKeyPanel[]=L"Panel";
const wchar_t NKeyPanelLeft[]=L"Panel\\Left";
const wchar_t NKeyPanelRight[]=L"Panel\\Right";
const wchar_t NKeyPanelLayout[]=L"Panel\\Layout";
const wchar_t NKeyPanelTree[]=L"Panel\\Tree";
const wchar_t NKeyPanelInfo[]=L"Panel\\Info";
const wchar_t NKeyLayout[]=L"Layout";
const wchar_t NKeyDescriptions[]=L"Descriptions";
const wchar_t NKeyKeyMacros[]=L"KeyMacros";
const wchar_t NKeyPolicies[]=L"Policies";
const wchar_t NKeyFileFilter[]=L"OperationsFilter";
const wchar_t NKeySavedHistory[]=L"SavedHistory";
const wchar_t NKeySavedViewHistory[]=L"SavedViewHistory";
const wchar_t NKeySavedFolderHistory[]=L"SavedFolderHistory";
const wchar_t NKeySavedDialogHistory[]=L"SavedDialogHistory";
const wchar_t NKeyCodePages[]=L"CodePages";
const wchar_t NParamHistoryCount[]=L"HistoryCount";

const wchar_t *constBatchExt=L".BAT;.CMD;";

void SystemSettings()
{
	const wchar_t *HistoryName=L"PersPath";
	DialogDataEx CfgDlgData[]=
	{
		/* 00 */ DI_DOUBLEBOX,3, 1,52,21,0,0,0,0,(const wchar_t *)MConfigSystemTitle,
		/* 01 */ DI_CHECKBOX, 5, 2, 0, 2,1,0,0,0,(const wchar_t *)MConfigRO,
		/* 02 */ DI_CHECKBOX, 5, 3, 0, 3,0,0,DIF_AUTOMATION,0,(const wchar_t *)MConfigRecycleBin,
		/* 03 */ DI_CHECKBOX, 9, 4, 0, 4,0,0,0,0,(const wchar_t *)MConfigRecycleBinLink,
		/* 04 */ DI_CHECKBOX, 5, 5, 0, 5,0,0,0,0,(const wchar_t *)MConfigSystemCopy,
		/* 05 */ DI_CHECKBOX, 5, 6, 0, 6,0,0,0,0,(const wchar_t *)MConfigCopySharing,
		/* 06 */ DI_CHECKBOX, 5, 7, 0, 7,0,0,0,0,(const wchar_t *)MConfigScanJunction,
		/* 07 */ DI_CHECKBOX, 5, 8, 0, 8,0,0,0,0,(const wchar_t *)MConfigCreateUppercaseFolders,
		/* 08 */ DI_CHECKBOX, 5, 9, 0, 9,0,0,DIF_AUTOMATION,0,(const wchar_t *)MConfigInactivity,
		/* 09 */ DI_FIXEDIT,  9,10,11,10,0,0,0,0,L"",
		/* 10 */ DI_TEXT,    13,10, 0,10,0,0,0,0,(const wchar_t *)MConfigInactivityMinutes,
		/* 11 */ DI_CHECKBOX, 5,11, 0,11,0,0,0,0,(const wchar_t *)MConfigSaveHistory,
		/* 12 */ DI_CHECKBOX, 5,12, 0,12,0,0,0,0,(const wchar_t *)MConfigSaveFoldersHistory,
		/* 13 */ DI_CHECKBOX, 5,13, 0,13,0,0,0,0,(const wchar_t *)MConfigSaveViewHistory,
		/* 14 */ DI_CHECKBOX, 5,14, 0,14,0,0,0,0,(const wchar_t *)MConfigRegisteredTypes,
		/* 15 */ DI_CHECKBOX, 5,15, 0,15,0,0,0,0,(const wchar_t *)MConfigCloseCDGate,
		/* 16 */ DI_TEXT,     5,16, 0,16,0,0,0,0,(const wchar_t *)MConfigPersonalPath,
		/* 17 */ DI_EDIT,     5,17,50,17,0,(DWORD_PTR)HistoryName,DIF_HISTORY,0,L"",
		/* 18 */ DI_CHECKBOX, 5,18, 0,18,0,0,0,0,(const wchar_t *)MConfigAutoSave,
		/* 19 */ DI_TEXT,     5,19, 0,19,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,L"",
		/* 20 */ DI_BUTTON,   0,20, 0,20,0,0,DIF_CENTERGROUP,1,(const wchar_t *)MOk,
		/* 21 */ DI_BUTTON,   0,20, 0,20,0,0,DIF_CENTERGROUP,0,(const wchar_t *)MCancel
	};
	MakeDialogItemsEx(CfgDlgData,CfgDlg);
	CfgDlg[1].Selected=Opt.ClearReadOnly;
	CfgDlg[2].Selected=Opt.DeleteToRecycleBin;
	CfgDlg[3].Selected=Opt.DeleteToRecycleBinKillLink;
	CfgDlg[4].Selected=Opt.CMOpt.UseSystemCopy;
	CfgDlg[5].Selected=Opt.CMOpt.CopyOpened;
	CfgDlg[6].Selected=Opt.ScanJunction;
	CfgDlg[7].Selected=Opt.CreateUppercaseFolders;
	CfgDlg[8].Selected=Opt.InactivityExit;
	CfgDlg[9].strData.Format(L"%d", Opt.InactivityExitTime);

	if (!Opt.InactivityExit)
	{
		CfgDlg[9].Flags|=DIF_DISABLE;
		CfgDlg[10].Flags|=DIF_DISABLE;
	}

	CfgDlg[11].Selected=Opt.SaveHistory;
	CfgDlg[12].Selected=Opt.SaveFoldersHistory;
	CfgDlg[13].Selected=Opt.SaveViewHistory;
	CfgDlg[14].Selected=Opt.UseRegisteredTypes;
	CfgDlg[15].Selected=Opt.CloseCDGate;
	CfgDlg[17].strData = Opt.LoadPlug.strPersonalPluginsPath;
	CfgDlg[18].Selected=Opt.AutoSaveSetup;
	{
		Dialog Dlg((DialogItemEx*)CfgDlg,countof(CfgDlg));
		Dlg.SetHelp(L"SystemSettings");
		Dlg.SetPosition(-1,-1,56,23);
		Dlg.SetAutomation(2,3,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
		Dlg.SetAutomation(8,9,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
		Dlg.SetAutomation(8,10,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
		Dlg.Process();

		if (Dlg.GetExitCode()!=20)
			return;
	}
	Opt.ClearReadOnly=CfgDlg[1].Selected;
	Opt.DeleteToRecycleBin=CfgDlg[2].Selected;
	Opt.DeleteToRecycleBinKillLink=CfgDlg[3].Selected;
	Opt.CMOpt.UseSystemCopy=CfgDlg[4].Selected;
	Opt.CMOpt.CopyOpened=CfgDlg[5].Selected;
	Opt.ScanJunction=CfgDlg[6].Selected;
	Opt.CreateUppercaseFolders=CfgDlg[7].Selected;
	Opt.InactivityExit=CfgDlg[8].Selected;

	if ((Opt.InactivityExitTime=_wtoi(CfgDlg[9].strData))<=0)
		Opt.InactivityExit=Opt.InactivityExitTime=0;

	Opt.SaveHistory=CfgDlg[11].Selected;
	Opt.SaveFoldersHistory=CfgDlg[12].Selected;
	Opt.SaveViewHistory=CfgDlg[13].Selected;
	Opt.UseRegisteredTypes=CfgDlg[14].Selected;
	Opt.CloseCDGate=CfgDlg[15].Selected;
	Opt.AutoSaveSetup=CfgDlg[18].Selected;
	Opt.LoadPlug.strPersonalPluginsPath = CfgDlg[17].strData;
}


void PanelSettings()
{
	enum enumPanelSettings
	{
		DLG_PANEL_TITLE,
		DLG_PANEL_HIDDEN,
		DLG_PANEL_HIGHLIGHT,
		DLG_PANEL_CHANGEFOLDER,
		DLG_PANEL_SELECTFOLDERS,
		DLG_PANEL_SORTFOLDEREXT,
		DLG_PANEL_REVERSESORT,
		DLG_PANEL_AUTOUPDATELIMIT,
		DLG_PANEL_AUTOUPDATELIMIT2,
		DLG_PANEL_AUTOUPDATELIMITVAL,
		DLG_PANEL_AUTOUPDATEREMOTE,
		DLG_PANEL_SEPARATOR,
		DLG_PANEL_SHOWCOLUMNTITLES,
		DLG_PANEL_SHOWPANELSTATUS,
		DLG_PANEL_SHOWPANELTOTALS,
		DLG_PANEL_SHOWPANELFREE,
		DLG_PANEL_SHOWPANELSCROLLBAR,
		DLG_PANEL_SHOWSCREENSNUMBER,
		DLG_PANEL_SHOWSORTMODE,
		DLG_PANEL_SEPARATOR2,
		DLG_PANEL_OK,
		DLG_PANEL_CANCEL
	};
	static DialogDataEx CfgDlgData[]=
	{
		/* 00 */DI_DOUBLEBOX, 3, 1,52,21,0,0,0,0,(const wchar_t *)MConfigPanelTitle,
		/* 01 */DI_CHECKBOX,  5, 2, 0, 2,1,0,0,0,(const wchar_t *)MConfigHidden,
		/* 02 */DI_CHECKBOX,  5, 3, 0, 3,0,0,0,0,(const wchar_t *)MConfigHighlight,
		/* 03 */DI_CHECKBOX,  5, 4, 0, 4,0,0,0,0,(const wchar_t *)MConfigAutoChange,
		/* 04 */DI_CHECKBOX,  5, 5, 0, 5,0,0,0,0,(const wchar_t *)MConfigSelectFolders,
		/* 05 */DI_CHECKBOX,  5, 6, 0, 6,0,0,0,0,(const wchar_t *)MConfigSortFolderExt,
		/* 06 */DI_CHECKBOX,  5, 7, 0, 7,0,0,0,0,(const wchar_t *)MConfigReverseSort,
		/* 07 */DI_CHECKBOX,  5, 8, 0, 8,0,0,DIF_AUTOMATION,0,(const wchar_t *)MConfigAutoUpdateLimit,
		/* 08 */DI_TEXT,      9, 9, 0, 9,0,0,0,0,(const wchar_t *)MConfigAutoUpdateLimit2,
		/* 09 */DI_EDIT,      9, 9,15, 9,0,0,0,0,L"",
		/* 10 */DI_CHECKBOX,  5,10, 0,10,0,0,0,0,(const wchar_t *)MConfigAutoUpdateRemoteDrive,
		/* 11 */DI_TEXT,      3,11, 0,11,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,L"",
		/* 12 */DI_CHECKBOX,  5,12, 0,12,0,0,0,0,(const wchar_t *)MConfigShowColumns,
		/* 13 */DI_CHECKBOX,  5,13, 0,13,0,0,0,0,(const wchar_t *)MConfigShowStatus,
		/* 14 */DI_CHECKBOX,  5,14, 0,14,0,0,0,0,(const wchar_t *)MConfigShowTotal,
		/* 15 */DI_CHECKBOX,  5,15, 0,15,0,0,0,0,(const wchar_t *)MConfigShowFree,
		/* 16 */DI_CHECKBOX,  5,16, 0,16,0,0,0,0,(const wchar_t *)MConfigShowScrollbar,
		/* 17 */DI_CHECKBOX,  5,17, 0,17,0,0,0,0,(const wchar_t *)MConfigShowScreensNumber,
		/* 18 */DI_CHECKBOX,  5,18, 0,18,0,0,0,0,(const wchar_t *)MConfigShowSortMode,
		/* 19 */DI_TEXT,      3,19, 0,19,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,L"",
		/* 20 */DI_BUTTON,    0,20, 0,20,0,0,DIF_CENTERGROUP,1,(const wchar_t *)MOk,
		/* 21 */DI_BUTTON,    0,20, 0,20,0,0,DIF_CENTERGROUP,0,(const wchar_t *)MCancel
	};
	MakeDialogItemsEx(CfgDlgData,CfgDlg);
	CfgDlg[DLG_PANEL_HIDDEN].Selected=Opt.ShowHidden;
	CfgDlg[DLG_PANEL_HIGHLIGHT].Selected=Opt.Highlight;
	CfgDlg[DLG_PANEL_CHANGEFOLDER].Selected=Opt.Tree.AutoChangeFolder;
	CfgDlg[DLG_PANEL_SELECTFOLDERS].Selected=Opt.SelectFolders;
	CfgDlg[DLG_PANEL_SORTFOLDEREXT].Selected=Opt.SortFolderExt;
	CfgDlg[DLG_PANEL_REVERSESORT].Selected=Opt.ReverseSort;
	CfgDlg[DLG_PANEL_SHOWCOLUMNTITLES].Selected=Opt.ShowColumnTitles;
	CfgDlg[DLG_PANEL_SHOWPANELSTATUS].Selected=Opt.ShowPanelStatus;
	CfgDlg[DLG_PANEL_SHOWPANELTOTALS].Selected=Opt.ShowPanelTotals;
	CfgDlg[DLG_PANEL_SHOWPANELFREE].Selected=Opt.ShowPanelFree;
	CfgDlg[DLG_PANEL_SHOWPANELSCROLLBAR].Selected=Opt.ShowPanelScrollbar;
	CfgDlg[DLG_PANEL_SHOWSCREENSNUMBER].Selected=Opt.ShowScreensNumber;
	CfgDlg[DLG_PANEL_SHOWSORTMODE].Selected=Opt.ShowSortMode;
	CfgDlg[DLG_PANEL_AUTOUPDATELIMITVAL].X1+=StrLength(MSG(MConfigAutoUpdateLimit2))+1;
	CfgDlg[DLG_PANEL_AUTOUPDATELIMITVAL].X2+=StrLength(MSG(MConfigAutoUpdateLimit2))+1;
	CfgDlg[DLG_PANEL_AUTOUPDATELIMIT].Selected=Opt.AutoUpdateLimit!=0;
	CfgDlg[DLG_PANEL_AUTOUPDATEREMOTE].Selected=Opt.AutoUpdateRemoteDrive;
	CfgDlg[DLG_PANEL_AUTOUPDATELIMITVAL].strData.Format(L"%u", Opt.AutoUpdateLimit);;

	if (Opt.AutoUpdateLimit==0)
		CfgDlg[DLG_PANEL_AUTOUPDATELIMITVAL].Flags|=DIF_DISABLE;

	{
		Dialog Dlg(CfgDlg,countof(CfgDlg));
		Dlg.SetHelp(L"PanelSettings");
		Dlg.SetPosition(-1,-1,56,23);
		Dlg.SetAutomation(DLG_PANEL_AUTOUPDATELIMIT,DLG_PANEL_AUTOUPDATELIMITVAL,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
		Dlg.Process();

		if (Dlg.GetExitCode() != DLG_PANEL_OK)
			return;
	}
	Opt.ShowHidden=CfgDlg[DLG_PANEL_HIDDEN].Selected;
	Opt.Highlight=CfgDlg[DLG_PANEL_HIGHLIGHT].Selected;
	Opt.Tree.AutoChangeFolder=CfgDlg[DLG_PANEL_CHANGEFOLDER].Selected;
	Opt.SelectFolders=CfgDlg[DLG_PANEL_SELECTFOLDERS].Selected;
	Opt.SortFolderExt=CfgDlg[DLG_PANEL_SORTFOLDEREXT].Selected;
	Opt.ReverseSort=CfgDlg[DLG_PANEL_REVERSESORT].Selected;

	if (!CfgDlg[DLG_PANEL_AUTOUPDATELIMIT].Selected)
		Opt.AutoUpdateLimit=0;
	else
	{
		wchar_t *endptr;
		Opt.AutoUpdateLimit=wcstoul(CfgDlg[DLG_PANEL_AUTOUPDATELIMITVAL].strData, &endptr, 10);
	}

	Opt.ShowColumnTitles=CfgDlg[DLG_PANEL_SHOWCOLUMNTITLES].Selected;
	Opt.ShowPanelStatus=CfgDlg[DLG_PANEL_SHOWPANELSTATUS].Selected;
	Opt.ShowPanelTotals=CfgDlg[DLG_PANEL_SHOWPANELTOTALS].Selected;
	Opt.ShowPanelFree=CfgDlg[DLG_PANEL_SHOWPANELFREE].Selected;
	Opt.ShowPanelScrollbar=CfgDlg[DLG_PANEL_SHOWPANELSCROLLBAR].Selected;
	Opt.ShowScreensNumber=CfgDlg[DLG_PANEL_SHOWSCREENSNUMBER].Selected;
	Opt.ShowSortMode=CfgDlg[DLG_PANEL_SHOWSORTMODE].Selected;
	Opt.AutoUpdateRemoteDrive=CfgDlg[DLG_PANEL_AUTOUPDATEREMOTE].Selected;
//  FrameManager->RefreshFrame();
	CtrlObject->Cp()->LeftPanel->Update(UPDATE_KEEP_SELECTION);
	CtrlObject->Cp()->RightPanel->Update(UPDATE_KEEP_SELECTION);
	CtrlObject->Cp()->Redraw();
}


/* $ 17.12.2001 IS
   ��������� ������� ������ ���� ��� �������. ������� ���� ����, ����� ����
   ��������� � ����������� ������ �� ���������������� ����.
*/
void InterfaceSettings()
{
	enum enumInterfaceSettings
	{
		DLG_INTERF_TITLE,
		DLG_INTERF_CLOCK,
		DLG_INTERF_VIEWEREDITORCLOCK,
		DLG_INTERF_MOUSE,
		DLG_INTERF_SHOWKEYBAR,
		DLG_INTERF_SHOWMENUBAR,
		DLG_INTERF_SCREENSAVER,
		DLG_INTERF_SCREENSAVERTIME,
		DLG_INTERF_SAVERMINUTES,
		DLG_INTERF_COPYSHOWTOTAL,
		DLG_INTERF_COPYTIMERULE,
		DLG_INTERF_DELSHOWTOTAL,
		DLG_INTERF_PGUPCHANGEDISK,
		DLG_INTERF_CLEARTYPE,
		DLG_INTERF_TITLEADDONS_TITLE,
		DLG_INTERF_TITLEADDONS,
		DLG_INTERF_SEPARATOR,
		DLG_INTERF_OK,
		DLG_INTERF_CANCEL
	};
	DialogDataEx CfgDlgData[]=
	{
		DI_DOUBLEBOX,3, 1,54,18,0,0,0,0,MSG(MConfigInterfaceTitle),
		DI_CHECKBOX, 5, 2, 0, 2,1,Opt.Clock,0,0,MSG(MConfigClock),
		DI_CHECKBOX, 5, 3, 0, 3,0,Opt.ViewerEditorClock,0,0,MSG(MConfigViewerEditorClock),
		DI_CHECKBOX, 5, 4, 0, 4,0,Opt.Mouse,DIF_AUTOMATION,0,MSG(MConfigMouse),
		DI_CHECKBOX, 5, 5, 0, 5,0,Opt.ShowKeyBar,0,0,MSG(MConfigKeyBar),
		DI_CHECKBOX, 5, 6, 0, 6,0,Opt.ShowMenuBar,0,0,MSG(MConfigMenuBar),
		DI_CHECKBOX, 5, 7, 0, 7,0,Opt.ScreenSaver,DIF_AUTOMATION,0,MSG(MConfigSaver),
		DI_FIXEDIT,  9, 8,11, 8,0,0,!Opt.ScreenSaver?DIF_DISABLE:0,0,L"",
		DI_TEXT,    13, 8, 0, 8,0,0,!Opt.ScreenSaver?DIF_DISABLE:0,0,MSG(MConfigSaverMinutes),
		DI_CHECKBOX, 5, 9, 0, 9,0,Opt.CMOpt.CopyShowTotal,0,0,MSG(MConfigCopyTotal),
		DI_CHECKBOX, 5,10, 0,10,0,Opt.CMOpt.CopyTimeRule,0,0,MSG(MConfigCopyTimeRule),
		DI_CHECKBOX, 5,11, 0,11,0,Opt.DelOpt.DelShowTotal,0,0,MSG(MConfigDeleteTotal),
		DI_CHECKBOX, 5,12, 0,12,0,Opt.PgUpChangeDisk,0,0,MSG(MConfigPgUpChangeDisk),
		DI_CHECKBOX, 5,13, 0,13,0,Opt.ClearType,0,0,MSG(MConfigClearType),
		DI_TEXT,     5,14, 0,14,0,0,0,0,MSG(MConfigTitleAddons),
		DI_EDIT,     5,15,52,15,0,0,0,0,Opt.strTitleAddons,
		DI_TEXT,     3,16, 0,16,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,L"",
		DI_BUTTON,   0,17, 0,17,0,0,DIF_CENTERGROUP,1,MSG(MOk),
		DI_BUTTON,   0,17, 0,17,0,0,DIF_CENTERGROUP,0,MSG(MCancel),
	};
	MakeDialogItemsEx(CfgDlgData,CfgDlg);

	CfgDlg[DLG_INTERF_SCREENSAVERTIME].strData.Format(L"%u", Opt.ScreenSaverTime);

	{
		Dialog Dlg(CfgDlg,countof(CfgDlg));
		Dlg.SetHelp(L"InterfSettings");
		Dlg.SetPosition(-1,-1,58,20);
		Dlg.SetAutomation(DLG_INTERF_SCREENSAVER,DLG_INTERF_SCREENSAVERTIME,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
		Dlg.SetAutomation(DLG_INTERF_SCREENSAVER,DLG_INTERF_SAVERMINUTES,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
		Dlg.Process();

		if (Dlg.GetExitCode() != DLG_INTERF_OK)
			return;
	}

	Opt.Clock=CfgDlg[DLG_INTERF_CLOCK].Selected;
	Opt.ViewerEditorClock=CfgDlg[DLG_INTERF_VIEWEREDITORCLOCK].Selected;
	Opt.Mouse=CfgDlg[DLG_INTERF_MOUSE].Selected;
	Opt.ShowKeyBar=CfgDlg[DLG_INTERF_SHOWKEYBAR].Selected;
	Opt.ShowMenuBar=CfgDlg[DLG_INTERF_SHOWMENUBAR].Selected;
	Opt.ScreenSaver=CfgDlg[DLG_INTERF_SCREENSAVER].Selected;
	wchar_t *endptr;
	Opt.ScreenSaverTime=wcstoul(CfgDlg[DLG_INTERF_SCREENSAVERTIME].strData, &endptr, 10);
	Opt.CMOpt.CopyShowTotal=CfgDlg[DLG_INTERF_COPYSHOWTOTAL].Selected;
	Opt.DelOpt.DelShowTotal=CfgDlg[DLG_INTERF_DELSHOWTOTAL].Selected==BSTATE_CHECKED;
	Opt.PgUpChangeDisk=CfgDlg[DLG_INTERF_PGUPCHANGEDISK].Selected;
	Opt.CMOpt.CopyTimeRule=0;

	if (CfgDlg[DLG_INTERF_COPYTIMERULE].Selected)
		Opt.CMOpt.CopyTimeRule=3;

	Opt.ClearType=CfgDlg[DLG_INTERF_CLEARTYPE].Selected;
	Opt.strTitleAddons=CfgDlg[DLG_INTERF_TITLEADDONS].strData;

	SetFarConsoleMode();
	CtrlObject->Cp()->LeftPanel->Update(UPDATE_KEEP_SELECTION);
	CtrlObject->Cp()->RightPanel->Update(UPDATE_KEEP_SELECTION);
	CtrlObject->Cp()->SetScreenPosition();
	// $ 10.07.2001 SKV ! ���� ��� ������, ����� ���� ������ ��������, ����� ������ ����.
	CtrlObject->Cp()->Redraw();
}

void InfoPanelSettings()
{
	enum enumInfoPanelSettings
	{
		DLG_INFOPANEL_TITLE,
		DLG_INFOPANEL_USERNAMETITLE,
		DLG_INFOPANEL_USERNAMELIST,
		DLG_INFOPANEL_SEPARATOR,
		DLG_INFOPANEL_OK,
		DLG_INFOPANEL_CANCEL
	};
	static DialogDataEx CfgDlgData[]=
	{
		/* 00 */DI_DOUBLEBOX,3, 1,58, 6,0,0,0,0,(const wchar_t *)MConfigInfoPanelTitle,
		/* 01 */DI_TEXT,     5, 2, 0, 2,0,0,0,0,(const wchar_t *)MConfigInfoPanelUNTitle,
		/* 02 */DI_COMBOBOX, 5, 3,56, 3,1,0,DIF_DROPDOWNLIST|DIF_LISTAUTOHIGHLIGHT|DIF_LISTWRAPMODE,0,L"",
		/* 03 */DI_TEXT,     3, 4, 0, 4,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,L"",
		/* 04 */DI_BUTTON,   0, 5, 0, 5,0,0,DIF_CENTERGROUP,1,(const wchar_t *)MOk,
		/* 05 */DI_BUTTON,   0, 5, 0, 5,0,0,DIF_CENTERGROUP,0,(const wchar_t *)MCancel
	};
	MakeDialogItemsEx(CfgDlgData,CfgDlg);
	struct
	{
		int MsgFormat;
		EXTENDED_NAME_FORMAT TypeFormat;
	} ExtendedNameFormat[]=
	{
		{ MConfigInfoPanelUNUnknown, NameUnknown },                            // 0  - unknown name type
		{ MConfigInfoPanelUNFullyQualifiedDN, NameFullyQualifiedDN },          // 1  - CN=John Doe, OU=Software, OU=Engineering, O=Widget, C=US
		{ MConfigInfoPanelUNSamCompatible, NameSamCompatible },                // 2  - Engineering\JohnDoe, If the user account is not in a domain, only NameSamCompatible is supported.
		{ MConfigInfoPanelUNDisplay, NameDisplay },                            // 3  - Probably "John Doe" but could be something else.  I.e. The display name is not necessarily the defining RDN.
		{ MConfigInfoPanelUNUniqueId, NameUniqueId },                          // 6  - String-ized GUID as returned by IIDFromString(). eg: {4fa050f0-f561-11cf-bdd9-00aa003a77b6}
		{ MConfigInfoPanelUNCanonical, NameCanonical },                        // 7  - engineering.widget.com/software/John Doe
		{ MConfigInfoPanelUNUserPrincipal, NameUserPrincipal },                // 8  - someone@example.com
		{ MConfigInfoPanelUNServicePrincipal, NameServicePrincipal },          // 10 - www/srv.engineering.com/engineering.com
		{ MConfigInfoPanelUNDnsDomain, NameDnsDomain },                        // 12 - DNS domain name + SAM username eg: engineering.widget.com\JohnDoe
	};
	/* TODO:
		COMPUTER_NAME_FORMAT:
			ComputerNameNetBIOS
				The NetBIOS name of the local computer or the cluster associated with the local computer. This name is limited to MAX_COMPUTERNAME_LENGTH + 1 characters and may be a truncated version of the DNS host name. For example, if the DNS host name is "corporate-mail-server", the NetBIOS name would be "corporate-mail-".
			ComputerNameDnsHostname
				The DNS name of the local computer or the cluster associated with the local computer.
			ComputerNameDnsDomain
				The name of the DNS domain assigned to the local computer or the cluster associated with the local computer.
			ComputerNameDnsFullyQualified
				The fully-qualified DNS name that uniquely identifies the local computer or the cluster associated with the local computer.
				This name is a combination of the DNS host name and the DNS domain name, using the form HostName.DomainName. For example, if the DNS host name is "corporate-mail-server" and the DNS domain name is "microsoft.com", the fully qualified DNS name is "corporate-mail-server.microsoft.com".
			ComputerNamePhysicalNetBIOS
				The NetBIOS name of the local computer. On a cluster, this is the NetBIOS name of the local node on the cluster.
			ComputerNamePhysicalDnsHostname
				The DNS host name of the local computer. On a cluster, this is the DNS host name of the local node on the cluster.
			ComputerNamePhysicalDnsDomain
				The name of the DNS domain assigned to the local computer. On a cluster, this is the DNS domain of the local node on the cluster.
			ComputerNamePhysicalDnsFullyQualified
				The fully-qualified DNS name that uniquely identifies the computer. On a cluster, this is the fully qualified DNS name of the local node on the cluster. The fully qualified DNS name is a combination of the DNS host name and the DNS domain name, using the form HostName.DomainName.
	*/
	FarListItem UserNameListItems[countof(ExtendedNameFormat)]={0};
	FarList UserNameList = {countof(UserNameListItems),UserNameListItems};
	CfgDlg[DLG_INFOPANEL_USERNAMELIST].ListItems = &UserNameList;

	for (size_t I=0; I < countof(ExtendedNameFormat); ++I)
	{
		UserNameListItems[I].Text=MSG(ExtendedNameFormat[I].MsgFormat);

		if (Opt.InfoPanel.UserNameFormat == ExtendedNameFormat[I].TypeFormat)
			UserNameListItems[I].Flags|=LIF_SELECTED;
	}

	{
		Dialog Dlg((DialogItemEx*)CfgDlg,countof(CfgDlg));
		Dlg.SetHelp(L"InfoPanelSettings");
		Dlg.SetPosition(-1,-1,62,8);
		Dlg.Process();

		if (Dlg.GetExitCode() != DLG_INFOPANEL_OK)
			return;
	}

	Opt.InfoPanel.UserNameFormat=ExtendedNameFormat[CfgDlg[DLG_INFOPANEL_USERNAMELIST].ListPos].TypeFormat;
}

void DialogSettings()
{
	enum enumDialogSettings
	{
		DLG_DIALOGS_TITLE,
		DLG_DIALOGS_DIALOGSEDITHISTORY,
		DLG_DIALOGS_DIALOGSEDITBLOCK,
		DLG_DIALOGS_DIALOGDELREMOVESBLOCKS,
		DLG_DIALOGS_AUTOCOMPLETE,
		DLG_DIALOGS_EULBSCLEAR,
		DLG_DIALOGS_MOUSEBUTTON,
		DLG_DIALOGS_SEPARATOR,
		DLG_DIALOGS_OK,
		DLG_DIALOGS_CANCEL
	};
	static DialogDataEx CfgDlgData[]=
	{
		/* 00 */DI_DOUBLEBOX,3, 1,54,10,0,0,0,0,(const wchar_t *)MConfigDlgSetsTitle,
		/* 01 */DI_CHECKBOX, 5, 2, 0, 2,0,0,0,0,(const wchar_t *)MConfigDialogsEditHistory,
		/* 02 */DI_CHECKBOX, 5, 3, 0, 3,0,0,0,0,(const wchar_t *)MConfigDialogsEditBlock,
		/* 03 */DI_CHECKBOX, 5, 4, 0, 4,0,0,0,0,(const wchar_t *)MConfigDialogsDelRemovesBlocks,
		/* 04 */DI_CHECKBOX, 5, 5, 0, 5,0,0,0,0,(const wchar_t *)MConfigDialogsAutoComplete,
		/* 05 */DI_CHECKBOX, 5, 6, 0, 6,0,0,0,0,(const wchar_t *)MConfigDialogsEULBsClear,
		/* 06 */DI_CHECKBOX, 5, 7, 0, 7,0,0,0,0,(const wchar_t *)MConfigDialogsMouseButton,
		/* 07 */DI_TEXT,     3, 8, 0, 8,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,L"",
		/* 08 */DI_BUTTON,   0, 9, 0, 9,0,0,DIF_CENTERGROUP,1,(const wchar_t *)MOk,
		/* 09 */DI_BUTTON,   0, 9, 0, 9,0,0,DIF_CENTERGROUP,0,(const wchar_t *)MCancel
	};
	MakeDialogItemsEx(CfgDlgData,CfgDlg);
	CfgDlg[DLG_DIALOGS_DIALOGSEDITHISTORY].Selected=Opt.Dialogs.EditHistory;
	CfgDlg[DLG_DIALOGS_DIALOGSEDITBLOCK].Selected=Opt.Dialogs.EditBlock;
	CfgDlg[DLG_DIALOGS_DIALOGDELREMOVESBLOCKS].Selected=Opt.Dialogs.DelRemovesBlocks;
	CfgDlg[DLG_DIALOGS_AUTOCOMPLETE].Selected=Opt.Dialogs.AutoComplete;
	CfgDlg[DLG_DIALOGS_EULBSCLEAR].Selected=Opt.Dialogs.EULBsClear;
	CfgDlg[DLG_DIALOGS_MOUSEBUTTON].Selected=Opt.Dialogs.MouseButton;
	{
		Dialog Dlg((DialogItemEx*)CfgDlg,countof(CfgDlg));
		Dlg.SetHelp(L"DialogSettings");
		Dlg.SetPosition(-1,-1,58,12);
		Dlg.Process();

		if (Dlg.GetExitCode() != DLG_DIALOGS_OK)
			return;
	}
	Opt.Dialogs.EditHistory=CfgDlg[DLG_DIALOGS_DIALOGSEDITHISTORY].Selected;
	Opt.Dialogs.EditBlock=CfgDlg[DLG_DIALOGS_DIALOGSEDITBLOCK].Selected;
	Opt.Dialogs.DelRemovesBlocks=CfgDlg[DLG_DIALOGS_DIALOGDELREMOVESBLOCKS].Selected;
	Opt.Dialogs.AutoComplete=CfgDlg[DLG_DIALOGS_AUTOCOMPLETE].Selected;
	Opt.Dialogs.EULBsClear=CfgDlg[DLG_DIALOGS_EULBSCLEAR].Selected;

	if ((Opt.Dialogs.MouseButton=CfgDlg[DLG_DIALOGS_MOUSEBUTTON].Selected) != 0)
		Opt.Dialogs.MouseButton=0xFFFF;
}

void CmdlineSettings()
{
	enum enumCmdlineSettings
	{
		DLG_CMDLINE_TITLE,
		DLG_CMDLINE_DIALOGSEDITBLOCK,
		DLG_CMDLINE_DIALOGDELREMOVESBLOCKS,
		DLG_CMDLINE_AUTOCOMPLETE,
		DLG_CMDLINE_USEPROMPTFORMAT,
		DLG_CMDLINE_PROMPTFORMAT,
		DLG_CMDLINE_SEPARATOR,
		DLG_CMDLINE_OK,
		DLG_CMDLINE_CANCEL
	};
	DialogDataEx CfgDlgData[]=
	{
		DI_DOUBLEBOX,3, 1,54, 9,0,0,0,0,MSG(MConfigCmdlineTitle),
		DI_CHECKBOX, 5, 2, 0, 2,0,Opt.CmdLine.EditBlock,0,0,MSG(MConfigCmdlineEditBlock),
		DI_CHECKBOX, 5, 3, 0, 3,0,Opt.CmdLine.DelRemovesBlocks,0,0,MSG(MConfigCmdlineDelRemovesBlocks),
		DI_CHECKBOX, 5, 4, 0, 4,0,Opt.CmdLine.AutoComplete,0,0,MSG(MConfigCmdlineAutoComplete),
		DI_CHECKBOX, 5, 5, 0, 5,0,Opt.CmdLine.UsePromptFormat,DIF_AUTOMATION,0,MSG(MConfigUsePromptFormat),
		DI_EDIT,     9, 6,24, 6,0,0,!Opt.CmdLine.UsePromptFormat?DIF_DISABLE:0,0,Opt.CmdLine.strPromptFormat,
		DI_TEXT,     3, 7, 0, 7,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,L"",
		DI_BUTTON,   0, 8, 0, 8,0,0,DIF_CENTERGROUP,1,MSG(MOk),
		DI_BUTTON,   0, 8, 0, 8,0,0,DIF_CENTERGROUP,0,MSG(MCancel),
	};
	MakeDialogItemsEx(CfgDlgData,CfgDlg);
	Dialog Dlg(CfgDlg,countof(CfgDlg));
	Dlg.SetHelp(L"CmdlineSettings");
	Dlg.SetPosition(-1,-1,58,11);
	Dlg.SetAutomation(DLG_CMDLINE_USEPROMPTFORMAT,DLG_CMDLINE_PROMPTFORMAT,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.Process();

	if (Dlg.GetExitCode()==DLG_CMDLINE_OK)
	{
		Opt.CmdLine.EditBlock=(CfgDlg[DLG_CMDLINE_DIALOGSEDITBLOCK].Selected==BSTATE_CHECKED);
		Opt.CmdLine.DelRemovesBlocks=(CfgDlg[DLG_CMDLINE_DIALOGDELREMOVESBLOCKS].Selected==BSTATE_CHECKED);
		Opt.CmdLine.AutoComplete=(CfgDlg[DLG_CMDLINE_AUTOCOMPLETE].Selected==BST_CHECKED);
		Opt.CmdLine.UsePromptFormat=(CfgDlg[DLG_CMDLINE_USEPROMPTFORMAT].Selected==BSTATE_CHECKED);
		Opt.CmdLine.strPromptFormat=CfgDlg[DLG_CMDLINE_PROMPTFORMAT].strData;
		CtrlObject->CmdLine->SetPersistentBlocks(Opt.CmdLine.EditBlock);
		CtrlObject->CmdLine->SetDelRemovesBlocks(Opt.CmdLine.DelRemovesBlocks);
	}
}

void SetConfirmations()
{
	enum ConfirmationsDlg
	{
		CF_DOUBLEBOX,
		CF_CHECKBOX_COPY,
		CF_CHECKBOX_MOVE,
		CF_CHECKBOX_RO,
		CF_CHECKBOX_DRAG,
		CF_CHECKBOX_DELETE,
		CF_CHECKBOX_DELETE_DIR,
		CF_CHECKBOX_ESC,
		CF_CHECKBOX_DISCONNECT,
		CF_CHECKBOX_SUBST,
		CF_CHECKBOX_HOTPLUG,
		CF_CHECKBOX_REEDIT,
		CF_CHECKBOX_HISTORY,
		CF_CHECKBOX_EXIT,
		CF_CHECKBOX_SEPARATOR,
		CF_CHECKBOX_OK,
		CF_CHECKBOX_CANCEL,
	};
	enum DlgCoord
	{
		DLG_X=50,
		DLG_Y=19,
	};
	DialogDataEx ConfDlgData[]=
	{
		DI_DOUBLEBOX,3, 1,DLG_X-4,DLG_Y-2,0,0,0,0,MSG(MSetConfirmTitle),
		DI_CHECKBOX, 5, 2, 0, 2,1,Opt.Confirm.Copy,0,0,MSG(MSetConfirmCopy),
		DI_CHECKBOX, 5, 3, 0, 3,0,Opt.Confirm.Move,0,0,MSG(MSetConfirmMove),
		DI_CHECKBOX, 5, 4, 0, 4,0,Opt.Confirm.RO,0,0,MSG(MSetConfirmRO),
		DI_CHECKBOX, 5, 5, 0, 5,0,Opt.Confirm.Drag,0,0,MSG(MSetConfirmDrag),
		DI_CHECKBOX, 5, 6, 0, 6,0,Opt.Confirm.Delete,0,0,MSG(MSetConfirmDelete),
		DI_CHECKBOX, 5, 7, 0, 7,0,Opt.Confirm.DeleteFolder,0,0,MSG(MSetConfirmDeleteFolders),
		DI_CHECKBOX, 5, 8, 0, 8,0,Opt.Confirm.Esc,0,0,MSG(MSetConfirmEsc),
		DI_CHECKBOX, 5, 9, 0, 9,0,Opt.Confirm.RemoveConnection,0,0,MSG(MSetConfirmRemoveConnection),
		DI_CHECKBOX, 5,10, 0,10,0,Opt.Confirm.RemoveSUBST,0,0,MSG(MSetConfirmRemoveSUBST),
		DI_CHECKBOX, 5,11, 0,11,0,Opt.Confirm.RemoveHotPlug,0,0,MSG(MSetConfirmRemoveHotPlug),
		DI_CHECKBOX, 5,12, 0,12,0,Opt.Confirm.AllowReedit,0,0,MSG(MSetConfirmAllowReedit),
		DI_CHECKBOX, 5,13, 0,13,0,Opt.Confirm.HistoryClear,0,0,MSG(MSetConfirmHistoryClear),
		DI_CHECKBOX, 5,14, 0,14,0,Opt.Confirm.Exit,0,0,MSG(MSetConfirmExit),
		DI_TEXT,     3,DLG_Y-4, 0,DLG_Y-4,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,L"",
		DI_BUTTON,   0,DLG_Y-3, 0,DLG_Y-3,0,0,DIF_CENTERGROUP,1,MSG(MOk),
		DI_BUTTON,   0,DLG_Y-3, 0,DLG_Y-3,0,0,DIF_CENTERGROUP,0,MSG(MCancel),
	};
	MakeDialogItemsEx(ConfDlgData,ConfDlg);
	Dialog Dlg(ConfDlg,countof(ConfDlg));
	Dlg.SetHelp(L"ConfirmDlg");
	Dlg.SetPosition(-1,-1,DLG_X,DLG_Y);
	Dlg.Process();

	if (Dlg.GetExitCode()==CF_CHECKBOX_OK)
	{
		Opt.Confirm.Copy=ConfDlg[CF_CHECKBOX_COPY].Selected;
		Opt.Confirm.Move=ConfDlg[CF_CHECKBOX_MOVE].Selected;
		Opt.Confirm.RO=ConfDlg[CF_CHECKBOX_RO].Selected;
		Opt.Confirm.Drag=ConfDlg[CF_CHECKBOX_DRAG].Selected;
		Opt.Confirm.Delete=ConfDlg[CF_CHECKBOX_DELETE].Selected;
		Opt.Confirm.DeleteFolder=ConfDlg[CF_CHECKBOX_DELETE_DIR].Selected;
		Opt.Confirm.Esc=ConfDlg[CF_CHECKBOX_ESC].Selected;
		Opt.Confirm.RemoveConnection=ConfDlg[CF_CHECKBOX_DISCONNECT].Selected;
		Opt.Confirm.RemoveSUBST=ConfDlg[CF_CHECKBOX_SUBST].Selected;
		Opt.Confirm.RemoveHotPlug=ConfDlg[CF_CHECKBOX_HOTPLUG].Selected;
		Opt.Confirm.AllowReedit=ConfDlg[CF_CHECKBOX_REEDIT].Selected;
		Opt.Confirm.HistoryClear=ConfDlg[CF_CHECKBOX_HISTORY].Selected;
		Opt.Confirm.Exit=ConfDlg[CF_CHECKBOX_EXIT].Selected;
	}
}

void SetPluginConfirmations()
{
	enum PluginConfirmationDlg
	{
		PC_DOUBLEBOX,
		PC_CHECKBOX_OFP,
		PC_CHECKBOX_STDASSOC,
		PC_CHECKBOX_EVENONE,
		PC_CHECKBOX_SFL,
		PC_CHECKBOX_PF,
		PC_SEPARATOR,
		PC_BUTTON_OK,
		PC_BUTTON_CANCEL,
	};
	const int DlgX=54,DlgY=11;
	DialogDataEx ConfDlgData[]=
	{
		DI_DOUBLEBOX, 3, 1,DlgX-4,DlgY-2,0,0,0,0,MSG(MSetPluginConfirmationTitle),
		DI_CHECKBOX,  5, 2, 0, 2,1,Opt.PluginConfirm.OpenFilePlugin,DIF_AUTOMATION,0,MSG(MSetPluginConfirmationOFP),
		DI_CHECKBOX,  7, 3, 0, 3,0,Opt.PluginConfirm.StandardAssociation,DIF_AUTOMATION|(Opt.PluginConfirm.OpenFilePlugin?0:DIF_DISABLE),0,MSG(MSetPluginConfirmationStdAssoc),
		DI_CHECKBOX,  7, 4, 0, 4,0,Opt.PluginConfirm.EvenIfOnlyOnePlugin,(Opt.PluginConfirm.OpenFilePlugin?0:DIF_DISABLE),0,MSG(MSetPluginConfirmationEvenOne),
		DI_CHECKBOX,  5, 5, 0, 5,0,Opt.PluginConfirm.SetFindList,0,0,MSG(MSetPluginConfirmationSFL),
		DI_CHECKBOX,  5, 6, 0, 6,0,Opt.PluginConfirm.Prefix,0,0,MSG(MSetPluginConfirmationPF),
		DI_TEXT,      3,DlgY-4, 0,DlgY-4,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,L"",
		DI_BUTTON,    0,DlgY-3, 0,DlgY-3,0,0,DIF_CENTERGROUP,1,MSG(MOk),
		DI_BUTTON,    0,DlgY-3, 0,DlgY-3,0,0,DIF_CENTERGROUP,0,MSG(MCancel),
	};
	MakeDialogItemsEx(ConfDlgData, ConfDlg);
	Dialog Dlg(ConfDlg,countof(ConfDlg));
	Dlg.SetHelp(L"ChoosePluginDlg");
	Dlg.SetPosition(-1,-1,DlgX,DlgY);
	Dlg.SetAutomation(PC_CHECKBOX_OFP,PC_CHECKBOX_STDASSOC,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.SetAutomation(PC_CHECKBOX_OFP,PC_CHECKBOX_EVENONE,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
	Dlg.Process();

	if (Dlg.GetExitCode()==PC_BUTTON_OK)
	{
		Opt.PluginConfirm.OpenFilePlugin = ConfDlg[PC_CHECKBOX_OFP].Selected;
		Opt.PluginConfirm.StandardAssociation = ConfDlg[PC_CHECKBOX_STDASSOC].Selected;
		Opt.PluginConfirm.EvenIfOnlyOnePlugin = ConfDlg[PC_CHECKBOX_EVENONE].Selected;
		Opt.PluginConfirm.SetFindList = ConfDlg[PC_CHECKBOX_SFL].Selected;
		Opt.PluginConfirm.Prefix = ConfDlg[PC_CHECKBOX_PF].Selected;
	}
}


void SetDizConfig()
{
	static DialogDataEx DizDlgData[]=
	{
		/* 00 */DI_DOUBLEBOX,3,1,72,17,0,0,0,0,(const wchar_t *)MCfgDizTitle,
		/* 01 */DI_TEXT,5,2,0,2,0,0,0,0,(const wchar_t *)MCfgDizListNames,
		/* 02 */DI_EDIT,5,3,70,3,1,0,0,0,L"",
		/* 03 */DI_TEXT,3,4,0,4,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,L"",
		/* 04 */DI_CHECKBOX,5,5,0,5,0,0,0,0,(const wchar_t *)MCfgDizSetHidden,
		/* 05 */DI_CHECKBOX,5,6,0,6,0,0,0,0,(const wchar_t *)MCfgDizROUpdate,
		/* 06 */DI_FIXEDIT,5,7,7,7,0,0,0,0,L"",
		/* 07 */DI_TEXT,9,7,0,7,0,0,0,0,(const wchar_t *)MCfgDizStartPos,
		/* 08 */DI_TEXT,3,8,0,8,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,L"",
		/* 09 */DI_RADIOBUTTON,5,9,0,9,0,0,DIF_GROUP,0,(const wchar_t *)MCfgDizNotUpdate,
		/* 10 */DI_RADIOBUTTON,5,10,0,10,0,0,0,0,(const wchar_t *)MCfgDizUpdateIfDisplayed,
		/* 11 */DI_RADIOBUTTON,5,11,0,11,0,0,0,0,(const wchar_t *)MCfgDizAlwaysUpdate,
		/* 12 */DI_TEXT,3,12,0,12,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,L"",
		/* 13 */DI_CHECKBOX,5,13,0,13,0,0,0,0,(const wchar_t *)MCfgDizAnsiByDefault,
		/* 14 */DI_CHECKBOX,5,14,0,14,0,0,0,0,(const wchar_t *)MCfgDizSaveInUTF,
		/* 15 */DI_TEXT,3,15,0,15,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,L"",
		/* 16 */DI_BUTTON,0,16,0,16,0,0,DIF_CENTERGROUP,1,(const wchar_t *)MOk,
		/* 17 */DI_BUTTON,0,16,0,16,0,0,DIF_CENTERGROUP,0,(const wchar_t *)MCancel
	};
	MakeDialogItemsEx(DizDlgData,DizDlg);
	DizDlg[2].strData=Opt.Diz.strListNames;
	DizDlg[4].Selected=Opt.Diz.SetHidden;
	DizDlg[5].Selected=Opt.Diz.ROUpdate;
	DizDlg[6].strData.Format(L"%d", Opt.Diz.StartPos);

	if (Opt.Diz.UpdateMode==DIZ_NOT_UPDATE)
	{
		DizDlg[9].Selected=TRUE;
	}
	else
	{
		if (Opt.Diz.UpdateMode==DIZ_UPDATE_IF_DISPLAYED)
			DizDlg[10].Selected=TRUE;
		else
			DizDlg[11].Selected=TRUE;
	}

	DizDlg[13].Selected=Opt.Diz.AnsiByDefault;
	DizDlg[14].Selected=Opt.Diz.SaveInUTF;
	Dialog Dlg((DialogItemEx*)DizDlg,countof(DizDlg));
	Dlg.SetPosition(-1,-1,76,19);
	Dlg.SetHelp(L"FileDiz");
	Dlg.Process();

	if (Dlg.GetExitCode()!=16)
		return;

	Opt.Diz.strListNames=DizDlg[2].strData;
	Opt.Diz.SetHidden=DizDlg[4].Selected;
	Opt.Diz.ROUpdate=DizDlg[5].Selected;
	Opt.Diz.StartPos=_wtoi(DizDlg[6].strData);

	if (DizDlg[9].Selected)
	{
		Opt.Diz.UpdateMode=DIZ_NOT_UPDATE;
	}
	else
	{
		if (DizDlg[10].Selected)
			Opt.Diz.UpdateMode=DIZ_UPDATE_IF_DISPLAYED;
		else
			Opt.Diz.UpdateMode=DIZ_UPDATE_ALWAYS;
	}

	Opt.Diz.AnsiByDefault=DizDlg[13].Selected;
	Opt.Diz.SaveInUTF=DizDlg[14].Selected;
}

void ViewerConfig(ViewerOptions &ViOpt,int Local)
{
	enum enumViewerConfig
	{
		ID_VC_TITLE,
		ID_VC_EXTERNALUSEF3,
		ID_VC_EXTENALCOMMAND,
		ID_VC_EXTERALCOMMANDEDIT,
		ID_VC_SEPARATOR1,
		ID_VC_PERSISTENTSELECTION,
		ID_VC_SHOWARROWS,
		ID_VC_SAVEPOSITION,
		ID_VC_SAVEBOOKMARKS,
		ID_VC_TABSIZE,
		ID_VC_TABSIZEEDIT,
		ID_VC_SHOWSCROLLBAR,
		ID_VC_AUTODETECTCODEPAGE,
		ID_VC_ANSIASDEFAULT,
		ID_VC_SEPARATOR2,
		ID_VC_OK,
		ID_VC_CANCEL
	};
	static DialogDataEx CfgDlgData[]=
	{
		/*  0 */  DI_DOUBLEBOX,  3, 1,70,14,0,0,0,0,(const wchar_t *)MViewConfigTitle,
		/*  1 */  DI_CHECKBOX,   5, 2, 0, 2,1,0,0,0,(const wchar_t *)MViewConfigExternalF3,
		/*  2 */  DI_TEXT,       5, 3, 0, 3,0,0,0,0,(const wchar_t *)MViewConfigExternalCommand,
		/*  3 */  DI_EDIT,       5, 4,68, 4,0,(DWORD_PTR)L"ExternalViewer", DIF_HISTORY,0,L"",
		/*  4 */  DI_TEXT,       0, 5, 0, 5, 0, 0, DIF_SEPARATOR, 0, (const wchar_t *)MViewConfigInternal,
		/*  5 */  DI_CHECKBOX,   5, 6, 0, 6,0,0,0,0,(const wchar_t *)MViewConfigPersistentSelection,
		/*  6 */  DI_CHECKBOX,  38, 6, 0, 6,0,0,0,0,(const wchar_t *)MViewConfigArrows,
		/*  7 */  DI_CHECKBOX,   5, 7, 0, 7,0,0,DIF_AUTOMATION,0,(const wchar_t *)MViewConfigSavePos,
		/*  8 */  DI_CHECKBOX,  38, 7, 0, 7,0,0,0,0,(const wchar_t *)MViewConfigSaveShortPos,
		/*  9 */  DI_TEXT,      10, 8, 0,8,0,0,0,0,(const wchar_t *)MViewConfigTabSize,
		/* 10 */  DI_FIXEDIT,    5, 8, 8,8,0,0,0,0,L"",
		/* 11 */  DI_CHECKBOX,  38, 8, 0,8,0,0,0,0,(const wchar_t *)MViewConfigScrollbar,
		/* 12 */  DI_CHECKBOX,   5,10, 0,10,0,0,0,0,(const wchar_t *)MViewAutoDetectCodePage,
		/* 13 */  DI_CHECKBOX,   5,11, 0,11,0,0,0,0,(const wchar_t *)MViewConfigAnsiCodePageAsDefault,
		/* 14 */  DI_TEXT,       0,12, 0,12, 0, 0, DIF_SEPARATOR, 0, L"",
		/* 15 */  DI_BUTTON,     0,13, 0,13,0,0,DIF_CENTERGROUP,1,(const wchar_t *)MOk,
		/* 16 */  DI_BUTTON,     0,13, 0,13,0,0,DIF_CENTERGROUP,0,(const wchar_t *)MCancel
	};
	MakeDialogItemsEx(CfgDlgData,CfgDlg);
	CfgDlg[ID_VC_EXTERNALUSEF3].Selected = Opt.ViOpt.UseExternalViewer;
	CfgDlg[ID_VC_SAVEPOSITION].Selected = Opt.ViOpt.SaveViewerPos;
	CfgDlg[ID_VC_SAVEBOOKMARKS].Selected = Opt.ViOpt.SaveViewerShortPos;

	if (!Opt.ViOpt.SaveViewerPos)
		CfgDlg[ID_VC_SAVEBOOKMARKS].Flags |= DIF_DISABLE;

	CfgDlg[ID_VC_AUTODETECTCODEPAGE].Selected = ViOpt.AutoDetectCodePage;
	CfgDlg[ID_VC_SHOWSCROLLBAR].Selected = ViOpt.ShowScrollbar;
	CfgDlg[ID_VC_SHOWARROWS].Selected = ViOpt.ShowArrows;
	CfgDlg[ID_VC_PERSISTENTSELECTION].Selected = ViOpt.PersistentBlocks;
	CfgDlg[ID_VC_ANSIASDEFAULT].Selected = ViOpt.AnsiCodePageAsDefault;
	CfgDlg[ID_VC_EXTERALCOMMANDEDIT].strData = Opt.strExternalViewer;
	CfgDlg[ID_VC_TABSIZEEDIT].strData.Format(L"%d",ViOpt.TabSize);
	int DialogHeight = 16;

	if (Local)
	{
		for (int i=ID_VC_EXTERNALUSEF3; i<=ID_VC_SEPARATOR1; i++)
			CfgDlg[i].Flags |= DIF_HIDDEN;

		for (int i=ID_VC_AUTODETECTCODEPAGE; i<ID_VC_SEPARATOR2; i++)
			CfgDlg[i].Flags |= DIF_HIDDEN;

		for (int i = ID_VC_PERSISTENTSELECTION; i < ID_VC_SEPARATOR2; i++)
			CfgDlg[i].Y1 -= 4;

		for (int i = ID_VC_SEPARATOR2; i <= ID_VC_CANCEL; i++)
			CfgDlg[i].Y1 -= 7;

		CfgDlg[ID_VC_TITLE].Y2 -= 7;
		DialogHeight -= 7;
	}

	{
		Dialog Dlg((DialogItemEx*)CfgDlg,countof(CfgDlg));
		Dlg.SetAutomation(ID_VC_SAVEPOSITION,ID_VC_SAVEBOOKMARKS,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
		Dlg.SetHelp(L"ViewerSettings");
		Dlg.SetPosition(-1,-1,74,DialogHeight);
		Dlg.Process();

		if (Dlg.GetExitCode() != ID_VC_OK)
			return;
	}

	if (!Local)
	{
		Opt.ViOpt.UseExternalViewer=CfgDlg[ID_VC_EXTERNALUSEF3].Selected;
		Opt.strExternalViewer = CfgDlg[ID_VC_EXTERALCOMMANDEDIT].strData;
	}

	Opt.ViOpt.SaveViewerPos=CfgDlg[ID_VC_SAVEPOSITION].Selected;
	Opt.ViOpt.SaveViewerShortPos=CfgDlg[ID_VC_SAVEBOOKMARKS].Selected;
	ViOpt.AutoDetectCodePage=CfgDlg[ID_VC_AUTODETECTCODEPAGE].Selected;
	ViOpt.TabSize=_wtoi(CfgDlg[ID_VC_TABSIZEEDIT].strData);
	ViOpt.ShowScrollbar=CfgDlg[ID_VC_SHOWSCROLLBAR].Selected;
	ViOpt.ShowArrows=CfgDlg[ID_VC_SHOWARROWS].Selected;
	ViOpt.PersistentBlocks=CfgDlg[ID_VC_PERSISTENTSELECTION].Selected;
	ViOpt.AnsiCodePageAsDefault=CfgDlg[ID_VC_ANSIASDEFAULT].Selected;

	if (ViOpt.TabSize<1 || ViOpt.TabSize>512)
		ViOpt.TabSize=8;
}

void EditorConfig(EditorOptions &EdOpt,int Local)
{
	enum enumEditorConfig
	{
		ID_EC_TITLE,
		ID_EC_EXTERNALUSEF4,
		ID_EC_EXTERNALCOMMAND,
		ID_EC_EXTERNALCOMMANDEDIT,
		ID_EC_SEPARATOR1,
		ID_EC_EXPANDTABSTITLE,
		ID_EC_EXPANDTABS,
		ID_EC_PERSISTENTBLOCKS,
		ID_EC_DELREMOVESBLOCKS,
		ID_EC_SAVEPOSITION,
		ID_EC_SAVEBOOKMARKS,
		ID_EC_AUTOINDENT,
		ID_EC_CURSORBEYONDEOL,
		ID_EC_TABSIZE,
		ID_EC_TABSIZEEDIT,
		ID_EC_SHOWSCROLLBAR,
		ID_EC_SHOWWHITESPACE,
		ID_EC_PICKUPWORD,
		ID_EC_SHARINGWRITE,
		ID_EC_LOCKREADONLY,
		ID_EC_READONLYWARNING,
		ID_EC_AUTODETECTCODEPAGE,
		ID_EC_ANSIASDEFAULT,
		ID_EC_ANSIFORNEWFILE,
		ID_EC_SEPARATOR2,
		ID_EC_OK,
		ID_EC_CANCEL
	};
	static DialogDataEx CfgDlgData[]=
	{
		DI_DOUBLEBOX, 3, 1,70,22,0,0,0,0,(const wchar_t *)MEditConfigTitle,
		DI_CHECKBOX,  5, 2, 0, 2,0,0,0,0,(const wchar_t *)MEditConfigEditorF4,
		DI_TEXT,      5, 3, 0, 3,0,0,0,0,(const wchar_t *)MEditConfigEditorCommand,
		DI_EDIT,      5, 4,68, 4,0,(DWORD_PTR)L"ExternalEditor",DIF_HISTORY,0,L"",
		DI_TEXT,      0, 5, 0, 5,0,0,DIF_SEPARATOR, 0, (const wchar_t *)MEditConfigInternal,
		DI_TEXT,      5, 6, 0, 6,0,0,0,0,(const wchar_t *)MEditConfigExpandTabsTitle,
		DI_COMBOBOX,  5, 7,68, 7,1,0,DIF_DROPDOWNLIST|DIF_LISTAUTOHIGHLIGHT|DIF_LISTWRAPMODE,0,L"",
		DI_CHECKBOX,  5, 8, 0, 8,0,0,0,0,(const wchar_t *)MEditConfigPersistentBlocks,
		DI_CHECKBOX, 38, 8, 0, 8,0,0,0,0,(const wchar_t *)MEditConfigDelRemovesBlocks,
		DI_CHECKBOX,  5, 9, 0, 9,0,0,DIF_AUTOMATION,0,(const wchar_t *)MEditConfigSavePos,
		DI_CHECKBOX, 38, 9, 0, 9,0,0,0,0,(const wchar_t *)MEditConfigSaveShortPos,
		DI_CHECKBOX,  5,10, 0,10,0,0,0,0,(const wchar_t *)MEditConfigAutoIndent,
		DI_CHECKBOX, 38,10, 0,10,0,0,0,0,(const wchar_t *)MEditCursorBeyondEnd,
		DI_TEXT,     10,11, 0,11,0,0,0,0,(const wchar_t *)MEditConfigTabSize,
		DI_FIXEDIT,   5,11, 8,11,0,0,0,0,L"",
		DI_CHECKBOX, 38,11, 0,11,0,0,0,0,(const wchar_t *)MEditConfigScrollbar,
		DI_CHECKBOX,  5,12, 0,12,0,0,0,0,(const wchar_t *)MEditShowWhiteSpace,
		DI_CHECKBOX, 38,12, 0,12,0,0,0,0,(const wchar_t *)MEditConfigPickUpWord,
		DI_CHECKBOX,  5,14, 0,14,0,0,0,0,(const wchar_t *)MEditShareWrite,
		DI_CHECKBOX,  5,15, 0,15,0,0,0,0,(const wchar_t *)MEditLockROFileModification,
		DI_CHECKBOX,  5,16, 0,16,0,0,0,0,(const wchar_t *)MEditWarningBeforeOpenROFile,
		DI_CHECKBOX,  5,17, 0,17,0,0,0,0,(const wchar_t *)MEditAutoDetectCodePage,
		DI_CHECKBOX,  5,18, 0,18,0,0,0,0,(const wchar_t *)MEditConfigAnsiCodePageAsDefault,
		DI_CHECKBOX,  5,19, 0,19,0,0,0,0,(const wchar_t *)MEditConfigAnsiCodePageForNewFile,
		DI_TEXT,      0,20, 0,20,0,0, DIF_SEPARATOR, 0, L"",
		DI_BUTTON,    0,21, 0,21,0,0,DIF_CENTERGROUP,1,(const wchar_t *)MOk,
		DI_BUTTON,    0,21, 0,21,0,0,DIF_CENTERGROUP,0,(const wchar_t *)MCancel,
	};
	MakeDialogItemsEx(CfgDlgData,CfgDlg);
	CfgDlg[ID_EC_EXTERNALUSEF4].Selected=Opt.EdOpt.UseExternalEditor;
	CfgDlg[ID_EC_EXTERNALCOMMANDEDIT].strData = Opt.strExternalEditor;
	FarListItem ExpandTabListItems[3]={0};
	FarList ExpandTabList = {countof(ExpandTabListItems),ExpandTabListItems};
	CfgDlg[ID_EC_EXPANDTABS].ListItems = &ExpandTabList;
	ExpandTabListItems[0].Text=MSG(MEditConfigDoNotExpandTabs);
	ExpandTabListItems[1].Text=MSG(MEditConfigExpandTabs);
	ExpandTabListItems[2].Text=MSG(MEditConfigConvertAllTabsToSpaces);
	//������� ������������ ������, ����� ��������� (�� �����������) ������ ���������

	if (EdOpt.ExpandTabs == EXPAND_NOTABS)
		ExpandTabListItems[0].Flags = LIF_SELECTED;

	if (EdOpt.ExpandTabs == EXPAND_NEWTABS)
		ExpandTabListItems[1].Flags = LIF_SELECTED;

	if (EdOpt.ExpandTabs == EXPAND_ALLTABS)
		ExpandTabListItems[2].Flags = LIF_SELECTED;

	CfgDlg[ID_EC_PERSISTENTBLOCKS].Selected = EdOpt.PersistentBlocks;
	CfgDlg[ID_EC_DELREMOVESBLOCKS].Selected = EdOpt.DelRemovesBlocks;
	CfgDlg[ID_EC_AUTOINDENT].Selected = EdOpt.AutoIndent;
	CfgDlg[ID_EC_SAVEPOSITION].Selected = EdOpt.SavePos;
	CfgDlg[ID_EC_SAVEBOOKMARKS].Selected = EdOpt.SaveShortPos;

	if (!EdOpt.SavePos)
		CfgDlg[ID_EC_SAVEBOOKMARKS].Flags |= DIF_DISABLE;

	CfgDlg[ID_EC_AUTODETECTCODEPAGE].Selected = EdOpt.AutoDetectCodePage;
	CfgDlg[ID_EC_CURSORBEYONDEOL].Selected = EdOpt.CursorBeyondEOL;
	CfgDlg[ID_EC_SHARINGWRITE].Selected = EdOpt.EditOpenedForWrite;
	CfgDlg[ID_EC_LOCKREADONLY].Selected = EdOpt.ReadOnlyLock & 1;
	CfgDlg[ID_EC_READONLYWARNING].Selected = EdOpt.ReadOnlyLock & 2;
	CfgDlg[ID_EC_ANSIASDEFAULT].Selected = EdOpt.AnsiCodePageAsDefault;
	CfgDlg[ID_EC_ANSIFORNEWFILE].Selected = EdOpt.AnsiCodePageForNewFile;
	CfgDlg[ID_EC_TABSIZEEDIT].strData.Format(L"%d",EdOpt.TabSize);
	CfgDlg[ID_EC_SHOWSCROLLBAR].Selected = EdOpt.ShowScrollBar;
	CfgDlg[ID_EC_SHOWWHITESPACE].Selected = EdOpt.ShowWhiteSpace;
	CfgDlg[ID_EC_PICKUPWORD].Selected = EdOpt.SearchPickUpWord;
	int DialogHeight=24;

	if (Local)
	{
		for (int i = ID_EC_EXTERNALUSEF4; i <= ID_EC_SEPARATOR1; i++)
			CfgDlg[i].Flags |= DIF_HIDDEN;

		for (int i = ID_EC_SHARINGWRITE; i < ID_EC_SEPARATOR2; i++)
			CfgDlg[i].Flags |= DIF_HIDDEN;

		for (int i = ID_EC_EXPANDTABSTITLE; i < ID_EC_SEPARATOR2; i++)
			CfgDlg[i].Y1 -= 4;

		for (int i = ID_EC_SEPARATOR2; i <= ID_EC_CANCEL; i++)
			CfgDlg[i].Y1 -= 11;

		CfgDlg[ID_EC_TITLE].Y2 -= 11;
		DialogHeight -= 11;
	}

	{
		Dialog Dlg((DialogItemEx*)CfgDlg,countof(CfgDlg));
		Dlg.SetAutomation(ID_EC_SAVEPOSITION,ID_EC_SAVEBOOKMARKS,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
		Dlg.SetHelp(L"EditorSettings");
		Dlg.SetPosition(-1,-1,74,DialogHeight);
		Dlg.Process();

		if (Dlg.GetExitCode()!=ID_EC_OK)
			return;
	}

	if (!Local)
	{
		Opt.EdOpt.UseExternalEditor=CfgDlg[ID_EC_EXTERNALUSEF4].Selected;
		Opt.strExternalEditor = CfgDlg[ID_EC_EXTERNALCOMMANDEDIT].strData;
	}

	switch (CfgDlg[ID_EC_EXPANDTABS].ListPos)
	{
		case 0:
			EdOpt.ExpandTabs = EXPAND_NOTABS;
			break;
		case 1:
			EdOpt.ExpandTabs = EXPAND_NEWTABS;
			break;
		case 2:
			EdOpt.ExpandTabs = EXPAND_ALLTABS;
			break;
	}

	EdOpt.PersistentBlocks = CfgDlg[ID_EC_PERSISTENTBLOCKS].Selected;
	EdOpt.DelRemovesBlocks = CfgDlg[ID_EC_DELREMOVESBLOCKS].Selected;
	EdOpt.AutoIndent = CfgDlg[ID_EC_AUTOINDENT].Selected;
	EdOpt.SavePos = CfgDlg[ID_EC_SAVEPOSITION].Selected;
	EdOpt.SaveShortPos = CfgDlg[ID_EC_SAVEBOOKMARKS].Selected;
	EdOpt.EditOpenedForWrite=CfgDlg[ID_EC_SHARINGWRITE].Selected;
	EdOpt.AutoDetectCodePage = CfgDlg[ID_EC_AUTODETECTCODEPAGE].Selected;
	EdOpt.AnsiCodePageAsDefault = CfgDlg[ID_EC_ANSIASDEFAULT].Selected;
	EdOpt.AnsiCodePageForNewFile = CfgDlg[ID_EC_ANSIFORNEWFILE].Selected;
	EdOpt.ShowWhiteSpace=CfgDlg[ID_EC_SHOWWHITESPACE].Selected;
	EdOpt.TabSize=_wtoi(CfgDlg[ID_EC_TABSIZEEDIT].strData);

	if (EdOpt.TabSize<1 || EdOpt.TabSize>512)
		EdOpt.TabSize=8;

	EdOpt.SearchPickUpWord=CfgDlg[ID_EC_PICKUPWORD].Selected;
	EdOpt.ShowScrollBar=CfgDlg[ID_EC_SHOWSCROLLBAR].Selected;
	EdOpt.CursorBeyondEOL=CfgDlg[ID_EC_CURSORBEYONDEOL].Selected;
	EdOpt.ReadOnlyLock&=~3;

	if (CfgDlg[ID_EC_LOCKREADONLY].Selected)
		EdOpt.ReadOnlyLock|=1;

	if (CfgDlg[ID_EC_READONLYWARNING].Selected)
		EdOpt.ReadOnlyLock|=2;
}


void SetFolderInfoFiles()
{
	string strFolderInfoFiles;

	if (GetString(MSG(MSetFolderInfoTitle),MSG(MSetFolderInfoNames),L"FolderInfoFiles",
	              Opt.InfoPanel.strFolderInfoFiles,strFolderInfoFiles,L"OptMenu",FIB_ENABLEEMPTY|FIB_BUTTONS))
	{
		Opt.InfoPanel.strFolderInfoFiles = strFolderInfoFiles;

		if (CtrlObject->Cp()->LeftPanel->GetType() == INFO_PANEL)
			CtrlObject->Cp()->LeftPanel->Update(0);

		if (CtrlObject->Cp()->RightPanel->GetType() == INFO_PANEL)
			CtrlObject->Cp()->RightPanel->Update(0);
	}
}


// ���������, ����������� ��� ������������(!)
static struct FARConfig
{
	int   IsSave;   // =1 - ����� ������������ � SaveConfig()
	DWORD ValType;  // REG_DWORD, REG_SZ, REG_BINARY
	const wchar_t *KeyName;
	const wchar_t *ValName;
	void *ValPtr;   // ����� ����������, ���� �������� ������
	DWORD DefDWord; // �� �� ������ ������ ��� REG_SZ � REG_BINARY
	const wchar_t *DefStr;   // ������/������ �� ���������
} CFG[]=
{
	{1, REG_BINARY, NKeyColors, L"CurrentPalette",(char*)Palette,SizeArrayPalette,(wchar_t*)DefaultPalette},

	{1, REG_DWORD,  NKeyScreen, L"Clock", &Opt.Clock, 1, 0},
	{1, REG_DWORD,  NKeyScreen, L"ViewerEditorClock",&Opt.ViewerEditorClock,0, 0},
	{1, REG_DWORD,  NKeyScreen, L"KeyBar",&Opt.ShowKeyBar,1, 0},
	{1, REG_DWORD,  NKeyScreen, L"ScreenSaver",&Opt.ScreenSaver, 0, 0},
	{1, REG_DWORD,  NKeyScreen, L"ScreenSaverTime",&Opt.ScreenSaverTime,5, 0},
	{0, REG_DWORD,  NKeyScreen, L"DeltaXY", &Opt.ScrSize.DeltaXY, 0, 0},

	{1, REG_DWORD,  NKeyCmdline, L"UsePromptFormat", &Opt.CmdLine.UsePromptFormat,0, 0},
	{1, REG_SZ,     NKeyCmdline, L"PromptFormat",&Opt.CmdLine.strPromptFormat, 0, L"$p>"},
	{1, REG_DWORD,  NKeyCmdline, L"DelRemovesBlocks", &Opt.CmdLine.DelRemovesBlocks,1, 0},
	{1, REG_DWORD,  NKeyCmdline, L"EditBlock", &Opt.CmdLine.EditBlock,0, 0},
	{1, REG_DWORD,  NKeyCmdline, L"AutoComplete",&Opt.CmdLine.AutoComplete,1, 0},


	{1, REG_DWORD,  NKeyInterface, L"Mouse",&Opt.Mouse,1, 0},
	{0, REG_DWORD,  NKeyInterface, L"UseVk_oem_x",&Opt.UseVk_oem_x,1, 0},
	{1, REG_DWORD,  NKeyInterface, L"ShowMenuBar",&Opt.ShowMenuBar,0, 0},
	{0, REG_DWORD,  NKeyInterface, L"CursorSize1",&Opt.CursorSize[0],15, 0},
	{0, REG_DWORD,  NKeyInterface, L"CursorSize2",&Opt.CursorSize[1],10, 0},
	{0, REG_DWORD,  NKeyInterface, L"CursorSize3",&Opt.CursorSize[2],99, 0},
	{0, REG_DWORD,  NKeyInterface, L"CursorSize4",&Opt.CursorSize[3],99, 0},
	{0, REG_DWORD,  NKeyInterface, L"ShiftsKeyRules",&Opt.ShiftsKeyRules,1, 0},
	{0, REG_DWORD,  NKeyInterface, L"AltF9",&Opt.AltF9, 1, 0},
	{1, REG_DWORD,  NKeyInterface, L"CtrlPgUp",&Opt.PgUpChangeDisk, 1, 0},
	{1, REG_DWORD,  NKeyInterface, L"ClearType",&Opt.ClearType, 0, 0},
	{0, REG_DWORD,  NKeyInterface, L"ShowTimeoutDelFiles",&Opt.ShowTimeoutDelFiles, 50, 0},
	{0, REG_DWORD,  NKeyInterface, L"ShowTimeoutDACLFiles",&Opt.ShowTimeoutDACLFiles, 50, 0},
	{0, REG_DWORD,  NKeyInterface, L"FormatNumberSeparators",&Opt.FormatNumberSeparators, 0, 0},

	{1, REG_SZ,     NKeyViewer,L"ExternalViewerName",&Opt.strExternalViewer, 0, L""},
	{1, REG_DWORD,  NKeyViewer,L"UseExternalViewer",&Opt.ViOpt.UseExternalViewer,0, 0},
	{1, REG_DWORD,  NKeyViewer,L"SaveViewerPos",&Opt.ViOpt.SaveViewerPos,1, 0},
	{1, REG_DWORD,  NKeyViewer,L"SaveViewerShortPos",&Opt.ViOpt.SaveViewerShortPos,1, 0},
	{1, REG_DWORD,  NKeyViewer,L"AutoDetectCodePage",&Opt.ViOpt.AutoDetectCodePage,0, 0},
	{1, REG_DWORD,  NKeyViewer,L"SearchRegexp",&Opt.ViOpt.SearchRegexp,0, 0},

	{1, REG_DWORD,  NKeyViewer,L"TabSize",&Opt.ViOpt.TabSize,8, 0},
	{1, REG_DWORD,  NKeyViewer,L"ShowKeyBar",&Opt.ViOpt.ShowKeyBar,1, 0},
	{0, REG_DWORD,  NKeyViewer,L"ShowTitleBar",&Opt.ViOpt.ShowTitleBar,1, 0},
	{1, REG_DWORD,  NKeyViewer,L"ShowArrows",&Opt.ViOpt.ShowArrows,1, 0},
	{1, REG_DWORD,  NKeyViewer,L"ShowScrollbar",&Opt.ViOpt.ShowScrollbar,0, 0},
	{1, REG_DWORD,  NKeyViewer,L"IsWrap",&Opt.ViOpt.ViewerIsWrap,1, 0},
	{1, REG_DWORD,  NKeyViewer,L"Wrap",&Opt.ViOpt.ViewerWrap,0, 0},
	{1, REG_DWORD,  NKeyViewer,L"PersistentBlocks",&Opt.ViOpt.PersistentBlocks,0, 0},
	{1, REG_DWORD,  NKeyViewer,L"AnsiCodePageAsDefault",&Opt.ViOpt.AnsiCodePageAsDefault,1, 0},

	{1, REG_DWORD,  NKeyDialog, L"EditHistory",&Opt.Dialogs.EditHistory,1, 0},
	{1, REG_DWORD,  NKeyDialog, L"EditBlock",&Opt.Dialogs.EditBlock,0, 0},
	{1, REG_DWORD,  NKeyDialog, L"AutoComplete",&Opt.Dialogs.AutoComplete,1, 0},
	{1, REG_DWORD,  NKeyDialog,L"EULBsClear",&Opt.Dialogs.EULBsClear,0, 0},
	{0, REG_DWORD,  NKeyDialog,L"SelectFromHistory",&Opt.Dialogs.SelectFromHistory,0, 0},
	{0, REG_DWORD,  NKeyDialog,L"EditLine",&Opt.Dialogs.EditLine,0, 0},
	{1, REG_DWORD,  NKeyDialog,L"MouseButton",&Opt.Dialogs.MouseButton,0xFFFF, 0},
	{1, REG_DWORD,  NKeyDialog,L"DelRemovesBlocks",&Opt.Dialogs.DelRemovesBlocks,1, 0},
	{0, REG_DWORD,  NKeyDialog,L"CBoxMaxHeight",&Opt.Dialogs.CBoxMaxHeight,8, 0},

	{1, REG_SZ,     NKeyEditor,L"ExternalEditorName",&Opt.strExternalEditor, 0, L""},
	{1, REG_DWORD,  NKeyEditor,L"UseExternalEditor",&Opt.EdOpt.UseExternalEditor,0, 0},
	{1, REG_DWORD,  NKeyEditor,L"ExpandTabs",&Opt.EdOpt.ExpandTabs,0, 0},
	{1, REG_DWORD,  NKeyEditor,L"TabSize",&Opt.EdOpt.TabSize,8, 0},
	{1, REG_DWORD,  NKeyEditor,L"PersistentBlocks",&Opt.EdOpt.PersistentBlocks,0, 0},
	{1, REG_DWORD,  NKeyEditor,L"DelRemovesBlocks",&Opt.EdOpt.DelRemovesBlocks,1, 0},
	{1, REG_DWORD,  NKeyEditor,L"AutoIndent",&Opt.EdOpt.AutoIndent,0, 0},
	{1, REG_DWORD,  NKeyEditor,L"SaveEditorPos",&Opt.EdOpt.SavePos,1, 0},
	{1, REG_DWORD,  NKeyEditor,L"SaveEditorShortPos",&Opt.EdOpt.SaveShortPos,1, 0},
	{1, REG_DWORD,  NKeyEditor,L"AutoDetectCodePage",&Opt.EdOpt.AutoDetectCodePage,0, 0},
	{1, REG_DWORD,  NKeyEditor,L"EditorCursorBeyondEOL",&Opt.EdOpt.CursorBeyondEOL,1, 0},
	{1, REG_DWORD,  NKeyEditor,L"ReadOnlyLock",&Opt.EdOpt.ReadOnlyLock,0, 0}, // ������ ����� ������ 1.65 - �� ������������� � �� �����������
	{0, REG_DWORD,  NKeyEditor,L"EditorUndoSize",&Opt.EdOpt.UndoSize,0, 0}, // $ 03.12.2001 IS ������ ������ undo � ���������
	{0, REG_SZ,     NKeyEditor,L"WordDiv",&Opt.strWordDiv, 0, WordDiv0},
	{0, REG_DWORD,  NKeyEditor,L"BSLikeDel",&Opt.EdOpt.BSLikeDel,1, 0},
	{0, REG_DWORD,  NKeyEditor,L"EditorF7Rules",&Opt.EdOpt.F7Rules,1, 0},
	{0, REG_DWORD,  NKeyEditor,L"FileSizeLimit",&Opt.EdOpt.FileSizeLimitLo,(DWORD)0, 0},
	{0, REG_DWORD,  NKeyEditor,L"FileSizeLimitHi",&Opt.EdOpt.FileSizeLimitHi,(DWORD)0, 0},
	{0, REG_DWORD,  NKeyEditor,L"CharCodeBase",&Opt.EdOpt.CharCodeBase,1, 0},
	{0, REG_DWORD,  NKeyEditor,L"AllowEmptySpaceAfterEof", &Opt.EdOpt.AllowEmptySpaceAfterEof,0,0},//skv
	{1, REG_DWORD,  NKeyEditor,L"AnsiCodePageForNewFile",&Opt.EdOpt.AnsiCodePageForNewFile,1, 0},
	{1, REG_DWORD,  NKeyEditor,L"AnsiCodePageAsDefault",&Opt.EdOpt.AnsiCodePageAsDefault,1, 0},
	{1, REG_DWORD,  NKeyEditor,L"ShowKeyBar",&Opt.EdOpt.ShowKeyBar,1, 0},
	{0, REG_DWORD,  NKeyEditor,L"ShowTitleBar",&Opt.EdOpt.ShowTitleBar,1, 0},
	{1, REG_DWORD,  NKeyEditor,L"ShowScrollBar",&Opt.EdOpt.ShowScrollBar,0, 0},
	{1, REG_DWORD,  NKeyEditor,L"EditOpenedForWrite",&Opt.EdOpt.EditOpenedForWrite,1, 0},
	{1, REG_DWORD,  NKeyEditor,L"SearchSelFound",&Opt.EdOpt.SearchSelFound,0, 0},
	{1, REG_DWORD,  NKeyEditor,L"SearchRegexp",&Opt.EdOpt.SearchRegexp,0, 0},
	{1, REG_DWORD,  NKeyEditor,L"SearchPickUpWord",&Opt.EdOpt.SearchPickUpWord,0, 0},
	{1, REG_DWORD,  NKeyEditor,L"ShowWhiteSpace",&Opt.EdOpt.ShowWhiteSpace,0, 0},

	{0, REG_DWORD,  NKeyXLat,L"Flags",&Opt.XLat.Flags,(DWORD)XLAT_SWITCHKEYBLAYOUT|XLAT_CONVERTALLCMDLINE, 0},
	{0, REG_SZ,     NKeyXLat,L"Table1",&Opt.XLat.Table[0],0,L""},
	{0, REG_SZ,     NKeyXLat,L"Table2",&Opt.XLat.Table[1],0,L""},
	{0, REG_SZ,     NKeyXLat,L"Rules1",&Opt.XLat.Rules[0],0,L""},
	{0, REG_SZ,     NKeyXLat,L"Rules2",&Opt.XLat.Rules[1],0,L""},
	{0, REG_SZ,     NKeyXLat,L"Rules3",&Opt.XLat.Rules[2],0,L""},
	{0, REG_SZ,     NKeyXLat,L"WordDivForXlat",&Opt.XLat.strWordDivForXlat, 0,WordDivForXlat0},

	{0, REG_DWORD,  NKeySavedHistory, NParamHistoryCount,&Opt.HistoryCount,64, 0},
	{0, REG_DWORD,  NKeySavedFolderHistory, NParamHistoryCount,&Opt.FoldersHistoryCount,64, 0},
	{0, REG_DWORD,  NKeySavedViewHistory, NParamHistoryCount,&Opt.ViewHistoryCount,64, 0},
	{0, REG_DWORD,  NKeySavedDialogHistory, NParamHistoryCount,&Opt.DialogsHistoryCount,64, 0},

	{1, REG_DWORD,  NKeySystem,L"SaveHistory",&Opt.SaveHistory,1, 0},
	{1, REG_DWORD,  NKeySystem,L"SaveFoldersHistory",&Opt.SaveFoldersHistory,1, 0},
	{0, REG_DWORD,  NKeySystem,L"SavePluginFoldersHistory",&Opt.SavePluginFoldersHistory,0, 0},
	{1, REG_DWORD,  NKeySystem,L"SaveViewHistory",&Opt.SaveViewHistory,1, 0},
	{1, REG_DWORD,  NKeySystem,L"UseRegisteredTypes",&Opt.UseRegisteredTypes,1, 0},
	{1, REG_DWORD,  NKeySystem,L"AutoSaveSetup",&Opt.AutoSaveSetup,0, 0},
	{1, REG_DWORD,  NKeySystem,L"ClearReadOnly",&Opt.ClearReadOnly,0, 0},
	{1, REG_DWORD,  NKeySystem,L"DeleteToRecycleBin",&Opt.DeleteToRecycleBin,1, 0},
	{1, REG_DWORD,  NKeySystem,L"DeleteToRecycleBinKillLink",&Opt.DeleteToRecycleBinKillLink,1, 0},
	{0, REG_DWORD,  NKeySystem,L"WipeSymbol",&Opt.WipeSymbol,0, 0},

	{1, REG_DWORD,  NKeySystem,L"UseSystemCopy",&Opt.CMOpt.UseSystemCopy,1, 0},
	{0, REG_DWORD,  NKeySystem,L"CopySecurityOptions",&Opt.CMOpt.CopySecurityOptions,0, 0},
	{1, REG_DWORD,  NKeySystem,L"CopyOpened",&Opt.CMOpt.CopyOpened,1, 0},
	{1, REG_DWORD,  NKeyInterface, L"CopyShowTotal",&Opt.CMOpt.CopyShowTotal,0, 0},
	{1, REG_DWORD,  NKeySystem, L"MultiCopy",&Opt.CMOpt.MultiCopy,0, 0},
	{1, REG_DWORD,  NKeySystem,L"CopyTimeRule",  &Opt.CMOpt.CopyTimeRule, 3, 0},

	{1, REG_DWORD,  NKeyInterface,L"DelShowTotal",&Opt.DelOpt.DelShowTotal,0, 0},
	{1, REG_SZ,     NKeyInterface,L"TitleAddons",&Opt.strTitleAddons, 0, L"%Ver.%Build %Platform %Admin"},

	{1, REG_DWORD,  NKeySystem,L"CreateUppercaseFolders",&Opt.CreateUppercaseFolders,0, 0},
	{1, REG_DWORD,  NKeySystem,L"InactivityExit",&Opt.InactivityExit,0, 0},
	{1, REG_DWORD,  NKeySystem,L"InactivityExitTime",&Opt.InactivityExitTime,15, 0},
	{1, REG_DWORD,  NKeySystem,L"DriveMenuMode",&Opt.ChangeDriveMode,DRIVE_SHOW_TYPE|DRIVE_SHOW_PLUGINS|DRIVE_SHOW_SIZE_FLOAT, 0},
	{1, REG_DWORD,  NKeySystem,L"DriveDisconnetMode",&Opt.ChangeDriveDisconnetMode,1, 0},
	{1, REG_DWORD,  NKeySystem,L"AutoUpdateRemoteDrive",&Opt.AutoUpdateRemoteDrive,1, 0},
	{1, REG_DWORD,  NKeySystem,L"FileSearchMode",&Opt.FindOpt.FileSearchMode,FFSEARCH_FROM_CURRENT, 0},
	{0, REG_DWORD,  NKeySystem,L"CollectFiles",&Opt.FindOpt.CollectFiles, 1, 0},
	{1, REG_SZ,     NKeySystem,L"SearchInFirstSize",&Opt.FindOpt.strSearchInFirstSize, 0, L""},
	{1, REG_DWORD,  NKeySystem,L"FindAlternateStreams",&Opt.FindOpt.FindAlternateStreams,0,0},
	{1, REG_DWORD,  NKeySystem,L"FindFolders",&Opt.FindOpt.FindFolders, 1, 0},
	{1, REG_DWORD,  NKeySystem,L"FindSymLinks",&Opt.FindOpt.FindSymLinks, 1, 0},
	{1, REG_DWORD,  NKeySystem,L"UseFilterInSearch",&Opt.FindOpt.UseFilter,0,0},
	{1, REG_DWORD,  NKeySystem,L"FindCodePage",&Opt.FindCodePage, CP_AUTODETECT, 0},
	{0, REG_DWORD,  NKeySystem,L"SubstPluginPrefix",&Opt.SubstPluginPrefix, 0, 0},
	{0, REG_DWORD,  NKeySystem,L"CmdHistoryRule",&Opt.CmdHistoryRule,0, 0},
	{0, REG_DWORD,  NKeySystem,L"SetAttrFolderRules",&Opt.SetAttrFolderRules,1, 0},
	{0, REG_DWORD,  NKeySystem,L"MaxPositionCache",&Opt.MaxPositionCache,64, 0},
	{0, REG_SZ,     NKeySystem,L"ConsoleDetachKey", &strKeyNameConsoleDetachKey, 0, L"CtrlAltTab"},
	{0, REG_DWORD,  NKeySystem,L"SilentLoadPlugin",  &Opt.LoadPlug.SilentLoadPlugin, 0, 0},
	{1, REG_DWORD,  NKeySystem,L"MultiMakeDir",&Opt.MultiMakeDir,0, 0},
	{0, REG_DWORD,  NKeySystem,L"FlagPosixSemantics", &Opt.FlagPosixSemantics, 1, 0},
	{0, REG_DWORD,  NKeySystem,L"MsWheelDelta", &Opt.MsWheelDelta, 1, 0},
	{0, REG_DWORD,  NKeySystem,L"MsWheelDeltaView", &Opt.MsWheelDeltaView, 1, 0},
	{0, REG_DWORD,  NKeySystem,L"MsWheelDeltaEdit", &Opt.MsWheelDeltaEdit, 1, 0},
	{0, REG_DWORD,  NKeySystem,L"MsWheelDeltaHelp", &Opt.MsWheelDeltaHelp, 1, 0},
	{0, REG_DWORD,  NKeySystem,L"MsHWheelDelta", &Opt.MsHWheelDelta, 1, 0},
	{0, REG_DWORD,  NKeySystem,L"MsHWheelDeltaView", &Opt.MsHWheelDeltaView, 1, 0},
	{0, REG_DWORD,  NKeySystem,L"MsHWheelDeltaEdit", &Opt.MsHWheelDeltaEdit, 1, 0},
	{0, REG_DWORD,  NKeySystem,L"SubstNameRule", &Opt.SubstNameRule, 2, 0},
	{0, REG_DWORD,  NKeySystem,L"ShowCheckingFile", &Opt.ShowCheckingFile, 0, 0},
	{0, REG_DWORD,  NKeySystem,L"DelThreadPriority", &Opt.DelThreadPriority, THREAD_PRIORITY_NORMAL, 0},
	{0, REG_SZ,     NKeySystem,L"QuotedSymbols",&Opt.strQuotedSymbols, 0, L" &()[]{}^=;!'+,`"},
	{0, REG_DWORD,  NKeySystem,L"QuotedName",&Opt.QuotedName,0xFFFFFFFFU, 0},
	//{0, REG_DWORD,  NKeySystem,L"CPAJHefuayor",&Opt.strCPAJHefuayor,0, 0},
	{0, REG_DWORD,  NKeySystem,L"CloseConsoleRule",&Opt.CloseConsoleRule,1, 0},
	{0, REG_DWORD,  NKeySystem,L"PluginMaxReadData",&Opt.PluginMaxReadData,0x20000, 0},
	{1, REG_DWORD,  NKeySystem,L"CloseCDGate",&Opt.CloseCDGate,1, 0},
	{0, REG_DWORD,  NKeySystem,L"UseNumPad",&Opt.UseNumPad,1, 0},
	{0, REG_DWORD,  NKeySystem,L"CASRule",&Opt.CASRule,0xFFFFFFFFU, 0},
	{0, REG_DWORD,  NKeySystem,L"AllCtrlAltShiftRule",&Opt.AllCtrlAltShiftRule,0x0000FFFF, 0},
	{1, REG_DWORD,  NKeySystem,L"ScanJunction",&Opt.ScanJunction,1, 0},
	{0, REG_DWORD,  NKeySystem,L"UsePrintManager",&Opt.UsePrintManager,1, 0},

	{0, REG_DWORD,  NKeySystemNowell,L"MoveRO",&Opt.Nowell.MoveRO,1, 0},

	{0, REG_DWORD,  NKeySystemExecutor,L"RestoreCP",&Opt.RestoreCPAfterExecute,1, 0},
	{0, REG_DWORD,  NKeySystemExecutor,L"UseAppPath",&Opt.ExecuteUseAppPath,1, 0},
	{0, REG_DWORD,  NKeySystemExecutor,L"ShowErrorMessage",&Opt.ExecuteShowErrorMessage,1, 0},
	{0, REG_SZ,     NKeySystemExecutor,L"BatchType",&Opt.strExecuteBatchType,0,constBatchExt},
	{0, REG_DWORD,  NKeySystemExecutor,L"FullTitle",&Opt.ExecuteFullTitle,0, 0},

	{0, REG_DWORD,  NKeyPanelTree,L"MinTreeCount",&Opt.Tree.MinTreeCount, 4, 0},
	{0, REG_DWORD,  NKeyPanelTree,L"TreeFileAttr",&Opt.Tree.TreeFileAttr, FILE_ATTRIBUTE_HIDDEN, 0},
	{0, REG_DWORD,  NKeyPanelTree,L"LocalDisk",&Opt.Tree.LocalDisk, 2, 0},
	{0, REG_DWORD,  NKeyPanelTree,L"NetDisk",&Opt.Tree.NetDisk, 2, 0},
	{0, REG_DWORD,  NKeyPanelTree,L"RemovableDisk",&Opt.Tree.RemovableDisk, 2, 0},
	{0, REG_DWORD,  NKeyPanelTree,L"NetPath",&Opt.Tree.NetPath, 2, 0},
	{1, REG_DWORD,  NKeyPanelTree,L"AutoChangeFolder",&Opt.Tree.AutoChangeFolder,0, 0}, // ???

	{0, REG_DWORD,  NKeyHelp,L"ActivateURL",&Opt.HelpURLRules,1, 0},

	{1, REG_SZ,     NKeyLanguage,L"Help",&Opt.strHelpLanguage, 0, L"English"},

	{1, REG_DWORD,  NKeyConfirmations,L"Copy",&Opt.Confirm.Copy,1, 0},
	{1, REG_DWORD,  NKeyConfirmations,L"Move",&Opt.Confirm.Move,1, 0},
	{1, REG_DWORD,  NKeyConfirmations,L"RO",&Opt.Confirm.RO,1, 0},
	{1, REG_DWORD,  NKeyConfirmations,L"Drag",&Opt.Confirm.Drag,1, 0},
	{1, REG_DWORD,  NKeyConfirmations,L"Delete",&Opt.Confirm.Delete,1, 0},
	{1, REG_DWORD,  NKeyConfirmations,L"DeleteFolder",&Opt.Confirm.DeleteFolder,1, 0},
	{1, REG_DWORD,  NKeyConfirmations,L"Esc",&Opt.Confirm.Esc,1, 0},
	{1, REG_DWORD,  NKeyConfirmations,L"RemoveConnection",&Opt.Confirm.RemoveConnection,1, 0},
	{1, REG_DWORD,  NKeyConfirmations,L"RemoveSUBST",&Opt.Confirm.RemoveSUBST,1, 0},
	{1, REG_DWORD,  NKeyConfirmations,L"RemoveHotPlug",&Opt.Confirm.RemoveHotPlug,1, 0},
	{1, REG_DWORD,  NKeyConfirmations,L"AllowReedit",&Opt.Confirm.AllowReedit,1, 0},
	{1, REG_DWORD,  NKeyConfirmations,L"HistoryClear",&Opt.Confirm.HistoryClear,1, 0},
	{1, REG_DWORD,  NKeyConfirmations,L"Exit",&Opt.Confirm.Exit,1, 0},
	{0, REG_DWORD,  NKeyConfirmations,L"EscTwiceToInterrupt",&Opt.Confirm.EscTwiceToInterrupt,0, 0},

	{1, REG_DWORD,  NKeyPluginConfirmations, L"OpenFilePlugin", &Opt.PluginConfirm.OpenFilePlugin, 0, 0},
	{1, REG_DWORD,  NKeyPluginConfirmations, L"StandardAssociation", &Opt.PluginConfirm.StandardAssociation, 0, 0},
	{1, REG_DWORD,  NKeyPluginConfirmations, L"EvenIfOnlyOnePlugin", &Opt.PluginConfirm.EvenIfOnlyOnePlugin, 0, 0},
	{1, REG_DWORD,  NKeyPluginConfirmations, L"SetFindList", &Opt.PluginConfirm.SetFindList, 0, 0},
	{1, REG_DWORD,  NKeyPluginConfirmations, L"Prefix", &Opt.PluginConfirm.Prefix, 0, 0},

	{0, REG_DWORD,  NKeyPanel,L"ShellRightLeftArrowsRule",&Opt.ShellRightLeftArrowsRule,0, 0},
	{1, REG_DWORD,  NKeyPanel,L"ShowHidden",&Opt.ShowHidden,1, 0},
	{1, REG_DWORD,  NKeyPanel,L"Highlight",&Opt.Highlight,1, 0},
	{1, REG_DWORD,  NKeyPanel,L"SortFolderExt",&Opt.SortFolderExt,0, 0},
	{1, REG_DWORD,  NKeyPanel,L"SelectFolders",&Opt.SelectFolders,0, 0},
	{1, REG_DWORD,  NKeyPanel,L"ReverseSort",&Opt.ReverseSort,1, 0},
	{0, REG_DWORD,  NKeyPanel,L"RightClickRule",&Opt.PanelRightClickRule,2, 0},
	{0, REG_DWORD,  NKeyPanel,L"CtrlFRule",&Opt.PanelCtrlFRule,1, 0},
	{0, REG_DWORD,  NKeyPanel,L"CtrlAltShiftRule",&Opt.PanelCtrlAltShiftRule,0, 0},
	{0, REG_DWORD,  NKeyPanel,L"RememberLogicalDrives",&Opt.RememberLogicalDrives, 0, 0},
	{1, REG_DWORD,  NKeyPanel,L"AutoUpdateLimit",&Opt.AutoUpdateLimit, 0, 0},

	{1, REG_DWORD,  NKeyPanelLeft,L"Type",&Opt.LeftPanel.Type,0, 0},
	{1, REG_DWORD,  NKeyPanelLeft,L"Visible",&Opt.LeftPanel.Visible,1, 0},
	{1, REG_DWORD,  NKeyPanelLeft,L"Focus",&Opt.LeftPanel.Focus,1, 0},
	{1, REG_DWORD,  NKeyPanelLeft,L"ViewMode",&Opt.LeftPanel.ViewMode,2, 0},
	{1, REG_DWORD,  NKeyPanelLeft,L"SortMode",&Opt.LeftPanel.SortMode,1, 0},
	{1, REG_DWORD,  NKeyPanelLeft,L"SortOrder",&Opt.LeftPanel.SortOrder,1, 0},
	{1, REG_DWORD,  NKeyPanelLeft,L"SortGroups",&Opt.LeftPanel.SortGroups,0, 0},
	{1, REG_DWORD,  NKeyPanelLeft,L"ShortNames",&Opt.LeftPanel.ShowShortNames,0, 0},
	{1, REG_DWORD,  NKeyPanelLeft,L"NumericSort",&Opt.LeftPanel.NumericSort,0, 0},
	{1, REG_SZ,     NKeyPanelLeft,L"Folder",&Opt.strLeftFolder, 0, L""},
	{1, REG_SZ,     NKeyPanelLeft,L"CurFile",&Opt.strLeftCurFile, 0, L""},
	{1, REG_DWORD,  NKeyPanelLeft,L"SelectedFirst",&Opt.LeftSelectedFirst,0,0},

	{1, REG_DWORD,  NKeyPanelRight,L"Type",&Opt.RightPanel.Type,0, 0},
	{1, REG_DWORD,  NKeyPanelRight,L"Visible",&Opt.RightPanel.Visible,1, 0},
	{1, REG_DWORD,  NKeyPanelRight,L"Focus",&Opt.RightPanel.Focus,0, 0},
	{1, REG_DWORD,  NKeyPanelRight,L"ViewMode",&Opt.RightPanel.ViewMode,2, 0},
	{1, REG_DWORD,  NKeyPanelRight,L"SortMode",&Opt.RightPanel.SortMode,1, 0},
	{1, REG_DWORD,  NKeyPanelRight,L"SortOrder",&Opt.RightPanel.SortOrder,1, 0},
	{1, REG_DWORD,  NKeyPanelRight,L"SortGroups",&Opt.RightPanel.SortGroups,0, 0},
	{1, REG_DWORD,  NKeyPanelRight,L"ShortNames",&Opt.RightPanel.ShowShortNames,0, 0},
	{1, REG_DWORD,  NKeyPanelRight,L"NumericSort",&Opt.RightPanel.NumericSort,0, 0},
	{1, REG_SZ,     NKeyPanelRight,L"Folder",&Opt.strRightFolder, 0,L""},
	{1, REG_SZ,     NKeyPanelRight,L"CurFile",&Opt.strRightCurFile, 0,L""},
	{1, REG_DWORD,  NKeyPanelRight,L"SelectedFirst",&Opt.RightSelectedFirst,0, 0},

	{1, REG_DWORD,  NKeyPanelLayout,L"ColumnTitles",&Opt.ShowColumnTitles,1, 0},
	{1, REG_DWORD,  NKeyPanelLayout,L"StatusLine",&Opt.ShowPanelStatus,1, 0},
	{1, REG_DWORD,  NKeyPanelLayout,L"TotalInfo",&Opt.ShowPanelTotals,1, 0},
	{1, REG_DWORD,  NKeyPanelLayout,L"FreeInfo",&Opt.ShowPanelFree,0, 0},
	{1, REG_DWORD,  NKeyPanelLayout,L"Scrollbar",&Opt.ShowPanelScrollbar,0, 0},
	{0, REG_DWORD,  NKeyPanelLayout,L"ScrollbarMenu",&Opt.ShowMenuScrollbar,1, 0},
	{1, REG_DWORD,  NKeyPanelLayout,L"ScreensNumber",&Opt.ShowScreensNumber,1, 0},
	{1, REG_DWORD,  NKeyPanelLayout,L"SortMode",&Opt.ShowSortMode,1, 0},

	{1, REG_DWORD,  NKeyLayout,L"LeftHeightDecrement",&Opt.LeftHeightDecrement,0, 0},
	{1, REG_DWORD,  NKeyLayout,L"RightHeightDecrement",&Opt.RightHeightDecrement,0, 0},
	{1, REG_DWORD,  NKeyLayout,L"WidthDecrement",&Opt.WidthDecrement,0, 0},
	{1, REG_DWORD,  NKeyLayout,L"FullscreenHelp",&Opt.FullScreenHelp,0, 0},

	{1, REG_SZ,     NKeyDescriptions,L"ListNames",&Opt.Diz.strListNames, 0, L"Descript.ion,Files.bbs"},
	{1, REG_DWORD,  NKeyDescriptions,L"UpdateMode",&Opt.Diz.UpdateMode,DIZ_UPDATE_IF_DISPLAYED, 0},
	{1, REG_DWORD,  NKeyDescriptions,L"ROUpdate",&Opt.Diz.ROUpdate,0, 0},
	{1, REG_DWORD,  NKeyDescriptions,L"SetHidden",&Opt.Diz.SetHidden,1, 0},
	{1, REG_DWORD,  NKeyDescriptions,L"StartPos",&Opt.Diz.StartPos,0, 0},
	{1, REG_DWORD,  NKeyDescriptions,L"AnsiByDefault",&Opt.Diz.AnsiByDefault,0, 0},
	{1, REG_DWORD,  NKeyDescriptions,L"SaveInUTF",&Opt.Diz.SaveInUTF,0, 0},

	{0, REG_DWORD,  NKeyKeyMacros,L"MacroReuseRules",&Opt.MacroReuseRules,0, 0},
	{0, REG_SZ,     NKeyKeyMacros,L"DateFormat",&Opt.strDateFormat, 0, L"%a %b %d %H:%M:%S %Z %Y"},
	{0, REG_SZ,     NKeyKeyMacros,L"CONVFMT",&Opt.strMacroCONVFMT, 0, L"%.6g"},

	{0, REG_DWORD,  NKeyPolicies,L"ShowHiddenDrives",&Opt.Policies.ShowHiddenDrives,1, 0},
	{0, REG_DWORD,  NKeyPolicies,L"DisabledOptions",&Opt.Policies.DisabledOptions,0, 0},


	{0, REG_DWORD,  NKeySystem,L"ExcludeCmdHistory",&Opt.ExcludeCmdHistory,0, 0}, //AN

	{1, REG_DWORD,  NKeyCodePages,L"CPMenuMode",&Opt.CPMenuMode,0,0},

	{1, REG_SZ,     NKeySystem,L"FolderInfo",&Opt.InfoPanel.strFolderInfoFiles, 0, L"DirInfo,File_Id.diz,Descript.ion,ReadMe.*,Read.Me"},
	{1, REG_DWORD,  NKeyPanelInfo,L"InfoUserNameFormat",&Opt.InfoPanel.UserNameFormat, NameUserPrincipal, 0},
};


bool IsUserAdmin()
{
	bool Result=false;
	SID_IDENTIFIER_AUTHORITY NtAuthority=SECURITY_NT_AUTHORITY;
	PSID AdministratorsGroup;
	if(AllocateAndInitializeSid(&NtAuthority,2,SECURITY_BUILTIN_DOMAIN_RID,DOMAIN_ALIAS_RID_ADMINS,0,0,0,0,0,0,&AdministratorsGroup))
	{
		BOOL IsMember=FALSE;
		if(CheckTokenMembership(NULL,AdministratorsGroup,&IsMember)&&IsMember)
		{
			Result=true;
		}
		FreeSid(AdministratorsGroup);
	}
	return Result;
}

void ReadConfig()
{
	Opt.IsUserAdmin=IsUserAdmin();
	DWORD OptPolicies_ShowHiddenDrives,  OptPolicies_DisabledOptions;
	string strKeyNameFromReg;
	string strPersonalPluginsPath;
	size_t I;
	/* <�����������> *************************************************** */
	// "��������" ���� ��� ��������������� ������ ��������
	SetRegRootKey(HKEY_LOCAL_MACHINE);
	GetRegKey(NKeySystem,L"TemplatePluginsPath",strPersonalPluginsPath,L"");
	OptPolicies_ShowHiddenDrives=GetRegKey(NKeyPolicies,L"ShowHiddenDrives",1)&1;
	OptPolicies_DisabledOptions=GetRegKey(NKeyPolicies,L"DisabledOptions",0);
	SetRegRootKey(HKEY_CURRENT_USER);
	GetRegKey(NKeySystem,L"PersonalPluginsPath",Opt.LoadPlug.strPersonalPluginsPath, strPersonalPluginsPath);

	if (Opt.ExceptRules == -1)
		GetRegKey(L"System",L"ExceptRules",Opt.ExceptRules,1);

	//Opt.LCIDSort=LOCALE_USER_DEFAULT; // ����������������� �� ������ ������
	/* *************************************************** </�����������> */

	for (I=0; I < countof(CFG); ++I)
	{
		switch (CFG[I].ValType)
		{
			case REG_DWORD:
				GetRegKey(CFG[I].KeyName, CFG[I].ValName,*(int *)CFG[I].ValPtr,(DWORD)CFG[I].DefDWord);
				break;
			case REG_SZ:
				GetRegKey(CFG[I].KeyName, CFG[I].ValName,*(string *)CFG[I].ValPtr,CFG[I].DefStr);
				break;
			case REG_BINARY:
				int Size=GetRegKey(CFG[I].KeyName, CFG[I].ValName,(BYTE*)CFG[I].ValPtr,(BYTE*)CFG[I].DefStr,CFG[I].DefDWord);

				if (Size && Size < (int)CFG[I].DefDWord)
					memset(((BYTE*)CFG[I].ValPtr)+Size,0,CFG[I].DefDWord-Size);

				break;
		}
	}

	/* <������������> *************************************************** */
	if (Opt.ShowMenuBar)
		Opt.ShowMenuBar=1;

	if (Opt.PluginMaxReadData < 0x1000) // || Opt.PluginMaxReadData > 0x80000)
		Opt.PluginMaxReadData=0x20000;

	Opt.HelpTabSize=8; // ���� ������ ��������...
	//   �������� �������� "������" �������.
	for (I=COL_PRIVATEPOSITION_FOR_DIF165ABOVE-COL_FIRSTPALETTECOLOR+1;
	        I < (COL_LASTPALETTECOLOR-COL_FIRSTPALETTECOLOR);
	        ++I)
	{
		if (!Palette[I])
		{
			if (!Palette[COL_PRIVATEPOSITION_FOR_DIF165ABOVE-COL_FIRSTPALETTECOLOR])
				Palette[I]=DefaultPalette[I];
			else if (Palette[COL_PRIVATEPOSITION_FOR_DIF165ABOVE-COL_FIRSTPALETTECOLOR] == 1)
				Palette[I]=BlackPalette[I];

			/*
			else
			  � ������ ������� ������ ������ �� ������, �.�.
			  ���� ������ �������...
			*/
		}
	}

	Opt.ViOpt.ViewerIsWrap&=1;
	Opt.ViOpt.ViewerWrap&=1;

	// ��������� ��������� �������� ������������ ;-)
	if (Opt.strWordDiv.IsEmpty())
		Opt.strWordDiv = WordDiv0;

	// ��������� ��������� �������� ������������
	if (Opt.XLat.strWordDivForXlat.IsEmpty())
		Opt.XLat.strWordDivForXlat = WordDivForXlat0;

	if (Opt.MaxPositionCache < 16 || Opt.MaxPositionCache > 128)
		Opt.MaxPositionCache=64;

	Opt.PanelRightClickRule%=3;
	Opt.PanelCtrlAltShiftRule%=3;
	Opt.ConsoleDetachKey=KeyNameToKey(strKeyNameConsoleDetachKey);

	if (Opt.EdOpt.TabSize<1 || Opt.EdOpt.TabSize>512)
		Opt.EdOpt.TabSize=8;

	if (Opt.ViOpt.TabSize<1 || Opt.ViOpt.TabSize>512)
		Opt.ViOpt.TabSize=8;

	GetRegKey(NKeyKeyMacros,L"KeyRecordCtrlDot",strKeyNameFromReg,szCtrlDot);

	if ((Opt.KeyMacroCtrlDot=KeyNameToKey(strKeyNameFromReg)) == (DWORD)-1)
		Opt.KeyMacroCtrlDot=KEY_CTRLDOT;

	GetRegKey(NKeyKeyMacros,L"KeyRecordCtrlShiftDot",strKeyNameFromReg,szCtrlShiftDot);

	if ((Opt.KeyMacroCtrlShiftDot=KeyNameToKey(strKeyNameFromReg)) == (DWORD)-1)
		Opt.KeyMacroCtrlShiftDot=KEY_CTRLSHIFTDOT;

	Opt.EdOpt.strWordDiv = Opt.strWordDiv;
	FileList::ReadPanelModes();
	apiGetTempPath(Opt.strTempPath);
	RemoveTrailingSpaces(Opt.strTempPath);
	AddEndSlash(Opt.strTempPath);
	CtrlObject->EditorPosCache->Read(L"Editor\\LastPositions");
	CtrlObject->ViewerPosCache->Read(L"Viewer\\LastPositions");
	// �������� ��������� ��������
	// ��� ������ HKCU ����� ������ �������� �����
	Opt.Policies.ShowHiddenDrives&=OptPolicies_ShowHiddenDrives;
	// ��� ����� HKCU ����� ������ ��������� ��������� �������
	Opt.Policies.DisabledOptions|=OptPolicies_DisabledOptions;

	if (Opt.strExecuteBatchType.IsEmpty()) // ��������������
		Opt.strExecuteBatchType=constBatchExt;

	{
		Opt.XLat.CurrentLayout=0;
		memset(Opt.XLat.Layouts,0,sizeof(Opt.XLat.Layouts));
		string strXLatLayouts;
		GetRegKey(NKeyXLat,L"Layouts",strXLatLayouts,L"");

		if (!strXLatLayouts.IsEmpty())
		{
			wchar_t *endptr;
			const wchar_t *ValPtr;
			UserDefinedList DestList;
			DestList.SetParameters(L';',0,ULF_UNIQUE);
			DestList.Set(strXLatLayouts);
			I=0;

			while (NULL!=(ValPtr=DestList.GetNext()))
			{
				DWORD res=(DWORD)wcstoul(ValPtr, &endptr, 16);
				Opt.XLat.Layouts[I]=(HKL)(LONG_PTR)(HIWORD(res) == 0?MAKELONG(res,res):res);
				++I;

				if (I >= countof(Opt.XLat.Layouts))
					break;
			}

			if (I <= 1) // ���� ������� ������ ���� - "����������" ���
				Opt.XLat.Layouts[0]=0;
		}
	}
	/* *************************************************** </������������> */
}


void SaveConfig(int Ask)
{
	if (Opt.Policies.DisabledOptions&0x20000) // Bit 17 - ��������� ���������
		return;

	if (Ask && Message(0,2,MSG(MSaveSetupTitle),MSG(MSaveSetupAsk1),MSG(MSaveSetupAsk2),MSG(MSaveSetup),MSG(MCancel))!=0)
		return;

	string strTemp;
	/* <�����������> *************************************************** */
	Panel *LeftPanel=CtrlObject->Cp()->LeftPanel;
	Panel *RightPanel=CtrlObject->Cp()->RightPanel;
	Opt.LeftPanel.Focus=LeftPanel->GetFocus();
	Opt.LeftPanel.Visible=LeftPanel->IsVisible();
	Opt.RightPanel.Focus=RightPanel->GetFocus();
	Opt.RightPanel.Visible=RightPanel->IsVisible();

	if (LeftPanel->GetMode()==NORMAL_PANEL)
	{
		Opt.LeftPanel.Type=LeftPanel->GetType();
		Opt.LeftPanel.ViewMode=LeftPanel->GetViewMode();
		Opt.LeftPanel.SortMode=LeftPanel->GetSortMode();
		Opt.LeftPanel.SortOrder=LeftPanel->GetSortOrder();
		Opt.LeftPanel.SortGroups=LeftPanel->GetSortGroups();
		Opt.LeftPanel.ShowShortNames=LeftPanel->GetShowShortNamesMode();
		Opt.LeftPanel.NumericSort=LeftPanel->GetNumericSort();
		Opt.LeftSelectedFirst=LeftPanel->GetSelectedFirstMode();
	}

	LeftPanel->GetCurDir(Opt.strLeftFolder);
	LeftPanel->GetCurBaseName(Opt.strLeftCurFile, strTemp);

	if (RightPanel->GetMode()==NORMAL_PANEL)
	{
		Opt.RightPanel.Type=RightPanel->GetType();
		Opt.RightPanel.ViewMode=RightPanel->GetViewMode();
		Opt.RightPanel.SortMode=RightPanel->GetSortMode();
		Opt.RightPanel.SortOrder=RightPanel->GetSortOrder();
		Opt.RightPanel.SortGroups=RightPanel->GetSortGroups();
		Opt.RightPanel.ShowShortNames=RightPanel->GetShowShortNamesMode();
		Opt.RightPanel.NumericSort=RightPanel->GetNumericSort();
		Opt.RightSelectedFirst=RightPanel->GetSelectedFirstMode();
	}

	RightPanel->GetCurDir(Opt.strRightFolder);
	RightPanel->GetCurBaseName(Opt.strRightCurFile,strTemp);
	CtrlObject->HiFiles->SaveHiData();
	/* *************************************************** </�����������> */
	SetRegKey(NKeySystem,L"PersonalPluginsPath",Opt.LoadPlug.strPersonalPluginsPath);
	SetRegKey(NKeyLanguage,L"Main",Opt.strLanguage);

	for (size_t I=0; I < countof(CFG); ++I)
	{
		if (CFG[I].IsSave)
			switch (CFG[I].ValType)
			{
				case REG_DWORD:
					SetRegKey(CFG[I].KeyName, CFG[I].ValName,*(int *)CFG[I].ValPtr);
					break;
				case REG_SZ:
					SetRegKey(CFG[I].KeyName, CFG[I].ValName,*(string *)CFG[I].ValPtr);
					break;
				case REG_BINARY:
					SetRegKey(CFG[I].KeyName, CFG[I].ValName,(BYTE*)CFG[I].ValPtr,CFG[I].DefDWord);
					break;
			}
	}

	/* <������������> *************************************************** */
	FileFilter::SaveFilters();
	FileList::SavePanelModes();

	if (Ask)
		CtrlObject->Macro.SaveMacros();

	/* *************************************************** </������������> */
}
