﻿/*
setattr.cpp

Установка атрибутов файлов
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

#include "flink.hpp"
#include "dialog.hpp"
#include "chgprior.hpp"
#include "scantree.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "ctrlobj.hpp"
#include "constitle.hpp"
#include "TPreRedrawFunc.hpp"
#include "TaskBar.hpp"
#include "keyboard.hpp"
#include "message.hpp"
#include "config.hpp"
#include "datetime.hpp"
#include "fileattr.hpp"
#include "setattr.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "network.hpp"
#include "fileowner.hpp"
#include "wakeful.hpp"
#include "DlgGuid.hpp"
#include "interf.hpp"
#include "plugins.hpp"
#include "imports.hpp"
#include "lang.hpp"
#include "locale.hpp"
#include "local.hpp"

enum SETATTRDLG
{
	SA_DOUBLEBOX,
	SA_TEXT_LABEL,
	SA_TEXT_NAME,
	SA_COMBO_HARDLINK,
	SA_TEXT_SYMLINK,
	SA_EDIT_SYMLINK,
	SA_COMBO_SYMLINK,
	SA_SEPARATOR1,
	SA_TEXT_OWNER,
	SA_EDIT_OWNER,
	SA_SEPARATOR2,
	SA_ATTR_FIRST,
	SA_CHECKBOX_RO=SA_ATTR_FIRST,
	SA_CHECKBOX_ARCHIVE,
	SA_CHECKBOX_HIDDEN,
	SA_CHECKBOX_SYSTEM,
	SA_CHECKBOX_COMPRESSED,
	SA_CHECKBOX_ENCRYPTED,
	SA_CHECKBOX_NOTINDEXED,
	SA_CHECKBOX_SPARSE,
	SA_CHECKBOX_TEMP,
	SA_CHECKBOX_OFFLINE,
	SA_CHECKBOX_REPARSEPOINT,
	SA_CHECKBOX_VIRTUAL,
	SA_CHECKBOX_INTEGRITY_STREAM,
	SA_CHECKBOX_NO_SCRUB_DATA,
	SA_ATTR_LAST = SA_CHECKBOX_NO_SCRUB_DATA,
	SA_SEPARATOR3,
	SA_TEXT_TITLEDATE,
	SA_TEXT_TITLETIME,
	SA_TEXT_LASTWRITE,
	SA_EDIT_WDATE,
	SA_EDIT_WTIME,
	SA_TEXT_CREATION,
	SA_EDIT_CDATE,
	SA_EDIT_CTIME,
	SA_TEXT_LASTACCESS,
	SA_EDIT_ADATE,
	SA_EDIT_ATIME,
	SA_TEXT_CHANGE,
	SA_EDIT_XDATE,
	SA_EDIT_XTIME,
	SA_BUTTON_ORIGINAL,
	SA_BUTTON_CURRENT,
	SA_BUTTON_BLANK,
	SA_SEPARATOR4,
	SA_CHECKBOX_SUBFOLDERS,
	SA_SEPARATOR5,
	SA_BUTTON_SET,
	SA_BUTTON_SYSTEMDLG,
	SA_BUTTON_CANCEL,
};

enum DIALOGMODE
{
	MODE_FILE,
	MODE_FOLDER,
	MODE_MULTIPLE,
};

struct SetAttrDlgParam
{
	bool Plugin;
	DWORD FileSystemFlags;
	DIALOGMODE DialogMode;
	string strSelName;
	string strOwner;
	bool OwnerChanged;
	// значения CheckBox`ов на момент старта диалога
	struct cb_data
	{
		FARDIALOGITEMFLAGS Flags;
		int Value;
		bool Changed;
	}
	cb[SA_ATTR_LAST - SA_ATTR_FIRST + 1];

	FARCHECKEDSTATE OSubfoldersState;
};

enum
{
	DM_SETATTR = DM_USER+1,
};

intptr_t SetAttrDlgProc(Dialog* Dlg,intptr_t Msg,intptr_t Param1,void* Param2)
{
	const auto DlgParam = reinterpret_cast<SetAttrDlgParam*>(Dlg->SendMessage(DM_GETDLGDATA, 0, nullptr));

	switch (Msg)
	{
		case DN_BTNCLICK:

			if ((Param1>=SA_ATTR_FIRST&&Param1<=SA_ATTR_LAST)||Param1==SA_CHECKBOX_SUBFOLDERS)
			{
				if(Param1!=SA_CHECKBOX_SUBFOLDERS)
				{
					DlgParam->cb[Param1 - SA_ATTR_FIRST].Value = static_cast<int>(reinterpret_cast<intptr_t>(Param2));
					DlgParam->cb[Param1 - SA_ATTR_FIRST].Changed = true;
				}
				int FocusPos = static_cast<int>(Dlg->SendMessage(DM_GETFOCUS, 0, nullptr));
				const auto CompressState = static_cast<FARCHECKEDSTATE>(Dlg->SendMessage(DM_GETCHECK, SA_CHECKBOX_COMPRESSED, nullptr));
				const auto EncryptState = static_cast<FARCHECKEDSTATE>(Dlg->SendMessage(DM_GETCHECK, SA_CHECKBOX_ENCRYPTED, nullptr));
				const auto SubfoldersState = static_cast<FARCHECKEDSTATE>(Dlg->SendMessage(DM_GETCHECK, SA_CHECKBOX_SUBFOLDERS, nullptr));

				if (DlgParam->DialogMode==MODE_FILE)
				{
					if (((DlgParam->FileSystemFlags & (FILE_FILE_COMPRESSION|FILE_SUPPORTS_ENCRYPTION))==
					        (FILE_FILE_COMPRESSION|FILE_SUPPORTS_ENCRYPTION)) &&
					        (FocusPos == SA_CHECKBOX_COMPRESSED || FocusPos == SA_CHECKBOX_ENCRYPTED))
					{
						if (FocusPos == SA_CHECKBOX_COMPRESSED && /*CompressState &&*/ EncryptState)
							Dlg->SendMessage(DM_SETCHECK,SA_CHECKBOX_ENCRYPTED,ToPtr(BSTATE_UNCHECKED));

						if (FocusPos == SA_CHECKBOX_ENCRYPTED && /*EncryptState &&*/ CompressState)
							Dlg->SendMessage(DM_SETCHECK,SA_CHECKBOX_COMPRESSED,ToPtr(BSTATE_UNCHECKED));
					}
				}
				// =1|2 Multi
				else
				{
					// отработаем взаимоисключения
					if (((DlgParam->FileSystemFlags & (FILE_FILE_COMPRESSION|FILE_SUPPORTS_ENCRYPTION))==
					        (FILE_FILE_COMPRESSION|FILE_SUPPORTS_ENCRYPTION)) &&
					        (FocusPos == SA_CHECKBOX_COMPRESSED || FocusPos == SA_CHECKBOX_ENCRYPTED))
					{
						if (FocusPos == SA_CHECKBOX_COMPRESSED)
						{
							if (CompressState == BSTATE_CHECKED && EncryptState)
								Dlg->SendMessage(DM_SETCHECK,SA_CHECKBOX_ENCRYPTED,ToPtr(BSTATE_UNCHECKED));
							else if (CompressState == BSTATE_3STATE)
								Dlg->SendMessage(DM_SETCHECK,SA_CHECKBOX_ENCRYPTED,ToPtr(BSTATE_3STATE));
						}
						else if (FocusPos == SA_CHECKBOX_ENCRYPTED)
						{
							if (EncryptState == BSTATE_CHECKED && CompressState)
								Dlg->SendMessage(DM_SETCHECK,SA_CHECKBOX_COMPRESSED,ToPtr(BSTATE_UNCHECKED));
							else if (EncryptState == BSTATE_3STATE)
								Dlg->SendMessage(DM_SETCHECK,SA_CHECKBOX_COMPRESSED,ToPtr(BSTATE_3STATE));
						}

						// еще одна проверка
						if (reinterpret_cast<intptr_t>(Param2)==BSTATE_CHECKED)
						{
							if (FocusPos == SA_CHECKBOX_COMPRESSED && EncryptState)
							{
								Dlg->SendMessage(DM_SETCHECK,SA_CHECKBOX_ENCRYPTED,ToPtr(BSTATE_UNCHECKED));
							}

							if (FocusPos == SA_CHECKBOX_ENCRYPTED && CompressState)
							{
								Dlg->SendMessage(DM_SETCHECK,SA_CHECKBOX_COMPRESSED,ToPtr(BSTATE_UNCHECKED));
							}
						}
					}

					// если снимаем атрибуты для SubFolders
					// этот кусок всегда работает если есть хотя бы одна папка
					// иначе SA_CHECKBOX_SUBFOLDERS недоступен и всегда снят.
					if (FocusPos == SA_CHECKBOX_SUBFOLDERS)
					{
						if (DlgParam->DialogMode==MODE_FOLDER) // каталог однозначно!
						{
							if (DlgParam->OSubfoldersState != SubfoldersState) // Состояние изменилось?
							{
								const auto Owner = reinterpret_cast<LPCWSTR>(Dlg->SendMessage(DM_GETCONSTTEXTPTR, SA_EDIT_OWNER, nullptr));
								if(*Owner)
								{
									if(!DlgParam->OwnerChanged)
									{
										DlgParam->OwnerChanged=StrCmpI(Owner,DlgParam->strOwner.data())!=0;
									}
									DlgParam->strOwner=Owner;
								}
								// установили?
								if (SubfoldersState)
								{
									for (int i=SA_ATTR_FIRST; i<=SA_ATTR_LAST; i++)
									{
										Dlg->SendMessage(DM_SET3STATE,i,ToPtr(TRUE));
										if (!DlgParam->cb[i - SA_ATTR_FIRST].Changed)
										{
											Dlg->SendMessage(DM_SETCHECK,i,ToPtr(BSTATE_3STATE));
										}
									}
									if(!DlgParam->OwnerChanged)
									{
										Dlg->SendMessage(DM_SETTEXTPTR,SA_EDIT_OWNER,nullptr);
									}
								}
								// сняли?
								else
								{
									for (int i=SA_ATTR_FIRST; i<=SA_ATTR_LAST; i++)
									{
										Dlg->SendMessage(DM_SET3STATE, i, ToPtr(FALSE));
										Dlg->SendMessage(DM_SETCHECK, i, ToPtr(DlgParam->cb[i - SA_ATTR_FIRST].Value));
									}
									if(!DlgParam->OwnerChanged)
									{
										Dlg->SendMessage(DM_SETTEXTPTR,SA_EDIT_OWNER, UNSAFE_CSTR(DlgParam->strOwner));
									}
								}


								os::FAR_FIND_DATA FindData;
								if (os::GetFindDataEx(DlgParam->strSelName, FindData))
								{
									const std::pair<SETATTRDLG, PFILETIME> Items[] =
									{
										{SA_TEXT_LASTWRITE, &FindData.ftLastWriteTime},
										{SA_TEXT_CREATION, &FindData.ftCreationTime},
										{SA_TEXT_LASTACCESS, &FindData.ftLastAccessTime},
										{SA_TEXT_CHANGE, &FindData.ftChangeTime},
									};

									std::for_each(CONST_RANGE(Items, i)
									{
										Dlg->SendMessage(DM_SETATTR, i.first, SubfoldersState? nullptr : i.second);
									});
								}
							}
						}
						// много объектов
						else
						{
							// Состояние изменилось?
							if (DlgParam->OSubfoldersState!=SubfoldersState)
							{
								const auto Owner = reinterpret_cast<LPCWSTR>(Dlg->SendMessage(DM_GETCONSTTEXTPTR, SA_EDIT_OWNER, nullptr));
								if(*Owner)
								{
									if(!DlgParam->OwnerChanged)
									{
										DlgParam->OwnerChanged=StrCmpI(Owner,DlgParam->strOwner.data())!=0;
									}
									DlgParam->strOwner=Owner;
								}
								// установили?
								if (SubfoldersState)
								{
									for (int i=SA_ATTR_FIRST; i<= SA_ATTR_LAST; i++)
									{
										if (!DlgParam->cb[i - SA_ATTR_FIRST].Changed)
										{
											Dlg->SendMessage(DM_SET3STATE,i,ToPtr(TRUE));
											Dlg->SendMessage(DM_SETCHECK,i,ToPtr(BSTATE_3STATE));
										}
									}
									if(!DlgParam->OwnerChanged)
									{
										Dlg->SendMessage(DM_SETTEXTPTR,SA_EDIT_OWNER,nullptr);
									}
								}
								// сняли?
								else
								{
									for (int i=SA_ATTR_FIRST; i<= SA_ATTR_LAST; i++)
									{
										Dlg->SendMessage(DM_SET3STATE, i, ToPtr((DlgParam->cb[i - SA_ATTR_FIRST].Flags & DIF_3STATE) != 0));
										Dlg->SendMessage(DM_SETCHECK, i, ToPtr(DlgParam->cb[i - SA_ATTR_FIRST].Value));
									}
									if(!DlgParam->OwnerChanged)
									{
										Dlg->SendMessage(DM_SETTEXTPTR,SA_EDIT_OWNER, UNSAFE_CSTR(DlgParam->strOwner));
									}
								}
							}
						}

						DlgParam->OSubfoldersState=SubfoldersState;
					}
				}

				// only remove, not set
				if (Param1 == SA_CHECKBOX_REPARSEPOINT && reinterpret_cast<intptr_t>(Param2) == BSTATE_CHECKED)
				{
					return FALSE;
				}
				return TRUE;
			}
			// Set Original? / Set All? / Clear All?
			else if (Param1 == SA_BUTTON_ORIGINAL)
			{
				os::FAR_FIND_DATA FindData;

				if (os::GetFindDataEx(DlgParam->strSelName, FindData))
				{
					Dlg->SendMessage(DM_SETATTR,SA_TEXT_LASTWRITE,&FindData.ftLastWriteTime);
					Dlg->SendMessage(DM_SETATTR,SA_TEXT_CREATION,&FindData.ftCreationTime);
					Dlg->SendMessage(DM_SETATTR,SA_TEXT_LASTACCESS,&FindData.ftLastAccessTime);
					Dlg->SendMessage(DM_SETATTR,SA_TEXT_CHANGE,&FindData.ftChangeTime);
				}

				Dlg->SendMessage(DM_SETFOCUS, SA_EDIT_WDATE, nullptr);
				return TRUE;
			}
			else if (Param1 == SA_BUTTON_CURRENT || Param1 == SA_BUTTON_BLANK)
			{
				void* Value = nullptr;
				FILETIME CurrentTime;
				if(Param1 == SA_BUTTON_CURRENT)
				{
					CurrentTime = get_utc_time_as_filetime();
					Value = &CurrentTime;
				}
				Dlg->SendMessage( DM_SETATTR, SA_TEXT_LASTWRITE, Value);
				Dlg->SendMessage( DM_SETATTR, SA_TEXT_CREATION, Value);
				Dlg->SendMessage( DM_SETATTR, SA_TEXT_LASTACCESS, Value);
				Dlg->SendMessage( DM_SETATTR, SA_TEXT_CHANGE, Value);
				Dlg->SendMessage(DM_SETFOCUS, SA_EDIT_WDATE, nullptr);
				return TRUE;
			}

			break;
		//BUGBUG: DefDlgProc вызывается дважды, второй раз Param1 может быть другим.
		case DN_CONTROLINPUT:
		{
			const INPUT_RECORD* record=(const INPUT_RECORD *)Param2;
			if (record->EventType==MOUSE_EVENT)
			{
				//_SVS(SysLog(L"Msg=DN_MOUSECLICK Param1=%d Param2=%d",Param1,Param2));
				if (Param1>=SA_TEXT_LASTWRITE && Param1<=SA_EDIT_XTIME)
				{
					if (record->Event.MouseEvent.dwEventFlags==DOUBLE_CLICK)
					{
						// Дадим Менеджеру диалогов "попотеть"
						Dlg->DefProc(Msg,Param1,Param2);
						Dlg->SendMessage(DM_SETATTR,Param1,ToPtr(-1));
					}

					if (Param1 == SA_TEXT_LASTWRITE || Param1 == SA_TEXT_CREATION || Param1 == SA_TEXT_LASTACCESS || Param1 == SA_TEXT_CHANGE)
					{
						Param1++;
					}

					Dlg->SendMessage(DM_SETFOCUS, Param1, nullptr);
				}
			}
		}
		break;
		case DN_EDITCHANGE:
		{
			switch (Param1)
			{
				case SA_COMBO_HARDLINK:
				case SA_COMBO_SYMLINK:
				{
					lng m = (Param1 == SA_COMBO_HARDLINK? lng::MSetAttrHardLinks : lng::MSetAttrDfsTargets);
					FarListInfo li={sizeof(FarListInfo)};
					Dlg->SendMessage(DM_LISTINFO,Param1,&li);
					Dlg->SendMessage(DM_SETTEXTPTR,Param1, UNSAFE_CSTR(concat(msg(m), L" (", str(li.ItemsNumber), L')')));
				}
				break;
			default:
				break;
			}

			break;
		}

		case DN_GOTFOCUS:
			{
				if(Param1 == SA_EDIT_WDATE || Param1 == SA_EDIT_CDATE || Param1 == SA_EDIT_ADATE || Param1 == SA_EDIT_XDATE)
				{
					if(locale::GetDateFormat() == 2)
					{
						if (reinterpret_cast<const wchar_t*>(Dlg->SendMessage(DM_GETCONSTTEXTPTR, Param1, nullptr))[0] == L' ')
						{
							COORD Pos;
							Dlg->SendMessage( DM_GETCURSORPOS, Param1, &Pos);
							if(Pos.X ==0)
							{
								Pos.X=1;
								Dlg->SendMessage( DM_SETCURSORPOS, Param1, &Pos);
							}
						}
					}
				}
			}
			break;

		case DM_SETATTR:
		{
			string strDate,strTime;

			if (Param2) // Set?
			{
				FILETIME ft;

				if (reinterpret_cast<intptr_t>(Param2)==-1)
				{
					ft = get_utc_time_as_filetime();
				}
				else
				{
					ft=*reinterpret_cast<PFILETIME>(Param2);
				}

				ConvertDate(ft,strDate,strTime,12,FALSE,FALSE,2);
			}

			// Глянем на место, где был клик
			int Set1=-1;
			int Set2=Param1;

			switch (Param1)
			{
				case SA_TEXT_LASTWRITE:
					Set1=SA_EDIT_WDATE;
					Set2=SA_EDIT_WTIME;
					break;
				case SA_TEXT_CREATION:
					Set1=SA_EDIT_CDATE;
					Set2=SA_EDIT_CTIME;
					break;
				case SA_TEXT_LASTACCESS:
					Set1=SA_EDIT_ADATE;
					Set2=SA_EDIT_ATIME;
					break;
				case SA_TEXT_CHANGE:
					Set1=SA_EDIT_XDATE;
					Set2=SA_EDIT_XTIME;
					break;
				case SA_EDIT_WDATE:
				case SA_EDIT_CDATE:
				case SA_EDIT_ADATE:
				case SA_EDIT_XDATE:
					Set1=Param1;
					Set2=-1;
					break;
				default:
					break;
			}

			if (Set1!=-1)
			{
				Dlg->SendMessage(DM_SETTEXTPTR,Set1, UNSAFE_CSTR(strDate));
			}

			if (Set2!=-1)
			{
				Dlg->SendMessage(DM_SETTEXTPTR,Set2, UNSAFE_CSTR(strTime));
			}

			return TRUE;
		}
	default:
		break;
	}

	return Dlg->DefProc(Msg,Param1,Param2);
}

static void PR_ShellSetFileAttributesMsg();

struct AttrPreRedrawItem : public PreRedrawItem
{
	AttrPreRedrawItem() : PreRedrawItem(PR_ShellSetFileAttributesMsg){}

	string Name;
};

void ShellSetFileAttributesMsg(const string& Name)
{
	static int Width=54;
	int WidthTemp;

	if (!Name.empty())
		WidthTemp=std::max(static_cast<int>(Name.size()), 54);
	else
		Width=WidthTemp=54;

	WidthTemp=std::min(WidthTemp, ScrX/2);
	Width=std::max(Width,WidthTemp);
	string strOutFileName=Name;
	TruncPathStr(strOutFileName,Width);
	strOutFileName = fit_to_center(strOutFileName, Width + 4);
	Message(0,0,msg(lng::MSetAttrTitle),msg(lng::MSetAttrSetting),strOutFileName.data());
	if (!PreRedrawStack().empty())
	{
		const auto item = dynamic_cast<AttrPreRedrawItem*>(PreRedrawStack().top());
		assert(item);
		if (item)
		{
			item->Name = Name;
		}
	}
}

bool ReadFileTime(int Type,const string& Name,FILETIME& FileTime,const string& OSrcDate,const string& OSrcTime)
{
	os::FAR_FIND_DATA ffd;
	if (!os::GetFindDataEx(Name, ffd))
		return false;

	FILETIME* Times[] =
	{
		&ffd.ftLastWriteTime,
		&ffd.ftCreationTime,
		&ffd.ftLastAccessTime,
		&ffd.ftChangeTime
	};

	auto& OriginalFileTime = *Times[Type];
	SYSTEMTIME ost;
	if (!Utc2Local(OriginalFileTime, ost))
		return false;

	WORD DateN[3]{};
	GetFileDateAndTime(OSrcDate, DateN, std::size(DateN), locale::GetDateSeparator());
	WORD TimeN[4]{};
	GetFileDateAndTime(OSrcTime, TimeN, std::size(TimeN), locale::GetTimeSeparator());

	SYSTEMTIME st{};

	enum indicies { i_day, i_month, i_year };
	std::array<indicies, 3> Indicies;

	switch (locale::GetDateFormat())
	{
	case 0:  Indicies = { i_month, i_day, i_year }; break;
	case 1:  Indicies = { i_day, i_month, i_year }; break;
	default: Indicies = { i_year, i_month, i_day }; break;
	}

	st.wDay   = DateN[Indicies[0]] != static_cast<WORD>(-1)? DateN[Indicies[0]] : ost.wDay;
	st.wMonth = DateN[Indicies[1]] != static_cast<WORD>(-1)? DateN[Indicies[1]] : ost.wMonth;
	st.wYear  = DateN[Indicies[2]] != static_cast<WORD>(-1)? DateN[Indicies[2]] : ost.wYear;

	st.wHour         = TimeN[0] != static_cast<WORD>(-1)? TimeN[0] : ost.wHour;
	st.wMinute       = TimeN[1] != static_cast<WORD>(-1)? TimeN[1] : ost.wMinute;
	st.wSecond       = TimeN[2] != static_cast<WORD>(-1)? TimeN[2] : ost.wSecond;
	st.wMilliseconds = TimeN[3] != static_cast<WORD>(-1)? TimeN[3] : ost.wMilliseconds;

	if (st.wYear < 100)
	{
		st.wYear = static_cast<WORD>(ConvertYearToFull(st.wYear));
	}

	if (!Local2Utc(st, FileTime))
		return false;

	return FileTime != OriginalFileTime;
}

void PR_ShellSetFileAttributesMsg()
{
	if (PreRedrawStack().empty())
		return;

	const auto item = dynamic_cast<const AttrPreRedrawItem*>(PreRedrawStack().top());
	assert(item);
	if (item)
	{
		ShellSetFileAttributesMsg(item->Name);
	}
}

bool ShellSetFileAttributes(Panel *SrcPanel, const string* Object)
{
	SCOPED_ACTION(ChangePriority)(THREAD_PRIORITY_NORMAL);
	short DlgX=74,DlgY=25;
	FarDialogItem AttrDlgData[]=
	{
		{DI_DOUBLEBOX,3,1,DlgX-4,DlgY-2,0,nullptr,nullptr,0,msg(lng::MSetAttrTitle)},
		{DI_TEXT,-1,2,0,2,0,nullptr,nullptr,0,msg(lng::MSetAttrFor)},
		{DI_TEXT,-1,3,0,3,0,nullptr,nullptr,DIF_SHOWAMPERSAND,L""},
		{DI_COMBOBOX,5,3,DlgX-6,3,0,nullptr,nullptr,DIF_SHOWAMPERSAND|DIF_DROPDOWNLIST|DIF_LISTWRAPMODE|DIF_HIDDEN,L""},
		{DI_TEXT,5,3,17,3,0,nullptr,nullptr,DIF_HIDDEN,L""},
		{DI_EDIT,18,3,DlgX-6,3,0,nullptr,nullptr,DIF_HIDDEN|DIF_EDITPATH,L""},
		{DI_COMBOBOX,18,3,DlgX-6,3,0,nullptr,nullptr,DIF_SHOWAMPERSAND|DIF_DROPDOWNLIST|DIF_LISTWRAPMODE|DIF_HIDDEN,L""},
		{DI_TEXT,-1,4,0,4,0,nullptr,nullptr,DIF_SEPARATOR,L""},
		{DI_TEXT,5,5,17,5,0,nullptr,nullptr,0,msg(lng::MSetAttrOwner)},
		{DI_EDIT,18,5,DlgX-6,5,0,nullptr,nullptr,0,L""},
		{DI_TEXT,-1,6,0,6,0,nullptr,nullptr,DIF_SEPARATOR,L""},

		{DI_CHECKBOX,5, 7,0,7,0,nullptr,nullptr,DIF_FOCUS|DIF_3STATE,msg(lng::MSetAttrRO)},
		{DI_CHECKBOX,5, 8,0,8,0,nullptr,nullptr,DIF_3STATE,msg(lng::MSetAttrArchive)},
		{DI_CHECKBOX,5, 9,0,9,0,nullptr,nullptr,DIF_3STATE,msg(lng::MSetAttrHidden)},
		{DI_CHECKBOX,5,10,0,10,0,nullptr,nullptr,DIF_3STATE,msg(lng::MSetAttrSystem)},
		{DI_CHECKBOX,5,11,0,11,0,nullptr,nullptr,DIF_3STATE,msg(lng::MSetAttrCompressed)},
		{DI_CHECKBOX,5,12,0,12,0,nullptr,nullptr,DIF_3STATE,msg(lng::MSetAttrEncrypted)},
		{DI_CHECKBOX,5,13,0,13,0,nullptr,nullptr,DIF_3STATE,msg(lng::MSetAttrNotIndexed)},

		{DI_CHECKBOX,DlgX/2,7,0,7,0,nullptr,nullptr,DIF_3STATE,msg(lng::MSetAttrSparse)},
		{DI_CHECKBOX,DlgX/2,8,0,8,0,nullptr,nullptr,DIF_3STATE,msg(lng::MSetAttrTemp)},
		{DI_CHECKBOX,DlgX/2,9,0,9,0,nullptr,nullptr,DIF_3STATE,msg(lng::MSetAttrOffline)},
		{DI_CHECKBOX,DlgX/2,10,0,10,0,nullptr,nullptr,DIF_3STATE,msg(lng::MSetAttrReparsePoint)},
		{DI_CHECKBOX,DlgX/2,11,0,11,0,nullptr,nullptr,DIF_3STATE|DIF_DISABLE,msg(lng::MSetAttrVirtual)},
		{DI_CHECKBOX,DlgX/2,12,0,12,0,nullptr,nullptr,DIF_3STATE,msg(lng::MSetAttrIntegrityStream)},
		{DI_CHECKBOX,DlgX/2,13,0,13,0,nullptr,nullptr,DIF_3STATE,msg(lng::MSetAttrNoScrubData)},

		{DI_TEXT,-1,14,0,14,0,nullptr,nullptr,DIF_SEPARATOR,L""},
		{DI_TEXT,DlgX-29,15,0,15,0,nullptr,nullptr,0,L""},
		{DI_TEXT,DlgX-17,15,0,15,0,nullptr,nullptr,0,L""},
		{DI_TEXT,    5,16,0,16,0,nullptr,nullptr,0,msg(lng::MSetAttrModification)},
		{DI_FIXEDIT,DlgX-29,16,DlgX-19,16,0,nullptr,nullptr,DIF_MASKEDIT,L""},
		{DI_FIXEDIT,DlgX-17,16,DlgX-6,16,0,nullptr,nullptr,DIF_MASKEDIT,L""},
		{DI_TEXT,    5,17,0,17,0,nullptr,nullptr,0,msg(lng::MSetAttrCreation)},
		{DI_FIXEDIT,DlgX-29,17,DlgX-19,17,0,nullptr,nullptr,DIF_MASKEDIT,L""},
		{DI_FIXEDIT,DlgX-17,17,DlgX-6,17,0,nullptr,nullptr,DIF_MASKEDIT,L""},
		{DI_TEXT,    5,18,0,18,0,nullptr,nullptr,0,msg(lng::MSetAttrLastAccess)},
		{DI_FIXEDIT,DlgX-29,18,DlgX-19,18,0,nullptr,nullptr,DIF_MASKEDIT,L""},
		{DI_FIXEDIT,DlgX-17,18,DlgX-6,18,0,nullptr,nullptr,DIF_MASKEDIT,L""},
		{DI_TEXT,    5,19,0,19,0,nullptr,nullptr,0,msg(lng::MSetAttrChange)},
		{DI_FIXEDIT,DlgX-29,19,DlgX-19,19,0,nullptr,nullptr,DIF_MASKEDIT,L""},
		{DI_FIXEDIT,DlgX-17,19,DlgX-6,19,0,nullptr,nullptr,DIF_MASKEDIT,L""},

		{DI_BUTTON,0,20,0,20,0,nullptr,nullptr,DIF_CENTERGROUP|DIF_BTNNOCLOSE,msg(lng::MSetAttrOriginal)},
		{DI_BUTTON,0,20,0,20,0,nullptr,nullptr,DIF_CENTERGROUP|DIF_BTNNOCLOSE,msg(lng::MSetAttrCurrent)},
		{DI_BUTTON,0,20,0,20,0,nullptr,nullptr,DIF_CENTERGROUP|DIF_BTNNOCLOSE,msg(lng::MSetAttrBlank)},
		{DI_TEXT,-1,21,0,21,0,nullptr,nullptr,DIF_SEPARATOR|DIF_HIDDEN,L""},
		{DI_CHECKBOX,5,22,0,22,0,nullptr,nullptr,DIF_DISABLE|DIF_HIDDEN,msg(lng::MSetAttrSubfolders)},
		{DI_TEXT,-1,DlgY-4,0,DlgY-4,0,nullptr,nullptr,DIF_SEPARATOR,L""},
		{DI_BUTTON,0,DlgY-3,0,DlgY-3,0,nullptr,nullptr,DIF_DEFAULTBUTTON|DIF_CENTERGROUP,msg(lng::MSetAttrSet)},
		{DI_BUTTON,0,DlgY-3,0,DlgY-3,0,nullptr,nullptr,DIF_CENTERGROUP|DIF_DISABLE,msg(lng::MSetAttrSystemDialog)},
		{DI_BUTTON,0,DlgY-3,0,DlgY-3,0,nullptr,nullptr,DIF_CENTERGROUP,msg(lng::MCancel)},
	};
	auto AttrDlg = MakeDialogItemsEx(AttrDlgData);
	SetAttrDlgParam DlgParam={};
	size_t SelCount=SrcPanel?SrcPanel->GetSelCount():1;

	if (!SelCount)
	{
		return false;
	}

	if(SelCount==1)
	{
		AttrDlg[SA_BUTTON_SYSTEMDLG].Flags&=~DIF_DISABLE;
	}

	if (SrcPanel && SrcPanel->GetMode() == panel_mode::PLUGIN_PANEL)
	{
		OpenPanelInfo Info;
		const auto hPlugin = SrcPanel->GetPluginHandle();

		if (hPlugin == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		Global->CtrlObject->Plugins->GetOpenPanelInfo(hPlugin,&Info);

		if (!(Info.Flags & OPIF_REALNAMES))
		{
			AttrDlg[SA_BUTTON_SET].Flags|=DIF_DISABLE;
			AttrDlg[SA_BUTTON_SYSTEMDLG].Flags|=DIF_DISABLE;
			DlgParam.Plugin=true;
		}
	}

	FarList NameList={sizeof(FarList)};
	std::vector<string> Links;
	std::vector<FarListItem> ListItems;

	if (!DlgParam.Plugin)
	{
		if (os::GetVolumeInformation(GetPathRoot(os::GetCurrentDirectory()), nullptr, nullptr, nullptr, &DlgParam.FileSystemFlags, nullptr))
		{
			if (!(DlgParam.FileSystemFlags&FILE_FILE_COMPRESSION))
			{
				AttrDlg[SA_CHECKBOX_COMPRESSED].Flags|=DIF_DISABLE;
			}

			if (!(DlgParam.FileSystemFlags&FILE_SUPPORTS_ENCRYPTION))
			{
				AttrDlg[SA_CHECKBOX_ENCRYPTED].Flags|=DIF_DISABLE;
			}

			if (!(DlgParam.FileSystemFlags&FILE_SUPPORTS_SPARSE_FILES))
			{
				AttrDlg[SA_CHECKBOX_SPARSE].Flags|=DIF_DISABLE;
			}
		}
	}

	{
		DWORD FileAttr=INVALID_FILE_ATTRIBUTES;
		string strSelName;
		os::FAR_FIND_DATA FindData;
		if(SrcPanel)
		{
			SrcPanel->GetSelName(nullptr,FileAttr);
			SrcPanel->GetSelName(&strSelName,FileAttr,nullptr,&FindData);
		}
		else
		{
			strSelName=*Object;
			os::GetFindDataEx(strSelName, FindData);
			FileAttr=FindData.dwFileAttributes;
		}

		if (!SelCount || (SelCount==1 && TestParentFolderName(strSelName)))
			return false;

		wchar_t DateSeparator = locale::GetDateSeparator();
		wchar_t TimeSeparator = locale::GetTimeSeparator();
		wchar_t DecimalSeparator = locale::GetDecimalSeparator();

		string DateMask, DateFormat;

		switch (locale::GetDateFormat())
		{
			case 0:
				DateMask = format(L"99{0}99{0}9999N", DateSeparator);
				DateFormat = format(lng::MSetAttrDateTitle1, DateSeparator);
				break;

			case 1:
				DateMask = format(L"99{0}99{0}9999N", DateSeparator);
				DateFormat = format(lng::MSetAttrDateTitle2, DateSeparator);
				break;

			default:
				DateMask = format(L"N9999{0}99{0}99", DateSeparator);
				DateFormat = format(lng::MSetAttrDateTitle3, DateSeparator);
				break;
		}

		AttrDlg[SA_TEXT_TITLEDATE].strData = DateFormat;
		AttrDlg[SA_TEXT_TITLETIME].strData = format(lng::MSetAttrTimeTitle, TimeSeparator, DecimalSeparator);

		AttrDlg[SA_EDIT_WDATE].strMask = AttrDlg[SA_EDIT_CDATE].strMask = AttrDlg[SA_EDIT_ADATE].strMask = AttrDlg[SA_EDIT_XDATE].strMask = DateMask;
		AttrDlg[SA_EDIT_WTIME].strMask = AttrDlg[SA_EDIT_CTIME].strMask = AttrDlg[SA_EDIT_ATIME].strMask = AttrDlg[SA_EDIT_XTIME].strMask = format(L"99{0}99{0}99{1}999", TimeSeparator, DecimalSeparator);

		bool FolderPresent=false,LinkPresent=false;
		string strLinkName;
		static const std::pair<SETATTRDLG, DWORD> AttributePair[] =
		{
			{SA_CHECKBOX_RO,FILE_ATTRIBUTE_READONLY},
			{SA_CHECKBOX_ARCHIVE,FILE_ATTRIBUTE_ARCHIVE},
			{SA_CHECKBOX_HIDDEN,FILE_ATTRIBUTE_HIDDEN},
			{SA_CHECKBOX_SYSTEM,FILE_ATTRIBUTE_SYSTEM},
			{SA_CHECKBOX_COMPRESSED,FILE_ATTRIBUTE_COMPRESSED},
			{SA_CHECKBOX_ENCRYPTED,FILE_ATTRIBUTE_ENCRYPTED},
			{SA_CHECKBOX_NOTINDEXED,FILE_ATTRIBUTE_NOT_CONTENT_INDEXED},
			{SA_CHECKBOX_SPARSE,FILE_ATTRIBUTE_SPARSE_FILE},
			{SA_CHECKBOX_TEMP,FILE_ATTRIBUTE_TEMPORARY},
			{SA_CHECKBOX_OFFLINE,FILE_ATTRIBUTE_OFFLINE},
			{SA_CHECKBOX_REPARSEPOINT,FILE_ATTRIBUTE_REPARSE_POINT},
			{SA_CHECKBOX_VIRTUAL,FILE_ATTRIBUTE_VIRTUAL},
			{SA_CHECKBOX_INTEGRITY_STREAM,FILE_ATTRIBUTE_INTEGRITY_STREAM},
			{SA_CHECKBOX_NO_SCRUB_DATA,FILE_ATTRIBUTE_NO_SCRUB_DATA},
		};

		if (SelCount==1)
		{
			if (FileAttr&FILE_ATTRIBUTE_DIRECTORY)
			{
				if (!DlgParam.Plugin)
				{
					DWORD AddFileAttr=os::GetFileAttributes(strSelName);
					if (AddFileAttr != INVALID_FILE_ATTRIBUTES)
						FileAttr|=AddFileAttr;
				}

				//_SVS(SysLog(L"SelName=%s  FileAttr=0x%08X",SelName,FileAttr));
				AttrDlg[SA_SEPARATOR4].Flags&=~DIF_HIDDEN;
				AttrDlg[SA_CHECKBOX_SUBFOLDERS].Flags&=~(DIF_DISABLE|DIF_HIDDEN);
				AttrDlg[SA_CHECKBOX_SUBFOLDERS].Selected=Global->Opt->SetAttrFolderRules?BSTATE_UNCHECKED:BSTATE_CHECKED;
				AttrDlg[SA_DOUBLEBOX].Y2+=2;
				for(int i=SA_SEPARATOR5;i<=SA_BUTTON_CANCEL;i++)
				{
					AttrDlg[i].Y1+=2;
					AttrDlg[i].Y2+=2;
				}
				DlgY+=2;

				if (Global->Opt->SetAttrFolderRules)
				{
					if (DlgParam.Plugin || os::GetFindDataEx(strSelName, FindData))
					{
						ConvertDate(FindData.ftLastWriteTime, AttrDlg[SA_EDIT_WDATE].strData,AttrDlg[SA_EDIT_WTIME].strData,12,FALSE,FALSE,2);
						ConvertDate(FindData.ftCreationTime,  AttrDlg[SA_EDIT_CDATE].strData,AttrDlg[SA_EDIT_CTIME].strData,12,FALSE,FALSE,2);
						ConvertDate(FindData.ftLastAccessTime,AttrDlg[SA_EDIT_ADATE].strData,AttrDlg[SA_EDIT_ATIME].strData,12,FALSE,FALSE,2);
						ConvertDate(FindData.ftChangeTime,    AttrDlg[SA_EDIT_XDATE].strData,AttrDlg[SA_EDIT_XTIME].strData,12,FALSE,FALSE,2);
					}

					if (FileAttr!=INVALID_FILE_ATTRIBUTES)
					{
						std::for_each(CONST_RANGE(AttributePair, i)
						{
							AttrDlg[i.first].Selected = (FileAttr & i.second)? BSTATE_CHECKED : BSTATE_UNCHECKED;
						});
					}

					for (size_t i=SA_ATTR_FIRST; i<= SA_ATTR_LAST; i++)
					{
						AttrDlg[i].Flags&=~DIF_3STATE;
					}
				}

				FolderPresent = true;
			}
			else
			{
				for (size_t i=SA_ATTR_FIRST; i<=SA_ATTR_LAST; i++)
				{
					AttrDlg[i].Flags&=~DIF_3STATE;
				}
			}

			bool IsMountPoint;
			{
				bool IsRoot = false;
				const auto PathType = ParsePath(strSelName, nullptr, &IsRoot);
				IsMountPoint = IsRoot && ((PathType == PATH_DRIVELETTER || PathType == PATH_DRIVELETTERUNC));
			}

			if ((FileAttr != INVALID_FILE_ATTRIBUTES && FileAttr&FILE_ATTRIBUTE_REPARSE_POINT) || IsMountPoint)
			{
				DWORD ReparseTag = FindData.dwReserved0;
				DWORD ReparseTagAlternative = 0;
				bool KnownReparsePoint = false;
				if (!DlgParam.Plugin)
				{
					if (IsMountPoint)
					{
						// BUGBUG, cheating
						KnownReparsePoint = true;
						ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
						os::GetVolumeNameForVolumeMountPoint(strSelName, strLinkName);
					}
					else
					{
						KnownReparsePoint = GetReparsePointInfo(strSelName, strLinkName, &ReparseTagAlternative);
						if (ReparseTagAlternative && !ReparseTag)
						{
							ReparseTag = ReparseTagAlternative;
						}

						if (!KnownReparsePoint)
						{
							if (ReparseTag == IO_REPARSE_TAG_DEDUP)
							{
								KnownReparsePoint = true;
								strLinkName = msg(lng::MListDEDUP);
							}
							else if (ReparseTag == IO_REPARSE_TAG_DFS)
							{
								string path(SrcPanel->GetCurDir() + L'\\' + strSelName);
								os::memory::netapi::ptr<DFS_INFO_3> DfsInfo;
								if (Imports().NetDfsGetInfo(UNSAFE_CSTR(path), nullptr, nullptr, 3, reinterpret_cast<LPBYTE*>(&ptr_setter(DfsInfo))) == NERR_Success)
								{
									KnownReparsePoint = true;

									auto DfsStorages = make_range(DfsInfo->Storage, DfsInfo->NumberOfStorages);
									ListItems.resize(DfsStorages.size());
									Links.resize(DfsStorages.size());

									for (const auto& i: zip(Links, ListItems, DfsStorages))
									{
										auto& Link = std::get<0>(i);
										auto& Item = std::get<1>(i);
										const auto& Storage = std::get<2>(i);

										Link = string(L"\\\\") + Storage.ServerName + L'\\' + Storage.ShareName;
										Item.Text = Link.data();
										Item.Flags =
											((Storage.State & DFS_STORAGE_STATE_ACTIVE)? (LIF_CHECKED | LIF_SELECTED) : LIF_NONE) |
											((Storage.State & DFS_STORAGE_STATE_OFFLINE)? LIF_GRAYED : LIF_NONE);
									};

									NameList.Items = ListItems.data();
									NameList.ItemsNumber = DfsInfo->NumberOfStorages;

									AttrDlg[SA_EDIT_SYMLINK].Flags |= DIF_HIDDEN;
									AttrDlg[SA_COMBO_SYMLINK].Flags &= ~DIF_HIDDEN;
									AttrDlg[SA_COMBO_SYMLINK].ListItems = &NameList;
									AttrDlg[SA_COMBO_SYMLINK].strData = concat(msg(lng::MSetAttrDfsTargets), L" (", str(NameList.ItemsNumber), L')');
								}
							}
						}
					}
				}
				AttrDlg[SA_DOUBLEBOX].Y2++;

				for (size_t i=SA_TEXT_SYMLINK; i<std::size(AttrDlgData); i++)
				{
					AttrDlg[i].Y1++;

					if (AttrDlg[i].Y2)
					{
						AttrDlg[i].Y2++;
					}
				}

				LinkPresent=true;
				NormalizeSymlinkName(strLinkName);
				auto ID_Msg = lng::MSetAttrSymlink;

				if (ReparseTag==IO_REPARSE_TAG_MOUNT_POINT)
				{
					bool Root;
					if(ParsePath(strLinkName, nullptr, &Root) == PATH_VOLUMEGUID && Root)
					{
						ID_Msg = lng::MSetAttrVolMount;
					}
					else
					{
						ID_Msg = lng::MSetAttrJunction;
					}
				}

				if (!KnownReparsePoint)
					strLinkName=msg(lng::MSetAttrUnknownJunction);

				AttrDlg[SA_TEXT_SYMLINK].Flags &= ~DIF_HIDDEN;
				AttrDlg[SA_TEXT_SYMLINK].strData = msg(ID_Msg);
				if (ReparseTag != IO_REPARSE_TAG_DFS)
					AttrDlg[SA_EDIT_SYMLINK].Flags &= ~DIF_HIDDEN;
				AttrDlg[SA_EDIT_SYMLINK].strData = strLinkName;
				if (ReparseTag == IO_REPARSE_TAG_DEDUP)
					AttrDlg[SA_EDIT_SYMLINK].Flags |= DIF_DISABLE;
				if (ReparseTag == IO_REPARSE_TAG_DEDUP || ReparseTag == IO_REPARSE_TAG_DFS)
					AttrDlg[SA_CHECKBOX_REPARSEPOINT].Flags |= DIF_DISABLE;

				DlgParam.FileSystemFlags=0;
				if (os::GetVolumeInformation(GetPathRoot(strSelName), nullptr, nullptr, nullptr, &DlgParam.FileSystemFlags, nullptr))
				{
					if (!(DlgParam.FileSystemFlags&FILE_FILE_COMPRESSION))
					{
						AttrDlg[SA_CHECKBOX_COMPRESSED].Flags|=DIF_DISABLE;
					}

					if (!(DlgParam.FileSystemFlags&FILE_SUPPORTS_ENCRYPTION))
					{
						AttrDlg[SA_CHECKBOX_ENCRYPTED].Flags|=DIF_DISABLE;
					}

					if (!(DlgParam.FileSystemFlags&FILE_SUPPORTS_SPARSE_FILES))
					{
						AttrDlg[SA_CHECKBOX_SPARSE].Flags|=DIF_DISABLE;
					}
				}
			}

			// обработка случая "несколько хардлинков"
			if (0 == (FileAttr&FILE_ATTRIBUTE_DIRECTORY))
			{
				if ((NameList.ItemsNumber = GetNumberOfLinks(strSelName)) > 1)
				{
					AttrDlg[SA_TEXT_NAME].Flags|=DIF_HIDDEN;
					AttrDlg[SA_COMBO_HARDLINK].Flags&=~DIF_HIDDEN;

					auto strRoot = GetPathRoot(strSelName);
					DeleteEndSlash(strRoot);

					Links.reserve(NameList.ItemsNumber);
					for (const auto& i: os::fs::enum_names(strSelName))
					{
						Links.emplace_back(strRoot + i);
					}

					if (!Links.empty())
					{
						ListItems.reserve(Links.size());
						std::transform(ALL_CONST_RANGE(Links), std::back_inserter(ListItems), [&](const auto& i) { return FarListItem{ 0, i.data() }; });

						NameList.Items = ListItems.data();
						AttrDlg[SA_COMBO_HARDLINK].ListItems = &NameList;
					}
					else
					{
						AttrDlg[SA_COMBO_HARDLINK].Flags|=DIF_DISABLE;
					}

					AttrDlg[SA_COMBO_HARDLINK].strData = concat(msg(lng::MSetAttrHardLinks), L" (", str(NameList.ItemsNumber), L')');
				}
			}

			string strSelNameQ(strSelName);
			QuoteOuterSpace(strSelNameQ);
			AttrDlg[SA_TEXT_NAME].strData = strSelNameQ;
			TruncStr(AttrDlg[SA_TEXT_NAME].strData,DlgX-10);

			if (FileAttr!=INVALID_FILE_ATTRIBUTES)
			{
				std::for_each(CONST_RANGE(AttributePair, i)
				{
					AttrDlg[i.first].Selected = (FileAttr & i.second)? BSTATE_CHECKED : BSTATE_UNCHECKED;
				});
			}

			const struct DateTimeId
			{
				SETATTRDLG DateId;
				SETATTRDLG TimeId;
				PFILETIME TimeValue;
			}
			Dates[] =
			{
				{SA_EDIT_WDATE, SA_EDIT_WTIME, &FindData.ftLastWriteTime},
				{SA_EDIT_CDATE, SA_EDIT_CTIME, &FindData.ftCreationTime},
				{SA_EDIT_ADATE, SA_EDIT_ATIME, &FindData.ftLastAccessTime},
				{SA_EDIT_XDATE, SA_EDIT_XTIME, &FindData.ftChangeTime},
			};

			if (DlgParam.Plugin || (!DlgParam.Plugin&&os::GetFindDataEx(strSelName, FindData)))
			{
				std::for_each(CONST_RANGE(Dates, i)
				{
					ConvertDate(*i.TimeValue, AttrDlg[i.DateId].strData, AttrDlg[i.TimeId].strData, 12, FALSE, FALSE, 2);
				});
			}

			string strComputerName;
			if(SrcPanel)
			{
				strComputerName = ExtractComputerName(SrcPanel->GetCurDir());
			}
			GetFileOwner(strComputerName,strSelName,AttrDlg[SA_EDIT_OWNER].strData);
		}
		else
		{
			std::for_each(CONST_RANGE(AttributePair, i)
			{
				AttrDlg[i.first].Selected = BSTATE_3STATE;
			});

			AttrDlg[SA_EDIT_WDATE].strData.clear();
			AttrDlg[SA_EDIT_WTIME].strData.clear();
			AttrDlg[SA_EDIT_CDATE].strData.clear();
			AttrDlg[SA_EDIT_CTIME].strData.clear();
			AttrDlg[SA_EDIT_ADATE].strData.clear();
			AttrDlg[SA_EDIT_ATIME].strData.clear();
			AttrDlg[SA_EDIT_XDATE].strData.clear();
			AttrDlg[SA_EDIT_XTIME].strData.clear();
			AttrDlg[SA_BUTTON_ORIGINAL].Flags|=DIF_DISABLE;
			AttrDlg[SA_TEXT_NAME].strData = msg(lng::MSetAttrSelectedObjects);

			for (size_t i=SA_ATTR_FIRST; i<=SA_ATTR_LAST; i++)
			{
				AttrDlg[i].Selected=BSTATE_UNCHECKED;
			}

			// проверка - есть ли среди выделенных - каталоги?
			// так же проверка на атрибуты
			if(SrcPanel)
			{
				SrcPanel->GetSelName(nullptr,FileAttr);
			}
			FolderPresent=false;

			if(SrcPanel)
			{
				const auto strComputerName = ExtractComputerName(SrcPanel->GetCurDir());

				bool CheckOwner=true;
				while (SrcPanel->GetSelName(&strSelName,FileAttr,nullptr,&FindData))
				{
					if (!FolderPresent&&(FileAttr&FILE_ATTRIBUTE_DIRECTORY))
					{
						FolderPresent=true;
						AttrDlg[SA_SEPARATOR4].Flags&=~DIF_HIDDEN;
						AttrDlg[SA_CHECKBOX_SUBFOLDERS].Flags&=~(DIF_DISABLE|DIF_HIDDEN);
						AttrDlg[SA_DOUBLEBOX].Y2+=2;
						for(int i=SA_SEPARATOR5;i<=SA_BUTTON_CANCEL;i++)
						{
							AttrDlg[i].Y1+=2;
							AttrDlg[i].Y2+=2;
						}
						DlgY+=2;
					}

					std::for_each(CONST_RANGE(AttributePair, i)
					{
						if (FileAttr & i.second)
						{
							++AttrDlg[i.first].Selected;
						}
					});

					if(CheckOwner)
					{
						string strCurOwner;
						GetFileOwner(strComputerName,strSelName,strCurOwner);
						if(AttrDlg[SA_EDIT_OWNER].strData.empty())
						{
							AttrDlg[SA_EDIT_OWNER].strData=strCurOwner;
						}
						else if(AttrDlg[SA_EDIT_OWNER].strData != strCurOwner)
						{
							AttrDlg[SA_EDIT_OWNER].strData=msg(lng::MSetAttrOwnerMultiple);
							CheckOwner=false;
						}
					}
				}
			}
			else
			{
				// BUGBUG, copy-paste
				if (!FolderPresent&&(FindData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
				{
					FolderPresent=true;
					AttrDlg[SA_SEPARATOR4].Flags&=~DIF_HIDDEN;
					AttrDlg[SA_CHECKBOX_SUBFOLDERS].Flags&=~(DIF_DISABLE|DIF_HIDDEN);
					AttrDlg[SA_DOUBLEBOX].Y2+=2;
					for(int i=SA_SEPARATOR5;i<=SA_BUTTON_CANCEL;i++)
					{
						AttrDlg[i].Y1+=2;
						AttrDlg[i].Y2+=2;
					}
					DlgY+=2;
				}
				std::for_each(CONST_RANGE(AttributePair, i)
				{
					if (FindData.dwFileAttributes & i.second)
					{
						++AttrDlg[i.first].Selected;
					}
				});
			}
			if(SrcPanel)
			{
				SrcPanel->GetSelName(nullptr,FileAttr);
				SrcPanel->GetSelName(&strSelName,FileAttr,nullptr,&FindData);
			}

			// выставим "неопределенку" или то, что нужно
			for (size_t i=SA_ATTR_FIRST; i<=SA_ATTR_LAST; i++)
			{
				// снимаем 3-state, если "есть все или нет ничего"
				// за исключением случая, если есть Фолдер среди объектов
				if ((!AttrDlg[i].Selected || static_cast<size_t>(AttrDlg[i].Selected) >= SelCount) && !FolderPresent)
				{
					AttrDlg[i].Flags&=~DIF_3STATE;
				}

				AttrDlg[i].Selected=(static_cast<size_t>(AttrDlg[i].Selected) >= SelCount)?BST_CHECKED:(!AttrDlg[i].Selected?BSTATE_UNCHECKED:BSTATE_3STATE);
			}
		}

		// запомним состояние переключателей.
		for (size_t i=SA_ATTR_FIRST; i<=SA_ATTR_LAST; i++)
		{
			DlgParam.cb[i - SA_ATTR_FIRST].Flags = AttrDlg[i].Flags;
			DlgParam.cb[i - SA_ATTR_FIRST].Value = AttrDlg[i].Selected;
			DlgParam.cb[i - SA_ATTR_FIRST].Changed = false;
		}
		DlgParam.strOwner=AttrDlg[SA_EDIT_OWNER].strData;
		string strInitOwner=AttrDlg[SA_EDIT_OWNER].strData;

		// поведение для каталогов как у 1.65?
		if (FolderPresent && !Global->Opt->SetAttrFolderRules)
		{
			AttrDlg[SA_CHECKBOX_SUBFOLDERS].Selected=BSTATE_CHECKED;
			AttrDlg[SA_EDIT_WDATE].strData.clear();
			AttrDlg[SA_EDIT_WTIME].strData.clear();
			AttrDlg[SA_EDIT_CDATE].strData.clear();
			AttrDlg[SA_EDIT_CTIME].strData.clear();
			AttrDlg[SA_EDIT_ADATE].strData.clear();
			AttrDlg[SA_EDIT_ATIME].strData.clear();
			AttrDlg[SA_EDIT_XDATE].strData.clear();
			AttrDlg[SA_EDIT_XTIME].strData.clear();

			for (size_t i=SA_ATTR_FIRST; i<= SA_ATTR_LAST; i++)
			{
				AttrDlg[i].Selected=BSTATE_3STATE;
				AttrDlg[i].Flags|=DIF_3STATE;
			}
			AttrDlg[SA_EDIT_OWNER].strData.clear();
		}

		DlgParam.DialogMode=((SelCount==1&&!(FileAttr&FILE_ATTRIBUTE_DIRECTORY))?MODE_FILE:(SelCount==1?MODE_FOLDER:MODE_MULTIPLE));
		DlgParam.strSelName=strSelName;
		DlgParam.OSubfoldersState=static_cast<FARCHECKEDSTATE>(AttrDlg[SA_CHECKBOX_SUBFOLDERS].Selected);

		const auto Dlg = Dialog::create(AttrDlg, SetAttrDlgProc, &DlgParam);
		Dlg->SetHelp(L"FileAttrDlg");                 //  ^ - это одиночный диалог!
		Dlg->SetId(FileAttrDlgId);

		if (LinkPresent)
		{
			DlgY++;
		}

		Dlg->SetPosition(-1,-1,DlgX,DlgY);
		Dlg->Process();

		switch(Dlg->GetExitCode())
		{
		case SA_BUTTON_SET:
			{
				//reparse point editor
				if (StrCmpI(AttrDlg[SA_EDIT_SYMLINK].strData, strLinkName))
				{
					string strTarget = AttrDlg[SA_EDIT_SYMLINK].strData;
					if(!ModifyReparsePoint(strSelName, Unquote(strTarget)))
					{
						Global->CatchError();
						Message(MSG_WARNING|MSG_ERRORTYPE,1,msg(lng::MError),msg(lng::MCopyCannotCreateLink),strSelName.data(),msg(lng::MHOk));
					}
				}

				const SETATTRDLG Times[] = {SA_EDIT_WTIME, SA_EDIT_CTIME, SA_EDIT_ATIME, SA_EDIT_XTIME};

				std::for_each(CONST_RANGE(Times, i)
				{
					AttrDlg[i].strData[8] = locale::GetTimeSeparator();
				});

				SCOPED_ACTION(TPreRedrawFuncGuard)(std::make_unique<AttrPreRedrawItem>());
				ShellSetFileAttributesMsg(SelCount==1? strSelName : string{});
				int SkipMode=-1;

				if (SelCount==1 && !(FileAttr & FILE_ATTRIBUTE_DIRECTORY))
				{
					DWORD NewAttr=FileAttr&FILE_ATTRIBUTE_DIRECTORY;

					std::for_each(CONST_RANGE(AttributePair, i)
					{
						if (AttrDlg[i.first].Selected)
						{
							NewAttr |= i.second;
						}
					});

					if(!AttrDlg[SA_EDIT_OWNER].strData.empty() && StrCmpI(strInitOwner, AttrDlg[SA_EDIT_OWNER].strData))
					{
						int Result=ESetFileOwner(strSelName,AttrDlg[SA_EDIT_OWNER].strData,SkipMode);
						if(Result==SETATTR_RET_SKIPALL)
						{
							SkipMode=SETATTR_RET_SKIP;
						}
						else if(Result==SETATTR_RET_ERROR)
						{
							break;
						}
					}

					FILETIME LastWriteTime={},CreationTime={},LastAccessTime={}, ChangeTime={};
					int SetWriteTime = ReadFileTime(0,strSelName,LastWriteTime,AttrDlg[SA_EDIT_WDATE].strData,AttrDlg[SA_EDIT_WTIME].strData);
					int SetCreationTime = ReadFileTime(1,strSelName,CreationTime,AttrDlg[SA_EDIT_CDATE].strData,AttrDlg[SA_EDIT_CTIME].strData);
					int SetLastAccessTime = ReadFileTime(2,strSelName,LastAccessTime,AttrDlg[SA_EDIT_ADATE].strData,AttrDlg[SA_EDIT_ATIME].strData);
					int SetChangeTime = ReadFileTime(3,strSelName,ChangeTime,AttrDlg[SA_EDIT_XDATE].strData,AttrDlg[SA_EDIT_XTIME].strData);
					//_SVS(SysLog(L"\n\tSetWriteTime=%d\n\tSetCreationTime=%d\n\tSetLastAccessTime=%d",SetWriteTime,SetCreationTime,SetLastAccessTime));

					if (SetWriteTime || SetCreationTime || SetLastAccessTime || SetChangeTime)
					{
						if(ESetFileTime(strSelName,SetWriteTime?&LastWriteTime:nullptr,SetCreationTime?&CreationTime:nullptr,SetLastAccessTime?&LastAccessTime:nullptr,SetChangeTime?&ChangeTime:nullptr,FileAttr,SkipMode)==SETATTR_RET_SKIPALL)
						{
							SkipMode=SETATTR_RET_SKIP;
						}
					}

					if ((NewAttr&FILE_ATTRIBUTE_COMPRESSED) && !(FileAttr&FILE_ATTRIBUTE_COMPRESSED))
					{
						if (ESetFileCompression(strSelName,1,FileAttr,SkipMode)==SETATTR_RET_SKIPALL)
						{
							SkipMode=SETATTR_RET_SKIP;
						}
					}
					else if (!(NewAttr&FILE_ATTRIBUTE_COMPRESSED) && (FileAttr&FILE_ATTRIBUTE_COMPRESSED))
					{
						if(ESetFileCompression(strSelName,0,FileAttr,SkipMode)==SETATTR_RET_SKIPALL)
						{
							SkipMode=SETATTR_RET_SKIP;
						}
					}

					if ((NewAttr&FILE_ATTRIBUTE_ENCRYPTED) && !(FileAttr&FILE_ATTRIBUTE_ENCRYPTED))
					{
						if (ESetFileEncryption(strSelName, true, FileAttr, SkipMode) == SETATTR_RET_SKIPALL)
						{
							SkipMode=SETATTR_RET_SKIP;
						}
					}
					else if (!(NewAttr&FILE_ATTRIBUTE_ENCRYPTED) && (FileAttr&FILE_ATTRIBUTE_ENCRYPTED))
					{
						if (ESetFileEncryption(strSelName, false, FileAttr, SkipMode) == SETATTR_RET_SKIPALL)
						{
							SkipMode=SETATTR_RET_SKIP;
						}
					}

					if ((NewAttr&FILE_ATTRIBUTE_SPARSE_FILE) && !(FileAttr&FILE_ATTRIBUTE_SPARSE_FILE))
					{
						if (ESetFileSparse(strSelName,true,FileAttr,SkipMode)==SETATTR_RET_SKIPALL)
						{
							SkipMode=SETATTR_RET_SKIP;
						}
					}
					else if (!(NewAttr&FILE_ATTRIBUTE_SPARSE_FILE) && (FileAttr&FILE_ATTRIBUTE_SPARSE_FILE))
					{
						if (ESetFileSparse(strSelName,false,FileAttr,SkipMode)==SETATTR_RET_SKIPALL)
						{
							SkipMode=SETATTR_RET_SKIP;
						}
					}

					if ((FileAttr&~(FILE_ATTRIBUTE_ENCRYPTED|FILE_ATTRIBUTE_COMPRESSED|FILE_ATTRIBUTE_SPARSE_FILE))!=(NewAttr&~(FILE_ATTRIBUTE_ENCRYPTED|FILE_ATTRIBUTE_COMPRESSED|FILE_ATTRIBUTE_SPARSE_FILE)))
					{
						if (ESetFileAttributes(strSelName,NewAttr&~(FILE_ATTRIBUTE_ENCRYPTED|FILE_ATTRIBUTE_COMPRESSED|FILE_ATTRIBUTE_SPARSE_FILE),SkipMode)==SETATTR_RET_SKIPALL)
						{
							SkipMode=SETATTR_RET_SKIP;
						}
					}

					if (!(NewAttr&FILE_ATTRIBUTE_REPARSE_POINT) && (FileAttr&FILE_ATTRIBUTE_REPARSE_POINT))
					{
						if (EDeleteReparsePoint(strSelName, FileAttr, SkipMode)==SETATTR_RET_SKIPALL)
						{
							SkipMode=SETATTR_RET_SKIP;
						}
					}
				}
				/* Multi *********************************************************** */
				else
				{
					ConsoleTitle::SetFarTitle(msg(lng::MSetAttrTitle));
					if(SrcPanel)
					{
						Global->CtrlObject->Cp()->GetAnotherPanel(SrcPanel)->CloseFile();
					}
					DWORD SetAttr=0,ClearAttr=0;

					std::for_each(CONST_RANGE(AttributePair, i)
					{
						switch (AttrDlg[i.first].Selected)
						{
							case BSTATE_CHECKED:
								SetAttr |= i.second;
								break;
							case BSTATE_UNCHECKED:
								ClearAttr |= i.second;
								break;
						}
					});

					if (AttrDlg[SA_CHECKBOX_COMPRESSED].Selected==BSTATE_CHECKED)
					{
						ClearAttr|=FILE_ATTRIBUTE_ENCRYPTED;
					}

					if (AttrDlg[SA_CHECKBOX_ENCRYPTED].Selected==BSTATE_CHECKED)
					{
						ClearAttr|=FILE_ATTRIBUTE_COMPRESSED;
					}

					if(SrcPanel)
					{
						SrcPanel->GetSelName(nullptr,FileAttr);
					}
					SCOPED_ACTION(IndeterminateTaskBar);
					SCOPED_ACTION(wakeful);
					bool Cancel=false;

					time_check TimeCheck(time_check::mode::immediate, GetRedrawTimeout());
					bool SingleFileDone=false;
					while ((SrcPanel?SrcPanel->GetSelName(&strSelName,FileAttr,nullptr,&FindData):!SingleFileDone) && !Cancel)
					{
						if(!SrcPanel)
						{
							SingleFileDone=true;
						}
		//_SVS(SysLog(L"SelName='%s'\n\tFileAttr =0x%08X\n\tSetAttr  =0x%08X\n\tClearAttr=0x%08X\n\tResult   =0x%08X",
		//    SelName,FileAttr,SetAttr,ClearAttr,((FileAttr|SetAttr)&(~ClearAttr))));

						if (TimeCheck)
						{
							ShellSetFileAttributesMsg(strSelName);

							if (CheckForEsc())
								break;
						}

						if(!AttrDlg[SA_EDIT_OWNER].strData.empty() && StrCmpI(strInitOwner, AttrDlg[SA_EDIT_OWNER].strData))
						{
							int Result=ESetFileOwner(strSelName,AttrDlg[SA_EDIT_OWNER].strData,SkipMode);
							if(Result==SETATTR_RET_SKIPALL)
							{
								SkipMode=SETATTR_RET_SKIP;
							}
							else if(Result==SETATTR_RET_ERROR)
							{
								break;
							}
						}

						FILETIME LastWriteTime,CreationTime,LastAccessTime, ChangeTime;
						bool SetWriteTime = ReadFileTime(0,strSelName,LastWriteTime,AttrDlg[SA_EDIT_WDATE].strData,AttrDlg[SA_EDIT_WTIME].strData);
						bool SetCreationTime = ReadFileTime(1,strSelName,CreationTime,AttrDlg[SA_EDIT_CDATE].strData,AttrDlg[SA_EDIT_CTIME].strData);
						bool SetLastAccessTime = ReadFileTime(2,strSelName,LastAccessTime,AttrDlg[SA_EDIT_ADATE].strData,AttrDlg[SA_EDIT_ATIME].strData);
						bool SetChangeTime = ReadFileTime(3,strSelName,ChangeTime,AttrDlg[SA_EDIT_XDATE].strData,AttrDlg[SA_EDIT_XTIME].strData);
						int RetCode=ESetFileTime(strSelName,SetWriteTime?&LastWriteTime:nullptr,SetCreationTime?&CreationTime:nullptr,SetLastAccessTime?&LastAccessTime:nullptr,SetChangeTime?&ChangeTime:nullptr,FileAttr,SkipMode);

						if (RetCode == SETATTR_RET_ERROR)
							break;
						else if (RetCode == SETATTR_RET_SKIP)
							continue;
						else if (RetCode == SETATTR_RET_SKIPALL)
						{
							SkipMode=SETATTR_RET_SKIP;
							continue;
						}

						if(FileAttr!=INVALID_FILE_ATTRIBUTES)
						{
							if (((FileAttr|SetAttr)&~ClearAttr) != FileAttr)
							{
								if (AttrDlg[SA_CHECKBOX_COMPRESSED].Selected != BSTATE_3STATE)
								{
									RetCode=ESetFileCompression(strSelName,AttrDlg[SA_CHECKBOX_COMPRESSED].Selected,FileAttr,SkipMode);

									if (RetCode == SETATTR_RET_ERROR)
										break;
									else if (RetCode == SETATTR_RET_SKIP)
										continue;
									else if (RetCode == SETATTR_RET_SKIPALL)
									{
										SkipMode=SETATTR_RET_SKIP;
										continue;
									}
								}

								if (AttrDlg[SA_CHECKBOX_ENCRYPTED].Selected != BSTATE_3STATE) // +E -C
								{
									if (AttrDlg[SA_CHECKBOX_COMPRESSED].Selected != BSTATE_CHECKED)
									{
										RetCode=ESetFileEncryption(strSelName, AttrDlg[SA_CHECKBOX_ENCRYPTED].Selected != 0, FileAttr, SkipMode);

										if (RetCode == SETATTR_RET_ERROR)
											break;
										else if (RetCode == SETATTR_RET_SKIP)
											continue;
										else if (RetCode == SETATTR_RET_SKIPALL)
										{
											SkipMode=SETATTR_RET_SKIP;
											continue;
										}
									}
								}

								if (AttrDlg[SA_CHECKBOX_SPARSE].Selected!=BSTATE_3STATE)
								{
									RetCode=ESetFileSparse(strSelName,AttrDlg[SA_CHECKBOX_SPARSE].Selected==BSTATE_CHECKED,FileAttr,SkipMode);

									if (RetCode == SETATTR_RET_ERROR)
									{
										break;
									}
									else if (RetCode == SETATTR_RET_SKIP)
									{
										continue;
									}
									else if (RetCode == SETATTR_RET_SKIPALL)
									{
										SkipMode=SETATTR_RET_SKIP;
										continue;
									}
								}

								if (AttrDlg[SA_CHECKBOX_REPARSEPOINT].Selected == BSTATE_UNCHECKED)
								{
									RetCode=EDeleteReparsePoint(strSelName, FileAttr, SkipMode);

									if (RetCode == SETATTR_RET_ERROR)
									{
										break;
									}
									else if (RetCode == SETATTR_RET_SKIP)
									{
										continue;
									}
									else if (RetCode == SETATTR_RET_SKIPALL)
									{
										SkipMode=SETATTR_RET_SKIP;
										continue;
									}
								}

								RetCode=ESetFileAttributes(strSelName,((FileAttr|SetAttr)&(~ClearAttr)),SkipMode);

								if (RetCode == SETATTR_RET_ERROR)
									break;
								else if (RetCode == SETATTR_RET_SKIP)
									continue;
								else if (RetCode == SETATTR_RET_SKIPALL)
								{
									SkipMode=SETATTR_RET_SKIP;
									continue;
								}
							}

							if ((FileAttr & FILE_ATTRIBUTE_DIRECTORY) && AttrDlg[SA_CHECKBOX_SUBFOLDERS].Selected)
							{
								ScanTree ScTree(false);
								ScTree.SetFindPath(strSelName,L"*");
								time_check TreeTimeCheck(time_check::mode::delayed, GetRedrawTimeout());
								string strFullName;

								while (ScTree.GetNextName(FindData,strFullName))
								{
									if (TreeTimeCheck)
									{
										ShellSetFileAttributesMsg(strFullName);

										if (CheckForEsc())
										{
											Cancel=true;
											break;
										}
									}

									if(!AttrDlg[SA_EDIT_OWNER].strData.empty() && (DlgParam.OSubfoldersState || StrCmpI(strInitOwner, AttrDlg[SA_EDIT_OWNER].strData)))
									{
										int Result=ESetFileOwner(strFullName,AttrDlg[SA_EDIT_OWNER].strData,SkipMode);
										if(Result==SETATTR_RET_SKIPALL)
										{
											SkipMode=SETATTR_RET_SKIP;
										}
										else if(Result==SETATTR_RET_ERROR)
										{
											break;
										}
									}

									SetWriteTime = ReadFileTime(0,strFullName,LastWriteTime,AttrDlg[SA_EDIT_WDATE].strData,AttrDlg[SA_EDIT_WTIME].strData);
									SetCreationTime = ReadFileTime(1,strFullName,CreationTime,AttrDlg[SA_EDIT_CDATE].strData,AttrDlg[SA_EDIT_CTIME].strData);
									SetLastAccessTime = ReadFileTime(2,strFullName,LastAccessTime,AttrDlg[SA_EDIT_ADATE].strData,AttrDlg[SA_EDIT_ATIME].strData);
									SetChangeTime = ReadFileTime(3,strFullName,ChangeTime,AttrDlg[SA_EDIT_XDATE].strData,AttrDlg[SA_EDIT_XTIME].strData);

									if (SetWriteTime || SetCreationTime || SetLastAccessTime || SetChangeTime)
									{
										RetCode=ESetFileTime(strFullName,SetWriteTime?&LastWriteTime:nullptr,SetCreationTime?&CreationTime:nullptr,SetLastAccessTime?&LastAccessTime:nullptr,SetChangeTime?&ChangeTime:nullptr,FindData.dwFileAttributes,SkipMode);

										if (RetCode == SETATTR_RET_ERROR)
										{
											Cancel=true;
											break;
										}
										else if (RetCode == SETATTR_RET_SKIP)
											continue;
										else if (RetCode == SETATTR_RET_SKIPALL)
										{
											SkipMode=SETATTR_RET_SKIP;
											continue;
										}
									}

									if (((FindData.dwFileAttributes|SetAttr)&(~ClearAttr)) !=
													FindData.dwFileAttributes)
									{
										if (AttrDlg[SA_CHECKBOX_COMPRESSED].Selected!=BSTATE_3STATE)
										{
											RetCode=ESetFileCompression(strFullName,AttrDlg[SA_CHECKBOX_COMPRESSED].Selected,FindData.dwFileAttributes,SkipMode);

											if (RetCode == SETATTR_RET_ERROR)
											{
												Cancel=true;
												break;
											}
											else if (RetCode == SETATTR_RET_SKIP)
												continue;
											else if (RetCode == SETATTR_RET_SKIPALL)
											{
												SkipMode=SETATTR_RET_SKIP;
												continue;
											}
										}

										if (AttrDlg[SA_CHECKBOX_ENCRYPTED].Selected!=BSTATE_3STATE) // +E -C
										{
											if (AttrDlg[SA_CHECKBOX_COMPRESSED].Selected != 1)
											{
												RetCode=ESetFileEncryption(strFullName, AttrDlg[SA_CHECKBOX_ENCRYPTED].Selected!=0, FindData.dwFileAttributes, SkipMode);

												if (RetCode == SETATTR_RET_ERROR)
												{
													Cancel=true;
													break;
												}
												else if (RetCode == SETATTR_RET_SKIP)
													continue;
												else if (RetCode == SETATTR_RET_SKIPALL)
												{
													SkipMode=SETATTR_RET_SKIP;
													continue;
												}
											}
										}

										if (AttrDlg[SA_CHECKBOX_SPARSE].Selected!=BSTATE_3STATE)
										{
											RetCode=ESetFileSparse(strFullName,AttrDlg[SA_CHECKBOX_SPARSE].Selected==BSTATE_CHECKED,FindData.dwFileAttributes,SkipMode);

											if (RetCode == SETATTR_RET_ERROR)
											{
												Cancel=true;
												break;
											}
											else if (RetCode == SETATTR_RET_SKIP)
											{
												continue;
											}
											else if (RetCode == SETATTR_RET_SKIPALL)
											{
												SkipMode=SETATTR_RET_SKIP;
												continue;
											}
										}

										if (AttrDlg[SA_CHECKBOX_REPARSEPOINT].Selected == BSTATE_UNCHECKED)
										{
											RetCode=EDeleteReparsePoint(strFullName, FindData.dwFileAttributes, SkipMode);

											if (RetCode == SETATTR_RET_ERROR)
											{
												break;
											}
											else if (RetCode == SETATTR_RET_SKIP)
											{
												continue;
											}
											else if (RetCode == SETATTR_RET_SKIPALL)
											{
												SkipMode=SETATTR_RET_SKIP;
												continue;
											}
										}

										RetCode=ESetFileAttributes(strFullName,(FindData.dwFileAttributes|SetAttr)&(~ClearAttr),SkipMode);

										if (RetCode == SETATTR_RET_ERROR)
										{
											Cancel=true;
											break;
										}
										else if (RetCode == SETATTR_RET_SKIP)
											continue;
										else if (RetCode == SETATTR_RET_SKIPALL)
										{
											SkipMode=SETATTR_RET_SKIP;
											continue;
										}
									}
								}
							}
						}
					} // END: while (SrcPanel->GetSelName(...))
				}
			}
			break;
		case SA_BUTTON_SYSTEMDLG:
			{
				SHELLEXECUTEINFOW seInfo={sizeof(seInfo)};
				seInfo.nShow = SW_SHOW;
				seInfo.fMask = SEE_MASK_INVOKEIDLIST;
				NTPath strFullName(strSelName);
				if(FindData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
				{
					AddEndSlash(strFullName);
				}
				seInfo.lpFile = strFullName.data();
				if (!IsWindowsVistaOrGreater() && ParsePath(seInfo.lpFile) == PATH_DRIVELETTERUNC)
				{	// "\\?\c:\..." fails on old windows
					seInfo.lpFile += 4;
				}
				seInfo.lpVerb = L"properties";
				const auto strCurDir = os::GetCurrentDirectory();
				seInfo.lpDirectory=strCurDir.data();
				ShellExecuteExW(&seInfo);
			}
			break;
		default:
			return false;
		}
	}

	if(SrcPanel)
	{
		SrcPanel->SaveSelection();
		SrcPanel->Update(UPDATE_KEEP_SELECTION);
		SrcPanel->ClearSelection();
		Global->CtrlObject->Cp()->GetAnotherPanel(SrcPanel)->Update(UPDATE_KEEP_SELECTION|UPDATE_SECONDARY);
	}
	Global->CtrlObject->Cp()->Redraw();
	return true;
}
