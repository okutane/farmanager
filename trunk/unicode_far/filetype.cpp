/*
filetype.cpp

������ � ������������ ������
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

#include "filetype.hpp"
#include "keys.hpp"
#include "dialog.hpp"
#include "vmenu2.hpp"
#include "plognmn.hpp"
#include "ctrlobj.hpp"
#include "cmdline.hpp"
#include "history.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "rdrwdsk.hpp"
#include "savescr.hpp"
#include "CFileMask.hpp"
#include "message.hpp"
#include "interf.hpp"
#include "config.hpp"
#include "execute.hpp"
#include "fnparce.hpp"
#include "strmix.hpp"
#include "configdb.hpp"
#include "pathmix.hpp"
#include "language.hpp"

/* $ 25.04.2001 DJ
   ��������� @ � IF EXIST: �������, ������� ��������� ������� �� ������
   � IF EXIST � ������ @ � ���������� TRUE, ���� ������� IF EXIST
   ��������, � FALSE � ��������� ������/
*/

bool ExtractIfExistCommand(string &strCommandText)
{
	bool Result=true;
	const wchar_t *wPtrCmd=PrepareOSIfExist(strCommandText);

	// ��! ������� �� ���������!!!
	// (��������, ���� ������������� ������, � ��� �����)
	// �����-�� ������� ��������� ���� ����!
	if (wPtrCmd)
	{
		if (!*wPtrCmd)
		{
			Result=false;
		}
		else
		{
			size_t offset = wPtrCmd-strCommandText.CPtr();
			wchar_t *CommandText = strCommandText.GetBuffer();
			wchar_t *PtrCmd = CommandText+offset;
			// �������� "if exist"
			wmemmove(CommandText+(*CommandText==L'@'?1:0),PtrCmd,StrLength(PtrCmd)+1);
			strCommandText.ReleaseBuffer();
		}
	}

	return Result;
}

int GetDescriptionWidth()
{
	int Width=0;
	DWORD Index=0;
	unsigned __int64 id;
	string strMask;
	string strDescription;
	CFileMask FMask;

	while (Global->Db->AssocConfig()->EnumMasks(Index++,&id,strMask))
	{
		if (!FMask.Set(strMask, FMF_SILENT))
			continue;

		Global->Db->AssocConfig()->GetDescription(id,strDescription);

		int CurWidth = HiStrlen(strDescription);

		if (CurWidth>Width)
			Width=CurWidth;
	}

	return Width;
}

/* $ 14.01.2001 SVS
   ������� �����������������.
   ���� ����������� "IF" � ��� �����������, �� �������
   ���������� � ������

   ���������� ��� F3, F4 - ����������
   Enter � ��� ������ - ����������.
*/
/* $ 06.07.2001
   + ���������� CFileMask ������ GetCommaWord, ���� ����� ���������� ����, ���
     ����� ������������ ����� ����������
   - ����� ���������� ��� ������ �� ������������� ����� ������ ���� "*.*"
     (��� �����-��, �����, ����� ���-������)
*/
bool ProcessLocalFileTypes(const string& Name, const string& ShortName, FILETYPE_MODE Mode, bool AlwaysWaitFinish)
{
	string strCommand, strDescription, strMask;
	{
		MenuItemEx TypesMenuItem;
		VMenu2 TypesMenu(MSG(MSelectAssocTitle),nullptr,0,ScrY-4);
		TypesMenu.SetHelp(L"FileAssoc");
		TypesMenu.SetFlags(VMENU_WRAPMODE);
		int ActualCmdCount=0; // ������������ ���������� � ����
		CFileMask FMask; // ��� ������ � ������� ������

		int CommandCount=0;
		DWORD Index=0;
		unsigned __int64 id;
		string FileName = PointToName(Name);

		while (Global->Db->AssocConfig()->EnumMasksForType(Mode,Index++,&id,strMask))
		{
			strCommand.Clear();

			if (FMask.Set(strMask,FMF_SILENT))
			{
				if (FMask.Compare(FileName))
				{
					Global->Db->AssocConfig()->GetCommand(id,Mode,strCommand);

					if (!strCommand.IsEmpty())
					{
						Global->Db->AssocConfig()->GetDescription(id,strDescription);
						CommandCount++;
					}
				}

				if (strCommand.IsEmpty())
					continue;
			}

			TypesMenuItem.Clear();
			string strCommandText = strCommand;
			SubstFileName(strCommandText,Name, ShortName,nullptr,nullptr,nullptr,nullptr,TRUE);

			// ��� "�����������", ������ �������� ������� "if exist"
			if (!ExtractIfExistCommand(strCommandText))
				continue;

			ActualCmdCount++;

			if (!strDescription.IsEmpty())
				SubstFileName(strDescription, Name, ShortName, nullptr, nullptr, nullptr, nullptr, TRUE);
			else
				strDescription = strCommandText;

			TypesMenuItem.strName = strDescription;
			TypesMenuItem.SetSelect(Index==1);
			TypesMenu.SetUserData(strCommand.CPtr(), (strCommand.GetLength()+1)*sizeof(wchar_t), TypesMenu.AddItem(&TypesMenuItem));
		}

		if (!CommandCount)
			return false;

		if (!ActualCmdCount)
			return true;

		int ExitCode=0;

		if (ActualCmdCount>1)
		{
			ExitCode=TypesMenu.Run();

			if (ExitCode<0)
				return true;
		}

		strCommand = static_cast<const wchar_t*>(TypesMenu.GetUserData(nullptr, 0, ExitCode));
	}

	string strListName, strAnotherListName;
	string strShortListName, strAnotherShortListName;
	int PreserveLFN=SubstFileName(strCommand, Name, ShortName, &strListName, &strAnotherListName, &strShortListName, &strAnotherShortListName);
	bool ListFileUsed=!strListName.IsEmpty()||!strAnotherListName.IsEmpty()||!strShortListName.IsEmpty()||!strAnotherShortListName.IsEmpty();

	// ����� ��� "�����������", ������ �������� ������� "if exist"
	if (ExtractIfExistCommand(strCommand))
	{
		PreserveLongName PreserveName(ShortName, PreserveLFN);
		RemoveExternalSpaces(strCommand);

		if (!strCommand.IsEmpty())
		{
			Global->CtrlObject->CmdLine->ExecString(strCommand,AlwaysWaitFinish, false, false, ListFileUsed, false,
				Mode == FILETYPE_VIEW || Mode == FILETYPE_ALTVIEW || Mode == FILETYPE_EDIT || Mode == FILETYPE_ALTEDIT);
			if (!(Global->Opt->ExcludeCmdHistory&EXCLUDECMDHISTORY_NOTFARASS) && !AlwaysWaitFinish) //AN
				Global->CtrlObject->CmdHistory->AddToHistory(strCommand);
		}
	}

	if (!strListName.IsEmpty())
		apiDeleteFile(strListName);

	if (!strAnotherListName.IsEmpty())
		apiDeleteFile(strAnotherListName);

	if (!strShortListName.IsEmpty())
		apiDeleteFile(strShortListName);

	if (!strAnotherShortListName.IsEmpty())
		apiDeleteFile(strAnotherShortListName);

	return true;
}


void ProcessGlobalFileTypes(const wchar_t *Name, bool AlwaysWaitFinish, bool RunAs)
{
	string strName(Name);
	QuoteSpace(strName);
	Global->CtrlObject->CmdLine->ExecString(strName, AlwaysWaitFinish, true, true, false, RunAs);

	if (!(Global->Opt->ExcludeCmdHistory&EXCLUDECMDHISTORY_NOTWINASS) && !AlwaysWaitFinish)
	{
		Global->CtrlObject->CmdHistory->AddToHistory(strName);
	}
}

/*
  ������������ ��� ������� �������� ��������� � �������
*/
void ProcessExternal(const string& Command, const string& Name, const string& ShortName, bool AlwaysWaitFinish)
{
	string strListName, strAnotherListName;
	string strShortListName, strAnotherShortListName;
	string strFullName, strFullShortName;
	string strExecStr = Command;
	string strFullExecStr = Command;
	{
		int PreserveLFN=SubstFileName(strExecStr, Name, ShortName, &strListName, &strAnotherListName, &strShortListName, &strAnotherShortListName);
		bool ListFileUsed=!strListName.IsEmpty()||!strAnotherListName.IsEmpty()||!strShortListName.IsEmpty()||!strAnotherShortListName.IsEmpty();

		// ����� ��� "�����������", ������ �������� ������� "if exist"
		if (!ExtractIfExistCommand(strExecStr))
			return;

		PreserveLongName PreserveName(ShortName,PreserveLFN);
		ConvertNameToFull(Name,strFullName);
		ConvertNameToShort(strFullName,strFullShortName);
		//BUGBUGBUGBUGBUGBUG !!! Same ListNames!!!
		SubstFileName(strFullExecStr,strFullName,strFullShortName,&strListName,&strAnotherListName, &strShortListName, &strAnotherShortListName);

		// ����� ��� "�����������", ������ �������� ������� "if exist"
		if (!ExtractIfExistCommand(strFullExecStr))
			return;

		Global->CtrlObject->ViewHistory->AddToHistory(strFullExecStr,AlwaysWaitFinish?3:2);

		Global->CtrlObject->CmdLine->ExecString(strExecStr,AlwaysWaitFinish, 0, 0, ListFileUsed, false, true);
	}

	if (!strListName.IsEmpty())
		apiDeleteFile(strListName);

	if (!strAnotherListName.IsEmpty())
		apiDeleteFile(strAnotherListName);


	if (!strShortListName.IsEmpty())
		apiDeleteFile(strShortListName);

	if (!strAnotherShortListName.IsEmpty())
		apiDeleteFile(strAnotherShortListName);
}

static int FillFileTypesMenu(VMenu2 *TypesMenu,int MenuPos)
{
	int DizWidth=GetDescriptionWidth();
	MenuItemEx TypesMenuItem;
	TypesMenu->DeleteItems();
	DWORD Index=0;
	string strMask;
	string strTitle;
	unsigned __int64 id;

	while (Global->Db->AssocConfig()->EnumMasks(Index++,&id,strMask))
	{
		TypesMenuItem.Clear();

		string strMenuText;

		if (DizWidth)
		{
			Global->Db->AssocConfig()->GetDescription(id,strTitle);

			size_t AddLen=strTitle.GetLength() - HiStrlen(strTitle);

			strMenuText.Format(L"%-*.*s %c ",DizWidth+AddLen,DizWidth+AddLen,strTitle.CPtr(),BoxSymbols[BS_V1]);
		}

		strMenuText += strMask;
		TypesMenuItem.strName = strMenuText;
		TypesMenuItem.SetSelect((int)(Index-1)==MenuPos);
		TypesMenu->SetUserData(&id, sizeof(id), TypesMenu->AddItem(&TypesMenuItem));
	}

	return (int)(Index-1);
}

enum EDITTYPERECORD
{
	ETR_DOUBLEBOX,
	ETR_TEXT_MASKS,
	ETR_EDIT_MASKS,
	ETR_TEXT_DESCR,
	ETR_EDIT_DESCR,
	ETR_SEPARATOR1,
	ETR_COMBO_EXEC,
	ETR_EDIT_EXEC,
	ETR_COMBO_ALTEXEC,
	ETR_EDIT_ALTEXEC,
	ETR_COMBO_VIEW,
	ETR_EDIT_VIEW,
	ETR_COMBO_ALTVIEW,
	ETR_EDIT_ALTVIEW,
	ETR_COMBO_EDIT,
	ETR_EDIT_EDIT,
	ETR_COMBO_ALTEDIT,
	ETR_EDIT_ALTEDIT,
	ETR_SEPARATOR2,
	ETR_BUTTON_OK,
	ETR_BUTTON_CANCEL,
};

intptr_t EditTypeRecordDlgProc(Dialog* Dlg,intptr_t Msg,intptr_t Param1,void* Param2)
{
	switch (Msg)
	{
		case DN_BTNCLICK:

			switch (Param1)
			{
				case ETR_COMBO_EXEC:
				case ETR_COMBO_ALTEXEC:
				case ETR_COMBO_VIEW:
				case ETR_COMBO_ALTVIEW:
				case ETR_COMBO_EDIT:
				case ETR_COMBO_ALTEDIT:
					Dlg->SendMessage(DM_ENABLE,Param1+1,ToPtr(reinterpret_cast<intptr_t>(Param2)==BSTATE_CHECKED?TRUE:FALSE));
					break;
				default:
					break;
			}

			break;
		case DN_CLOSE:

			if (Param1==ETR_BUTTON_OK)
			{
				BOOL Result=TRUE;
				string Masks(reinterpret_cast<LPCWSTR>(Dlg->SendMessage(DM_GETCONSTTEXTPTR,ETR_EDIT_MASKS,0)));
				CFileMask FMask;

				if (!FMask.Set(Masks,0))
				{
					Result=FALSE;
				}

				return Result;
			}

			break;
		default:
			break;
	}

	return Dlg->DefProc(Msg,Param1,Param2);
}

bool EditTypeRecord(unsigned __int64 EditPos,bool NewRec)
{
	const int DlgX=76,DlgY=23;
	FarDialogItem EditDlgData[]=
	{
		{DI_DOUBLEBOX,3, 1,DlgX-4,DlgY-2,0,nullptr,nullptr,0,MSG(MFileAssocTitle)},
		{DI_TEXT,     5, 2, 0, 2,0,nullptr,nullptr,0,MSG(MFileAssocMasks)},
		{DI_EDIT,     5, 3,DlgX-6, 3,0,L"Masks",nullptr,DIF_FOCUS|DIF_HISTORY,L""},
		{DI_TEXT,     5, 4, 0, 4,0,nullptr,nullptr,0,MSG(MFileAssocDescr)},
		{DI_EDIT,     5, 5,DlgX-6, 5,0,nullptr,nullptr,0,L""},
		{DI_TEXT,     -1, 6, 0, 6,0,nullptr,nullptr,DIF_SEPARATOR,L""},
		{DI_CHECKBOX, 5, 7, 0, 7,1,nullptr,nullptr,0,MSG(MFileAssocExec)},
		{DI_EDIT,     9, 8,DlgX-6, 8,0,nullptr,nullptr,DIF_EDITPATH|DIF_EDITPATHEXEC,L""},
		{DI_CHECKBOX, 5, 9, 0, 9,1,nullptr,nullptr,0,MSG(MFileAssocAltExec)},
		{DI_EDIT,     9,10,DlgX-6,10,0,nullptr,nullptr,DIF_EDITPATH|DIF_EDITPATHEXEC,L""},
		{DI_CHECKBOX, 5,11, 0,11,1,nullptr,nullptr,0,MSG(MFileAssocView)},
		{DI_EDIT,     9,12,DlgX-6,12,0,nullptr,nullptr,DIF_EDITPATH|DIF_EDITPATHEXEC,L""},
		{DI_CHECKBOX, 5,13, 0,13,1,nullptr,nullptr,0,MSG(MFileAssocAltView)},
		{DI_EDIT,     9,14,DlgX-6,14,0,nullptr,nullptr,DIF_EDITPATH|DIF_EDITPATHEXEC,L""},
		{DI_CHECKBOX, 5,15, 0,15,1,nullptr,nullptr,0,MSG(MFileAssocEdit)},
		{DI_EDIT,     9,16,DlgX-6,16,0,nullptr,nullptr,DIF_EDITPATH|DIF_EDITPATHEXEC,L""},
		{DI_CHECKBOX, 5,17, 0,17,1,nullptr,nullptr,0,MSG(MFileAssocAltEdit)},
		{DI_EDIT,     9,18,DlgX-6,18,0,nullptr,nullptr,DIF_EDITPATH|DIF_EDITPATHEXEC,L""},
		{DI_TEXT,     -1,DlgY-4, 0,DlgY-4,0,nullptr,nullptr,DIF_SEPARATOR,L""},
		{DI_BUTTON,   0,DlgY-3, 0,DlgY-3,0,nullptr,nullptr,DIF_DEFAULTBUTTON|DIF_CENTERGROUP,MSG(MOk)},
		{DI_BUTTON,   0,DlgY-3, 0,DlgY-3,0,nullptr,nullptr,DIF_CENTERGROUP,MSG(MCancel)},
	};
	MakeDialogItemsEx(EditDlgData,EditDlg);

	if (!NewRec)
	{
		Global->Db->AssocConfig()->GetMask(EditPos,EditDlg[ETR_EDIT_MASKS].strData);
		Global->Db->AssocConfig()->GetDescription(EditPos,EditDlg[ETR_EDIT_DESCR].strData);
		for (int i=FILETYPE_EXEC,Item=ETR_EDIT_EXEC; i<=FILETYPE_ALTEDIT; i++,Item+=2)
		{
			bool on=false;
			if (!Global->Db->AssocConfig()->GetCommand(EditPos,i,EditDlg[Item].strData,&on) || !on)
			{
				EditDlg[Item-1].Selected = BSTATE_UNCHECKED;
				EditDlg[Item].Flags |= DIF_DISABLE;
			}
			else if (on)
			{
				EditDlg[Item-1].Selected = BSTATE_CHECKED;
			}
		}
	}

	Dialog Dlg(EditDlg,ARRAYSIZE(EditDlg),EditTypeRecordDlgProc);
	Dlg.SetHelp(L"FileAssocModify");
	Dlg.SetPosition(-1,-1,DlgX,DlgY);
	Dlg.Process();

	if (Dlg.GetExitCode()==ETR_BUTTON_OK)
	{
		if (NewRec)
		{
			EditPos = Global->Db->AssocConfig()->AddType(EditPos,EditDlg[ETR_EDIT_MASKS].strData,EditDlg[ETR_EDIT_DESCR].strData);
		}
		else
		{
			Global->Db->AssocConfig()->UpdateType(EditPos,EditDlg[ETR_EDIT_MASKS].strData,EditDlg[ETR_EDIT_DESCR].strData);
		}

		for (int i=FILETYPE_EXEC,Item=ETR_EDIT_EXEC; i<=FILETYPE_ALTEDIT; i++,Item+=2)
		{
			Global->Db->AssocConfig()->SetCommand(EditPos,i,EditDlg[Item].strData,EditDlg[Item-1].Selected==BSTATE_CHECKED);
		}

		return true;
	}

	return false;
}

bool DeleteTypeRecord(unsigned __int64 DeletePos)
{
	string strMask;
	Global->Db->AssocConfig()->GetMask(DeletePos,strMask);
	InsertQuote(strMask);

	if (!Message(MSG_WARNING,2,MSG(MAssocTitle),MSG(MAskDelAssoc),strMask,MSG(MDelete),MSG(MCancel)))
	{
		Global->Db->AssocConfig()->DelType(DeletePos);
		return true;
	}

	return false;
}

void EditFileTypes()
{
	Global->Db->AssocConfig()->BeginTransaction();

	int NumLine=0;
	int MenuPos=0;
	unsigned __int64 id;
	VMenu2 TypesMenu(MSG(MAssocTitle),nullptr,0,ScrY-4);
	TypesMenu.SetHelp(L"FileAssoc");
	TypesMenu.SetFlags(VMENU_WRAPMODE);
	TypesMenu.SetBottomTitle(MSG(MAssocBottom));

	while (1)
	{
		NumLine=FillFileTypesMenu(&TypesMenu,MenuPos);
		int ExitCode=TypesMenu.Run([&](int Key)->int
		{
			MenuPos=TypesMenu.GetSelectPos();

			int KeyProcessed = 1;

			switch (Key)
			{
				case KEY_NUMDEL:
				case KEY_DEL:

					if (MenuPos<NumLine)
					{
						if (TypesMenu.GetUserData(&id,sizeof(id),MenuPos))
							DeleteTypeRecord(id);
						NumLine=FillFileTypesMenu(&TypesMenu,MenuPos);
					}

					break;
				case KEY_NUMPAD0:
				case KEY_INS:
					if (MenuPos-1 >= 0 && TypesMenu.GetUserData(&id,sizeof(id),MenuPos-1))
						EditTypeRecord(id,true);
					else
						EditTypeRecord(0,true);
					NumLine=FillFileTypesMenu(&TypesMenu,MenuPos);
					break;
				case KEY_NUMENTER:
				case KEY_ENTER:
				case KEY_F4:

					if (MenuPos<NumLine)
					{
						if (TypesMenu.GetUserData(&id,sizeof(id),MenuPos))
							EditTypeRecord(id,false);
						NumLine=FillFileTypesMenu(&TypesMenu,MenuPos);
					}
					return 1;

				case KEY_CTRLUP:
				case KEY_RCTRLUP:
				case KEY_CTRLDOWN:
				case KEY_RCTRLDOWN:
				{
					if (!((Key==KEY_CTRLUP || Key==KEY_RCTRLUP) && !MenuPos) &&
						!((Key==KEY_CTRLDOWN || Key==KEY_RCTRLDOWN) && MenuPos==TypesMenu.GetItemCount()-1))
					{
						int NewMenuPos=MenuPos+((Key==KEY_CTRLUP || Key==KEY_RCTRLUP)?-1:+1);
						unsigned __int64 id2=0;
						if (TypesMenu.GetUserData(&id,sizeof(id),MenuPos))
							if (TypesMenu.GetUserData(&id2,sizeof(id2),NewMenuPos))
								if (Global->Db->AssocConfig()->SwapPositions(id,id2))
									MenuPos=NewMenuPos;
						NumLine=FillFileTypesMenu(&TypesMenu,MenuPos);
					}
				}
				break;

				default:
					KeyProcessed = 0;
			}
			return KeyProcessed;
		});

		if (ExitCode!=-1)
		{
			MenuPos=ExitCode;
			TypesMenu.Key(KEY_F4);
			continue;
		}

		break;
	}

	Global->Db->AssocConfig()->EndTransaction();
}
