/*
mkdir.cpp

�������� ��������
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

#include "mkdir.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "treelist.hpp"
#include "ctrlobj.hpp"
#include "message.hpp"
#include "config.hpp"
#include "dialog.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "dirmix.hpp"
#include "DlgGuid.hpp"
#include "flink.hpp"
#include "stddlg.hpp"
#include "language.hpp"

enum
{
	MKDIR_BORDER,
	MKDIR_TEXT,
	MKDIR_EDIT,
	MKDIR_SEPARATOR0,
	MKDIR_TEXT_LINKTYPE,
	MKDIR_COMBOBOX_LINKTYPE,
	MKDIR_TEXT_LINKPATH,
	MKDIR_EDIT_LINKPATH,
	MKDIR_CHECKBOX,
	MKDIR_SEPARATOR2,
	MKDIR_OK,
	MKDIR_CANCEL,
};

intptr_t MkDirDlgProc(Dialog* Dlg,intptr_t Msg,intptr_t Param1,void* Param2)
{
	switch (Msg)
	{
		case DN_LISTCHANGE:
			{
				if (Param1 == MKDIR_COMBOBOX_LINKTYPE)
				{
					Dlg->SendMessage( DM_ENABLE, MKDIR_EDIT_LINKPATH, ToPtr(Param2 != 0));
				}
			}
			break;
		case DN_CLOSE:
		{
			if (Param1==MKDIR_OK)
			{
				string strDirName=reinterpret_cast<LPCWSTR>(Dlg->SendMessage(DM_GETCONSTTEXTPTR,MKDIR_EDIT,0));
				Global->Opt->MultiMakeDir=(Dlg->SendMessage(DM_GETCHECK,MKDIR_CHECKBOX,0)==BSTATE_CHECKED);

				// ��� �� ������ �������� ���������� ��������, �������
				// ���������� � �������! ����� ������� �� ���������
				// ����� ������� � �������
				if (Global->Opt->MultiMakeDir && !wcspbrk(strDirName.c_str(),L";,\""))
				{
					QuoteSpaceOnly(strDirName);
				}

				// ����� ������� ������ ���� �������
				if (!Global->Opt->MultiMakeDir)
				{
					// ������ ��� ������ �������
					Unquote(strDirName);
					// ������� � �������, �.�. ����� ���� �����������
					InsertQuote(strDirName);
				}

				auto pDirList=reinterpret_cast<std::list<string>*>(Dlg->SendMessage(DM_GETDLGDATA,0,0));
				*pDirList = StringToList(strDirName, STLF_UNIQUE);
				if (pDirList->empty())
				{
					Message(MSG_WARNING,1,MSG(MWarning),MSG(MIncorrectDirList),MSG(MOk));
					return FALSE;
				}
			}
		}
		break;
	default:
		break;
	}

	return Dlg->DefProc(Msg,Param1,Param2);
}

void ShellMakeDir(Panel *SrcPanel)
{
	FarList ComboList={sizeof(FarList)};
	FarListItem LinkTypeItems[3]={};
	ComboList.ItemsNumber=ARRAYSIZE(LinkTypeItems);
	ComboList.Items=LinkTypeItems;
	ComboList.Items[0].Text=MSG(MMakeFolderLinkNone);
	ComboList.Items[1].Text=MSG(MMakeFolderLinkJunction);
	ComboList.Items[2].Text=MSG(MMakeFolderLinkSymlink);
	ComboList.Items[0].Flags|=LIF_SELECTED;

	FarDialogItem MkDirDlgData[]=
	{
		{DI_DOUBLEBOX,3,1,72,10,0,nullptr,nullptr,0,MSG(MMakeFolderTitle)},
		{DI_TEXT,     5,2, 0,2,0,nullptr,nullptr,0,MSG(MCreateFolder)},
		{DI_EDIT,     5,3,70,3,0,L"NewFolder",nullptr,DIF_FOCUS|DIF_EDITEXPAND|DIF_HISTORY|DIF_USELASTHISTORY|DIF_EDITPATH,L""},
		{DI_TEXT,    -1,4, 0,4,0,nullptr,nullptr,DIF_SEPARATOR,L""},
		{DI_TEXT,     5,5, 0,5,0,nullptr,nullptr,0,MSG(MMakeFolderLinkType)},
		{DI_COMBOBOX,20,5,70,5,0,nullptr,nullptr,DIF_DROPDOWNLIST|DIF_LISTNOAMPERSAND|DIF_LISTWRAPMODE,L""},
		{DI_TEXT,     5,6, 0,6,0,nullptr,nullptr,0,MSG(MMakeFolderLinkTarget)},
		{DI_EDIT,    20,6,70,6,0,L"NewFolderLinkTarget",nullptr,DIF_DISABLE|DIF_EDITEXPAND|DIF_HISTORY|DIF_USELASTHISTORY|DIF_EDITPATH,L""},
		{DI_CHECKBOX, 5,7, 0,7,Global->Opt->MultiMakeDir,nullptr,nullptr,0,MSG(MMultiMakeDir)},
		{DI_TEXT,    -1,8, 0,8,0,nullptr,nullptr,DIF_SEPARATOR,L""},
		{DI_BUTTON,   0,9, 0,9,0,nullptr,nullptr,DIF_DEFAULTBUTTON|DIF_CENTERGROUP,MSG(MOk)},
		{DI_BUTTON,   0,9, 0,9,0,nullptr,nullptr,DIF_CENTERGROUP,MSG(MCancel)},
	};
	MakeDialogItemsEx(MkDirDlgData,MkDirDlg);
	MkDirDlg[MKDIR_COMBOBOX_LINKTYPE].ListItems=&ComboList;
	std::list<string> DirList;
	Dialog Dlg(MkDirDlg,ARRAYSIZE(MkDirDlg),MkDirDlgProc,&DirList);
	Dlg.SetPosition(-1,-1,76,12);
	Dlg.SetHelp(L"MakeFolder");
	Dlg.SetId(MakeFolderId);
	Dlg.Process();

	if (Dlg.GetExitCode()==MKDIR_OK)
	{
		string strDirName=MkDirDlg[MKDIR_EDIT].strData;
		string strOriginalDirName;
		bool SkipAll = false;
		FOR_CONST_RANGE(DirList, i)
		{
			strDirName = *i;
			strOriginalDirName = strDirName;

			//Unquote(DirName);
			if (Global->Opt->CreateUppercaseFolders && !IsCaseMixed(strDirName))
				strDirName.Upper();

			DeleteEndSlash(strDirName,true);
			wchar_t* lpwszDirName = strDirName.GetBuffer();
			bool bSuccess = false;

			wchar_t *ChPtr = lpwszDirName;
			size_t DirOffset = 0;
			if(ParsePath(strDirName, &DirOffset) == PATH_UNKNOWN)
			{
				ChPtr = lpwszDirName + DirOffset;
			}

			for (; ; ChPtr++)
			{
				if (IsSlash(*ChPtr) || !*ChPtr)
				{
					WCHAR Ch=0;
					if(*ChPtr)
					{
						Ch = ChPtr[1];
						ChPtr[1] = 0;
					}
					if (*lpwszDirName)
					{
						string _strDirName(lpwszDirName);
						if (apiGetFileAttributes(_strDirName) == INVALID_FILE_ATTRIBUTES || !*ChPtr) // skip all intermediate dirs, but not last.
						{
							while(!(bSuccess=(apiCreateDirectory(_strDirName, nullptr)!=FALSE)) && !SkipAll)
							{
								int Ret = OperationFailed(strOriginalDirName, MError, MSG(MCannotCreateFolder));
								if(Ret == 1) // skip
								{
									break;
								}
								else if(Ret == 2)
								{
									SkipAll = true;
									break;
								}
								else if (Ret < 0 || Ret == 3) // cancel
								{
									return;
								}
							}
							if(bSuccess)
							{
								TreeList::AddTreeName(_strDirName);
							}
						}
					}
					if(*ChPtr)
					{
						ChPtr[1] = Ch;
					}
					else
					{
						break;
					}
				}
			}

			strDirName.ReleaseBuffer();

			if (bSuccess)
			{
				if(MkDirDlg[MKDIR_COMBOBOX_LINKTYPE].ListPos)
				{
					string strTarget=MkDirDlg[MKDIR_EDIT_LINKPATH].strData;
					Unquote(strTarget);
					while(!CreateReparsePoint(strTarget, strDirName, MkDirDlg[MKDIR_COMBOBOX_LINKTYPE].ListPos==1?RP_JUNCTION:RP_SYMLINKDIR) && !SkipAll)
					{
						int Ret = OperationFailed(strDirName, MError, MSG(MCopyCannotCreateLink));
						if(Ret == 1) // skip
						{
							break;
						}
						else if(Ret == 2)
						{
							SkipAll = true;
							break;
						}
						else if (Ret < 0 || Ret == 3) // cancel
						{
							return;
						}
					}
				}

				TreeList::AddTreeName(strDirName);
			}
		}

		SrcPanel->Update(UPDATE_KEEP_SELECTION);

		if (!strDirName.empty())
		{
			size_t pos;

			if (FindSlash(pos,strDirName))
				strDirName.resize(pos);

			if (!SrcPanel->GoToFile(strDirName) && strDirName.back() == L'.')
			{
				strDirName.pop_back();
				SrcPanel->GoToFile(strDirName);
			}
		}

		SrcPanel->Redraw();
		Panel *AnotherPanel=Global->CtrlObject->Cp()->GetAnotherPanel(SrcPanel);
		int AnotherType=AnotherPanel->GetType();

		if (AnotherPanel->NeedUpdatePanel(SrcPanel) || AnotherType==QVIEW_PANEL)
		{
			AnotherPanel->Update(UPDATE_KEEP_SELECTION|UPDATE_SECONDARY);
			AnotherPanel->Redraw();
		}
	}
}
