﻿/*
stddlg.cpp

Куча разных стандартных диалогов
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

#include "stddlg.hpp"
#include "dialog.hpp"
#include "ctrlobj.hpp"
#include "farexcpt.hpp"
#include "strmix.hpp"
#include "macro.hpp"
#include "imports.hpp"
#include "message.hpp"
#include "language.hpp"
#include "DlgGuid.hpp"
#include "datetime.hpp"

int GetSearchReplaceString(
	bool IsReplaceMode,
	const wchar_t *Title,
	const wchar_t *SubTitle,
	string& SearchStr,
	string& ReplaceStr,
	const wchar_t *TextHistoryName,
	const wchar_t *ReplaceHistoryName,
	bool* pCase,
	bool* pWholeWords,
	bool* pReverse,
	bool* pRegexp,
	bool* pPreserveStyle,
	const wchar_t *HelpTopic,
	bool HideAll,
	const GUID* Id,
	const std::function<string(bool)>& Picker)
{
	int Result = 0;

	if (!TextHistoryName)
		TextHistoryName = L"SearchText";

	if (!ReplaceHistoryName)
		ReplaceHistoryName = L"ReplaceText";

	if (!Title)
		Title=MSG(IsReplaceMode?MEditReplaceTitle:MEditSearchTitle);

	if (!SubTitle)
		SubTitle=MSG(MEditSearchFor);


	bool Case=pCase?*pCase:false;
	bool WholeWords=pWholeWords?*pWholeWords:false;
	bool Reverse=pReverse?*pReverse:false;
	bool Regexp=pRegexp?*pRegexp:false;
	bool PreserveStyle=pPreserveStyle?*pPreserveStyle:false;

	const auto DlgWidth = 76;
	const auto WordLabel = MSG(MEditSearchPickWord);
	const auto SelectionLabel = MSG(MEditSearchPickSelection);
	const auto WordButtonSize = wcslen(WordLabel) + 4;
	const auto SelectionButtonSize = wcslen(SelectionLabel) + 4;
	const auto SelectionButtonX2 = static_cast<intptr_t>(DlgWidth - 4 - 1);
	const auto SelectionButtonX1 = static_cast<intptr_t>(SelectionButtonX2 - SelectionButtonSize);
	const auto WordButtonX2 = static_cast<intptr_t>(SelectionButtonX1 - 1);
	const auto WordButtonX1 = static_cast<intptr_t>(WordButtonX2 - WordButtonSize);

	// BUGBUG, awful copy-paste
	if (IsReplaceMode)
	{
		/*
		  0         1         2         3         4         5         6         7
		  0123456789012345678901234567890123456789012345678901234567890123456789012345
		00
		01   +----------------------------- Replace ------------------------------+
		02   | Search for                                                         |
		03   |                                                                   |
		04   | Replace with                                                       |
		05   |                                                                   |
		06   +--------------------------------------------------------------------+
		07   | [ ] Case sensitive                 [ ] Regular expressions         |
		08   | [ ] Whole words                    [ ] Preserve style              |
		09   | [ ] Reverse search                                                 |
		10   +--------------------------------------------------------------------+
		11   |                      [ Replace ]  [ Cancel ]                       |
		12   +--------------------------------------------------------------------+
		13
		*/

		enum item_id
		{
			dlg_border,
			dlg_label_search,
			dlg_edit_search,
			dlg_label_replace,
			dlg_edit_replace,
			dlg_button_word,
			dlg_button_selection,
			dlg_separator_1,
			dlg_checkbox_case,
			dlg_checkbox_words,
			dlg_checkbox_reverse,
			dlg_checkbox_regex,
			dlg_checkbox_style,
			dlg_separator_2,
			dlg_button_replace,
			dlg_button_cancel,
		};

		FarDialogItem ReplaceDlgData[]=
		{
			{DI_DOUBLEBOX,3,1,DlgWidth-4,12,0,nullptr,nullptr,0,Title},
			{DI_TEXT,5,2,0,2,0,nullptr,nullptr,0,SubTitle},
			{DI_EDIT,5,3,70,3,0,TextHistoryName,nullptr,DIF_FOCUS|DIF_USELASTHISTORY|(*TextHistoryName?DIF_HISTORY:0),SearchStr.data()},
			{DI_TEXT,5,4,0,4,0,nullptr,nullptr,0,MSG(MEditReplaceWith)},
			{DI_EDIT,5,5,70,5,0,ReplaceHistoryName,nullptr,(*ReplaceHistoryName?DIF_HISTORY:0)/*|DIF_USELASTHISTORY*/,ReplaceStr.data()},
			{DI_BUTTON, WordButtonX1, 2, WordButtonX2, 2, 0, nullptr, nullptr, 0, WordLabel},
			{DI_BUTTON, SelectionButtonX1, 2, SelectionButtonX2, 2, 0, nullptr, nullptr, 0, SelectionLabel},
			{DI_TEXT,-1,6,0,6,0,nullptr,nullptr,DIF_SEPARATOR,L""},
			{DI_CHECKBOX,5,7,0,7,Case,nullptr,nullptr,0,MSG(MEditSearchCase)},
			{DI_CHECKBOX,5,8,0,8,WholeWords,nullptr,nullptr,0,MSG(MEditSearchWholeWords)},
			{DI_CHECKBOX,5,9,0,9,Reverse,nullptr,nullptr,0,MSG(MEditSearchReverse)},
			{DI_CHECKBOX,40,7,0,7,Regexp,nullptr,nullptr,0,MSG(MEditSearchRegexp)},
			{DI_CHECKBOX,40,8,0,8,PreserveStyle,nullptr,nullptr,0,MSG(MEditSearchPreserveStyle)},
			{DI_TEXT,-1,10,0,10,0,nullptr,nullptr,DIF_SEPARATOR,L""},
			{DI_BUTTON,0,11,0,11,0,nullptr,nullptr,DIF_DEFAULTBUTTON|DIF_CENTERGROUP,MSG(MEditReplaceReplace)},
			{DI_BUTTON,0,11,0,11,0,nullptr,nullptr,DIF_CENTERGROUP,MSG(MEditSearchCancel)},
		};
		auto ReplaceDlg = MakeDialogItemsEx(ReplaceDlgData);

		if (!Picker)
		{
			ReplaceDlg[dlg_button_word].Flags |= DIF_HIDDEN;
			ReplaceDlg[dlg_button_selection].Flags |= DIF_HIDDEN;
		}

		if (!pCase)
			ReplaceDlg[dlg_checkbox_case].Flags |= DIF_DISABLE; // DIF_HIDDEN ??
		if (!pWholeWords)
			ReplaceDlg[dlg_checkbox_words].Flags |= DIF_DISABLE; // DIF_HIDDEN ??
		if (!pReverse)
			ReplaceDlg[dlg_checkbox_reverse].Flags |= DIF_DISABLE; // DIF_HIDDEN ??
		if (!pRegexp)
			ReplaceDlg[dlg_checkbox_regex].Flags |= DIF_DISABLE; // DIF_HIDDEN ??
		if (!pPreserveStyle)
			ReplaceDlg[dlg_checkbox_style].Flags |= DIF_DISABLE; // DIF_HIDDEN ??

		// explicit variables to make buggy VC10 happy
		const auto word_id = dlg_button_word, selection_id = dlg_button_selection, edit_id = dlg_edit_search;
		const auto Handler = [&](Dialog* Dlg, intptr_t Msg, intptr_t Param1, void* Param2) -> intptr_t
		{
			if (Msg == DN_CLOSE && Picker && (Param1 == word_id || Param1 == selection_id))
			{
				Dlg->SendMessage(DM_SETTEXTPTR, edit_id, UNSAFE_CSTR(Picker(Param1 == selection_id)));
				return FALSE;
			}
			return Dlg->DefProc(Msg, Param1, Param2);
		};

		const auto Dlg = Dialog::create(ReplaceDlg, Handler);
		Dlg->SetPosition(-1, -1, DlgWidth, 14);

		if (HelpTopic && *HelpTopic)
			Dlg->SetHelp(HelpTopic);

		if(Id) Dlg->SetId(*Id);

		Dlg->Process();

		if(Dlg->GetExitCode() == dlg_button_replace)
		{
			Result = 1;
			SearchStr = ReplaceDlg[dlg_edit_search].strData;
			ReplaceStr = ReplaceDlg[dlg_edit_replace].strData;
			Case=ReplaceDlg[dlg_checkbox_case].Selected == BSTATE_CHECKED;
			WholeWords=ReplaceDlg[dlg_checkbox_words].Selected == BSTATE_CHECKED;
			Reverse=ReplaceDlg[dlg_checkbox_reverse].Selected == BSTATE_CHECKED;
			Regexp=ReplaceDlg[dlg_checkbox_regex].Selected == BSTATE_CHECKED;
			PreserveStyle=ReplaceDlg[dlg_checkbox_style].Selected == BSTATE_CHECKED;
		}
	}
	else
	{
		/*
		  0         1         2         3         4         5         6         7
		  0123456789012345678901234567890123456789012345678901234567890123456789012345
		00
		01   +------------------------------ Search ------------------------------+
		02   | Search for                                                         |
		03   |                                                                   |
		04   +--------------------------------------------------------------------+
		05   | [ ] Case sensitive                 [ ] Regular expressions         |
		06   | [ ] Whole words                    [ ] Reverse search              |
		07   +--------------------------------------------------------------------+
		08   |                   { Search } [ All ] [ Cancel ]                    |
		09   +--------------------------------------------------------------------+
		*/

		enum item_id
		{
			dlg_border,
			dlg_label_search,
			dlg_edit_search,
			dlg_button_word,
			dlg_button_selection,
			dlg_separator_1,
			dlg_checkbox_case,
			dlg_checkbox_words,
			dlg_checkbox_regex,
			dlg_checkbox_reverse,
			dlg_separator_2,
			dlg_button_search,
			dlg_button_all,
			dlg_button_cancel,
		};

		FarDialogItem SearchDlgData[]=
		{
			{DI_DOUBLEBOX,3,1,DlgWidth-4,9,0,nullptr,nullptr,0,Title},
			{DI_TEXT,5,2,0,2,0,nullptr,nullptr,0,SubTitle},
			{DI_EDIT,5,3,70,3,0,TextHistoryName,nullptr,DIF_FOCUS|DIF_USELASTHISTORY|(*TextHistoryName?DIF_HISTORY:0),SearchStr.data()},
			{DI_BUTTON, WordButtonX1, 2, WordButtonX2, 2, 0, nullptr, nullptr, 0, WordLabel},
			{DI_BUTTON, SelectionButtonX1, 2, SelectionButtonX2, 2, 0, nullptr, nullptr, 0, SelectionLabel},
			{DI_TEXT,-1,4,0,4,0,nullptr,nullptr,DIF_SEPARATOR,L""},
			{DI_CHECKBOX,5,5,0,5,Case,nullptr,nullptr,0,MSG(MEditSearchCase)},
			{DI_CHECKBOX,5,6,0,6,WholeWords,nullptr,nullptr,0,MSG(MEditSearchWholeWords)},
			{DI_CHECKBOX,40,5,0,5,Regexp,nullptr,nullptr,0,MSG(MEditSearchRegexp)},
			{DI_CHECKBOX,40,6,0,6,Reverse,nullptr,nullptr,0,MSG(MEditSearchReverse)},
			{DI_TEXT,-1,7,0,7,0,nullptr,nullptr,DIF_SEPARATOR,L""},
			{DI_BUTTON,0,8,0,8,0,nullptr,nullptr,DIF_DEFAULTBUTTON|DIF_CENTERGROUP,MSG(MEditSearchSearch)},
			{DI_BUTTON,0,8,0,8,0,nullptr,nullptr,DIF_CENTERGROUP,MSG(MEditSearchAll)},
			{DI_BUTTON,0,8,0,8,0,nullptr,nullptr,DIF_CENTERGROUP,MSG(MEditSearchCancel)},
		};
		auto SearchDlg = MakeDialogItemsEx(SearchDlgData);

		if (!Picker)
		{
			SearchDlg[dlg_button_word].Flags |= DIF_HIDDEN;
			SearchDlg[dlg_button_selection].Flags |= DIF_HIDDEN;
		}

		if (!pCase)
			SearchDlg[dlg_checkbox_case].Flags |= DIF_DISABLE; // DIF_HIDDEN ??
		if (!pWholeWords)
			SearchDlg[dlg_checkbox_words].Flags |= DIF_DISABLE; // DIF_HIDDEN ??
		if (!pRegexp)
			SearchDlg[dlg_checkbox_regex].Flags |= DIF_DISABLE; // DIF_HIDDEN ??
		if (!pReverse)
			SearchDlg[dlg_checkbox_reverse].Flags |= DIF_DISABLE; // DIF_HIDDEN ??

		if (HideAll)
			SearchDlg[dlg_button_all].Flags |= DIF_HIDDEN;

		// explicit variables to make buggy VC10 happy
		const auto word_id = dlg_button_word, selection_id = dlg_button_selection, edit_id = dlg_edit_search;
		const auto Handler = [&](Dialog* Dlg, intptr_t Msg, intptr_t Param1, void* Param2) -> intptr_t
		{
			if (Msg == DN_CLOSE && Picker && (Param1 == word_id || Param1 == selection_id))
			{
				Dlg->SendMessage(DM_SETTEXTPTR, edit_id, UNSAFE_CSTR(Picker(Param1 == selection_id)));
				return FALSE;
			}
			return Dlg->DefProc(Msg, Param1, Param2);
		};

		const auto Dlg = Dialog::create(SearchDlg, Handler);
		Dlg->SetPosition(-1, -1, DlgWidth, 11);

		if (HelpTopic && *HelpTopic)
			Dlg->SetHelp(HelpTopic);

		if(Id) Dlg->SetId(*Id);

		Dlg->Process();
		int ExitCode = Dlg->GetExitCode();

		if (ExitCode == dlg_button_search || ExitCode == dlg_button_all)
		{
			Result = ExitCode == dlg_button_search? 1 : 2;
			SearchStr = SearchDlg[dlg_edit_search].strData;
			ReplaceStr.clear();
			Case=SearchDlg[dlg_checkbox_case].Selected == BSTATE_CHECKED;
			WholeWords=SearchDlg[dlg_checkbox_words].Selected == BSTATE_CHECKED;
			Regexp=SearchDlg[dlg_checkbox_regex].Selected == BSTATE_CHECKED;
			Reverse=SearchDlg[dlg_checkbox_reverse].Selected == BSTATE_CHECKED;
		}
	}

	if (pCase)
		*pCase=Case;
	if (pWholeWords)
		*pWholeWords=WholeWords;
	if (pReverse)
		*pReverse=Reverse;
	if (pRegexp)
		*pRegexp=Regexp;
	if (pPreserveStyle)
		*pPreserveStyle=PreserveStyle;

	return Result;
}

int GetString(
	const wchar_t *Title,
	const wchar_t *Prompt,
	const wchar_t *HistoryName,
	const wchar_t *SrcText,
	string &strDestText,
	const wchar_t *HelpTopic,
	DWORD Flags,
	int *CheckBoxValue,
	const wchar_t *CheckBoxText,
	Plugin* PluginNumber,
	const GUID* Id
)
{
	int Substract=5; // дополнительная величина :-)
	int ExitCode;
	bool addCheckBox=Flags&FIB_CHECKBOX && CheckBoxValue && CheckBoxText;
	int offset=addCheckBox?2:0;
	FarDialogItem StrDlgData[]=
	{
		{DI_DOUBLEBOX, 3, 1, 72, 4, 0, nullptr, nullptr, 0,                                L""},
		{DI_TEXT,      5, 2,  0, 2, 0, nullptr, nullptr, DIF_SHOWAMPERSAND,                L""},
		{DI_EDIT,      5, 3, 70, 3, 0, nullptr, nullptr, DIF_FOCUS|DIF_DEFAULTBUTTON,      L""},
		{DI_TEXT,     -1, 4,  0, 4, 0, nullptr, nullptr, DIF_SEPARATOR,                    L""},
		{DI_CHECKBOX,  5, 5,  0, 5, 0, nullptr, nullptr, 0,                                L""},
		{DI_TEXT,     -1, 6,  0, 6, 0, nullptr, nullptr, DIF_SEPARATOR,                    L""},
		{DI_BUTTON,    0, 7,  0, 7, 0, nullptr, nullptr, DIF_CENTERGROUP,                  L""},
		{DI_BUTTON,    0, 7,  0, 7, 0, nullptr, nullptr, DIF_CENTERGROUP,                  L""},
	};
	auto StrDlg = MakeDialogItemsEx(StrDlgData);

	if (addCheckBox)
	{
		Substract-=2;
		StrDlg[0].Y2+=2;
		StrDlg[4].Selected = *CheckBoxValue != 0;
		StrDlg[4].strData = CheckBoxText;
	}

	if (Flags&FIB_BUTTONS)
	{
		Substract-=3;
		StrDlg[0].Y2+=2;
		StrDlg[2].Flags&=~DIF_DEFAULTBUTTON;
		StrDlg[5+offset].Y1=StrDlg[4+offset].Y1=5+offset;
		StrDlg[4+offset].Type=StrDlg[5+offset].Type=DI_BUTTON;
		StrDlg[4+offset].Flags=StrDlg[5+offset].Flags=DIF_CENTERGROUP;
		StrDlg[4+offset].Flags|=DIF_DEFAULTBUTTON;
		StrDlg[4+offset].strData = MSG(MOk);
		StrDlg[5+offset].strData = MSG(MCancel);
	}

	if (Flags&FIB_EXPANDENV)
	{
		StrDlg[2].Flags|=DIF_EDITEXPAND;
	}

	if (Flags&FIB_EDITPATH)
	{
		StrDlg[2].Flags|=DIF_EDITPATH;
	}

	if (Flags&FIB_EDITPATHEXEC)
	{
		StrDlg[2].Flags|=DIF_EDITPATHEXEC;
	}

	if (HistoryName)
	{
		StrDlg[2].strHistory=HistoryName;
		StrDlg[2].Flags|=DIF_HISTORY|(Flags&FIB_NOUSELASTHISTORY?0:DIF_USELASTHISTORY);
	}

	if (Flags&FIB_PASSWORD)
		StrDlg[2].Type=DI_PSWEDIT;

	if (Title)
		StrDlg[0].strData = Title;

	if (Prompt)
	{
		StrDlg[1].strData = Prompt;
		TruncStrFromEnd(StrDlg[1].strData, 66);

		if (Flags&FIB_NOAMPERSAND)
			StrDlg[1].Flags&=~DIF_SHOWAMPERSAND;
	}

	if (SrcText)
		StrDlg[2].strData = SrcText;

	{
		const auto Dlg = Dialog::create(make_range(StrDlg.data(), StrDlg.data() + StrDlg.size() - Substract));
		Dlg->SetPosition(-1,-1,76,offset+((Flags&FIB_BUTTONS)?8:6));
		if(Id) Dlg->SetId(*Id);

		if (HelpTopic)
			Dlg->SetHelp(HelpTopic);

		Dlg->SetPluginOwner(PluginNumber);

		Dlg->Process();

		ExitCode=Dlg->GetExitCode();

		if (ExitCode == -2 && Global->CtrlObject->Macro.IsExecuting() != MACROSTATE_NOMACRO)
			Global->CtrlObject->Macro.SendDropProcess();
	}

	if (ExitCode == 2 || ExitCode == 4 || (addCheckBox && ExitCode == 6))
	{
		if (!(Flags&FIB_ENABLEEMPTY) && StrDlg[2].strData.empty())
			return FALSE;

		strDestText = StrDlg[2].strData;

		if (addCheckBox)
			*CheckBoxValue=StrDlg[4].Selected;

		return TRUE;
	}

	return FALSE;
}

/*
  Стандартный диалог ввода пароля.
  Умеет сам запоминать последнего юзвера и пароль.

  Name      - сюда будет помещен юзвер (max 256 символов!!!)
  Password  - сюда будет помещен пароль (max 256 символов!!!)
  Title     - заголовок диалога (может быть nullptr)
  HelpTopic - тема помощи (может быть nullptr)
  Flags     - флаги (GNP_*)
*/
int GetNameAndPassword(const string& Title, string &strUserName, string &strPassword,const wchar_t *HelpTopic,DWORD Flags)
{
	static string strLastName, strLastPassword;
	int ExitCode;
	/*
	  0         1         2         3         4         5         6         7
	  0123456789012345678901234567890123456789012345678901234567890123456789012345
	|0                                                                             |
	|1   +------------------------------- Title -------------------------------+   |
	|2   | User name                                                           |   |
	|3   | *******************************************************************|   |
	|4   | User password                                                       |   |
	|5   | ******************************************************************* |   |
	|6   +---------------------------------------------------------------------+   |
	|7   |                         [ Ok ]   [ Cancel ]                         |   |
	|8   +---------------------------------------------------------------------+   |
	|9                                                                             |
	*/
	FarDialogItem PassDlgData[]=
	{
		{DI_DOUBLEBOX,  3, 1,72, 8,0,nullptr,nullptr,0,NullToEmpty(Title.data())},
		{DI_TEXT,       5, 2, 0, 2,0,nullptr,nullptr,0,MSG(MNetUserName)},
		{DI_EDIT,       5, 3,70, 3,0,L"NetworkUser",nullptr,DIF_FOCUS|DIF_USELASTHISTORY|DIF_HISTORY,(Flags&GNP_USELAST)?strLastName.data():strUserName.data()},
		{DI_TEXT,       5, 4, 0, 4,0,nullptr,nullptr,0,MSG(MNetUserPassword)},
		{DI_PSWEDIT,    5, 5,70, 5,0,nullptr,nullptr,0,(Flags&GNP_USELAST)?strLastPassword.data():strPassword.data()},
		{DI_TEXT,      -1, 6, 0, 6,0,nullptr,nullptr,DIF_SEPARATOR,L""},
		{DI_BUTTON,     0, 7, 0, 7,0,nullptr,nullptr,DIF_DEFAULTBUTTON|DIF_CENTERGROUP,MSG(MOk)},
		{DI_BUTTON,     0, 7, 0, 7,0,nullptr,nullptr,DIF_CENTERGROUP,MSG(MCancel)},
	};
	auto PassDlg = MakeDialogItemsEx(PassDlgData);

	{
		const auto Dlg = Dialog::create(PassDlg);
		Dlg->SetPosition(-1,-1,76,10);
		Dlg->SetId(GetNameAndPasswordId);

		if (HelpTopic)
			Dlg->SetHelp(HelpTopic);

		Dlg->Process();
		ExitCode=Dlg->GetExitCode();
	}

	if (ExitCode!=6)
		return FALSE;

	// запоминаем всегда.
	strUserName = PassDlg[2].strData;
	strLastName = strUserName;
	strPassword = PassDlg[4].strData;
	strLastPassword = strPassword;
	return TRUE;
}

IFileIsInUse* CreateIFileIsInUse(const string& File)
{
	IFileIsInUse *pfiu = nullptr;
	IRunningObjectTable *prot;
	if (SUCCEEDED(GetRunningObjectTable(0, &prot)))
	{
		IMoniker *pmkFile;
		if (SUCCEEDED(CreateFileMoniker(File.data(), &pmkFile)))
		{
			IEnumMoniker *penumMk;
			if (SUCCEEDED(prot->EnumRunning(&penumMk)))
			{
				HRESULT hr = E_FAIL;
				ULONG celt;
				IMoniker *pmk;
				while (FAILED(hr) && (penumMk->Next(1, &pmk, &celt) == S_OK))
				{
					DWORD dwType;
					if (SUCCEEDED(pmk->IsSystemMoniker(&dwType)) && dwType == MKSYS_FILEMONIKER)
					{
						IMoniker *pmkPrefix;
						if (SUCCEEDED(pmkFile->CommonPrefixWith(pmk, &pmkPrefix)))
						{
							if (pmkFile->IsEqual(pmkPrefix) == S_OK)
							{
								IUnknown *punk;
								if (prot->GetObject(pmk, &punk) == S_OK)
								{
									hr = punk->QueryInterface(
#if COMPILER == C_GCC
										IID_IFileIsInUse, IID_PPV_ARGS_Helper(&pfiu)
#else
										IID_PPV_ARGS(&pfiu)
#endif
										);
									punk->Release();
								}
							}
							pmkPrefix->Release();
						}
					}
					pmk->Release();
				}
				penumMk->Release();
			}
			pmkFile->Release();
		}
		prot->Release();
	}
	return pfiu;
}

int OperationFailed(const string& Object, LNGID Title, const string& Description, bool AllowSkip)
{
	std::vector<string> Msg;
	IFileIsInUse *pfiu = nullptr;
	LNGID Reason = MObjectLockedReasonOpened;
	bool SwitchBtn = false, CloseBtn = false;
	DWORD Error = Global->CaughtError();
	if(Error == ERROR_ACCESS_DENIED ||
		Error == ERROR_SHARING_VIOLATION ||
		Error == ERROR_LOCK_VIOLATION ||
		Error == ERROR_DRIVE_LOCKED)
	{
		string FullName;
		ConvertNameToFull(Object, FullName);
		pfiu = CreateIFileIsInUse(FullName);
		if (pfiu)
		{
			FILE_USAGE_TYPE UsageType = FUT_GENERIC;
			pfiu->GetUsage(&UsageType);
			switch(UsageType)
			{
			case FUT_PLAYING:
				Reason = MObjectLockedReasonPlayed;
				break;
			case FUT_EDITING:
				Reason = MObjectLockedReasonEdited;
				break;
			case FUT_GENERIC:
				Reason = MObjectLockedReasonOpened;
				break;
			}
			DWORD Capabilities = 0;
			pfiu->GetCapabilities(&Capabilities);
			if(Capabilities&OF_CAP_CANSWITCHTO)
			{
				SwitchBtn = true;
			}
			if(Capabilities&OF_CAP_CANCLOSE)
			{
				CloseBtn = true;
			}
			LPWSTR AppName = nullptr;
			if(SUCCEEDED(pfiu->GetAppName(&AppName)))
			{
				Msg.emplace_back(AppName);
			}
		}
		else
		{
			DWORD dwSession;
			WCHAR szSessionKey[CCH_RM_SESSION_KEY+1] = {};
			if (Imports().RmStartSession(&dwSession, 0, szSessionKey) == ERROR_SUCCESS)
			{
				PCWSTR pszFile = FullName.data();
				if (Imports().RmRegisterResources(dwSession, 1, &pszFile, 0, nullptr, 0, nullptr) == ERROR_SUCCESS)
				{
					DWORD dwReason;
					DWORD RmGetListResult;
					UINT nProcInfoNeeded;
					UINT nProcInfo = 1;
					std::vector<RM_PROCESS_INFO> rgpi(nProcInfo);
					while((RmGetListResult=Imports().RmGetList(dwSession, &nProcInfoNeeded, &nProcInfo, rgpi.data(), &dwReason)) == ERROR_MORE_DATA)
					{
						nProcInfo = nProcInfoNeeded;
						rgpi.resize(nProcInfo);
					}
					if(RmGetListResult ==ERROR_SUCCESS)
					{
						FOR(const auto& i, rgpi)
						{
							string tmp = i.strAppName;
							if (*i.strServiceShortName)
							{
								tmp.append(L" [").append(i.strServiceShortName).append(L"]");
							}
							tmp += L" (PID: " + std::to_wstring(i.Process.dwProcessId);
							if (const auto Process = os::handle(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, i.Process.dwProcessId)))
							{
								FILETIME ftCreate, ftExit, ftKernel, ftUser;
								if (GetProcessTimes(Process.native_handle(), &ftCreate, &ftExit, &ftKernel, &ftUser) && i.Process.ProcessStartTime == ftCreate)
								{
									string Name;
									if (os::GetModuleFileNameEx(Process.native_handle(), nullptr, Name))
									{
										tmp += L", " + Name;
									}
								}
							}
							tmp += L")";
							Msg.emplace_back(tmp);
						}
					}
				}
				Imports().RmEndSession(dwSession);
			}
		}
	}

	auto Msgs = make_vector<string>(Description, QuoteOuterSpace(string(Object)));
	if(!Msg.empty())
	{
		Msgs.emplace_back(string_format(MObjectLockedReason, MSG(Reason)));
		Msgs.insert(Msgs.end(), ALL_CONST_RANGE(Msg));
	}

	std::vector<string> Buttons;
	Buttons.reserve(4);
	if(SwitchBtn)
	{
		Buttons.emplace_back(MSG(MObjectLockedSwitchTo));
	}
	Buttons.emplace_back(MSG(CloseBtn? MObjectLockedClose : MDeleteRetry));
	if(AllowSkip)
	{
		Buttons.emplace_back(MSG(MDeleteSkip));
		Buttons.emplace_back(MSG(MDeleteFileSkipAll));
	}
	Buttons.emplace_back(MSG(MDeleteCancel));

	int Result = -1;
	for(;;)
	{
		Result = Message(MSG_WARNING|MSG_ERRORTYPE, MSG(Title), Msgs, Buttons);

		if(SwitchBtn)
		{
			if(Result == 0)
			{
				HWND Wnd = nullptr;
				if (pfiu && SUCCEEDED(pfiu->GetSwitchToHWND(&Wnd)))
				{
					SetForegroundWindow(Wnd);
					if (IsIconic(Wnd))
						ShowWindow(Wnd, SW_RESTORE);
				}
				continue;
			}
			else if(Result > 0)
			{
				--Result;
			}
		}

		if(CloseBtn && Result == 0)
		{
			// close & retry
			if (pfiu)
			{
				pfiu->CloseFile();
			}
		}
		break;
	}

	if (pfiu)
	{
		pfiu->Release();
	}

	return Result;
}

static string GetReErrorString(int code)
{
	// TODO: localization
	switch (code)
	{
	case errNone:
		return L"No errors";
	case errNotCompiled:
		return L"RegExp wasn't even tried to compile";
	case errSyntax:
		return L"Expression contains a syntax error";
	case errBrackets:
		return L"Unbalanced brackets";
	case errMaxDepth:
		return L"Max recursive brackets level reached";
	case errOptions:
		return L"Invalid options combination";
	case errInvalidBackRef:
		return L"Reference to nonexistent bracket";
	case errInvalidEscape:
		return L"Invalid escape char";
	case errInvalidRange:
		return L"Invalid range value";
	case errInvalidQuantifiersCombination:
		return L"Quantifier applied to invalid object. f.e. lookahead assertion";
	case errNotEnoughMatches:
		return L"Size of match array isn't large enough";
	case errNoStorageForNB:
		return L"Attempt to match RegExp with Named Brackets but no storage class provided";
	case errReferenceToUndefinedNamedBracket:
		return L"Reference to undefined named bracket";
	case errVariableLengthLookBehind:
		return L"Only fixed length look behind assertions are supported";
	default:
		return L"Unknown error";
	}
};

void ReCompileErrorMessage(const RegExp& re, const string& str)
{
	Message(MSG_WARNING | MSG_LEFTALIGN, MSG(MError),
		make_vector<string>(GetReErrorString(re.LastError()), str, string(re.ErrorPosition(), L' ') + L'^'),
		make_vector<string>(MSG(MOk)));
}

void ReMatchErrorMessage(const RegExp& re)
{
	if (re.LastError() != errNone)
	{

		Message(MSG_WARNING | MSG_LEFTALIGN, MSG(MError),
			make_vector<string>(GetReErrorString(re.LastError())),
			make_vector<string>(MSG(MOk)));
	}
}
