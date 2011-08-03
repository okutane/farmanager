/*
history.cpp

������� (Alt-F8, Alt-F11, Alt-F12)
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

#include "history.hpp"
#include "language.hpp"
#include "keys.hpp"
#include "vmenu.hpp"
#include "lang.hpp"
#include "message.hpp"
#include "clipboard.hpp"
#include "config.hpp"
#include "strmix.hpp"
#include "dialog.hpp"
#include "interf.hpp"
#include "ctrlobj.hpp"
#include "configdb.hpp"
#include "datetime.hpp"

History::History(enumHISTORYTYPE TypeHistory, const wchar_t *HistoryName, size_t HistoryCount, const int *EnableSave, bool SaveType):
	strHistoryName(HistoryName),
	EnableAdd(true),
	KeepSelectedPos(false),
	SaveType(SaveType),
	RemoveDups(1),
	TypeHistory(TypeHistory),
	HistoryCount(HistoryCount),
	EnableSave(EnableSave),
	CurrentItem(0)
{
}

History::~History()
{
}

void History::CompactHistory()
{
	HistoryCfg->BeginTransaction();

	HistoryCfg->DeleteOldUnlocked(HISTORYTYPE_CMD,L"",90,1000);
	HistoryCfg->DeleteOldUnlocked(HISTORYTYPE_FOLDER,L"",90,1000);
	HistoryCfg->DeleteOldUnlocked(HISTORYTYPE_VIEW,L"",90,1000);

	DWORD index=0;
	string strName;
	while (HistoryCfg->EnumLargeHistories(index++, 1000, HISTORYTYPE_DIALOG, strName))
	{
		HistoryCfg->DeleteOldUnlocked(HISTORYTYPE_DIALOG,strName,90,1000);
	}

	HistoryCfg->EndTransaction();
}

/*
   SaveForbid - ������������� ��������� ������ ����������� ������.
                ������������ �� ������ �������
*/
void History::AddToHistory(const wchar_t *Str, int Type, const wchar_t *Prefix, bool SaveForbid)
{
	if (!EnableAdd || !*EnableSave || SaveForbid)
		return;

	if (CtrlObject->Macro.IsExecuting() && !CtrlObject->Macro.IsHistroyEnable((int)TypeHistory))
		return;

	if (!Str || !*Str)
		return;

	bool Lock = false;
	string strName;

	if (TypeHistory == HISTORYTYPE_FOLDER && Prefix && *Prefix)
	{
		strName = Prefix;
		strName += L":";
	}

	strName += Str;

	HistoryCfg->BeginTransaction();

	if (RemoveDups) // ������� ���������?
	{
		DWORD index=0;
		string strHName;
		int HType;
		bool HLock;
		unsigned __int64 id;
		unsigned __int64 Time;
		while (HistoryCfg->Enum(index++,TypeHistory,strHistoryName,&id,strHName,&HType,&HLock,&Time))
		{
			if (EqualType(Type,HType))
			{
				if ((RemoveDups==1 && !StrCmp(strName,strHName)) || (RemoveDups==2 && !StrCmpI(strName,strHName)))
				{
					Lock = Lock || HLock;
					HistoryCfg->Delete(id);
					break;
				}
			}
		}
	}

	HistoryCfg->Add(TypeHistory, strHistoryName, strName, Type, Lock);

	ResetPosition();

	HistoryCfg->EndTransaction();
}

bool History::ReadLastItem(const wchar_t *HistoryName, string &strStr)
{
	strStr.Clear();
	return HistoryCfg->GetNewest(HISTORYTYPE_DIALOG, HistoryName, strStr);
}

const wchar_t *History::GetTitle(int Type)
{
	switch (Type)
	{
		case 0: // ������
			return MSG(MHistoryView);
		case 1: // ������� �������� � ���������
		case 4: // �������� � �����
			return MSG(MHistoryEdit);
		case 2: // external - ��� ��������
		case 3: // external - AlwaysWaitFinish
			return MSG(MHistoryExt);
	}

	return L"";
}

int History::Select(const wchar_t *Title, const wchar_t *HelpTopic, string &strStr, int &Type)
{
	int Height=ScrY-8;
	VMenu HistoryMenu(Title,nullptr,0,Height);
	HistoryMenu.SetFlags(VMENU_SHOWAMPERSAND|VMENU_WRAPMODE);

	if (HelpTopic)
		HistoryMenu.SetHelp(HelpTopic);

	HistoryMenu.SetPosition(-1,-1,0,0);
	HistoryMenu.AssignHighlights(TRUE);
	return ProcessMenu(strStr, Title, HistoryMenu, Height, Type, nullptr);
}

int History::Select(VMenu &HistoryMenu, int Height, Dialog *Dlg, string &strStr)
{
	int Type=0;
	return ProcessMenu(strStr, nullptr, HistoryMenu, Height, Type, Dlg);
}

/*
 Return:
  -1 - Error???
   0 - Esc
   1 - Enter
   2 - Shift-Enter
   3 - Ctrl-Enter
   4 - F3
   5 - F4
   6 - Ctrl-Shift-Enter
   7 - Ctrl-Alt-Enter
*/

int History::ProcessMenu(string &strStr, const wchar_t *Title, VMenu &HistoryMenu, int Height, int &Type, Dialog *Dlg)
{
	MenuItemEx MenuItem;
	unsigned __int64 SelectedRecord = 0;
	string strSelectedRecordName;
	int SelectedRecordType = 0;
	FarListPos Pos={0,0};
	int Code=-1;
	int RetCode=1;
	bool Done=false;
	bool SetUpMenuPos=false;

	if (TypeHistory == HISTORYTYPE_DIALOG && !HistoryCfg->Count(TypeHistory,strHistoryName))
		return 0;

	while (!Done)
	{
		bool IsUpdate=false;
		HistoryMenu.DeleteItems();
		HistoryMenu.Modal::ClearDone();

		{
			bool bSelected=false;
			DWORD index=0;
			string strHName;
			int HType;
			bool HLock;
			unsigned __int64 id;
			unsigned __int64 Time;
			SYSTEMTIME st;
			GetLocalTime(&st);
			int LastDay=0, LastMonth = 0, LastYear = 0;
			while (HistoryCfg->Enum(index++,TypeHistory,strHistoryName,&id,strHName,&HType,&HLock,&Time,TypeHistory==HISTORYTYPE_DIALOG))
			{
				string strRecord;

				if (TypeHistory == HISTORYTYPE_VIEW)
				{
					strRecord += GetTitle(HType);
					strRecord += L":";
					strRecord += (HType==4?L"-":L" ");
				}
				SYSTEMTIME SavedTime;
				FILETIME FTTime;
				UI64ToFileTime(Time, &FTTime);
				FILETIME LFTTime;
				FileTimeToLocalFileTime(&FTTime, &LFTTime);
				FileTimeToSystemTime(&LFTTime, &SavedTime);
				if(LastDay != SavedTime.wDay || LastMonth != SavedTime.wMonth || LastYear != SavedTime.wYear)
				{
					LastDay = SavedTime.wDay;
					LastMonth = SavedTime.wMonth;
					LastYear = SavedTime.wYear;
					MenuItemEx Separator={};
					Separator.Flags = LIF_SEPARATOR;
					string strTime;
					ConvertDate(FTTime, Separator.strName, strTime, 5, FALSE, FALSE, TRUE, TRUE);
					HistoryMenu.AddItem(&Separator);
				}
				strRecord += strHName;

				if (TypeHistory != HISTORYTYPE_DIALOG)
					ReplaceStrings(strRecord, L"&",L"&&", -1);

				MenuItem.Clear();
				MenuItem.strName = strRecord;
				MenuItem.SetCheck(HLock?1:0);

				if (!SetUpMenuPos && CurrentItem==id)
				{
					MenuItem.SetSelect(TRUE);
					bSelected=true;
				}

				HistoryMenu.SetUserData(&id,sizeof(id),HistoryMenu.AddItem(&MenuItem));
			}

			if (!SetUpMenuPos && !bSelected && TypeHistory!=HISTORYTYPE_DIALOG)
			{
				FarListPos p;
				p.SelectPos = HistoryMenu.GetItemCount()-1;
				p.TopPos = 0;
				HistoryMenu.SetSelectPos(&p);
			}
		}

		//MenuItem.Clear ();
		//MenuItem.strName = L"                    ";
		//if (!SetUpMenuPos)
		//MenuItem.SetSelect(CurLastPtr==-1 || CurLastPtr>=HistoryList.Length);
		//HistoryMenu.SetUserData(nullptr,sizeof(OneItem *),HistoryMenu.AddItem(&MenuItem));

		if (TypeHistory == HISTORYTYPE_DIALOG)
			Dlg->SetComboBoxPos();
		else
			HistoryMenu.SetPosition(-1,-1,0,0);

		if (SetUpMenuPos)
		{
			Pos.SelectPos=Pos.SelectPos < HistoryMenu.GetItemCount() ? Pos.SelectPos : HistoryMenu.GetItemCount()-1;
			Pos.TopPos=Min(Pos.TopPos,HistoryMenu.GetItemCount()-Height);
			HistoryMenu.SetSelectPos(&Pos);
			SetUpMenuPos=false;
		}

		/*BUGBUG???
			if (TypeHistory == HISTORYTYPE_DIALOG)
			{
					//  ����� ���������� ������� �� ��������� �������� ���������
					BYTE RealColors[VMENU_COLOR_COUNT];
					FarListColors ListColors={0};
					ListColors.ColorCount=VMENU_COLOR_COUNT;
					ListColors.Colors=RealColors;
					HistoryMenu.GetColors(&ListColors);
					if(DlgProc(this,DN_CTLCOLORDLGLIST,CurItem->ID,(INT_PTR)&ListColors))
						HistoryMenu.SetColors(&ListColors);
				}
		*/
		HistoryMenu.Show();

		while (!HistoryMenu.Done())
		{
			if (TypeHistory == HISTORYTYPE_DIALOG && (!Dlg->GetDropDownOpened() || !HistoryMenu.GetItemCount()))
			{
				HistoryMenu.ProcessKey(KEY_ESC);
				continue;
			}

			int Key=HistoryMenu.ReadInput();

			if (TypeHistory == HISTORYTYPE_DIALOG && Key==KEY_TAB) // Tab � ������ ������� �������� - ������ Enter
			{
				HistoryMenu.ProcessKey(KEY_ENTER);
				continue;
			}

			HistoryMenu.GetSelectPos(&Pos);
			void* Data = HistoryMenu.GetUserData(nullptr, 0,Pos.SelectPos);
			unsigned __int64 CurrentRecord = Data? *static_cast<unsigned __int64*>(Data) : 0;

			switch (Key)
			{
				case KEY_CTRLR: // �������� � ��������� �����������
				case KEY_RCTRLR:
				{
					if (TypeHistory == HISTORYTYPE_FOLDER || TypeHistory == HISTORYTYPE_VIEW)
					{
						bool ModifiedHistory=false;

						HistoryCfg->BeginTransaction();

						DWORD index=0;
						string strHName;
						int HType;
						bool HLock;
						unsigned __int64 id;
						unsigned __int64 Time;
						while (HistoryCfg->Enum(index++,TypeHistory,strHistoryName,&id,strHName,&HType,&HLock,&Time))
						{
							if (HLock) // ���������� �� �������
								continue;

							// ����� ������ �� �������
							if (apiGetFileAttributes(strHName) == INVALID_FILE_ATTRIBUTES)
							{
								HistoryCfg->Delete(id);
								ModifiedHistory=true;
							}
						}

						HistoryCfg->EndTransaction();

						if (ModifiedHistory) // ����������� �� ������ ������������
						{
							HistoryMenu.Modal::SetExitCode(Pos.SelectPos);
							HistoryMenu.SetUpdateRequired(TRUE);
							IsUpdate=true;
						}

						ResetPosition();
					}

					break;
				}
				case KEY_CTRLSHIFTNUMENTER:
				case KEY_RCTRLSHIFTNUMENTER:
				case KEY_CTRLNUMENTER:
				case KEY_RCTRLNUMENTER:
				case KEY_SHIFTNUMENTER:
				case KEY_CTRLSHIFTENTER:
				case KEY_RCTRLSHIFTENTER:
				case KEY_CTRLENTER:
				case KEY_RCTRLENTER:
				case KEY_SHIFTENTER:
				case KEY_CTRLALTENTER:
				case KEY_RCTRLRALTENTER:
				case KEY_CTRLALTNUMENTER:
				case KEY_RCTRLRALTNUMENTER:
				{
					if (TypeHistory == HISTORYTYPE_DIALOG)
						break;

					HistoryMenu.Modal::SetExitCode(Pos.SelectPos);
					Done=true;
					RetCode = (Key==KEY_CTRLALTENTER||Key==KEY_RCTRLRALTENTER||Key==KEY_CTRLALTNUMENTER||Key==KEY_RCTRLRALTNUMENTER)?7
						:((Key==KEY_CTRLSHIFTENTER||Key==KEY_RCTRLSHIFTENTER||Key==KEY_CTRLSHIFTNUMENTER||Key==KEY_RCTRLSHIFTNUMENTER)?6
							:((Key==KEY_SHIFTENTER||Key==KEY_SHIFTNUMENTER)?2:3));
					break;
				}
				case KEY_F3:
				case KEY_F4:
				case KEY_NUMPAD5:  case KEY_SHIFTNUMPAD5:
				{
					if (TypeHistory == HISTORYTYPE_DIALOG)
						break;

					HistoryMenu.Modal::SetExitCode(Pos.SelectPos);
					Done=true;
					RetCode=(Key==KEY_F4? 5 : 4);
					break;
				}
				// $ 09.04.2001 SVS - ���� - ����������� �� ������� ������ � Clipboard
				case KEY_CTRLC:
				case KEY_RCTRLC:
				case KEY_CTRLINS:  case KEY_CTRLNUMPAD0:
				case KEY_RCTRLINS: case KEY_RCTRLNUMPAD0:
				{
					if (CurrentRecord)
					{
						string strName;
						if (HistoryCfg->Get(CurrentRecord, strName))
							CopyToClipboard(strName);
					}

					break;
				}
				// Lock/Unlock
				case KEY_INS:
				case KEY_NUMPAD0:
				{
					if (CurrentRecord)
					{
						HistoryCfg->FlipLock(CurrentRecord);
						HistoryMenu.Hide();
						ResetPosition();
						HistoryMenu.Modal::SetExitCode(Pos.SelectPos);
						HistoryMenu.SetUpdateRequired(TRUE);
						IsUpdate=true;
						SetUpMenuPos=true;
					}

					break;
				}
				case KEY_SHIFTNUMDEL:
				case KEY_SHIFTDEL:
				{
					if (CurrentRecord && !HistoryCfg->IsLocked(CurrentRecord))
					{
						HistoryMenu.Hide();
						HistoryCfg->Delete(CurrentRecord);
						ResetPosition();
						HistoryMenu.Modal::SetExitCode(Pos.SelectPos);
						HistoryMenu.SetUpdateRequired(TRUE);
						IsUpdate=true;
						SetUpMenuPos=true;
					}

					break;
				}
				case KEY_NUMDEL:
				case KEY_DEL:
				{
					if (HistoryMenu.GetItemCount()/* > 1*/ &&
					        (!Opt.Confirm.HistoryClear ||
					         (Opt.Confirm.HistoryClear &&
					          !Message(MSG_WARNING,2,
					                  MSG((TypeHistory==HISTORYTYPE_CMD || TypeHistory==HISTORYTYPE_DIALOG?MHistoryTitle:
					                       (TypeHistory==HISTORYTYPE_FOLDER?MFolderHistoryTitle:MViewHistoryTitle))),
					                  MSG(MHistoryClear),
					                  MSG(MClear),MSG(MCancel)))))
					{
						HistoryCfg->DeleteAllUnlocked(TypeHistory,strHistoryName);

						ResetPosition();
						HistoryMenu.Hide();
						HistoryMenu.Modal::SetExitCode(Pos.SelectPos);
						HistoryMenu.SetUpdateRequired(TRUE);
						IsUpdate=true;
					}

					break;
				}
				default:
					HistoryMenu.ProcessInput();
					break;
			}
		}

		if (IsUpdate)
			continue;

		Done=true;
		Code=HistoryMenu.Modal::GetExitCode();

		if (Code >= 0)
		{
			SelectedRecord = *static_cast<unsigned __int64*>(HistoryMenu.GetUserData(nullptr, 0, Code));

			if (!SelectedRecord)
				return -1;


			if (!HistoryCfg->Get(SelectedRecord, strSelectedRecordName, &SelectedRecordType))
				return -1;

			//BUGUBUG: eliminate those magic numbers!
			if (SelectedRecordType != 2 && SelectedRecordType != 3 // ignore external
				&& RetCode != 3 && ((TypeHistory == HISTORYTYPE_FOLDER && !SelectedRecordType) || TypeHistory == HISTORYTYPE_VIEW) && apiGetFileAttributes(strSelectedRecordName) == INVALID_FILE_ATTRIBUTES)
			{
				SetLastError(ERROR_FILE_NOT_FOUND);

				if (SelectedRecordType == 1 && TypeHistory == HISTORYTYPE_VIEW) // Edit? ����� ������� � ���� ���� ��������
				{
					if (!Message(MSG_WARNING|MSG_ERRORTYPE,2,Title,strSelectedRecordName,MSG(MViewHistoryIsCreate),MSG(MHYes),MSG(MHNo)))
						break;
				}
				else
				{
					Message(MSG_WARNING|MSG_ERRORTYPE,1,Title,strSelectedRecordName,MSG(MOk));
				}

				Done=false;
				SetUpMenuPos=true;
				HistoryMenu.Modal::SetExitCode(Pos.SelectPos=Code);
				continue;
			}
		}
	}

	if (Code < 0 || !SelectedRecord)
		return 0;

	if (KeepSelectedPos)
	{
		CurrentItem = SelectedRecord;
	}

	strStr = strSelectedRecordName;

	if (RetCode < 4 || RetCode == 6 || RetCode == 7)
	{
		Type=SelectedRecordType;
	}
	else
	{
		Type=RetCode-4;

		if (Type == 1 && SelectedRecordType == 4)
			Type=4;

		RetCode=1;
	}

	return RetCode;
}

void History::GetPrev(string &strStr)
{

	CurrentItem = HistoryCfg->GetPrev(TypeHistory, strHistoryName, CurrentItem, strStr);
}


void History::GetNext(string &strStr)
{
	CurrentItem = HistoryCfg->GetNext(TypeHistory, strHistoryName, CurrentItem, strStr);
}


bool History::GetSimilar(string &strStr, int LastCmdPartLength, bool bAppend)
{
	int Length=(int)strStr.GetLength();

	if (LastCmdPartLength!=-1 && LastCmdPartLength<Length)
		Length=LastCmdPartLength;

	if (LastCmdPartLength==-1)
	{
		ResetPosition();
	}

	int i=0;
	string strName;
	unsigned __int64 HistoryItem=HistoryCfg->CyclicGetPrev(TypeHistory, strHistoryName, CurrentItem, strName);
	while (HistoryItem != CurrentItem)
	{
		if (!HistoryItem)
		{
			if (++i > 1) //infinite loop
				break;
			HistoryItem = HistoryCfg->CyclicGetPrev(TypeHistory, strHistoryName, HistoryItem, strName);
			continue;
		}

		if (!StrCmpNI(strStr,strName,Length) && StrCmp(strStr,strName))
		{
			if (bAppend)
				strStr += &strName[Length];
			else
				strStr = strName;

			CurrentItem = HistoryItem;
			return true;
		}

		HistoryItem = HistoryCfg->CyclicGetPrev(TypeHistory, strHistoryName, HistoryItem, strName);
	}

	return false;
}

bool History::GetAllSimilar(VMenu &HistoryMenu,const wchar_t *Str)
{
	int Length=StrLength(Str);
	DWORD index=0;
	string strHName;
	int HType;
	bool HLock;
	unsigned __int64 id;
	unsigned __int64 Time;
	while (HistoryCfg->Enum(index++,TypeHistory,strHistoryName,&id,strHName,&HType,&HLock,&Time,true))
	{
		if (!StrCmpNI(Str,strHName,Length))
		{
			MenuItemEx NewItem={};
			NewItem.strName = strHName;
			if(HLock)
			{
				NewItem.Flags|=LIF_CHECKED;
			}
			HistoryMenu.SetUserData(&id,sizeof(id),HistoryMenu.AddItem(&NewItem));
		}
	}
	if(HistoryMenu.GetItemCount() == 1 && HistoryMenu.GetItemPtr(0)->strName.GetLength() == static_cast<size_t>(Length))
	{
		HistoryMenu.DeleteItems();
		return false;
	}

	return true;
}

bool History::DeleteIfUnlocked(unsigned __int64 id)
{
	bool b = false;
	if (id && !HistoryCfg->IsLocked(id))
	{
		if (HistoryCfg->Delete(id))
		{
			b = true;
			ResetPosition();
		}
	}
	return b;
}

void History::SetAddMode(bool EnableAdd, int RemoveDups, bool KeepSelectedPos)
{
	History::EnableAdd=EnableAdd;
	History::RemoveDups=RemoveDups;
	History::KeepSelectedPos=KeepSelectedPos;
}

bool History::EqualType(int Type1, int Type2)
{
	return Type1 == Type2 || (TypeHistory == HISTORYTYPE_VIEW && ((Type1 == 4 && Type2 == 1) || (Type1 == 1 && Type2 == 4)))?true:false;
}
