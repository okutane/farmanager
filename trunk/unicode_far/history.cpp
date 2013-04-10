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
#include "vmenu2.hpp"
#include "message.hpp"
#include "clipboard.hpp"
#include "config.hpp"
#include "strmix.hpp"
#include "dialog.hpp"
#include "interf.hpp"
#include "ctrlobj.hpp"
#include "configdb.hpp"
#include "datetime.hpp"
#include "FarGuid.hpp"
#include "DlgGuid.hpp"
#include "scrbuf.hpp"
#include "plugins.hpp"

History::History(enumHISTORYTYPE TypeHistory, const string& HistoryName, const BoolOption& EnableSave, bool SaveType):
	strHistoryName(HistoryName),
	EnableAdd(true),
	KeepSelectedPos(false),
	SaveType(SaveType),
	RemoveDups(1),
	TypeHistory(TypeHistory),
	EnableSave(EnableSave),
	CurrentItem(0)
{
}

History::~History()
{
}

void History::CompactHistory()
{
	Global->Db->HistoryCfg()->BeginTransaction();

	Global->Db->HistoryCfg()->DeleteOldUnlocked(HISTORYTYPE_CMD, L"", Global->Opt->HistoryLifetime, Global->Opt->HistoryCount);
	Global->Db->HistoryCfg()->DeleteOldUnlocked(HISTORYTYPE_FOLDER, L"", Global->Opt->FoldersHistoryLifetime, Global->Opt->FoldersHistoryCount);
	Global->Db->HistoryCfg()->DeleteOldUnlocked(HISTORYTYPE_VIEW, L"", Global->Opt->ViewHistoryLifetime, Global->Opt->ViewHistoryCount);

	DWORD index=0;
	string strName;
	while (Global->Db->HistoryCfg()->EnumLargeHistories(index++, Global->Opt->DialogsHistoryCount, HISTORYTYPE_DIALOG, strName))
	{
		Global->Db->HistoryCfg()->DeleteOldUnlocked(HISTORYTYPE_DIALOG, strName, Global->Opt->DialogsHistoryLifetime, Global->Opt->DialogsHistoryCount);
	}

	Global->Db->HistoryCfg()->EndTransaction();
}

/*
   SaveForbid - ������������� ��������� ������ ����������� ������.
                ������������ �� ������ �������
*/
void History::AddToHistory(const string& Str, int Type, const GUID* Guid, const wchar_t *File, const wchar_t *Data, bool SaveForbid)
{
	if (!EnableAdd || SaveForbid)
		return;

	if (Global->CtrlObject->Macro.IsExecuting() && Global->CtrlObject->Macro.IsHistoryDisable((int)TypeHistory))
		return;

	if (TypeHistory!=HISTORYTYPE_DIALOG && (TypeHistory!=HISTORYTYPE_FOLDER || !Guid || *Guid == FarGuid) && Str.IsEmpty())
		return;

	bool Lock = false;
	string strName(Str),strGuid,strFile(File),strData(Data);
	if(Guid) strGuid=GuidToStr(*Guid);

	unsigned __int64 DeleteId = 0;

	if (RemoveDups) // ������� ���������?
	{
		DWORD index=0;
		string strHName,strHGuid,strHFile,strHData;
		int HType;
		bool HLock;
		unsigned __int64 id;
		unsigned __int64 Time;
		while (HistoryCfgRef()->Enum(index++,TypeHistory,strHistoryName,&id,strHName,&HType,&HLock,&Time,strHGuid,strHFile,strHData))
		{
			if (EqualType(Type,HType))
			{
				int (__cdecl* StrCmpFn)(const wchar_t*,const wchar_t*)=(RemoveDups==2)?StrCmpI:StrCmp;

				if (!StrCmpFn(strName.CPtr(),strHName.CPtr())&&!StrCmpFn(strGuid.CPtr(),strHGuid.CPtr())&&!StrCmpFn(strFile.CPtr(),strHFile.CPtr())&&!StrCmpFn(strData.CPtr(),strHData.CPtr()))
				{
					Lock = Lock || HLock;
					DeleteId = id;
					break;
				}
			}
		}
	}

	HistoryCfgRef()->DeleteAndAddAsync(DeleteId, TypeHistory, strHistoryName, strName, Type, Lock, strGuid, strFile, strData);  //Async - should never be used in a transaction

	ResetPosition();
}

bool History::ReadLastItem(const string& HistoryName, string &strStr)
{
	strStr.Clear();
	return HistoryCfgRef()->GetNewest(HISTORYTYPE_DIALOG, HistoryName, strStr);
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

int History::Select(const wchar_t *Title, const wchar_t *HelpTopic, string &strStr, int &Type, GUID* Guid, string *File, string *Data)
{
	int Height=ScrY-8;
	VMenu2 HistoryMenu(Title,nullptr,0,Height);
	HistoryMenu.SetFlags(VMENU_SHOWAMPERSAND|VMENU_WRAPMODE);

	if (HelpTopic)
		HistoryMenu.SetHelp(HelpTopic);

	HistoryMenu.SetPosition(-1,-1,0,0);
	HistoryMenu.AssignHighlights(TRUE);

	if (TypeHistory == HISTORYTYPE_CMD || TypeHistory == HISTORYTYPE_FOLDER || TypeHistory == HISTORYTYPE_VIEW)
		HistoryMenu.SetId(TypeHistory == HISTORYTYPE_CMD?HistoryCmdId:(TypeHistory == HISTORYTYPE_FOLDER?HistoryFolderId:HistoryEditViewId));

	int ret=ProcessMenu(strStr, Guid, File, Data, Title, HistoryMenu, Height, Type, nullptr);
	Global->ScrBuf->Flush();
	return ret;
}

int History::Select(VMenu2 &HistoryMenu, int Height, Dialog *Dlg, string &strStr)
{
	int Type=0;
	return ProcessMenu(strStr,nullptr ,nullptr ,nullptr , nullptr, HistoryMenu, Height, Type, Dlg);
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

int History::ProcessMenu(string &strStr, GUID* Guid, string *pstrFile, string *pstrData, const wchar_t *Title, VMenu2 &HistoryMenu, int Height, int &Type, Dialog *Dlg)
{
	MenuItemEx MenuItem;
	unsigned __int64 SelectedRecord = 0;
	string strSelectedRecordName,strSelectedRecordGuid,strSelectedRecordFile,strSelectedRecordData;
	int SelectedRecordType = 0;
	FarListPos Pos={sizeof(FarListPos)};
	int Code=-1;
	int RetCode=1;
	bool Done=false;
	bool SetUpMenuPos=false;

	if (TypeHistory == HISTORYTYPE_DIALOG && !HistoryCfgRef()->Count(TypeHistory,strHistoryName))
		return 0;

	while (!Done)
	{
		bool IsUpdate=false;
		HistoryMenu.DeleteItems();
		{
			bool bSelected=false;
			DWORD index=0;
			string strHName,strHGuid,strHFile,strHData;
			int HType;
			bool HLock;
			unsigned __int64 id;
			unsigned __int64 Time;
			SYSTEMTIME st;
			GetLocalTime(&st);
			int LastDay=0, LastMonth = 0, LastYear = 0;
			while (HistoryCfgRef()->Enum(index++,TypeHistory,strHistoryName,&id,strHName,&HType,&HLock,&Time,strHGuid,strHFile,strHData,TypeHistory==HISTORYTYPE_DIALOG))
			{
				string strRecord;

				if (TypeHistory == HISTORYTYPE_VIEW)
				{
					strRecord += GetTitle(HType);
					strRecord += L":";
					strRecord += (HType==4?L"-":L" ");
				}
				if (TypeHistory == HISTORYTYPE_FOLDER)
				{
					GUID HGuid;
					if(StrToGuid(strHGuid,HGuid) &&  HGuid != FarGuid)
					{
						Plugin *pPlugin = Global->CtrlObject->Plugins->FindPlugin(HGuid);
						if(pPlugin)
						{
							strRecord += pPlugin->GetTitle();
							strRecord += L":";
							if(!strHFile.IsEmpty())
							{
								strRecord += strHFile;
								strRecord += L":";
							}
						}
					}
				}
				FILETIME FTTime;
				SYSTEMTIME SavedTime;
				UI64ToFileTime(Time, &FTTime);
				Utc2Local(FTTime, SavedTime);
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
				FarListPos p={sizeof(FarListPos)};
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
		{
			int X1,Y1,X2,Y2;
			Dlg->CalcComboBoxPos(nullptr, HistoryMenu.GetItemCount(), X1, Y1, X2, Y2);
			HistoryMenu.SetPosition(X1, Y1, X2, Y2);
		}
		else
			HistoryMenu.SetPosition(-1,-1,0,0);

		if (SetUpMenuPos)
		{
			Pos.SelectPos=Pos.SelectPos < HistoryMenu.GetItemCount() ? Pos.SelectPos : HistoryMenu.GetItemCount()-1;
			Pos.TopPos=std::min(Pos.TopPos,HistoryMenu.GetItemCount()-Height);
			HistoryMenu.SetSelectPos(&Pos);
			SetUpMenuPos=false;
		}

		/*BUGBUG???
			if (TypeHistory == HISTORYTYPE_DIALOG)
			{
					//  ����� ���������� ������� �� ��������� �������� ���������
					BYTE RealColors[VMENU_COLOR_COUNT];
					FarListColors ListColors={};
					ListColors.ColorCount=VMENU_COLOR_COUNT;
					ListColors.Colors=RealColors;
					HistoryMenu.GetColors(&ListColors);
					if(DlgProc(this,DN_CTLCOLORDLGLIST,CurItem->ID,(intptr_t)&ListColors))
						HistoryMenu.SetColors(&ListColors);
				}
		*/

		if(TypeHistory == HISTORYTYPE_DIALOG && !HistoryMenu.GetItemCount())
			return 0;

		Code=HistoryMenu.Run([&](int Key)->int
		{
			if (TypeHistory == HISTORYTYPE_DIALOG && Key==KEY_TAB) // Tab � ������ ������� �������� - ������ Enter
			{
				HistoryMenu.Close();
				return 1;
			}

			HistoryMenu.GetSelectPos(&Pos);
			void* Data = HistoryMenu.GetUserData(nullptr, 0,Pos.SelectPos);
			unsigned __int64 CurrentRecord = Data? *static_cast<unsigned __int64*>(Data) : 0;
			int KeyProcessed = 1;

			switch (Key)
			{
				case KEY_CTRLR: // �������� � ��������� �����������
				case KEY_RCTRLR:
				{
					if (TypeHistory == HISTORYTYPE_FOLDER || TypeHistory == HISTORYTYPE_VIEW)
					{
						bool ModifiedHistory=false;

						HistoryCfgRef()->BeginTransaction();

						DWORD index=0;
						string strHName,strHGuid,strHFile,strHData;
						int HType;
						bool HLock;
						unsigned __int64 id;
						unsigned __int64 Time;
						while (HistoryCfgRef()->Enum(index++,TypeHistory,strHistoryName,&id,strHName,&HType,&HLock,&Time,strHGuid,strHFile,strHData))
						{
							if (HLock) // ���������� �� �������
								continue;

							// ����� ������ �� �������
							bool kill=false;
							GUID HGuid;
							if(StrToGuid(strHGuid,HGuid) && HGuid != FarGuid)
							{
								Plugin *pPlugin = Global->CtrlObject->Plugins->FindPlugin(HGuid);
								if(!pPlugin) kill=true;
								else if (!strHFile.IsEmpty()&&apiGetFileAttributes(strHFile) == INVALID_FILE_ATTRIBUTES) kill=true;
							}
							else if (apiGetFileAttributes(strHName) == INVALID_FILE_ATTRIBUTES) kill=true;

							if(kill)
							{
								HistoryCfgRef()->Delete(id);
								ModifiedHistory=true;
							}
						}

						HistoryCfgRef()->EndTransaction();

						if (ModifiedHistory) // ����������� �� ������ ������������
						{
							IsUpdate=true;
							HistoryMenu.Close(Pos.SelectPos);
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
				case KEY_CTRLRALTENTER:
				case KEY_RCTRLALTENTER:
				case KEY_CTRLALTNUMENTER:
				case KEY_RCTRLRALTNUMENTER:
				case KEY_CTRLRALTNUMENTER:
				case KEY_RCTRLALTNUMENTER:
				{
					if (TypeHistory == HISTORYTYPE_DIALOG)
						break;

					HistoryMenu.Close(Pos.SelectPos);
					Done=true;
					RetCode = (Key==KEY_CTRLALTENTER||Key==KEY_RCTRLRALTENTER||Key==KEY_CTRLRALTENTER||Key==KEY_RCTRLALTENTER||
							Key==KEY_CTRLALTNUMENTER||Key==KEY_RCTRLRALTNUMENTER||Key==KEY_CTRLRALTNUMENTER||Key==KEY_RCTRLALTNUMENTER)?7
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

					HistoryMenu.Close(Pos.SelectPos);
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
						if (HistoryCfgRef()->Get(CurrentRecord, strName))
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
						HistoryCfgRef()->FlipLock(CurrentRecord);
						ResetPosition();
						HistoryMenu.Close(Pos.SelectPos);
						IsUpdate=true;
						SetUpMenuPos=true;
					}

					break;
				}
				case KEY_SHIFTNUMDEL:
				case KEY_SHIFTDEL:
				{
					if (CurrentRecord && !HistoryCfgRef()->IsLocked(CurrentRecord))
					{
						HistoryCfgRef()->Delete(CurrentRecord);
						ResetPosition();
						HistoryMenu.Close(Pos.SelectPos);
						IsUpdate=true;
						SetUpMenuPos=true;
					}

					break;
				}
				case KEY_NUMDEL:
				case KEY_DEL:
				{
					if (HistoryMenu.GetItemCount()/* > 1*/ &&
					        (!Global->Opt->Confirm.HistoryClear ||
					         (Global->Opt->Confirm.HistoryClear &&
					          !Message(MSG_WARNING,2,
					                  MSG((TypeHistory==HISTORYTYPE_CMD || TypeHistory==HISTORYTYPE_DIALOG?MHistoryTitle:
					                       (TypeHistory==HISTORYTYPE_FOLDER?MFolderHistoryTitle:MViewHistoryTitle))),
					                  MSG(MHistoryClear),
					                  MSG(MClear),MSG(MCancel)))))
					{
						HistoryCfgRef()->DeleteAllUnlocked(TypeHistory,strHistoryName);

						ResetPosition();
						HistoryMenu.Close(Pos.SelectPos);
						IsUpdate=true;
					}

					break;
				}

				default:
					KeyProcessed = 0;
			}
			return KeyProcessed;
		});

		if (IsUpdate)
			continue;

		Done=true;

		if (Code >= 0)
		{
			SelectedRecord = *static_cast<unsigned __int64*>(HistoryMenu.GetUserData(nullptr, 0, Code));

			if (!SelectedRecord)
				return -1;


			if (!HistoryCfgRef()->Get(SelectedRecord, strSelectedRecordName, &SelectedRecordType, strSelectedRecordGuid, strSelectedRecordFile, strSelectedRecordData))
				return -1;

			//BUGUBUG: eliminate those magic numbers!
			if (SelectedRecordType != 2 && SelectedRecordType != 3 // ignore external
				&& RetCode != 3 && ((TypeHistory == HISTORYTYPE_FOLDER && strSelectedRecordGuid.IsEmpty()) || TypeHistory == HISTORYTYPE_VIEW) && apiGetFileAttributes(strSelectedRecordName) == INVALID_FILE_ATTRIBUTES)
			{
				SetLastError(ERROR_FILE_NOT_FOUND);

				if (SelectedRecordType == 1 && TypeHistory == HISTORYTYPE_VIEW) // Edit? ����� ������� � ���� ���� ��������
				{
					if (!Message(MSG_WARNING|MSG_ERRORTYPE,2,Title,strSelectedRecordName.CPtr(),MSG(MViewHistoryIsCreate),MSG(MHYes),MSG(MHNo)))
						break;
				}
				else
				{
					Message(MSG_WARNING|MSG_ERRORTYPE,1,Title,strSelectedRecordName.CPtr(),MSG(MOk));
				}

				Done=false;
				SetUpMenuPos=true;
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
	if(Guid)
	{
		if(!StrToGuid(strSelectedRecordGuid,*Guid)) *Guid = FarGuid;
	}
	if(pstrFile) *pstrFile = strSelectedRecordFile;
	if(pstrData) *pstrData = strSelectedRecordData;

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
	CurrentItem = HistoryCfgRef()->GetPrev(TypeHistory, strHistoryName, CurrentItem, strStr);
}


void History::GetNext(string &strStr)
{
	CurrentItem = HistoryCfgRef()->GetNext(TypeHistory, strHistoryName, CurrentItem, strStr);
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
	unsigned __int64 HistoryItem=HistoryCfgRef()->CyclicGetPrev(TypeHistory, strHistoryName, CurrentItem, strName);
	while (HistoryItem != CurrentItem)
	{
		if (!HistoryItem)
		{
			if (++i > 1) //infinite loop
				break;
			HistoryItem = HistoryCfgRef()->CyclicGetPrev(TypeHistory, strHistoryName, HistoryItem, strName);
			continue;
		}

		if (!StrCmpNI(strStr.CPtr(),strName.CPtr(),Length) && strStr != strName)
		{
			if (bAppend)
				strStr += strName.CPtr() + Length;
			else
				strStr = strName;

			CurrentItem = HistoryItem;
			return true;
		}

		HistoryItem = HistoryCfgRef()->CyclicGetPrev(TypeHistory, strHistoryName, HistoryItem, strName);
	}

	return false;
}

bool History::GetAllSimilar(VMenu2 &HistoryMenu,const string& Str)
{
	int Length=static_cast<int>(Str.GetLength());
	DWORD index=0;
	string strHName,strHGuid,strHFile,strHData;
	int HType;
	bool HLock;
	unsigned __int64 id;
	unsigned __int64 Time;
	while (HistoryCfgRef()->Enum(index++,TypeHistory,strHistoryName,&id,strHName,&HType,&HLock,&Time,strHGuid,strHFile,strHData,true))
	{
		if (!StrCmpNI(Str.CPtr(),strHName.CPtr(),Length))
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
	if (id && !HistoryCfgRef()->IsLocked(id))
	{
		if (HistoryCfgRef()->Delete(id))
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

HistoryConfig* History::HistoryCfgRef(void)
{
	return EnableSave? Global->Db->HistoryCfg().get() : Global->Db->HistoryCfgMem().get();
}
