/*
findfile.cpp

����� (Alt-F7)
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

#include "findfile.hpp"
#include "flink.hpp"
#include "keys.hpp"
#include "ctrlobj.hpp"
#include "dialog.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "fileview.hpp"
#include "fileedit.hpp"
#include "filelist.hpp"
#include "cmdline.hpp"
#include "chgprior.hpp"
#include "namelist.hpp"
#include "scantree.hpp"
#include "manager.hpp"
#include "scrbuf.hpp"
#include "filemasks.hpp"
#include "filefilter.hpp"
#include "farexcpt.hpp"
#include "syslog.hpp"
#include "codepage.hpp"
#include "cddrv.hpp"
#include "TaskBar.hpp"
#include "interf.hpp"
#include "colormix.hpp"
#include "message.hpp"
#include "delete.hpp"
#include "datetime.hpp"
#include "drivemix.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "mix.hpp"
#include "constitle.hpp"
#include "DlgGuid.hpp"
#include "synchro.hpp"
#include "console.hpp"
#include "wakeful.hpp"
#include "panelmix.hpp"
#include "setattr.hpp"
#include "keyboard.hpp"
#include "configdb.hpp"

const int CHAR_TABLE_SIZE=5;

const size_t readBufferSize = 32768;

// ������ �������. ���� ���� ������ � ������, �� FindList->ArcIndex ��������� ����.
struct ArcListItem
{
	string strArcName;
	HANDLE hPlugin;    // Plugin handle
	UINT64 Flags;       // OpenPanelInfo.Flags
	string strRootPath; // Root path in plugin after opening.
};

// ������ ��������� ������. ������ �� ������ �������� � ����.
struct FindListItem
{
	FAR_FIND_DATA FindData;
	ArcListItem* Arc;
	DWORD Used;
	void* Data;
	FARPANELITEMFREECALLBACK FreeData;
};

struct InterThreadData
{
private:
	CriticalSection DataCS;
	ArcListItem* FindFileArcItem;
	int Percent;
	int LastFoundNumber;
	int FileCount;
	int DirCount;

	std::list<FindListItem> FindList;
	std::list<ArcListItem> ArcList;
	string strFindMessage;

public:
	InterThreadData() {Init();}
	void Init()
	{
		CriticalSectionLock Lock(DataCS);
		FindFileArcItem = nullptr;
		Percent=0;
		LastFoundNumber=0;
		FileCount=0;
		DirCount=0;
		FindList.clear();
		ArcList.clear();
		strFindMessage.clear();
	}

	std::list<FindListItem>& GetFindList() {return FindList;}

	void Lock() {DataCS.Enter();}
	void Unlock() {DataCS.Leave();}

	ArcListItem* GetFindFileArcItem(){CriticalSectionLock Lock(DataCS); return FindFileArcItem;}
	void SetFindFileArcItem(ArcListItem* Value){CriticalSectionLock Lock(DataCS); FindFileArcItem = Value;}

	int GetPercent(){CriticalSectionLock Lock(DataCS); return Percent;}
	void SetPercent(int Value){CriticalSectionLock Lock(DataCS); Percent=Value;}

	int GetLastFoundNumber(){CriticalSectionLock Lock(DataCS); return LastFoundNumber;}
	void SetLastFoundNumber(int Value){CriticalSectionLock Lock(DataCS); LastFoundNumber=Value;}

	int GetFileCount(){CriticalSectionLock Lock(DataCS); return FileCount;}
	void SetFileCount(int Value){CriticalSectionLock Lock(DataCS); FileCount=Value;}

	int GetDirCount(){CriticalSectionLock Lock(DataCS); return DirCount;}
	void SetDirCount(int Value){CriticalSectionLock Lock(DataCS); DirCount=Value;}

	size_t GetFindListCount(){CriticalSectionLock Lock(DataCS); return FindList.size();}

	void GetFindMessage(string& To)
	{
		CriticalSectionLock Lock(DataCS);
		To=strFindMessage;
	}

	void SetFindMessage(const string& From)
	{
		CriticalSectionLock Lock(DataCS);
		strFindMessage=From;
	}

	void ClearAllLists()
	{
		CriticalSectionLock Lock(DataCS);
		FindFileArcItem = nullptr;

		if (!FindList.empty())
		{
			std::for_each(CONST_RANGE(FindList, i)
			{
				if (i.FreeData)
				{
					FarPanelItemFreeInfo info={sizeof(FarPanelItemFreeInfo),nullptr};
					if(i.Arc)
					{
						info.hPlugin=i.Arc->hPlugin;
					}
					i.FreeData(i.Data,&info);
				}
			});
			FindList.clear();
		}

		ArcList.clear();
	}

	ArcListItem& AddArcListItem(const string& ArcName,HANDLE hPlugin,UINT64 dwFlags,const string& RootPath)
	{
		CriticalSectionLock Lock(DataCS);

		ArcListItem NewItem;
		NewItem.strArcName = ArcName;
		NewItem.hPlugin = hPlugin;
		NewItem.Flags = dwFlags;
		NewItem.strRootPath = RootPath;
		AddEndSlash(NewItem.strRootPath);
		ArcList.emplace_back(NewItem);
		return ArcList.back();
	}

	FindListItem& AddFindListItem(const FAR_FIND_DATA& FindData, void* Data, FARPANELITEMFREECALLBACK FreeData)
	{
		CriticalSectionLock Lock(DataCS);

		FindListItem NewItem;
		NewItem.FindData = FindData;
		NewItem.Arc = nullptr;
		NewItem.Data = Data;
		NewItem.FreeData = FreeData;
		FindList.emplace_back(NewItem);
		return FindList.back();
	}
}
*itd;


enum
{
	FIND_EXIT_NONE,
	FIND_EXIT_SEARCHAGAIN,
	FIND_EXIT_GOTO,
	FIND_EXIT_PANEL
};

enum ADVANCEDDLG
{
	AD_DOUBLEBOX,
	AD_TEXT_SEARCHFIRST,
	AD_EDIT_SEARCHFIRST,
	AD_CHECKBOX_FINDALTERNATESTREAMS,
	AD_SEPARATOR1,
	AD_TEXT_COLUMNSFORMAT,
	AD_EDIT_COLUMNSFORMAT,
	AD_TEXT_COLUMNSWIDTH,
	AD_EDIT_COLUMNSWIDTH,
	AD_SEPARATOR2,
	AD_BUTTON_OK,
	AD_BUTTON_CANCEL,
};

enum FINDASKDLG
{
	FAD_DOUBLEBOX,
	FAD_TEXT_MASK,
	FAD_EDIT_MASK,
	FAD_SEPARATOR0,
	FAD_TEXT_TEXTHEX,
	FAD_EDIT_TEXT,
	FAD_EDIT_HEX,
	FAD_TEXT_CP,
	FAD_COMBOBOX_CP,
	FAD_SEPARATOR1,
	FAD_CHECKBOX_CASE,
	FAD_CHECKBOX_WHOLEWORDS,
	FAD_CHECKBOX_HEX,
	FAD_CHECKBOX_ARC,
	FAD_CHECKBOX_DIRS,
	FAD_CHECKBOX_LINKS,
	FAD_SEPARATOR_2,
	FAD_SEPARATOR_3,
	FAD_TEXT_WHERE,
	FAD_COMBOBOX_WHERE,
	FAD_CHECKBOX_FILTER,
	FAD_SEPARATOR_4,
	FAD_BUTTON_FIND,
	FAD_BUTTON_DRIVE,
	FAD_BUTTON_FILTER,
	FAD_BUTTON_ADVANCED,
	FAD_BUTTON_CANCEL,
};

enum FINDASKDLGCOMBO
{
	FADC_ALLDISKS,
	FADC_ALLBUTNET,
	FADC_PATH,
	FADC_ROOT,
	FADC_FROMCURRENT,
	FADC_INCURRENT,
	FADC_SELECTED,
};

enum FINDDLG
{
	FD_DOUBLEBOX,
	FD_LISTBOX,
	FD_SEPARATOR1,
	FD_TEXT_STATUS,
	FD_TEXT_STATUS_PERCENTS,
	FD_SEPARATOR2,
	FD_BUTTON_NEW,
	FD_BUTTON_GOTO,
	FD_BUTTON_VIEW,
	FD_BUTTON_PANEL,
	FD_BUTTON_STOP,
};

void FindFiles::InitInFileSearch()
{
	if (!InFileSearchInited && !strFindStr.empty())
	{
		size_t findStringCount = strFindStr.size();
		// �������������� ������ ������ �� �����
		readBufferA = new char[readBufferSize];
		readBuffer = new wchar_t[readBufferSize];

		if (!SearchHex)
		{
			// ��������� ������ ������
			if (!CmpCase)
			{
				findStringBuffer = new wchar_t[2 * findStringCount];
				findString=findStringBuffer;

				for (size_t index = 0; index<strFindStr.size(); index++)
				{
					wchar_t ch = strFindStr[index];

					if (IsCharLower(ch))
					{
						findString[index]=Upper(ch);
						findString[index+findStringCount]=ch;
					}
					else
					{
						findString[index]=ch;
						findString[index+findStringCount]=Lower(ch);
					}
				}
			}
			else
				findString = strFindStr.GetBuffer();

			// �������������� ������ ��� ��������� ������
			skipCharsTable = new size_t[WCHAR_MAX + 1];

			for (size_t index = 0; index < WCHAR_MAX+1; index++)
				skipCharsTable[index] = findStringCount;

			for (size_t index = 0; index < findStringCount-1; index++)
				skipCharsTable[findString[index]] = findStringCount-1-index;

			if (!CmpCase)
				for (size_t index = 0; index < findStringCount-1; index++)
					skipCharsTable[findString[index+findStringCount]] = findStringCount-1-index;

			// ��������� ������ ������� �������
			if (CodePage == CP_DEFAULT)
			{
				DWORD data;
				string codePageName;
				bool hasSelected = false;

				// ��������� ������� ��������� ������� ��������
				for (DWORD i=0; Global->Db->GeneralCfg()->EnumValues(FavoriteCodePagesKey, i, codePageName, &data); i++)
				{
					if (data & CPST_FIND)
					{
						hasSelected = true;
						break;
					}
				}

				// ��������� ����������� ������� ��������
				if (!hasSelected)
				{
					codePages.emplace_back(GetOEMCP());
					codePages.emplace_back(GetACP());
					codePages.emplace_back(CP_UTF8);
					codePages.emplace_back(CP_UNICODE);
					codePages.emplace_back(CP_REVERSEBOM);
				}
				else
				{
					codePages.clear();
				}

				// ��������� ������� ������� ��������
				for (DWORD i=0; Global->Db->GeneralCfg()->EnumValues(FavoriteCodePagesKey, i, codePageName, &data); i++)
				{
					if (data & (hasSelected?CPST_FIND:CPST_FAVORITE))
					{
						uintptr_t codePage = _wtoi(codePageName.c_str());

						// ��������� �����
						if (!hasSelected)
						{
							if (std::any_of(CONST_RANGE(codePages, i) {return i.CodePage == CodePage;}))
								continue;
						}

						codePages.emplace_back(codePage);
					}
				}
			}
			else
			{
				codePages.emplace_back(CodePage);
			}

			std::for_each(RANGE(codePages, i)
			{
				if (IsUnicodeCodePage(i.CodePage))
					i.MaxCharSize = 2;
				else
				{
					CPINFO cpi;

					if (!GetCPInfo(i.CodePage, &cpi))
						cpi.MaxCharSize = 0; //�������, ��� ������ � ����� ����� ������� � ������ ����������

					i.MaxCharSize = cpi.MaxCharSize;
				}

				i.LastSymbol = 0;
				i.WordFound = false;
			});
		}
		else
		{
			// ��������� hex-������ ��� ������
			hexFindStringSize = 0;

			if (SearchHex)
			{
				bool flag = false;
				hexFindString = new unsigned char[(findStringCount - findStringCount/3+1)/2];

				for (size_t index = 0; index < strFindStr.size(); index++)
				{
					wchar_t symbol = strFindStr.at(index);
					byte offset = 0;

					if (symbol >= L'a' && symbol <= L'f')
						offset = 87;
					else if (symbol >= L'A' && symbol <= L'F')
						offset = 55;
					else if (symbol >= L'0' && symbol <= L'9')
						offset = 48;
					else
						continue;

					if (!flag)
						hexFindString[hexFindStringSize++] = ((byte)symbol-offset)<<4;
					else
						hexFindString[hexFindStringSize-1] |= ((byte)symbol-offset);

					flag = !flag;
				}
			}

			// �������������� ������ ��� ��������� ������
			skipCharsTable = new size_t[255 + 1];

			for (size_t index = 0; index < 255+1; index++)
				skipCharsTable[index] = hexFindStringSize;

			for (size_t index = 0; index < (size_t)hexFindStringSize-1; index++)
				skipCharsTable[hexFindString[index]] = hexFindStringSize-1-index;
		}

		InFileSearchInited=true;
	}
}

void FindFiles::ReleaseInFileSearch()
{
	if (InFileSearchInited && !strFindStr.empty())
	{
		delete[] readBufferA;
		readBufferA = nullptr;

		delete[] readBuffer;
		readBuffer = nullptr;

		delete[] skipCharsTable;
		skipCharsTable=nullptr;

		codePages.clear();

		delete[] findStringBuffer;
		findStringBuffer=nullptr;

		delete[] hexFindString;
		hexFindString=nullptr;

		InFileSearchInited=false;
	}
}

string& FindFiles::PrepareDriveNameStr(string &strSearchFromRoot)
{
	string strCurDir;
	Global->CtrlObject->CmdLine->GetCurDir(strCurDir);
	GetPathRoot(strCurDir,strCurDir);
	DeleteEndSlash(strCurDir);

	if (
	    strCurDir.empty()||
	    (Global->CtrlObject->Cp()->ActivePanel->GetMode()==PLUGIN_PANEL && Global->CtrlObject->Cp()->ActivePanel->IsVisible())
	)
	{
		strSearchFromRoot = MSG(MSearchFromRootFolder);
	}
	else
	{
		strSearchFromRoot= MSG(MSearchFromRootOfDrive);
		strSearchFromRoot+=L" ";
		strSearchFromRoot+=strCurDir;
	}

	return strSearchFromRoot;
}

// ��������� ������ �� �������������� ������������ ����
bool FindFiles::IsWordDiv(const wchar_t symbol)
{
	// ��� �� ������������ �������� ����� ������ � ���������� �������
	return !symbol||IsSpace(symbol)||IsEol(symbol)||::IsWordDiv(Global->Opt->strWordDiv,symbol);
}

#if defined(MANTIS_0002207)
static intptr_t GetUserDataFromPluginItem(const wchar_t *Name, const struct PluginPanelItem * const* PanelData,size_t ItemCount)
{
	intptr_t UserData=0;

	if (Name && *Name)
	{
		for (size_t Index=0; Index < ItemCount; ++Index)
		{
			if (!StrCmp(PanelData[Index]->FileName,Name))
			{
				UserData=(intptr_t)PanelData[Index]->UserData.Data;
				break;
			}
		}
	}

	return UserData;
}
#endif

void FindFiles::SetPluginDirectory(const string& DirName,HANDLE hPlugin,bool UpdatePanel,struct UserDataItem *UserData)
{
	if (!DirName.empty())
	{
		string strName(DirName);
		wchar_t* DirPtr = strName.GetBuffer();
		wchar_t* NamePtr = (wchar_t*) PointToName(DirPtr);

		if (NamePtr != DirPtr)
		{
			*(NamePtr-1) = 0;
			// force plugin to update its file list (that can be empty at this time)
			// if not done SetDirectory may fail
			{
				size_t FileCount=0;
				PluginPanelItem *PanelData=nullptr;

				if (Global->CtrlObject->Plugins->GetFindData(hPlugin,&PanelData,&FileCount,OPM_SILENT))
					Global->CtrlObject->Plugins->FreeFindData(hPlugin,PanelData,FileCount,true);
			}

			if (*DirPtr)
			{
				Global->CtrlObject->Plugins->SetDirectory(hPlugin,DirPtr,OPM_SILENT,UserData);
			}
			else
			{
				Global->CtrlObject->Plugins->SetDirectory(hPlugin,L"\\",OPM_SILENT);
			}
		}

		// �������� ������ ��� �������������.
		if (UpdatePanel)
		{
			Global->CtrlObject->Cp()->ActivePanel->Update(UPDATE_KEEP_SELECTION);
			Global->CtrlObject->Cp()->ActivePanel->GoToFile(NamePtr);
			Global->CtrlObject->Cp()->ActivePanel->Show();
		}

		//strName.ReleaseBuffer(); �� ����. ������ ��� ����� ���������, ������ ����� StrLength.
	}
}

intptr_t FindFiles::AdvancedDlgProc(Dialog* Dlg, intptr_t Msg, intptr_t Param1, void* Param2)
{
	switch (Msg)
	{
		case DN_CLOSE:

			if (Param1==AD_BUTTON_OK)
			{
				LPCWSTR Data=reinterpret_cast<LPCWSTR>(Dlg->SendMessage(DM_GETCONSTTEXTPTR,AD_EDIT_SEARCHFIRST,0));

				if (Data && *Data && !CheckFileSizeStringFormat(Data))
				{
					Message(MSG_WARNING,1,MSG(MFindFileAdvancedTitle),MSG(MBadFileSizeFormat),MSG(MOk));
					return FALSE;
				}
			}

			break;
		default:
			break;
	}

	return Dlg->DefProc(Msg,Param1,Param2);
}

void FindFiles::AdvancedDialog()
{
	FarDialogItem AdvancedDlgData[]=
	{
		{DI_DOUBLEBOX,3,1,52,12,0,nullptr,nullptr,0,MSG(MFindFileAdvancedTitle)},
		{DI_TEXT,5,2,0,2,0,nullptr,nullptr,0,MSG(MFindFileSearchFirst)},
		{DI_EDIT,5,3,50,3,0,nullptr,nullptr,0,Global->Opt->FindOpt.strSearchInFirstSize.c_str()},
		{DI_CHECKBOX,5,4,0,4,Global->Opt->FindOpt.FindAlternateStreams,nullptr,nullptr,0,MSG(MFindAlternateStreams)},
		{DI_TEXT,-1,5,0,5,0,nullptr,nullptr,DIF_SEPARATOR,L""},
		{DI_TEXT,5,6, 0, 6,0,nullptr,nullptr,0,MSG(MFindAlternateModeTypes)},
		{DI_EDIT,5,7,35, 7,0,nullptr,nullptr,0,Global->Opt->FindOpt.strSearchOutFormat.c_str()},
		{DI_TEXT,5,8, 0, 8,0,nullptr,nullptr,0,MSG(MFindAlternateModeWidths)},
		{DI_EDIT,5,9,35, 9,0,nullptr,nullptr,0,Global->Opt->FindOpt.strSearchOutFormatWidth.c_str()},
		{DI_TEXT,-1,10,0,10,0,nullptr,nullptr,DIF_SEPARATOR,L""},
		{DI_BUTTON,0,11,0,11,0,nullptr,nullptr,DIF_DEFAULTBUTTON|DIF_CENTERGROUP,MSG(MOk)},
		{DI_BUTTON,0,11,0,11,0,nullptr,nullptr,DIF_CENTERGROUP,MSG(MCancel)},
	};
	MakeDialogItemsEx(AdvancedDlgData,AdvancedDlg);
	Dialog Dlg(this, &FindFiles::AdvancedDlgProc, nullptr, AdvancedDlg,ARRAYSIZE(AdvancedDlg));
	Dlg.SetHelp(L"FindFileAdvanced");
	Dlg.SetPosition(-1,-1,52+4,7+7);
	Dlg.Process();
	int ExitCode=Dlg.GetExitCode();

	if (ExitCode==AD_BUTTON_OK)
	{
		Global->Opt->FindOpt.strSearchInFirstSize = AdvancedDlg[AD_EDIT_SEARCHFIRST].strData;
		SearchInFirst=ConvertFileSizeString(Global->Opt->FindOpt.strSearchInFirstSize);

		Global->Opt->FindOpt.strSearchOutFormat = AdvancedDlg[AD_EDIT_COLUMNSFORMAT].strData;
		Global->Opt->FindOpt.strSearchOutFormatWidth = AdvancedDlg[AD_EDIT_COLUMNSWIDTH].strData;

		ClearArray(Global->Opt->FindOpt.OutColumnTypes);
		ClearArray(Global->Opt->FindOpt.OutColumnWidths);
		ClearArray(Global->Opt->FindOpt.OutColumnWidthType);
		Global->Opt->FindOpt.OutColumnCount=0;

		if (!Global->Opt->FindOpt.strSearchOutFormat.empty())
		{
			if (Global->Opt->FindOpt.strSearchOutFormatWidth.empty())
				Global->Opt->FindOpt.strSearchOutFormatWidth=L"0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0";

			TextToViewSettings(Global->Opt->FindOpt.strSearchOutFormat,Global->Opt->FindOpt.strSearchOutFormatWidth,
                                  Global->Opt->FindOpt.OutColumnTypes,Global->Opt->FindOpt.OutColumnWidths,Global->Opt->FindOpt.OutColumnWidthType,
                                  Global->Opt->FindOpt.OutColumnCount);
        }

		Global->Opt->FindOpt.FindAlternateStreams=(AdvancedDlg[AD_CHECKBOX_FINDALTERNATESTREAMS].Selected==BSTATE_CHECKED);
	}
}

intptr_t FindFiles::MainDlgProc(Dialog* Dlg, intptr_t Msg, intptr_t Param1, void* Param2)
{
	switch (Msg)
	{
		case DN_INITDIALOG:
		{
			bool Hex=(Dlg->SendMessage(DM_GETCHECK,FAD_CHECKBOX_HEX,0)==BSTATE_CHECKED);
			Dlg->SendMessage(DM_SHOWITEM,FAD_EDIT_TEXT,ToPtr(!Hex));
			Dlg->SendMessage(DM_SHOWITEM,FAD_EDIT_HEX,ToPtr(Hex));
			Dlg->SendMessage(DM_ENABLE,FAD_TEXT_CP,ToPtr(!Hex));
			Dlg->SendMessage(DM_ENABLE,FAD_COMBOBOX_CP,ToPtr(!Hex));
			Dlg->SendMessage(DM_ENABLE,FAD_CHECKBOX_CASE,ToPtr(!Hex));
			Dlg->SendMessage(DM_ENABLE,FAD_CHECKBOX_WHOLEWORDS,ToPtr(!Hex));
			Dlg->SendMessage(DM_ENABLE,FAD_CHECKBOX_DIRS,ToPtr(!Hex));
			Dlg->SendMessage(DM_EDITUNCHANGEDFLAG,FAD_EDIT_TEXT,ToPtr(1));
			Dlg->SendMessage(DM_EDITUNCHANGEDFLAG,FAD_EDIT_HEX,ToPtr(1));
			Dlg->SendMessage(DM_SETTEXTPTR,FAD_TEXT_TEXTHEX,const_cast<wchar_t*>(Hex?MSG(MFindFileHex):MSG(MFindFileText)));
			Dlg->SendMessage(DM_SETTEXTPTR,FAD_TEXT_CP,const_cast<wchar_t*>(MSG(MFindFileCodePage)));
			Dlg->SendMessage(DM_SETCOMBOBOXEVENT,FAD_COMBOBOX_CP,ToPtr(CBET_KEY));
			FarListTitles Titles={sizeof(FarListTitles),0,nullptr,0,MSG(MFindFileCodePageBottom)};
			Dlg->SendMessage(DM_LISTSETTITLES,FAD_COMBOBOX_CP,&Titles);
			// ��������� ����������� ����� ����������
			CodePage = Global->Opt->FindCodePage;
			favoriteCodePages = Global->CodePages->FillCodePagesList(Dlg, FAD_COMBOBOX_CP, CodePage, false, true, false, false);
			// ������� �������� � � ������ ������ ������� ������� � ����� ������ ����� �� ��������� � CodePage,
			// ��� ��� �������� CodePage �� ������ ������
			FarListPos Position={sizeof(FarListPos)};
			Dlg->SendMessage( DM_LISTGETCURPOS, FAD_COMBOBOX_CP, &Position);
			FarListGetItem Item = { sizeof(FarListGetItem), Position.SelectPos };
			Dlg->SendMessage( DM_LISTGETITEM, FAD_COMBOBOX_CP, &Item);
			CodePage = *(uintptr_t*)Dlg->SendMessage( DM_LISTGETDATA, FAD_COMBOBOX_CP, ToPtr(Position.SelectPos));
			return TRUE;
		}
		case DN_CLOSE:
		{
			switch (Param1)
			{
				case FAD_BUTTON_FIND:
				{
					string Mask((LPCWSTR)Dlg->SendMessage(DM_GETCONSTTEXTPTR,FAD_EDIT_MASK,0));

					if (Mask.empty())
						Mask=L"*";

					return FileMaskForFindFile->Set(Mask);
				}
				case FAD_BUTTON_DRIVE:
				{
					Global->IsRedrawFramesInProcess++;
					Global->CtrlObject->Cp()->ActivePanel->ChangeDisk();
					// �� ��� �, ��� ����� ����� ������ ��������� ������
					// ����� ����� ��������.
					//FrameManager->ProcessKey(KEY_CONSOLE_BUFFER_RESIZE);
					FrameManager->ResizeAllFrame();
					Global->IsRedrawFramesInProcess--;
					string strSearchFromRoot;
					PrepareDriveNameStr(strSearchFromRoot);
					FarListGetItem item={sizeof(FarListGetItem),FADC_ROOT};
					Dlg->SendMessage(DM_LISTGETITEM,FAD_COMBOBOX_WHERE,&item);
					item.Item.Text=strSearchFromRoot.c_str();
					Dlg->SendMessage(DM_LISTUPDATE,FAD_COMBOBOX_WHERE,&item);
					PluginMode=Global->CtrlObject->Cp()->ActivePanel->GetMode()==PLUGIN_PANEL;
					Dlg->SendMessage(DM_ENABLE,FAD_CHECKBOX_DIRS,ToPtr(!PluginMode));
					item.ItemIndex=FADC_ALLDISKS;
					Dlg->SendMessage(DM_LISTGETITEM,FAD_COMBOBOX_WHERE,&item);

					if (PluginMode)
						item.Item.Flags|=LIF_GRAYED;
					else
						item.Item.Flags&=~LIF_GRAYED;

					Dlg->SendMessage(DM_LISTUPDATE,FAD_COMBOBOX_WHERE,&item);
					item.ItemIndex=FADC_ALLBUTNET;
					Dlg->SendMessage(DM_LISTGETITEM,FAD_COMBOBOX_WHERE,&item);

					if (PluginMode)
						item.Item.Flags|=LIF_GRAYED;
					else
						item.Item.Flags&=~LIF_GRAYED;

					Dlg->SendMessage(DM_LISTUPDATE,FAD_COMBOBOX_WHERE,&item);
				}
				break;
				case FAD_BUTTON_FILTER:
					Filter->FilterEdit();
					break;
				case FAD_BUTTON_ADVANCED:
					AdvancedDialog();
					break;
				case -2:
				case -1:
				case FAD_BUTTON_CANCEL:
					return TRUE;
			}

			return FALSE;
		}
		case DN_BTNCLICK:
		{
			switch (Param1)
			{
				case FAD_CHECKBOX_DIRS:
					{
						FindFoldersChanged = true;
					}
					break;

				case FAD_CHECKBOX_HEX:
				{
					Dlg->SendMessage(DM_ENABLEREDRAW,FALSE,0);
					string strDataStr;
					Transform(strDataStr,(LPCWSTR)Dlg->SendMessage(DM_GETCONSTTEXTPTR,Param2?FAD_EDIT_TEXT:FAD_EDIT_HEX,0),Param2?L'X':L'S');
					Dlg->SendMessage(DM_SETTEXTPTR,Param2?FAD_EDIT_HEX:FAD_EDIT_TEXT, UNSAFE_CSTR(strDataStr));
					intptr_t iParam = reinterpret_cast<intptr_t>(Param2);
					Dlg->SendMessage(DM_SHOWITEM,FAD_EDIT_TEXT,ToPtr(!iParam));
					Dlg->SendMessage(DM_SHOWITEM,FAD_EDIT_HEX,ToPtr(iParam));
					Dlg->SendMessage(DM_ENABLE,FAD_TEXT_CP,ToPtr(!iParam));
					Dlg->SendMessage(DM_ENABLE,FAD_COMBOBOX_CP,ToPtr(!iParam));
					Dlg->SendMessage(DM_ENABLE,FAD_CHECKBOX_CASE,ToPtr(!iParam));
					Dlg->SendMessage(DM_ENABLE,FAD_CHECKBOX_WHOLEWORDS,ToPtr(!iParam));
					Dlg->SendMessage(DM_ENABLE,FAD_CHECKBOX_DIRS,ToPtr(!iParam));
					Dlg->SendMessage(DM_SETTEXTPTR,FAD_TEXT_TEXTHEX, const_cast<wchar_t*>(Param2?MSG(MFindFileHex):MSG(MFindFileText)));

					if (strDataStr.size()>0)
					{
						int UnchangeFlag=(int)Dlg->SendMessage(DM_EDITUNCHANGEDFLAG,FAD_EDIT_TEXT,ToPtr(-1));
						Dlg->SendMessage(DM_EDITUNCHANGEDFLAG,FAD_EDIT_HEX,ToPtr(UnchangeFlag));
					}

					Dlg->SendMessage(DM_ENABLEREDRAW,TRUE,0);
				}
				break;
			}

			break;
		}
		case DN_CONTROLINPUT:
		{
			const INPUT_RECORD* record=(const INPUT_RECORD *)Param2;
			if (record->EventType!=KEY_EVENT) break;
			int key = InputRecordToKey(record);
			switch (Param1)
			{
				case FAD_COMBOBOX_CP:
				{
					switch (key)
					{
						case KEY_INS:
						case KEY_NUMPAD0:
						case KEY_SPACE:
						{
							// ��������� ���������/������ ������� ��� ����������� � ������� ������ ��������
							// �������� ������� ������� � ���������� ������ ������ ��������
							FarListPos Position={sizeof(FarListPos)};
							Dlg->SendMessage( DM_LISTGETCURPOS, FAD_COMBOBOX_CP, &Position);
							// �������� ����� ��������� ������� �������
							FarListGetItem Item = { sizeof(FarListGetItem), Position.SelectPos };
							Dlg->SendMessage( DM_LISTGETITEM, FAD_COMBOBOX_CP, &Item);
							UINT SelectedCodePage = *(UINT*)Dlg->SendMessage( DM_LISTGETDATA, FAD_COMBOBOX_CP, ToPtr(Position.SelectPos));
							// ��������� �������� ������ ����������� � ������� ������� ��������
							int FavoritesIndex = 2 + StandardCPCount + 2;

							if (Position.SelectPos > 1 && Position.SelectPos < FavoritesIndex + (favoriteCodePages ? favoriteCodePages + 1 : 0))
							{
								// ����������� ����� ������� ������� � ������
								string strCodePageName;
								strCodePageName = FormatString() << SelectedCodePage;
								// �������� ������� ��������� ����� � �������
								long long SelectType = 0;
								Global->Db->GeneralCfg()->GetValue(FavoriteCodePagesKey, strCodePageName, &SelectType, 0);

								// ��������/����������� ������� ��������
								if (Item.Item.Flags & LIF_CHECKED)
								{
									// ��� ����������� ������ �������� ������ ������� �������� �� ������, ���
									// ������� �� ��������� � ������� ����, ��� ������� �������� �������
									if (SelectType & CPST_FAVORITE)
										Global->Db->GeneralCfg()->SetValue(FavoriteCodePagesKey, strCodePageName, CPST_FAVORITE);
									else
										Global->Db->GeneralCfg()->DeleteValue(FavoriteCodePagesKey, strCodePageName);

									Item.Item.Flags &= ~LIF_CHECKED;
								}
								else
								{
									Global->Db->GeneralCfg()->SetValue(FavoriteCodePagesKey, strCodePageName, CPST_FIND | (SelectType & CPST_FAVORITE ?  CPST_FAVORITE : 0));
									Item.Item.Flags |= LIF_CHECKED;
								}

								// ��������� ������� ������� � ���������� ������
								Dlg->SendMessage( DM_LISTUPDATE, FAD_COMBOBOX_CP, &Item);

								if (Position.SelectPos<FavoritesIndex + (favoriteCodePages ? favoriteCodePages + 1 : 0)-2)
								{
									FarListPos Pos={sizeof(FarListPos),Position.SelectPos+1,Position.TopPos};
									Dlg->SendMessage( DM_LISTSETCURPOS, FAD_COMBOBOX_CP,&Pos);
								}

								// ������������ ������, ����� ������� �������� ����� ��������������, ��� � �����������, ��� � � �������,
								// �.�. �����/������ ����� �������������� ����������� � ����� ���������
								bool bStandardCodePage = Position.SelectPos < FavoritesIndex;

								for (int Index = bStandardCodePage ? FavoritesIndex : 0; Index < (bStandardCodePage ? FavoritesIndex + favoriteCodePages : FavoritesIndex); Index++)
								{
									// �������� ������� ������� �������
									FarListGetItem CheckItem = { sizeof(FarListGetItem), Index };
									Dlg->SendMessage( DM_LISTGETITEM, FAD_COMBOBOX_CP, &CheckItem);

									// ������������ ������ ������� ��������
									if (!(CheckItem.Item.Flags&LIF_SEPARATOR))
									{
										if (SelectedCodePage == *(UINT*)Dlg->SendMessage( DM_LISTGETDATA, FAD_COMBOBOX_CP, ToPtr(Index)))
										{
											if (Item.Item.Flags & LIF_CHECKED)
												CheckItem.Item.Flags |= LIF_CHECKED;
											else
												CheckItem.Item.Flags &= ~LIF_CHECKED;

											Dlg->SendMessage( DM_LISTUPDATE, FAD_COMBOBOX_CP, &CheckItem);
											break;
										}
									}
								}
							}
						}
						break;
					}
				}
				break;
			}

			break;
		}
		case DN_EDITCHANGE:
		{
			FarDialogItem &Item=*reinterpret_cast<FarDialogItem*>(Param2);

			switch (Param1)
			{
				case FAD_EDIT_TEXT:
					{
						// ������ "���������� �����"
						if (!FindFoldersChanged)
						{
							BOOL Checked = (Item.Data && *Item.Data)?FALSE:(int)Global->Opt->FindOpt.FindFolders;
							Dlg->SendMessage( DM_SETCHECK, FAD_CHECKBOX_DIRS, ToPtr(Checked?BSTATE_CHECKED:BSTATE_UNCHECKED));
						}

						return TRUE;
					}
					break;

				case FAD_COMBOBOX_CP:
				{
					// �������� ��������� � ���������� ������ ������� ��������
					CodePage = *(UINT*)Dlg->SendMessage( DM_LISTGETDATA, FAD_COMBOBOX_CP, ToPtr(Dlg->SendMessage( DM_LISTGETCURPOS, FAD_COMBOBOX_CP, 0)));
				}
				return TRUE;
				case FAD_COMBOBOX_WHERE:
					{
						SearchFromChanged=true;
					}
					return TRUE;
			}
		}
		case DN_HOTKEY:
		{
			if (Param1==FAD_TEXT_TEXTHEX)
			{
				bool Hex=(Dlg->SendMessage(DM_GETCHECK,FAD_CHECKBOX_HEX,0)==BSTATE_CHECKED);
				Dlg->SendMessage(DM_SETFOCUS,Hex?FAD_EDIT_HEX:FAD_EDIT_TEXT,0);
				return FALSE;
			}
		}
		default:
			break;
	}

	return Dlg->DefProc(Msg,Param1,Param2);
}

bool FindFiles::GetPluginFile(ArcListItem* ArcItem, const FAR_FIND_DATA& FindData, const string& DestPath, string &strResultName,struct UserDataItem *UserData)
{
	_ALGO(CleverSysLog clv(L"FindFiles::GetPluginFile()"));
	OpenPanelInfo Info;
	Global->CtrlObject->Plugins->GetOpenPanelInfo(ArcItem->hPlugin,&Info);
	string strSaveDir = NullToEmpty(Info.CurDir);
	AddEndSlash(strSaveDir);
	Global->CtrlObject->Plugins->SetDirectory(ArcItem->hPlugin,L"\\",OPM_SILENT);
	//SetPluginDirectory(ArcItem->strRootPath,hPlugin);
	SetPluginDirectory(FindData.strFileName,ArcItem->hPlugin,false,UserData);
	const wchar_t *lpFileNameToFind = PointToName(FindData.strFileName);
	const wchar_t *lpFileNameToFindShort = PointToName(FindData.strAlternateFileName);
	PluginPanelItem *pItems;
	size_t nItemsNumber;
	bool nResult=false;

	if (Global->CtrlObject->Plugins->GetFindData(ArcItem->hPlugin,&pItems,&nItemsNumber,OPM_SILENT))
	{
		for (size_t i=0; i<nItemsNumber; i++)
		{
			PluginPanelItem Item = pItems[i];
			Item.FileName=const_cast<LPWSTR>(PointToName(NullToEmpty(pItems[i].FileName)));
			Item.AlternateFileName=const_cast<LPWSTR>(PointToName(NullToEmpty(pItems[i].AlternateFileName)));

			if (!StrCmp(lpFileNameToFind,Item.FileName) && !StrCmp(lpFileNameToFindShort,Item.AlternateFileName))
			{
				nResult=Global->CtrlObject->Plugins->GetFile(ArcItem->hPlugin,&Item,DestPath,strResultName,OPM_SILENT)!=0;
				break;
			}
		}

		Global->CtrlObject->Plugins->FreeFindData(ArcItem->hPlugin,pItems,nItemsNumber,true);
	}

	Global->CtrlObject->Plugins->SetDirectory(ArcItem->hPlugin,L"\\",OPM_SILENT);
	SetPluginDirectory(strSaveDir,ArcItem->hPlugin);
	return nResult;
}

// ��������� ������-����-�������� ������ ��������� (Unicode ������)
const int FindFiles::FindStringBMH(const wchar_t* searchBuffer, size_t searchBufferCount)
{
	size_t findStringCount = strFindStr.size();
	const wchar_t *buffer = searchBuffer;
	const wchar_t *findStringLower = CmpCase ? nullptr : findString+findStringCount;
	size_t lastBufferChar = findStringCount-1;

	while (searchBufferCount>=findStringCount)
	{
		for (size_t index = lastBufferChar; buffer[index]==findString[index] || (CmpCase ? 0 : buffer[index]==findStringLower[index]); index--)
			if (!index)
				return static_cast<int>(buffer-searchBuffer);

		size_t offset = skipCharsTable[buffer[lastBufferChar]];
		searchBufferCount -= offset;
		buffer += offset;
	}

	return -1;
}

// ��������� ������-����-�������� ������ ��������� (Char ������)
const int FindFiles::FindStringBMH(const unsigned char* searchBuffer, size_t searchBufferCount)
{
	const unsigned char *buffer = searchBuffer;
	size_t lastBufferChar = hexFindStringSize-1;

	while (searchBufferCount>=hexFindStringSize)
	{
		for (size_t index = lastBufferChar; buffer[index]==hexFindString[index]; index--)
			if (!index)
				return static_cast<int>(buffer-searchBuffer);

		size_t offset = skipCharsTable[buffer[lastBufferChar]];
		searchBufferCount -= offset;
		buffer += offset;
	}

	return -1;
}


int FindFiles::LookForString(const string& Name)
{
	// ����� ������ ������
	size_t findStringCount;

	// ���� ������ ������ ������, �� �������, ��� �� ������ ���-������ �����
	if (!(findStringCount = strFindStr.size()))
		return true;

	File file;
	// ��������� ����
	if(!file.Open(Name, FILE_READ_DATA, FILE_SHARE_READ|FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN))
	{
		return false;
	}
	// ���������� ��������� �� ����� ����
	DWORD readBlockSize = 0;
	// ���������� ����������� �� ����� ����
	unsigned __int64 alreadyRead = 0;
	// �������� �� ������� �� ��������� ��� �������� ����� �������
	int offset=0;

	if (SearchHex)
		offset = (int)hexFindStringSize-1;

	UINT64 FileSize=0;
	file.GetSize(FileSize);

	if (SearchInFirst)
	{
		FileSize=std::min(SearchInFirst,FileSize);
	}

	UINT LastPercents=0;

	// �������� ���� ������ �� �����
	while (!StopEvent.Signaled() && file.Read(readBufferA, (!SearchInFirst || alreadyRead + readBufferSize <= SearchInFirst)? readBufferSize : static_cast<DWORD>(SearchInFirst - alreadyRead), readBlockSize))
	{
		UINT Percents=static_cast<UINT>(FileSize?alreadyRead*100/FileSize:0);

		if (Percents!=LastPercents)
		{
			itd->SetPercent(Percents);
			LastPercents=Percents;
		}

		// ����������� ������� ���������� ����
		alreadyRead += readBlockSize;

		// ��� hex � ������������� ������ ������ �����
		if (SearchHex)
		{
			// �������, ���� ������ �� ��������� ��� ��������� ����
			if (!readBlockSize || readBlockSize<hexFindStringSize)
				return false;

			// ����
			if (FindStringBMH((unsigned char *)readBufferA, readBlockSize)!=-1)
				return true;
		}
		else
		{
			bool ErrorState = false;
			FOR_RANGE(codePages, cpi)
			{
				ErrorState = false;
				// ���������� ��������� ������� ��������
				if (!cpi->MaxCharSize)
				{
					ErrorState = true;
					continue;
				}

				// ���� ������ ����� ������� ���������� � ������ �� ������
				if (WholeWords && alreadyRead==readBlockSize)
				{
					cpi->WordFound = false;
					cpi->LastSymbol = 0;
				}

				// ���� ������ �� ���������
				if (!readBlockSize)
				{
					// ���� ����� �� ������ � � ����� ����������� ����� ���� ���-�� �������,
					// �� �������, ��� ����� ��, ��� �����
					if(WholeWords && cpi->WordFound)
						return true;
					else
					{
						ErrorState = true;
						continue;
					}
					// �������, ���� ��������� ������ ������� ������ ������ � ��� ������ �� ������
				}

				if (readBlockSize < findStringCount && !(WholeWords && cpi->WordFound))
				{
					ErrorState = true;
					continue;
				}

				// ���������� �������� � �������� ������
				size_t bufferCount;

				// ����� ��� ������
				wchar_t *buffer;

				// ���������� ����� � UTF-16
				if (IsUnicodeCodePage(cpi->CodePage))
				{
					// ��������� ������ ������ � UTF-16
					bufferCount = readBlockSize/sizeof(wchar_t);

					// �������, ���� ������ ������ ������ ����� ������ ������
					if (bufferCount < findStringCount)
					{
						ErrorState = true;
						continue;
					}

					// �������� ����� ������ � ����� ���������
					if (cpi->CodePage==CP_REVERSEBOM)
					{
						// ��� UTF-16 (big endian) ����������� ����� ������ � ����� ���������
						bufferCount = LCMapStringW(
							                LOCALE_NEUTRAL,//LOCALE_INVARIANT,
							                LCMAP_BYTEREV,
							                (wchar_t *)readBufferA,
							                (int)bufferCount,
							                readBuffer,
							                readBufferSize * sizeof(wchar_t)
							            );

						if (!bufferCount)
						{
							ErrorState = true;
							continue;
						}
						// ������������� ����� ����������
						buffer = readBuffer;
					}
					else
					{
						// ���� ����� � UTF-16 (little endian), �� ���������� �������� �����
						buffer = (wchar_t *)readBufferA;
					}
				}
				else
				{
					// ������������ ����� ������ �� ��������� ������ � UTF-16
					bufferCount = MultiByteToWideChar(cpi->CodePage, 0, readBufferA, readBlockSize, readBuffer, readBufferSize * sizeof(wchar_t));

					// �������, ���� ��� �� ������� ��������������� ������
					if (!bufferCount)
					{
						ErrorState = true;
						continue;
					}

					// ���� ��������� ������ ������� ������ ������ � ������ �� ������, �� ���������
					// ������ ������ ����� �� ����������� � �������
					// ���� � ��� ����� �� ������ � � ����� ����������� ����� ���� ���������
					if (WholeWords && cpi->WordFound)
					{
						// ���� ����� �����, �� �������, ��� ���� ����������� � �����
						if (findStringCount-1>=bufferCount)
							return true;
						// ��������� ������ ������ �������� ����� � ������ ��������� ��������, ������� ��������
						// ��� �������� ����� �������
						cpi->LastSymbol = readBuffer[findStringCount-1];

						if (IsWordDiv(cpi->LastSymbol))
							return true;

						// ���� ������ ������ ������ ������� �����, �� �������
						if (readBlockSize < findStringCount)
						{
							ErrorState = true;
							continue;
						}
					}

					// ������������� ����� ����������
					buffer = readBuffer;
				}

				unsigned int index = 0;

				do
				{
					// ���� ��������� � ������ � ���������� ������ � ������ � ������ ������
					int foundIndex = FindStringBMH(buffer+index, bufferCount-index);

					// ���� ��������� �� ������� ��� �� ��������� ���
					if (foundIndex == -1)
						break;

					// ���� ���������� ������� � �������� ����� �� ������, �� ������� ��� �� ������
					if (!WholeWords)
						return true;
					// ������������� ������� � �������� ������
					index += foundIndex;

					// ���� ��� ����� �� ������, �� ������ ������������� ��������
					bool firstWordDiv = false;

					// ���� �� ��������� ������� �����
					if (!index)
					{
						// ���� �� ��������� ������� �����, �� �������, ��� ����������� ����
						// ���� �� ��������� ������� �����, �� ��������� ��������
						// ��� ��� ��������� ������ ����������� ����� ������������
						if (alreadyRead==readBlockSize || IsWordDiv(cpi->LastSymbol))
							firstWordDiv = true;
					}
					else
					{
						// ��������� �������� ��� ��� ���������� ��������� ������ ����� ������������
						cpi->LastSymbol = buffer[index-1];

						if (IsWordDiv(cpi->LastSymbol))
							firstWordDiv = true;
					}

					// ��������� ����������� � �����, ������ ���� ������ ����������� �������
					if (firstWordDiv)
					{
						// ���� ���� ������ �� �� �����
						if (index+findStringCount!=bufferCount)
						{
							// ��������� �������� ��� ��� ����������� �� �������� ������ ����� ������������
							cpi->LastSymbol = buffer[index+findStringCount];

							if (IsWordDiv(cpi->LastSymbol))
								return true;
						}
						else
							cpi->WordFound = true;
					}
				}
				while (++index<=bufferCount-findStringCount);

				// �������, ���� �� ����� �� ������� ���������� ���� ����������� ��� ������
				if (SearchInFirst && SearchInFirst>=alreadyRead)
				{
					ErrorState = true;
					continue;
				}
				// ���������� ��������� ������ �����
				cpi->LastSymbol = buffer[bufferCount-1];
			}

			if (ErrorState)
				return false;

			// �������� �������� �� ������� �� ��������� ��� �������� ����� �������
			offset = (int)((CodePage==CP_DEFAULT?sizeof(wchar_t):codePages.begin()->MaxCharSize)*(findStringCount-1));
		}

		// ���� �� ������������ ��������� �� ���� ����
		if (readBlockSize==readBufferSize)
		{
			// ��������� ����� �� ����� ����� ������ ����� 1
			if (!file.SetPointer(-1*offset, nullptr, FILE_CURRENT))
				return false;
			alreadyRead -= offset;
		}
	}

	return false;
}

bool FindFiles::IsFileIncluded(PluginPanelItem* FileItem, const string& FullName, DWORD FileAttr, const string &strDisplayName)
{
	bool FileFound=FileMaskForFindFile->Compare(PointToName(FullName));
	ArcListItem* ArcItem = itd->GetFindFileArcItem();
	HANDLE hPlugin=nullptr;
	if(ArcItem)
	{
		hPlugin = ArcItem->hPlugin;
	}

	if (FileFound)
	{
		// ���� ������� ����� ������ hex-�����, ����� ����� � ����� �� ��������
		if ((FileAttr & FILE_ATTRIBUTE_DIRECTORY) && (!Global->Opt->FindOpt.FindFolders || SearchHex))
			return FALSE;

		if (!strFindStr.empty() && FileFound)
		{
			FileFound=false;

			if (!(FileAttr & FILE_ATTRIBUTE_DIRECTORY))
			{
				itd->SetFindMessage(strDisplayName);

				string strSearchFileName;
				bool RemoveTemp=false;

				if (hPlugin)
				{
					if (!Global->CtrlObject->Plugins->UseFarCommand(hPlugin, PLUGIN_FARGETFILES))
					{
						string strTempDir;
						FarMkTempEx(strTempDir); // � �������� �� nullptr???
						apiCreateDirectory(strTempDir,nullptr);

						bool GetFileResult=false;
						{
							CriticalSectionLock Lock(PluginCS);
							GetFileResult=Global->CtrlObject->Plugins->GetFile(hPlugin,FileItem,strTempDir,strSearchFileName,OPM_SILENT|OPM_FIND)!=FALSE;
						}
						if (GetFileResult)
						{
							RemoveTemp=true;
						}
						else
						{
							apiRemoveDirectory(strTempDir);
						}
					}
					else
					{
						strSearchFileName = strPluginSearchPath + FullName;
					}
				}
				else
				{
					strSearchFileName = FullName;
				}

				if (!strSearchFileName.empty() && LookForString(strSearchFileName))
					FileFound=true;

				if (RemoveTemp)
				{
					DeleteFileWithFolder(strSearchFileName);
				}
			}
		}
	}

	return FileFound;
}

intptr_t FindFiles::FindDlgProc(Dialog* Dlg, intptr_t Msg, intptr_t Param1, void* Param2)
{
	CriticalSectionLock Lock(PluginCS);
	VMenu *ListBox=Dlg->GetAllItem()[FD_LISTBOX].ListPtr;

	static bool Recurse=false;

	if(!Finalized && !Recurse)
	{
		static DWORD ShowTime=0;
		Recurse=true;
		DWORD Time=GetTickCount();
		if(Time-ShowTime>(DWORD)Global->Opt->RedrawTimeout)
		{
			ShowTime=Time;
			if (!StopEvent.Signaled())
			{
				LangString strDataStr(MFindFound);
				strDataStr << itd->GetFileCount() << itd->GetDirCount();
				Dlg->SendMessage(DM_SETTEXTPTR,FD_SEPARATOR1, UNSAFE_CSTR(strDataStr));

				LangString strSearchStr(MFindSearchingIn);

				if (!strFindStr.empty())
				{
					string strFStr(strFindStr);
					TruncStrFromEnd(strFStr,10);
					string strTemp(L" \"");
					strTemp+=strFStr+=L"\"";
					strSearchStr << strTemp;
				}
				else
					strSearchStr << L"";

				string strFM;
				itd->GetFindMessage(strFM);
				SMALL_RECT Rect;
				Dlg->SendMessage( DM_GETITEMPOSITION, FD_TEXT_STATUS, &Rect);
				TruncStrFromCenter(strFM, Rect.Right-Rect.Left+1 - static_cast<int>(strSearchStr.size()) - 1);
				strDataStr=strSearchStr;
				strDataStr += L" " + strFM;
				Dlg->SendMessage( DM_SETTEXTPTR, FD_TEXT_STATUS, UNSAFE_CSTR(strDataStr));

				strDataStr.Format(L"%3d%%",itd->GetPercent());
				Dlg->SendMessage( DM_SETTEXTPTR,FD_TEXT_STATUS_PERCENTS, UNSAFE_CSTR(strDataStr));

				if (itd->GetLastFoundNumber())
				{
					itd->SetLastFoundNumber(0);

					if (ListBox->UpdateRequired())
						Dlg->SendMessage(DM_SHOWITEM,FD_LISTBOX,ToPtr(1));
				}
			}
		}
		Recurse=false;
	}

	if(!Finalized && StopEvent.Signaled())
	{
		LangString strMessage(MFindDone);
		strMessage << itd->GetFileCount() << itd->GetDirCount();
		Dlg->SendMessage( DM_ENABLEREDRAW, FALSE, 0);
		Dlg->SendMessage( DM_SETTEXTPTR, FD_SEPARATOR1, nullptr);
		Dlg->SendMessage( DM_SETTEXTPTR, FD_TEXT_STATUS, UNSAFE_CSTR(strMessage));
		Dlg->SendMessage( DM_SETTEXTPTR, FD_TEXT_STATUS_PERCENTS, nullptr);
		Dlg->SendMessage( DM_SETTEXTPTR, FD_BUTTON_STOP, const_cast<wchar_t*>(MSG(MFindCancel)));
		Dlg->SendMessage( DM_ENABLEREDRAW, TRUE, 0);
		ConsoleTitle::SetFarTitle(strMessage);
		if(TB)
		{
			delete TB;
			TB=nullptr;
		}
		Finalized=true;
	}

	switch (Msg)
	{
	case DN_INITDIALOG:
		{
			Dlg->GetAllItem()[FD_LISTBOX].ListPtr->SetFlags(VMENU_NOMERGEBORDER);
		}
		break;

	case DN_DRAWDIALOGDONE:
		{
			Dlg->DefProc(Msg,Param1,Param2);

			// ���������� ����� �� ������ [Go To]
			if ((itd->GetDirCount() || itd->GetFileCount()) && !FindPositionChanged)
			{
				FindPositionChanged=true;
				Dlg->SendMessage(DM_SETFOCUS,FD_BUTTON_GOTO,0);
			}
			return TRUE;
		}
		break;

	case DN_CONTROLINPUT:
		{
			const INPUT_RECORD* record=(const INPUT_RECORD *)Param2;
			if (record->EventType!=KEY_EVENT) break;
			int key = InputRecordToKey(record);
			switch (key)
			{
			case KEY_ESC:
			case KEY_F10:
				{
					if (!StopEvent.Signaled())
					{
						PauseEvent.Reset();
						bool LocalRes=true;
						if (Global->Opt->Confirm.Esc)
							LocalRes=ConfirmAbortOp()!=0;
						PauseEvent.Set();
						if(LocalRes)
						{
							StopEvent.Set();
						}
						return TRUE;
					}
				}
				break;

			case KEY_CTRLALTSHIFTPRESS:
			case KEY_RCTRLALTSHIFTPRESS:
			case KEY_ALTF9:
			case KEY_RALTF9:
			case KEY_F11:
			case KEY_CTRLW:
			case KEY_RCTRLW:
				{
					FrameManager->ProcessKey((DWORD)key);
					return TRUE;
				}
				break;

			case KEY_RIGHT:
			case KEY_NUMPAD6:
			case KEY_TAB:
				{
					if (Param1==FD_BUTTON_STOP)
					{
						FindPositionChanged=true;
						Dlg->SendMessage(DM_SETFOCUS,FD_BUTTON_NEW,0);
						return TRUE;
					}
				}
				break;

			case KEY_LEFT:
			case KEY_NUMPAD4:
			case KEY_SHIFTTAB:
				{
					if (Param1==FD_BUTTON_NEW)
					{
						FindPositionChanged=true;
						Dlg->SendMessage(DM_SETFOCUS,FD_BUTTON_STOP,0);
						return TRUE;
					}
				}
				break;

			case KEY_UP:
			case KEY_DOWN:
			case KEY_NUMPAD8:
			case KEY_NUMPAD2:
			case KEY_PGUP:
			case KEY_PGDN:
			case KEY_NUMPAD9:
			case KEY_NUMPAD3:
			case KEY_HOME:
			case KEY_END:
			case KEY_NUMPAD7:
			case KEY_NUMPAD1:
			case KEY_MSWHEEL_UP:
			case KEY_MSWHEEL_DOWN:
			case KEY_ALTLEFT:
			case KEY_RALTLEFT:
			case KEY_ALT|KEY_NUMPAD4:
			case KEY_RALT|KEY_NUMPAD4:
			case KEY_MSWHEEL_LEFT:
			case KEY_ALTRIGHT:
			case KEY_RALTRIGHT:
			case KEY_ALT|KEY_NUMPAD6:
			case KEY_RALT|KEY_NUMPAD6:
			case KEY_MSWHEEL_RIGHT:
			case KEY_ALTSHIFTLEFT:
			case KEY_RALTSHIFTLEFT:
			case KEY_ALT|KEY_SHIFT|KEY_NUMPAD4:
			case KEY_RALT|KEY_SHIFT|KEY_NUMPAD4:
			case KEY_ALTSHIFTRIGHT:
			case KEY_RALTSHIFTRIGHT:
			case KEY_ALT|KEY_SHIFT|KEY_NUMPAD6:
			case KEY_RALT|KEY_SHIFT|KEY_NUMPAD6:
			case KEY_ALTHOME:
			case KEY_RALTHOME:
			case KEY_ALT|KEY_NUMPAD7:
			case KEY_RALT|KEY_NUMPAD7:
			case KEY_ALTEND:
			case KEY_RALTEND:
			case KEY_ALT|KEY_NUMPAD1:
			case KEY_RALT|KEY_NUMPAD1:
				{
					ListBox->ProcessKey((unsigned)key);
					return TRUE;
				}
				break;

			/*
			case KEY_CTRLA:
			case KEY_RCTRLA:
			{
				if (!ListBox->GetItemCount())
				{
					return TRUE;
				}

				size_t ItemIndex = *static_cast<size_t*>(ListBox->GetUserData(nullptr,0));

				FINDLIST FindItem;
				itd->GetFindListItem(ItemIndex, FindItem);

				if (ShellSetFileAttributes(nullptr,FindItem.FindData.strFileName))
				{
					itd->SetFindListItem(ItemIndex, FindItem);
					Dlg->SendMessage(DM_REDRAW,0,0);
				}
				return TRUE;
			}
			*/

			case KEY_F3:
			case KEY_NUMPAD5:
			case KEY_SHIFTNUMPAD5:
			case KEY_F4:
				{
					if (!ListBox->GetItemCount())
					{
						return TRUE;
					}

					FindListItem* FindItem = *reinterpret_cast<FindListItem**>(ListBox->GetUserData(nullptr,0));
					bool RemoveTemp=false;
					string strSearchFileName;
					string strTempDir;

					if (FindItem->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{
						return TRUE;
					}

					bool real_name = true;

					if(FindItem->Arc)
					{
						if(!(FindItem->Arc->Flags & OPIF_REALNAMES))
						{
							// ������� ���� ���������, ���� �������.
							bool ClosePanel=false;
							real_name = false;

							string strFindArcName = FindItem->Arc->strArcName;
							if(!FindItem->Arc->hPlugin)
							{
								int SavePluginsOutput=Global->DisablePluginsOutput;
								Global->DisablePluginsOutput=TRUE;
								{
									CriticalSectionLock Lock(PluginCS);
									FindItem->Arc->hPlugin = Global->CtrlObject->Plugins->OpenFilePlugin(&strFindArcName, 0, OFP_SEARCH);
								}
								Global->DisablePluginsOutput=SavePluginsOutput;

								if (FindItem->Arc->hPlugin == PANEL_STOP ||
										!FindItem->Arc->hPlugin)
								{
									FindItem->Arc->hPlugin = nullptr;
									return TRUE;
								}

								ClosePanel = true;
							}
							FarMkTempEx(strTempDir);
							apiCreateDirectory(strTempDir, nullptr);
							CriticalSectionLock Lock(PluginCS);
							struct UserDataItem UserData={FindItem->Data,FindItem->FreeData};
							bool bGet=GetPluginFile(FindItem->Arc,FindItem->FindData,strTempDir,strSearchFileName,&UserData);
							if (!bGet)
							{
								apiRemoveDirectory(strTempDir);

								if (ClosePanel)
								{
									Global->CtrlObject->Plugins->ClosePanel(FindItem->Arc->hPlugin);
									FindItem->Arc->hPlugin = nullptr;
								}
								return FALSE;
							}
							else
							{
								if (ClosePanel)
								{
									Global->CtrlObject->Plugins->ClosePanel(FindItem->Arc->hPlugin);
									FindItem->Arc->hPlugin = nullptr;
								}
							}
							RemoveTemp=true;
						}
					}

					if (real_name)
					{
						strSearchFileName = FindItem->FindData.strFileName;
						if (apiGetFileAttributes(strSearchFileName) == INVALID_FILE_ATTRIBUTES && apiGetFileAttributes(FindItem->FindData.strAlternateFileName) != INVALID_FILE_ATTRIBUTES)
							strSearchFileName = FindItem->FindData.strAlternateFileName;
					}

					DWORD FileAttr=apiGetFileAttributes(strSearchFileName);

					if (FileAttr!=INVALID_FILE_ATTRIBUTES)
					{
						string strOldTitle;
						Global->Console->GetTitle(strOldTitle);

						if (key==KEY_F3 || key==KEY_NUMPAD5 || key==KEY_SHIFTNUMPAD5)
						{
							NamesList ViewList;
							int list_count = 0;

							// ������� ��� �����, ������� ����� �������� �����...
							if (Global->Opt->FindOpt.CollectFiles)
							{
								itd->Lock();
								std::for_each(CONST_RANGE(itd->GetFindList(), i)
								{
									bool RealNames=true;
									if(i.Arc)
									{
										if(!(i.Arc->Flags & OPIF_REALNAMES))
										{
											RealNames=false;
										}
									}
									if (RealNames)
									{
										if (!i.FindData.strFileName.empty() && !(i.FindData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
										{
											++list_count;
											ViewList.AddName(i.FindData.strFileName, i.FindData.strAlternateFileName);
										}
									}
								});
								itd->Unlock();
								string strCurDir = FindItem->FindData.strFileName;
								ViewList.SetCurName(strCurDir);
							}

							Dlg->SendMessage(DM_SHOWDIALOG,FALSE,0);
							Dlg->SendMessage(DM_ENABLEREDRAW,FALSE,0);
							{
								FileViewer ShellViewer(strSearchFileName,FALSE,FALSE,FALSE,-1,nullptr,(list_count > 1 ? &ViewList : nullptr));
								ShellViewer.SetDynamicallyBorn(FALSE);
								ShellViewer.SetEnableF6(TRUE);

								if(FindItem->Arc)
								{
									if(!(FindItem->Arc->Flags & OPIF_REALNAMES))
									{
										ShellViewer.SetSaveToSaveAs(true);
									}
								}
								FrameManager->EnterModalEV();
								FrameManager->ExecuteModal();
								FrameManager->ExitModalEV();
								// ���������� ���������� �����
								FrameManager->ProcessKey(KEY_CONSOLE_BUFFER_RESIZE);
							}
							Dlg->SendMessage(DM_ENABLEREDRAW,TRUE,0);
							Dlg->SendMessage(DM_SHOWDIALOG,TRUE,0);
						}
						else
						{
							Dlg->SendMessage(DM_SHOWDIALOG,FALSE,0);
							Dlg->SendMessage(DM_ENABLEREDRAW,FALSE,0);
							{
								/* $ 14.08.2002 VVM
								  ! ����-��� �������� �� ������ ������������� � �������� ��������.
								    � ���������, ������� �� ��� �� �������� ������
															int FramePos=FrameManager->FindFrameByFile(MODALTYPE_EDITOR,SearchFileName);
															int SwitchTo=FALSE;
															if (FramePos!=-1)
															{
																if (!(*FrameManager)[FramePos]->GetCanLoseFocus(TRUE) ||
																	Global->Opt->Confirm.AllowReedit)
																{
																	char MsgFullFileName[NM];
																	xstrncpy(MsgFullFileName,SearchFileName,sizeof(MsgFullFileName));
																	int MsgCode=Message(0,2,MSG(MFindFileTitle),
																				TruncPathStr(MsgFullFileName,ScrX-16),
																				MSG(MAskReload),
																				MSG(MCurrent),MSG(MNewOpen));
																	if (!MsgCode)
																	{
																		SwitchTo=TRUE;
																	}
																	else if (MsgCode==1)
																	{
																		SwitchTo=FALSE;
																	}
																	else
																	{
																		Dlg->SendMessage(DM_ENABLEREDRAW,TRUE,0);
																		Dlg->SendMessage(DM_SHOWDIALOG,TRUE,0);
																		return TRUE;
																	}
																}
																else
																{
																	SwitchTo=TRUE;
																}
															}
															if (SwitchTo)
															{
																(*FrameManager)[FramePos]->SetCanLoseFocus(FALSE);
																(*FrameManager)[FramePos]->SetDynamicallyBorn(FALSE);
																FrameManager->ActivateFrame(FramePos);
																FrameManager->EnterModalEV();
																FrameManager->ExecuteModal ();
																FrameManager->ExitModalEV();
																// FrameManager->ExecuteNonModal();
																// ���������� ���������� �����
																FrameManager->ProcessKey(KEY_CONSOLE_BUFFER_RESIZE);
															}
															else
								*/
								{
									FileEditor ShellEditor(strSearchFileName,CP_DEFAULT,0);
									ShellEditor.SetDynamicallyBorn(FALSE);
									ShellEditor.SetEnableF6(TRUE);

									if(FindItem->Arc)
									{
										if(!(FindItem->Arc->Flags & OPIF_REALNAMES))
										{
											ShellEditor.SetSaveToSaveAs(TRUE);
										}
									}
									FrameManager->EnterModalEV();
									FrameManager->ExecuteModal();
									FrameManager->ExitModalEV();
									// ���������� ���������� �����
									FrameManager->ProcessKey(KEY_CONSOLE_BUFFER_RESIZE);
								}
							}
							Dlg->SendMessage(DM_ENABLEREDRAW,TRUE,0);
							Dlg->SendMessage(DM_SHOWDIALOG,TRUE,0);
						}
						Global->Console->SetTitle(strOldTitle);
					}
					if (RemoveTemp)
					{
						DeleteFileWithFolder(strSearchFileName);
					}
					return TRUE;
				}
				break;
			default:
				break;
			}
		}
		break;

	case DN_BTNCLICK:
		{
			FindPositionChanged = true;
			switch (Param1)
			{
			case FD_BUTTON_NEW:
				{
					StopEvent.Set();
					return FALSE;
				}
				break;

			case FD_BUTTON_STOP:
				{
					if(!StopEvent.Signaled())
					{
						StopEvent.Set();
						return TRUE;
					}
					else
					{
						return FALSE;
					}
				}
				break;

			case FD_BUTTON_VIEW:
				{
					INPUT_RECORD key;
					KeyToInputRecord(KEY_F3,&key);
					FindDlgProc(Dlg,DN_CONTROLINPUT,FD_LISTBOX,&key);
					return TRUE;
				}
				break;

			case FD_BUTTON_GOTO:
			case FD_BUTTON_PANEL:
				{
					// ������� � ����� �� ������ ����� ������ �� � �������, � ����� ��������� ������.
					// ����� �������� ��������, ����� �� ���� �� ������, ����� �� ������� � ������� �����
					// (� �����-�� ����!) � � ���������� ��� ���������.
					if(!ListBox->GetItemCount())
					{
						return TRUE;
					}
					FindExitItem = *reinterpret_cast<FindListItem**>(ListBox->GetUserData(nullptr, 0));
					if (TB)
					{
						delete TB;
						TB=nullptr;
					}
					return FALSE;
				}
				break;
			default:
				break;
			}
		}
		break;

	case DN_CLOSE:
		{
			BOOL Result = TRUE;
			if (Param1==FD_LISTBOX)
			{
				if(ListBox->GetItemCount())
				{
					FindDlgProc(Dlg,DN_BTNCLICK,FD_BUTTON_GOTO,0); // emulates a [ Go to ] button pressing;
				}
				else
				{
					Result = FALSE;
				}
			}
			if(Result)
			{
				StopEvent.Set();
			}
			return Result;
		}
		break;

	case DN_RESIZECONSOLE:
		{
			PCOORD pCoord = static_cast<PCOORD>(Param2);
			SMALL_RECT DlgRect;
			Dlg->SendMessage( DM_GETDLGRECT, 0, &DlgRect);
			int DlgWidth=DlgRect.Right-DlgRect.Left+1;
			int DlgHeight=DlgRect.Bottom-DlgRect.Top+1;
			int IncX = pCoord->X - DlgWidth - 2;
			int IncY = pCoord->Y - DlgHeight - 2;
			Dlg->SendMessage( DM_ENABLEREDRAW, FALSE, 0);

			for (int i = 0; i <= FD_BUTTON_STOP; i++)
			{
				Dlg->SendMessage( DM_SHOWITEM, i, FALSE);
			}

			if ((IncX > 0) || (IncY > 0))
			{
				pCoord->X = DlgWidth + (IncX > 0 ? IncX : 0);
				pCoord->Y = DlgHeight + (IncY > 0 ? IncY : 0);
				Dlg->SendMessage( DM_RESIZEDIALOG, 0, pCoord);
			}

			DlgWidth += IncX;
			DlgHeight += IncY;

			for (int i = 0; i < FD_SEPARATOR1; i++)
			{
				SMALL_RECT rect;
				Dlg->SendMessage( DM_GETITEMPOSITION, i, &rect);
				rect.Right += IncX;
				rect.Bottom += IncY;
				Dlg->SendMessage( DM_SETITEMPOSITION, i, &rect);
			}

			for (int i = FD_SEPARATOR1; i <= FD_BUTTON_STOP; i++)
			{
				SMALL_RECT rect;
				Dlg->SendMessage( DM_GETITEMPOSITION, i, &rect);

				if (i == FD_TEXT_STATUS)
				{
					rect.Right += IncX;
				}
				else if (i==FD_TEXT_STATUS_PERCENTS)
				{
					rect.Right+=IncX;
					rect.Left+=IncX;
				}

				rect.Top += IncY;
				Dlg->SendMessage( DM_SETITEMPOSITION, i, &rect);
			}

			if ((IncX <= 0) || (IncY <= 0))
			{
				pCoord->X = DlgWidth;
				pCoord->Y = DlgHeight;
				Dlg->SendMessage( DM_RESIZEDIALOG, 0, pCoord);
			}

			for (int i = 0; i <= FD_BUTTON_STOP; i++)
			{
				Dlg->SendMessage( DM_SHOWITEM, i, ToPtr(TRUE));
			}

			Dlg->SendMessage( DM_ENABLEREDRAW, TRUE, 0);
			return TRUE;
		}
		break;
	default:
		break;
	}

	return Dlg->DefProc(Msg,Param1,Param2);
}

void FindFiles::AddMenuRecord(Dialog* Dlg,const string& FullName, const FAR_FIND_DATA& FindData, void* Data, FARPANELITEMFREECALLBACK FreeData)
{
	if (!Dlg)
		return;

	VMenu *ListBox=Dlg->GetAllItem()[FD_LISTBOX].ListPtr;

	if(!ListBox->GetItemCount())
	{
		Dlg->SendMessage( DM_ENABLE, FD_BUTTON_GOTO, ToPtr(TRUE));
		Dlg->SendMessage( DM_ENABLE, FD_BUTTON_VIEW, ToPtr(TRUE));
		if(AnySetFindList)
		{
			Dlg->SendMessage( DM_ENABLE, FD_BUTTON_PANEL, ToPtr(TRUE));
		}
		Dlg->SendMessage( DM_ENABLE, FD_LISTBOX, ToPtr(TRUE));
	}

	MenuItemEx ListItem = {};

	FormatString MenuText;

	string strDateStr, strTimeStr;
	const wchar_t *DisplayName=FindData.strFileName.c_str();

	unsigned __int64 *ColumnType=Global->Opt->FindOpt.OutColumnTypes;
	int *ColumnWidth=Global->Opt->FindOpt.OutColumnWidths;
	int ColumnCount=Global->Opt->FindOpt.OutColumnCount;
	//int *ColumnWidthType=Global->Opt->FindOpt.OutColumnWidthType;

	MenuText << L' ';

	for (int Count=0; Count < ColumnCount; ++Count)
	{
		int CurColumnType = static_cast<int>(ColumnType[Count] & 0xFF);

		switch (CurColumnType)
		{
			case DIZ_COLUMN:
			case OWNER_COLUMN:
			{
				// ����������, �� �����������
				break;
			}
			case NAME_COLUMN:
			{
				// ���� ���� �������, ����������, �.�. ���� ����� ������������ � ���� � �����.
				break;
			}

			case ATTR_COLUMN:
			{
				MenuText << FormatStr_Attribute(FindData.dwFileAttributes) << BoxSymbols[BS_V1];
				break;
			}
			case NUMSTREAMS_COLUMN:
			case STREAMSSIZE_COLUMN:
			case SIZE_COLUMN:
			case PACKED_COLUMN:
			case NUMLINK_COLUMN:
			{
				UINT64 StreamsSize=0;
				DWORD StreamsCount=0;

				if (itd->GetFindFileArcItem())
				{
					if (CurColumnType == NUMSTREAMS_COLUMN || CurColumnType == STREAMSSIZE_COLUMN)
						EnumStreams(FindData.strFileName,StreamsSize,StreamsCount);
					else if(CurColumnType == NUMLINK_COLUMN)
						StreamsCount=GetNumberOfLinks(FindData.strFileName);
				}

				MenuText << FormatStr_Size(
								FindData.nFileSize,
								FindData.nAllocationSize,
								(CurColumnType == NUMSTREAMS_COLUMN || CurColumnType == NUMLINK_COLUMN)?StreamsCount:StreamsSize,
								DisplayName,
								FindData.dwFileAttributes,
								0,
								FindData.dwReserved0,
								(CurColumnType == NUMSTREAMS_COLUMN || CurColumnType == NUMLINK_COLUMN)?STREAMSSIZE_COLUMN:CurColumnType,
								ColumnType[Count],
								ColumnWidth[Count]);

				MenuText << BoxSymbols[BS_V1];
				break;
			}

			case DATE_COLUMN:
			case TIME_COLUMN:
			case WDATE_COLUMN:
			case ADATE_COLUMN:
			case CDATE_COLUMN:
			case CHDATE_COLUMN:
			{
				const FILETIME *FileTime;
				switch (CurColumnType)
				{
					case CDATE_COLUMN:
						FileTime=&FindData.ftCreationTime;
						break;
					case ADATE_COLUMN:
						FileTime=&FindData.ftLastAccessTime;
						break;
					case CHDATE_COLUMN:
						FileTime=&FindData.ftChangeTime;
						break;
					case DATE_COLUMN:
					case TIME_COLUMN:
					case WDATE_COLUMN:
					default:
						FileTime=&FindData.ftLastWriteTime;
						break;
				}

				MenuText << FormatStr_DateTime(FileTime,CurColumnType,ColumnType[Count],ColumnWidth[Count]) << BoxSymbols[BS_V1];
				break;
			}
		}
	}


	// � �������� ������������� �������� ��������� � ����� �� ���
	// ��� ����������� ��� ����������� � ������, �������� ����,
	// �.�. ��������� ������� ���������� ��� ������ � ������ ����,
	// � ������� ��������� ������.

	const wchar_t *DisplayName0=DisplayName;
	if (itd->GetFindFileArcItem())
		DisplayName0 = PointToName(DisplayName0);
	MenuText << DisplayName0;

	string strPathName=FullName;
	{
		size_t pos;

		if (FindLastSlash(pos,strPathName))
			strPathName.resize(pos);
		else
			strPathName.clear();
	}
	AddEndSlash(strPathName);

	if (StrCmpI(strPathName.c_str(),strLastDirName.c_str()))
	{
		if (ListBox->GetItemCount())
		{
			ListItem.Flags|=LIF_SEPARATOR;
			ListBox->AddItem(&ListItem);
			ListItem.Flags&=~LIF_SEPARATOR;
		}

		strLastDirName = strPathName;

		if (itd->GetFindFileArcItem())
		{
			ArcListItem* ArcItem = itd->GetFindFileArcItem();
			if(!(ArcItem->Flags & OPIF_REALNAMES) && !ArcItem->strArcName.empty())
			{
				string strArcPathName=ArcItem->strArcName;
				strArcPathName+=L":";

				if (!IsSlash(strPathName.at(0)))
					AddEndSlash(strArcPathName);

				strArcPathName += strPathName == L".\\"? L"\\" : strPathName.c_str();
				strPathName = strArcPathName;
			}
		}
		ListItem.strName = strPathName;
		FindListItem& FindItem = itd->AddFindListItem(FindData,Data,nullptr);
		// ������� ������ � FindData. ��� ��� �� �����
		FindItem.FindData.Clear();
		// ���������� LastDirName, �.�. PathName ��� ����� ���� ��������
		FindItem.FindData.strFileName = strLastDirName;
		// Used=0 - ��� �� �������� �� ��������� ������.
		FindItem.Used=0;
		// �������� ������� � ��������, ���-�� �� �� ��� ������ :)
		FindItem.FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		FindItem.Arc = itd->GetFindFileArcItem();;

		auto Ptr = &FindItem;
		ListBox->SetUserData(&Ptr,sizeof(Ptr),ListBox->AddItem(&ListItem));
	}

	FindListItem& FindItem = itd->AddFindListItem(FindData,Data,FreeData);
	FindItem.FindData.strFileName = FullName;
	FindItem.Used=1;
	FindItem.Arc = itd->GetFindFileArcItem();

	ListItem.strName = MenuText;
	int ListPos = ListBox->AddItem(&ListItem);
	auto Ptr = &FindItem;
	ListBox->SetUserData(&Ptr, sizeof(Ptr), ListPos);

	// ������� ��� �������� - � ������.
	int FC=itd->GetFileCount(), DC=itd->GetDirCount(), LF=itd->GetLastFoundNumber();
	if (!FC && !DC)
	{
		ListBox->SetSelectPos(ListPos, -1);
	}

	if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		DC++;
	}
	else
	{
		FC++;
	}

	LF++;

	itd->SetFileCount(FC);
	itd->SetDirCount(DC);
	itd->SetLastFoundNumber(LF);
}

void FindFiles::AddMenuRecord(Dialog* Dlg,const string& FullName, PluginPanelItem& FindData)
{
	FAR_FIND_DATA fdata;
	PluginPanelItemToFindDataEx(&FindData, &fdata);
	AddMenuRecord(Dlg,FullName, fdata, FindData.UserData.Data, FindData.UserData.FreeData);
	FindData.UserData.FreeData = nullptr; //�������� � FINDLIST
}

void FindFiles::ArchiveSearch(Dialog* Dlg, const string& ArcName)
{
	_ALGO(CleverSysLog clv(L"FindFiles::ArchiveSearch()"));
	_ALGO(SysLog(L"ArcName='%s'",(ArcName?ArcName:L"nullptr")));

	int SavePluginsOutput=Global->DisablePluginsOutput;
	Global->DisablePluginsOutput=TRUE;
	string strArcName = ArcName;
	HANDLE hArc;
	{
		CriticalSectionLock Lock(PluginCS);
		hArc = Global->CtrlObject->Plugins->OpenFilePlugin(&strArcName, OPM_FIND, OFP_SEARCH);
	}
	Global->DisablePluginsOutput=SavePluginsOutput;

	if (hArc==PANEL_STOP)
	{
		//StopEvent.Set(); // ??
		_ALGO(SysLog(L"return: hArc==(HANDLE)-2"));
		return;
	}

	if (!hArc)
	{
		_ALGO(SysLog(L"return: hArc==nullptr"));
		return;
	}

	int SaveSearchMode=SearchMode;
	ArcListItem* SaveArcItem = itd->GetFindFileArcItem();
	{
		int SavePluginsOutput=Global->DisablePluginsOutput;
		Global->DisablePluginsOutput=TRUE;

		SearchMode=FINDAREA_FROM_CURRENT;
		OpenPanelInfo Info;
		Global->CtrlObject->Plugins->GetOpenPanelInfo(hArc,&Info);
		itd->SetFindFileArcItem(&itd->AddArcListItem(ArcName, hArc, Info.Flags, Info.CurDir));
		// �������� ������� ����� ������� � ������. � ���� ������ �� ����� - �� ������ ��� �����.
		{
			string strSaveDirName, strSaveSearchPath;
			size_t SaveListCount = itd->GetFindListCount();
			// �������� ���� ������ � �������, ��� ����� ����������.
			strSaveSearchPath = strPluginSearchPath;
			strSaveDirName = strLastDirName;
			strLastDirName.clear();
			DoPreparePluginList(Dlg,true);
			strPluginSearchPath = strSaveSearchPath;
			ArcListItem* ArcItem = itd->GetFindFileArcItem();
			{
				CriticalSectionLock Lock(PluginCS);
				Global->CtrlObject->Plugins->ClosePanel(ArcItem->hPlugin);
			}
			ArcItem->hPlugin = nullptr;

			if (SaveListCount == itd->GetFindListCount())
				strLastDirName = strSaveDirName;
		}

		Global->DisablePluginsOutput=SavePluginsOutput;
	}
	itd->SetFindFileArcItem(SaveArcItem);
	SearchMode=SaveSearchMode;
}

void FindFiles::DoScanTree(Dialog* Dlg, const string& strRoot)
{
	ScanTree ScTree(
		false,
		!(SearchMode==FINDAREA_CURRENT_ONLY||SearchMode==FINDAREA_INPATH),
		Global->Opt->FindOpt.FindSymLinks
	);
	string strSelName;
	DWORD FileAttr;

	if (SearchMode==FINDAREA_SELECTED)
		Global->CtrlObject->Cp()->ActivePanel->GetSelName(nullptr,FileAttr);

	while (!StopEvent.Signaled())
	{
		string strCurRoot;

		if (SearchMode==FINDAREA_SELECTED)
		{
			if (!Global->CtrlObject->Cp()->ActivePanel->GetSelName(&strSelName,FileAttr))
				break;

			if (!(FileAttr & FILE_ATTRIBUTE_DIRECTORY) || TestParentFolderName(strSelName) || strSelName == L".")
				continue;

			strCurRoot = strRoot;
			AddEndSlash(strCurRoot);
			strCurRoot += strSelName;
		}
		else
		{
			strCurRoot = strRoot;
		}

		ScTree.SetFindPath(strCurRoot,L"*");
		itd->SetFindMessage(strCurRoot);
		FAR_FIND_DATA FindData;
		string strFullName;

		while (!StopEvent.Signaled() && ScTree.GetNextName(&FindData,strFullName))
		{
			Sleep(0);
			PauseEvent.Wait();

			bool bContinue=false;
			WIN32_FIND_STREAM_DATA sd;
			HANDLE hFindStream = INVALID_HANDLE_VALUE;
			bool FirstCall=true;
			string strFindDataFileName=FindData.strFileName;

			if (Global->Opt->FindOpt.FindAlternateStreams)
			{
				hFindStream=apiFindFirstStream(strFullName,FindStreamInfoStandard,&sd);
			}

			// process default streams first
			bool ProcessAlternateStreams = false;
			while (!StopEvent.Signaled())
			{
				string strFullStreamName=strFullName;

				if (ProcessAlternateStreams)
				{
					if (hFindStream != INVALID_HANDLE_VALUE)
					{
						if (!FirstCall)
						{
							if (!apiFindNextStream(hFindStream,&sd))
							{
								apiFindStreamClose(hFindStream);
								break;
							}
						}
						else
						{
							FirstCall=false;
						}

						LPWSTR NameEnd=wcschr(sd.cStreamName+1,L':');

						if (NameEnd)
						{
							*NameEnd=L'\0';
						}

						if (sd.cStreamName[1]) // alternate stream
						{
							strFullStreamName+=sd.cStreamName;
							FindData.strFileName=strFindDataFileName+sd.cStreamName;
							FindData.nFileSize=sd.StreamSize.QuadPart;
							FindData.dwFileAttributes &= ~FILE_ATTRIBUTE_DIRECTORY;
						}
						else
						{
							// default stream is already processed
							continue;
						}
					}
					else
					{
						if (bContinue)
						{
							break;
						}
					}
				}

				if (UseFilter)
				{
					enumFileInFilterType foundType;

					if (!Filter->FileInFilter(FindData, &foundType, &strFullName))
					{
						// ���� �������, ���� �� ������ � ������ ��� ������ � Exclude-������
						if ((FindData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) && foundType==FIFT_EXCLUDE)
							ScTree.SkipDir(); // ������� ������ �� Exclude-�������, �.�. ������ ���� ����� �����������

						{
							bContinue=true;

							if (ProcessAlternateStreams)
							{
								continue;
							}
							else
							{
								break;
							}
						}
					}
				}

				if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					itd->SetFindMessage(strFullName);
				}

				if (IsFileIncluded(nullptr,strFullStreamName,FindData.dwFileAttributes,strFullName))
				{
					AddMenuRecord(Dlg,strFullStreamName, FindData, nullptr, nullptr);
				}

				ProcessAlternateStreams = Global->Opt->FindOpt.FindAlternateStreams;
				if (!Global->Opt->FindOpt.FindAlternateStreams || hFindStream == INVALID_HANDLE_VALUE)
				{
					break;
				}
			}

			if (bContinue)
			{
				continue;
			}

			if (SearchInArchives)
				ArchiveSearch(Dlg,strFullName);
		}

		if (SearchMode!=FINDAREA_SELECTED)
			break;
	}
}

void FindFiles::ScanPluginTree(Dialog* Dlg, HANDLE hPlugin, UINT64 Flags, int& RecurseLevel)
{
	PluginPanelItem *PanelData=nullptr;
	size_t ItemCount=0;
	bool GetFindDataResult=false;
	{
		CriticalSectionLock Lock(PluginCS);
		{
			if(!StopEvent.Signaled())
			{
				GetFindDataResult=Global->CtrlObject->Plugins->GetFindData(hPlugin,&PanelData,&ItemCount,OPM_FIND)!=FALSE;
			}
		}
	}
	if (!GetFindDataResult)
	{
		return;
	}

	RecurseLevel++;

	if (SearchMode!=FINDAREA_SELECTED || RecurseLevel!=1)
	{
		for (size_t I=0; I<ItemCount && !StopEvent.Signaled(); I++)
		{
			Sleep(0);
			PauseEvent.Wait();

			PluginPanelItem *CurPanelItem=PanelData+I;
			string strCurName=CurPanelItem->FileName;
			string strFullName;

			if (strCurName == L"." || TestParentFolderName(strCurName))
				continue;

			strFullName = strPluginSearchPath;
			strFullName += strCurName;

			if (!UseFilter || Filter->FileInFilter(*CurPanelItem))
			{
				if (CurPanelItem->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					itd->SetFindMessage(strFullName);
				}

				if (IsFileIncluded(CurPanelItem,strCurName,CurPanelItem->FileAttributes,strFullName))
					AddMenuRecord(Dlg,strFullName, *CurPanelItem);

				if (SearchInArchives && hPlugin && (Flags & OPIF_REALNAMES))
					ArchiveSearch(Dlg,strFullName);
			}
		}
	}

	if (SearchMode!=FINDAREA_CURRENT_ONLY)
	{
		for (size_t I=0; I<ItemCount && !StopEvent.Signaled(); I++)
		{
			PluginPanelItem *CurPanelItem=PanelData+I;

			if ((CurPanelItem->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			 && StrCmp(CurPanelItem->FileName, L".") && !TestParentFolderName(CurPanelItem->FileName)
			 && (!UseFilter || Filter->FileInFilter(*CurPanelItem))
			 && (SearchMode!=FINDAREA_SELECTED || RecurseLevel!=1 || Global->CtrlObject->Cp()->ActivePanel->IsSelected(CurPanelItem->FileName))
			) {
				bool SetDirectoryResult=false;
				{
					CriticalSectionLock Lock(PluginCS);
					SetDirectoryResult=Global->CtrlObject->Plugins->SetDirectory(hPlugin, CurPanelItem->FileName, OPM_FIND, &CurPanelItem->UserData)!=FALSE;
				}
				if (SetDirectoryResult)
				{
					strPluginSearchPath += CurPanelItem->FileName;
					strPluginSearchPath += L"\\";
					ScanPluginTree(Dlg, hPlugin, Flags, RecurseLevel);

					size_t pos = strPluginSearchPath.rfind(L'\\');
					if (pos != string::npos)
						strPluginSearchPath.resize(pos);

					if ((pos = strPluginSearchPath.rfind(L'\\')) != string::npos)
						strPluginSearchPath.resize(pos+1);
					else
						strPluginSearchPath.clear();

					SetDirectoryResult=false;
					{
						CriticalSectionLock Lock(PluginCS);
						SetDirectoryResult=Global->CtrlObject->Plugins->SetDirectory(hPlugin,L"..",OPM_FIND)!=FALSE;
					}
					if (!SetDirectoryResult)
					{
						StopEvent.Set();
					}
				}
			}
		}
	}

	Global->CtrlObject->Plugins->FreeFindData(hPlugin,PanelData,ItemCount,true);
	RecurseLevel--;
}

void FindFiles::DoPrepareFileList(Dialog* Dlg)
{
	string strRoot;
	Global->CtrlObject->CmdLine->GetCurDir(strRoot);
	if (strRoot.find(L';') != string::npos)
		InsertQuote(strRoot);

	string InitString;

	if (SearchMode==FINDAREA_INPATH)
	{
		string strPathEnv;
		apiGetEnvironmentVariable(L"PATH",strPathEnv);
		InitString = strPathEnv;
	}
	else if (SearchMode==FINDAREA_ROOT)
	{
		GetPathRoot(strRoot,strRoot);
		InitString = strRoot;
	}
	else if (SearchMode==FINDAREA_ALL || SearchMode==FINDAREA_ALL_BUTNETWORK)
	{
		std::list<string> Volumes;
		DWORD DiskMask=FarGetLogicalDrives();
		for (WCHAR CurrentDisk=0; DiskMask; CurrentDisk++,DiskMask>>=1)
		{
			if (!(DiskMask & 1))
				continue;

			string Root(L"?:\\");
			Root[0] = L'A' + CurrentDisk;
			int DriveType=FAR_GetDriveType(Root);

			if (DriveType==DRIVE_REMOVABLE || IsDriveTypeCDROM(DriveType) || (DriveType==DRIVE_REMOTE && SearchMode==FINDAREA_ALL_BUTNETWORK))
			{
				continue;
			}
			string strGuidVolime;
			if(apiGetVolumeNameForVolumeMountPoint(Root, strGuidVolime))
			{
				Volumes.emplace_back(strGuidVolime);
			}
			InitString += Root + L";";
		}
		WCHAR VolumeName[50];

		bool End = false;
		HANDLE hFind = INVALID_HANDLE_VALUE;

		for(hFind = FindFirstVolume(VolumeName, ARRAYSIZE(VolumeName)); hFind != INVALID_HANDLE_VALUE && !End; End = FindNextVolume(hFind, VolumeName, ARRAYSIZE(VolumeName)) == FALSE)
		{
			int DriveType=FAR_GetDriveType(VolumeName);

			if (DriveType==DRIVE_REMOVABLE || IsDriveTypeCDROM(DriveType) || (DriveType==DRIVE_REMOTE && SearchMode==FINDAREA_ALL_BUTNETWORK))
			{
				continue;
			}

			if (std::none_of(CONST_RANGE(Volumes, i) {return i.compare(0, wcslen(VolumeName), VolumeName) == 0;}))
			{
				InitString.append(VolumeName).append(L";");
			}
		}
		if (hFind != INVALID_HANDLE_VALUE)
		{
			FindVolumeClose(hFind);
		}
	}
	else
	{
		InitString = strRoot;
	}
	auto List(StringToList(InitString, STLF_UNIQUE, L";"));
	std::for_each(CONST_RANGE(List, i)
	{
		DoScanTree(Dlg, i);
	});

	itd->SetPercent(0);
	StopEvent.Set();
}

void FindFiles::DoPreparePluginList(Dialog* Dlg, bool Internal)
{
	ArcListItem* ArcItem = itd->GetFindFileArcItem();
	OpenPanelInfo Info;
	string strSaveDir;
	{
		CriticalSectionLock Lock(PluginCS);
		Global->CtrlObject->Plugins->GetOpenPanelInfo(ArcItem->hPlugin,&Info);
		strSaveDir = Info.CurDir;
		if (SearchMode==FINDAREA_ROOT || SearchMode==FINDAREA_ALL || SearchMode==FINDAREA_ALL_BUTNETWORK || SearchMode==FINDAREA_INPATH)
		{
			Global->CtrlObject->Plugins->SetDirectory(ArcItem->hPlugin,L"\\",OPM_FIND);
			Global->CtrlObject->Plugins->GetOpenPanelInfo(ArcItem->hPlugin,&Info);
		}
	}

	strPluginSearchPath=Info.CurDir;

	if (!strPluginSearchPath.empty())
		AddEndSlash(strPluginSearchPath);

	int RecurseLevel=0;
	ScanPluginTree(Dlg,ArcItem->hPlugin,ArcItem->Flags, RecurseLevel);

	if (SearchMode==FINDAREA_ROOT || SearchMode==FINDAREA_ALL || SearchMode==FINDAREA_ALL_BUTNETWORK || SearchMode==FINDAREA_INPATH)
	{
		CriticalSectionLock Lock(PluginCS);
		Global->CtrlObject->Plugins->SetDirectory(ArcItem->hPlugin,strSaveDir,OPM_FIND,&Info.UserData);
	}

	if (!Internal)
	{
		itd->SetPercent(0);
		StopEvent.Set();
	}
}

struct THREADPARAM
{
	bool PluginMode;
	Dialog* Dlg;
};

unsigned int FindFiles::ThreadRoutine(LPVOID Param)
{
	try
	{
		InitInFileSearch();
		THREADPARAM* tParam=static_cast<THREADPARAM*>(Param);
		tParam->PluginMode?DoPreparePluginList(tParam->Dlg, false):DoPrepareFileList(tParam->Dlg);
		ReleaseInFileSearch();
	}
	catch (SException& e)
	{
		if (xfilter(EXCEPT_KERNEL, e.GetInfo(), nullptr, 1) == EXCEPTION_EXECUTE_HANDLER)
			TerminateProcess(GetCurrentProcess(), 1);
		throw;
	}
	return 0;
}

bool FindFiles::FindFilesProcess()
{
	_ALGO(CleverSysLog clv(L"FindFiles::FindFilesProcess()"));
	// ���� ������������ ������ ��������, �� �� ����� ������ �������� �� ����
	string strTitle=MSG(MFindFileTitle);

	itd->Init();

	if (!strFindMask.empty())
	{
		strTitle+=L": ";
		strTitle+=strFindMask;

		if (UseFilter)
		{
			strTitle+=L" (";
			strTitle+=MSG(MFindUsingFilter);
			strTitle+=L")";
		}
	}
	else
	{
		if (UseFilter)
		{
			strTitle+=L" (";
			strTitle+=MSG(MFindUsingFilter);
			strTitle+=L")";
		}
	}

	LangString strSearchStr(MFindSearchingIn);

	if (!strFindStr.empty())
	{
		string strFStr=strFindStr;
		TruncStrFromEnd(strFStr,10);
		InsertQuote(strFStr);
		string strTemp=L" ";
		strTemp+=strFStr;
		strSearchStr << strTemp;
	}
	else
	{
		strSearchStr << L"";
	}

	int DlgWidth = ScrX + 1 - 2;
	int DlgHeight = ScrY + 1 - 2;
	FarDialogItem FindDlgData[]=
	{
		{DI_DOUBLEBOX,3,1,DlgWidth-4,DlgHeight-2,0,nullptr,nullptr,DIF_SHOWAMPERSAND,strTitle.c_str()},
		{DI_LISTBOX,4,2,DlgWidth-5,DlgHeight-7,0,nullptr,nullptr,DIF_LISTNOBOX|DIF_DISABLE,0},
		{DI_TEXT,-1,DlgHeight-6,0,DlgHeight-6,0,nullptr,nullptr,DIF_SEPARATOR2,L""},
		{DI_TEXT,5,DlgHeight-5,DlgWidth-(strFindStr.empty()?6:12),DlgHeight-5,0,nullptr,nullptr,DIF_SHOWAMPERSAND,strSearchStr.c_str()},
		{DI_TEXT,DlgWidth-9,DlgHeight-5,DlgWidth-6,DlgHeight-5,0,nullptr,nullptr,(strFindStr.empty()?DIF_HIDDEN:0),L""},
		{DI_TEXT,-1,DlgHeight-4,0,DlgHeight-4,0,nullptr,nullptr,DIF_SEPARATOR,L""},
		{DI_BUTTON,0,DlgHeight-3,0,DlgHeight-3,0,nullptr,nullptr,DIF_FOCUS|DIF_DEFAULTBUTTON|DIF_CENTERGROUP,MSG(MFindNewSearch)},
		{DI_BUTTON,0,DlgHeight-3,0,DlgHeight-3,0,nullptr,nullptr,DIF_CENTERGROUP|DIF_DISABLE,MSG(MFindGoTo)},
		{DI_BUTTON,0,DlgHeight-3,0,DlgHeight-3,0,nullptr,nullptr,DIF_CENTERGROUP|DIF_DISABLE,MSG(MFindView)},
		{DI_BUTTON,0,DlgHeight-3,0,DlgHeight-3,0,nullptr,nullptr,DIF_CENTERGROUP|DIF_DISABLE,MSG(MFindPanel)},
		{DI_BUTTON,0,DlgHeight-3,0,DlgHeight-3,0,nullptr,nullptr,DIF_CENTERGROUP,MSG(MFindStop)},
	};
	MakeDialogItemsEx(FindDlgData,FindDlg);
	ChangePriority ChPriority(THREAD_PRIORITY_NORMAL);

	if (PluginMode)
	{
		Panel *ActivePanel=Global->CtrlObject->Cp()->ActivePanel;
		HANDLE hPlugin=ActivePanel->GetPluginHandle();
		OpenPanelInfo Info;
		Global->CtrlObject->Plugins->GetOpenPanelInfo(hPlugin,&Info);
		itd->SetFindFileArcItem(&itd->AddArcListItem(Info.HostFile, hPlugin, Info.Flags, Info.CurDir));

		if (!itd->GetFindFileArcItem())
			return false;

		if (!(Info.Flags & OPIF_REALNAMES))
		{
			FindDlg[FD_BUTTON_PANEL].Type=DI_TEXT;
			FindDlg[FD_BUTTON_PANEL].strData.clear();
		}
	}

	AnySetFindList = std::any_of(CONST_RANGE(*Global->CtrlObject->Plugins, i)
	{
		return i->HasSetFindList();
	});

	if (!AnySetFindList)
	{
		FindDlg[FD_BUTTON_PANEL].Flags|=DIF_DISABLE;
	}

	Dialog Dlg(this, &FindFiles::FindDlgProc, nullptr, FindDlg,ARRAYSIZE(FindDlg));
//  pDlg->SetDynamicallyBorn();
	Dlg.SetHelp(L"FindFileResult");
	Dlg.SetPosition(-1, -1, DlgWidth, DlgHeight);
	Dlg.SetId(FindFileResultId);
	// ���� �� �������� ������, � �� ������������� ��������� �����������
	// ������ ��� ������ � ������ �������� �� �����������
	Dlg.InitDialog();
	Dlg.Show();

	strLastDirName.clear();

	THREADPARAM Param={PluginMode, &Dlg};
	Thread FindThread;
	if (FindThread.MemberStart(this, &FindFiles::ThreadRoutine, &Param))
	{
		TB=new TaskBar;
		wakeful W;
		Dlg.Process();
		FindThread.Wait();
		FindThread.Close();

		PauseEvent.Set();
		StopEvent.Reset();

		switch (Dlg.GetExitCode())
		{
			case FD_BUTTON_NEW:
			{
				return true;
			}

			case FD_BUTTON_PANEL:
			// ���������� ���������� �� ��������� ������
			{
				itd->Lock();
				size_t ListSize = itd->GetFindList().size();
				PluginPanelItem *PanelItems=new PluginPanelItem[ListSize];

				int ItemsNumber=0;

				std::for_each(RANGE(itd->GetFindList(), i)
				{
					if (!i.FindData.strFileName.empty() && i.Used)
					// ��������� ������, ���� ��� ������
					{
						// ��� �������� � ������������ ������� ������� ��� ����� �� ��� ������.
						// ������ ���� ������ ������ �����.
						bool IsArchive=false;
						if(i.Arc)
						{
							if(!(i.Arc->Flags&OPIF_REALNAMES))
							{
								IsArchive=true;
							}
						}
						// ��������� ������ ����� ��� ����� ������� ��� ����� ����� �������
						if (IsArchive || (Global->Opt->FindOpt.FindFolders && !SearchHex) ||
							    !(i.FindData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
						{
							if (IsArchive)
							{
								i.FindData.strFileName = i.Arc->strArcName;
							}
							PluginPanelItem *pi=&PanelItems[ItemsNumber++];
							ClearStruct(*pi);
							FindDataExToPluginPanelItem(&i.FindData, pi);

							if (IsArchive)
								pi->FileAttributes = 0;

							if (pi->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
							{
								DeleteEndSlash(pi->FileName);
							}
						}
					}
				});
				itd->Unlock();
				HANDLE hNewPlugin=Global->CtrlObject->Plugins->OpenFindListPlugin(PanelItems,ItemsNumber);

				if (hNewPlugin)
				{
					Panel *ActivePanel=Global->CtrlObject->Cp()->ActivePanel;
					Panel *NewPanel=Global->CtrlObject->Cp()->ChangePanel(ActivePanel,FILE_PANEL,TRUE,TRUE);
					NewPanel->SetPluginMode(hNewPlugin,L"",true);
					NewPanel->SetVisible(TRUE);
					NewPanel->Update(0);
					//if (FindExitItem)
					//NewPanel->GoToFile(FindExitItem->FindData.cFileName);
					NewPanel->Show();
				}

				for (int i = 0; i < ItemsNumber; i++)
					FreePluginPanelItem(&PanelItems[i]);

				delete[] PanelItems;
				break;
			}
			case FD_BUTTON_GOTO:
			case FD_LISTBOX:
			{
				string strFileName=FindExitItem->FindData.strFileName;
				Panel *FindPanel=Global->CtrlObject->Cp()->ActivePanel;

				if (FindExitItem->Arc)
				{
					if (!FindExitItem->Arc->hPlugin)
					{
						string strArcName = FindExitItem->Arc->strArcName;

						if (FindPanel->GetType()!=FILE_PANEL)
						{
							FindPanel=Global->CtrlObject->Cp()->ChangePanel(FindPanel,FILE_PANEL,TRUE,TRUE);
						}

						string strArcPath=strArcName;
						CutToSlash(strArcPath);
						FindPanel->SetCurDir(strArcPath,true);
						FindExitItem->Arc->hPlugin=((FileList *)FindPanel)->OpenFilePlugin(&strArcName, FALSE, OFP_SEARCH);
						if (FindExitItem->Arc->hPlugin==PANEL_STOP)
							FindExitItem->Arc->hPlugin = nullptr;
					}

					if (FindExitItem->Arc->hPlugin)
					{
						OpenPanelInfo Info;
						Global->CtrlObject->Plugins->GetOpenPanelInfo(FindExitItem->Arc->hPlugin,&Info);

						if (SearchMode==FINDAREA_ROOT ||
							    SearchMode==FINDAREA_ALL ||
							    SearchMode==FINDAREA_ALL_BUTNETWORK ||
							    SearchMode==FINDAREA_INPATH)
							Global->CtrlObject->Plugins->SetDirectory(FindExitItem->Arc->hPlugin,L"\\",0);

						SetPluginDirectory(strFileName, FindExitItem->Arc->hPlugin, true); // ??? ,FindItem.Data ???
					}
				}
				else
				{
					string strSetName;
					size_t Length=strFileName.size();

					if (!Length)
						break;

					if (Length>1 && IsSlash(strFileName.at(Length-1)) && strFileName.at(Length-2)!=L':')
						strFileName.resize(Length-1);

					if ((apiGetFileAttributes(strFileName)==INVALID_FILE_ATTRIBUTES) && (GetLastError() != ERROR_ACCESS_DENIED))
						break;

					const wchar_t *NamePtr = PointToName(strFileName);
					strSetName = NamePtr;

					if (Global->Opt->FindOpt.FindAlternateStreams)
					{
						size_t Pos = strSetName.find(L':');

						if (Pos != string::npos)
							strSetName.resize(Pos);
					}

					strFileName.resize(NamePtr-strFileName.c_str());
					Length=strFileName.size();

					if (Length>1 && IsSlash(strFileName.at(Length-1)) && strFileName.at(Length-2)!=L':')
						strFileName.resize(Length-1);

					if (strFileName.empty())
						break;

					if (FindPanel->GetType()!=FILE_PANEL &&
						    Global->CtrlObject->Cp()->GetAnotherPanel(FindPanel)->GetType()==FILE_PANEL)
						FindPanel=Global->CtrlObject->Cp()->GetAnotherPanel(FindPanel);

					if ((FindPanel->GetType()!=FILE_PANEL) || (FindPanel->GetMode()!=NORMAL_PANEL))
					// ������ ������ �� ������� ��������...
					{
						FindPanel=Global->CtrlObject->Cp()->ChangePanel(FindPanel,FILE_PANEL,TRUE,TRUE);
						FindPanel->SetVisible(TRUE);
						FindPanel->Update(0);
					}

					// ! �� ������ �������, ���� �� ��� � ��� ���������.
					// ��� ����� ���������� ����, ��� ��������� � ��������� ������ �� ������������.
					string strDirTmp = FindPanel->GetCurDir();
					Length=strDirTmp.size();

					if (Length>1 && IsSlash(strDirTmp.at(Length-1)) && strDirTmp.at(Length-2)!=L':')
						strDirTmp.resize(Length-1);

					if (StrCmpI(strFileName.c_str(), strDirTmp.c_str()))
						FindPanel->SetCurDir(strFileName,true);

					if (!strSetName.empty())
						FindPanel->GoToFile(strSetName);

					FindPanel->Show();
					FindPanel->SetFocus();
				}
				break;
			}
		}
	}
	return false;
}

FindFiles::FindFiles():
	TB(nullptr)
{

	PauseEvent.Open(true, true);
	StopEvent.Open(true, false);

	// BUGBUG
	FileMaskForFindFile = new filemasks;
	AnySetFindList = false;
	CmpCase = false;
	WholeWords = false;
	SearchInArchives = false;
	SearchHex = false;
	SearchMode = 0;
	UseFilter = false;
	CodePage=CP_DEFAULT;
	SearchInFirst=0;
	readBufferA = nullptr;
	readBuffer = nullptr;
	hexFindString = nullptr;
	hexFindStringSize = 0;
	findString = nullptr;
	findStringBuffer = nullptr;
	skipCharsTable = nullptr;
	favoriteCodePages = 0;
	InFileSearchInited = false;

	_ALGO(CleverSysLog clv(L"FindFiles::FindFiles()"));
	static string strLastFindMask=L"*.*", strLastFindStr;
	// ����������� ��������� � ����������� ����������
	static string strSearchFromRoot;
	static bool LastCmpCase=0,LastWholeWords=0,LastSearchInArchives=0,LastSearchHex=0;
	// �������� ������ �������
	Filter=new FileFilter(Global->CtrlObject->Cp()->ActivePanel,FFT_FINDFILE);
	CmpCase=LastCmpCase;
	WholeWords=LastWholeWords;
	SearchInArchives=LastSearchInArchives;
	SearchHex=LastSearchHex;
	SearchMode=Global->Opt->FindOpt.FileSearchMode;
	UseFilter=Global->Opt->FindOpt.UseFilter!=0;
	strFindMask = strLastFindMask;
	strFindStr = strLastFindStr;
	strSearchFromRoot = MSG(MSearchFromRootFolder);

	itd = new InterThreadData;

	do
	{
		FindExitItem = nullptr;
		FindFoldersChanged=false;
		SearchFromChanged=false;
		FindPositionChanged=false;
		Finalized=false;
		if(TB)
		{
			delete TB;
			TB=nullptr;
		}
		itd->ClearAllLists();
		Panel *ActivePanel=Global->CtrlObject->Cp()->ActivePanel;
		PluginMode=ActivePanel->GetMode()==PLUGIN_PANEL && ActivePanel->IsVisible();
		PrepareDriveNameStr(strSearchFromRoot);
		const wchar_t *MasksHistoryName=L"Masks",*TextHistoryName=L"SearchText";
		const wchar_t *HexMask=L"HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH";
		const wchar_t VSeparator[]={BoxSymbols[BS_T_H1V1],BoxSymbols[BS_V1],BoxSymbols[BS_V1],BoxSymbols[BS_V1],BoxSymbols[BS_B_H1V1],0};
		FarDialogItem FindAskDlgData[]=
		{
			{DI_DOUBLEBOX,3,1,74,18,0,nullptr,nullptr,0,MSG(MFindFileTitle)},
			{DI_TEXT,5,2,0,2,0,nullptr,nullptr,0,MSG(MFindFileMasks)},
			{DI_EDIT,5,3,72,3,0,MasksHistoryName,nullptr,DIF_FOCUS|DIF_HISTORY|DIF_USELASTHISTORY,L""},
			{DI_TEXT,-1,4,0,4,0,nullptr,nullptr,DIF_SEPARATOR,L""},
			{DI_TEXT,5,5,0,5,0,nullptr,nullptr,0,L""},
			{DI_EDIT,5,6,72,6,0,TextHistoryName,nullptr,DIF_HISTORY,L""},
			{DI_FIXEDIT,5,6,72,6,0,nullptr,HexMask,DIF_MASKEDIT,L""},
			{DI_TEXT,5,7,0,7,0,nullptr,nullptr,0,L""},
			{DI_COMBOBOX,5,8,72,8,0,nullptr,nullptr,DIF_DROPDOWNLIST|DIF_LISTNOAMPERSAND,L""},
			{DI_TEXT,-1,9,0,9,0,nullptr,nullptr,DIF_SEPARATOR,L""},
			{DI_CHECKBOX,5,10,0,10,0,nullptr,nullptr,0,MSG(MFindFileCase)},
			{DI_CHECKBOX,5,11,0,11,0,nullptr,nullptr,0,MSG(MFindFileWholeWords)},
			{DI_CHECKBOX,5,12,0,12,0,nullptr,nullptr,0,MSG(MSearchForHex)},
			{DI_CHECKBOX,40,10,0,10,0,nullptr,nullptr,0,MSG(MFindArchives)},
			{DI_CHECKBOX,40,11,0,11,0,nullptr,nullptr,0,MSG(MFindFolders)},
			{DI_CHECKBOX,40,12,0,12,0,nullptr,nullptr,0,MSG(MFindSymLinks)},
			{DI_TEXT,-1,13,0,13,0,nullptr,nullptr,DIF_SEPARATOR,L""},
			{DI_VTEXT,38,9,0,9,0,nullptr,nullptr,DIF_BOXCOLOR,VSeparator},
			{DI_TEXT,5,14,0,14,0,nullptr,nullptr,0,MSG(MSearchWhere)},
			{DI_COMBOBOX,5,15,36,15,0,nullptr,nullptr,DIF_DROPDOWNLIST|DIF_LISTNOAMPERSAND,L""},
			{DI_CHECKBOX,40,15,0,15,(int)(UseFilter?BSTATE_CHECKED:BSTATE_UNCHECKED),nullptr,nullptr,DIF_AUTOMATION,MSG(MFindUseFilter)},
			{DI_TEXT,-1,16,0,16,0,nullptr,nullptr,DIF_SEPARATOR,L""},
			{DI_BUTTON,0,17,0,17,0,nullptr,nullptr,DIF_DEFAULTBUTTON|DIF_CENTERGROUP,MSG(MFindFileFind)},
			{DI_BUTTON,0,17,0,17,0,nullptr,nullptr,DIF_CENTERGROUP,MSG(MFindFileDrive)},
			{DI_BUTTON,0,17,0,17,0,nullptr,nullptr,DIF_CENTERGROUP|DIF_AUTOMATION|(UseFilter?0:DIF_DISABLE),MSG(MFindFileSetFilter)},
			{DI_BUTTON,0,17,0,17,0,nullptr,nullptr,DIF_CENTERGROUP,MSG(MFindFileAdvanced)},
			{DI_BUTTON,0,17,0,17,0,nullptr,nullptr,DIF_CENTERGROUP,MSG(MCancel)},
		};
		MakeDialogItemsEx(FindAskDlgData,FindAskDlg);

		if (strFindStr.empty())
			FindAskDlg[FAD_CHECKBOX_DIRS].Selected=Global->Opt->FindOpt.FindFolders;

		FarListItem li[]=
		{
			{0,MSG(MSearchAllDisks)},
			{0,MSG(MSearchAllButNetwork)},
			{0,MSG(MSearchInPATH)},
			{0,strSearchFromRoot.c_str()},
			{0,MSG(MSearchFromCurrent)},
			{0,MSG(MSearchInCurrent)},
			{0,MSG(MSearchInSelected)},
		};
		li[FADC_ALLDISKS+SearchMode].Flags|=LIF_SELECTED;
		FarList l={sizeof(FarList),ARRAYSIZE(li),li};
		FindAskDlg[FAD_COMBOBOX_WHERE].ListItems=&l;

		if (PluginMode)
		{
			OpenPanelInfo Info;
			Global->CtrlObject->Plugins->GetOpenPanelInfo(ActivePanel->GetPluginHandle(),&Info);

			if (!(Info.Flags & OPIF_REALNAMES))
				FindAskDlg[FAD_CHECKBOX_ARC].Flags |= DIF_DISABLE;

			if (SearchMode == FINDAREA_ALL || SearchMode == FINDAREA_ALL_BUTNETWORK)
			{
				li[FADC_ALLDISKS].Flags=0;
				li[FADC_ALLBUTNET].Flags=0;
				li[FADC_ROOT].Flags|=LIF_SELECTED;
			}

			li[FADC_ALLDISKS].Flags|=LIF_GRAYED;
			li[FADC_ALLBUTNET].Flags|=LIF_GRAYED;
			FindAskDlg[FAD_CHECKBOX_LINKS].Selected=0;
			FindAskDlg[FAD_CHECKBOX_LINKS].Flags|=DIF_DISABLE;
		}
		else
			FindAskDlg[FAD_CHECKBOX_LINKS].Selected=Global->Opt->FindOpt.FindSymLinks;

		if (!(FindAskDlg[FAD_CHECKBOX_ARC].Flags & DIF_DISABLE))
			FindAskDlg[FAD_CHECKBOX_ARC].Selected=SearchInArchives;

		FindAskDlg[FAD_EDIT_MASK].strData = strFindMask;

		if (SearchHex)
			FindAskDlg[FAD_EDIT_HEX].strData = strFindStr;
		else
			FindAskDlg[FAD_EDIT_TEXT].strData = strFindStr;

		FindAskDlg[FAD_CHECKBOX_CASE].Selected=CmpCase;
		FindAskDlg[FAD_CHECKBOX_WHOLEWORDS].Selected=WholeWords;
		FindAskDlg[FAD_CHECKBOX_HEX].Selected=SearchHex;
		int ExitCode;
		Dialog Dlg(this, &FindFiles::MainDlgProc, nullptr, FindAskDlg,ARRAYSIZE(FindAskDlg));
		Dlg.SetAutomation(FAD_CHECKBOX_FILTER,FAD_BUTTON_FILTER,DIF_DISABLE,DIF_NONE,DIF_NONE,DIF_DISABLE);
		Dlg.SetHelp(L"FindFile");
		Dlg.SetId(FindFileId);
		Dlg.SetPosition(-1,-1,78,20);
		Dlg.Process();
		ExitCode=Dlg.GetExitCode();
		//������ �������� ������� ��� ������� ����� ����� ������ �� �������
		Filter->UpdateCurrentTime();

		if (ExitCode!=FAD_BUTTON_FIND)
		{
			return;
		}

		Global->Opt->FindCodePage = CodePage;
		CmpCase=FindAskDlg[FAD_CHECKBOX_CASE].Selected == BSTATE_CHECKED;
		WholeWords=FindAskDlg[FAD_CHECKBOX_WHOLEWORDS].Selected == BSTATE_CHECKED;
		SearchHex=FindAskDlg[FAD_CHECKBOX_HEX].Selected == BSTATE_CHECKED;
		SearchInArchives=FindAskDlg[FAD_CHECKBOX_ARC].Selected == BSTATE_CHECKED;

		if (FindFoldersChanged)
		{
			Global->Opt->FindOpt.FindFolders=(FindAskDlg[FAD_CHECKBOX_DIRS].Selected==BSTATE_CHECKED);
		}

		if (!PluginMode)
		{
			Global->Opt->FindOpt.FindSymLinks=(FindAskDlg[FAD_CHECKBOX_LINKS].Selected==BSTATE_CHECKED);
		}

		UseFilter=(FindAskDlg[FAD_CHECKBOX_FILTER].Selected==BSTATE_CHECKED);
		Global->Opt->FindOpt.UseFilter=UseFilter;
		strFindMask = !FindAskDlg[FAD_EDIT_MASK].strData.empty() ? FindAskDlg[FAD_EDIT_MASK].strData:L"*";

		if (SearchHex)
		{
			strFindStr = FindAskDlg[FAD_EDIT_HEX].strData;
			RemoveTrailingSpaces(strFindStr);
		}
		else
			strFindStr = FindAskDlg[FAD_EDIT_TEXT].strData;

		if (!strFindStr.empty())
		{
			Global->strGlobalSearchString = strFindStr;
			Global->GlobalSearchCase=CmpCase;
			Global->GlobalSearchWholeWords=WholeWords;
			Global->GlobalSearchHex=SearchHex;
		}

		switch (FindAskDlg[FAD_COMBOBOX_WHERE].ListPos)
		{
			case FADC_ALLDISKS:
				SearchMode=FINDAREA_ALL;
				break;
			case FADC_ALLBUTNET:
				SearchMode=FINDAREA_ALL_BUTNETWORK;
				break;
			case FADC_PATH:
				SearchMode=FINDAREA_INPATH;
				break;
			case FADC_ROOT:
				SearchMode=FINDAREA_ROOT;
				break;
			case FADC_FROMCURRENT:
				SearchMode=FINDAREA_FROM_CURRENT;
				break;
			case FADC_INCURRENT:
				SearchMode=FINDAREA_CURRENT_ONLY;
				break;
			case FADC_SELECTED:
				SearchMode=FINDAREA_SELECTED;
				break;
		}

		if (SearchFromChanged)
		{
			Global->Opt->FindOpt.FileSearchMode=SearchMode;
		}

		LastCmpCase=CmpCase;
		LastWholeWords=WholeWords;
		LastSearchHex=SearchHex;
		LastSearchInArchives=SearchInArchives;
		strLastFindMask = strFindMask;
		strLastFindStr = strFindStr;

		if (!strFindStr.empty())
			Editor::SetReplaceMode(false);
	}
	while (FindFilesProcess());

	Global->CtrlObject->Cp()->ActivePanel->SetTitle();
}

FindFiles::~FindFiles()
{
	itd->ClearAllLists();
	Global->ScrBuf->ResetShadow();

	delete itd;

	if (Filter)
	{
		delete Filter;
	}
	delete FileMaskForFindFile;

	if(TB)
	{
		delete TB;
	}
}
