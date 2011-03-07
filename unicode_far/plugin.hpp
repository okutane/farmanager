#pragma once
#ifndef __PLUGIN_HPP__
#define __PLUGIN_HPP__

#ifndef FAR_USE_INTERNALS
#define FAR_USE_INTERNALS
#endif // END FAR_USE_INTERNALS
/*
  plugin.hpp

  Plugin API for Far Manager <%VERSION%>
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

EXCEPTION:
Far Manager plugins that use this header file can be distributed under any
other possible license with no implications from the above license on them.
*/

#define FARMANAGERVERSION_MAJOR 1
#define FARMANAGERVERSION_MINOR 80
#define FARMANAGERVERSION_BUILD 496

#ifndef RC_INVOKED

#define MAKEFARVERSION(major,minor,build) ( ((major)<<24) | ((minor)<<16) | (build) )

#define FARMANAGERVERSION MAKEFARVERSION(FARMANAGERVERSION_MAJOR,FARMANAGERVERSION_MINOR,FARMANAGERVERSION_BUILD)

#ifdef FAR_USE_INTERNALS
#else // ELSE FAR_USE_INTERNALS
#include <windows.h>
#endif // END FAR_USE_INTERNALS

#undef DefDlgProc

#define FARMACRO_KEY_EVENT  (KEY_EVENT|0x8000)

#define CP_UNICODE 1200
#define CP_REVERSEBOM 1201
#define CP_AUTODETECT ((UINT)-1)

typedef unsigned __int64 FARCOLORFLAGS;
static const FARCOLORFLAGS
	FMSG_FG_4BIT  = 0x0000000000000001ULL,
	FMSG_BG_4BIT  = 0x0000000000000002ULL;

struct FarColor
{
	unsigned __int64 Flags;
	unsigned int ForegroundColor;
	unsigned int BackgroundColor;
	void* Reserved;
};

typedef unsigned __int64 FARMESSAGEFLAGS;
static const FARMESSAGEFLAGS
	FMSG_NONE                = 0,
	FMSG_WARNING             = 0x0000000000000001ULL,
	FMSG_ERRORTYPE           = 0x0000000000000002ULL,
	FMSG_KEEPBACKGROUND      = 0x0000000000000004ULL,
	FMSG_LEFTALIGN           = 0x0000000000000008ULL,
	FMSG_ALLINONE            = 0x0000000000000010ULL,
	FMSG_MB_OK               = 0x0000000000010000ULL,
	FMSG_MB_OKCANCEL         = 0x0000000000020000ULL,
	FMSG_MB_ABORTRETRYIGNORE = 0x0000000000030000ULL,
	FMSG_MB_YESNO            = 0x0000000000040000ULL,
	FMSG_MB_YESNOCANCEL      = 0x0000000000050000ULL,
	FMSG_MB_RETRYCANCEL      = 0x0000000000060000ULL;

typedef int (WINAPI *FARAPIMESSAGE)(
    const GUID* PluginId,
    FARMESSAGEFLAGS Flags,
    const wchar_t *HelpTopic,
    const wchar_t * const *Items,
    int ItemsNumber,
    int ButtonsNumber
);

enum FARDIALOGITEMTYPES
{
	DI_TEXT,
	DI_VTEXT,
	DI_SINGLEBOX,
	DI_DOUBLEBOX,
	DI_EDIT,
	DI_PSWEDIT,
	DI_FIXEDIT,
	DI_BUTTON,
	DI_CHECKBOX,
	DI_RADIOBUTTON,
	DI_COMBOBOX,
	DI_LISTBOX,
#ifdef FAR_USE_INTERNALS
	DI_MEMOEDIT,
#endif // END FAR_USE_INTERNALS

	DI_USERCONTROL=255,
};

/*
   Check diagol element type has inputstring?
   (DI_EDIT, DI_FIXEDIT, DI_PSWEDIT, etc)
*/
static __inline BOOL IsEdit(FARDIALOGITEMTYPES Type)
{
	switch (Type)
	{
		case DI_EDIT:
		case DI_FIXEDIT:
		case DI_PSWEDIT:
#ifdef FAR_USE_INTERNALS
		case DI_MEMOEDIT:
#endif // END FAR_USE_INTERNALS
		case DI_COMBOBOX:
			return TRUE;
		default:
			return FALSE;
	}
}

typedef unsigned __int64 FARDIALOGITEMFLAGS;
static const FARDIALOGITEMFLAGS
	DIF_NONE                  = 0,
	DIF_COLORMASK             = 0x00000000000000ffULL,
	DIF_SETCOLOR              = 0x0000000000000100ULL,
	DIF_BOXCOLOR              = 0x0000000000000200ULL,
	DIF_GROUP                 = 0x0000000000000400ULL,
	DIF_LEFTTEXT              = 0x0000000000000800ULL,
	DIF_MOVESELECT            = 0x0000000000001000ULL,
	DIF_SHOWAMPERSAND         = 0x0000000000002000ULL,
	DIF_CENTERGROUP           = 0x0000000000004000ULL,
	DIF_NOBRACKETS            = 0x0000000000008000ULL,
	DIF_MANUALADDHISTORY      = 0x0000000000008000ULL,
	DIF_SEPARATOR             = 0x0000000000010000ULL,
	DIF_SEPARATOR2            = 0x0000000000020000ULL,
	DIF_EDITOR                = 0x0000000000020000ULL,
	DIF_LISTNOAMPERSAND       = 0x0000000000020000ULL,
	DIF_LISTNOBOX             = 0x0000000000040000ULL,
	DIF_HISTORY               = 0x0000000000040000ULL,
	DIF_BTNNOCLOSE            = 0x0000000000040000ULL,
	DIF_CENTERTEXT            = 0x0000000000040000ULL,
#ifdef FAR_USE_INTERNALS
	DIF_SEPARATORUSER         = 0x0000000000080000ULL,
#endif // END FAR_USE_INTERNALS
	DIF_SETSHIELD             = 0x0000000000080000ULL,
	DIF_EDITEXPAND            = 0x0000000000080000ULL,
	DIF_DROPDOWNLIST          = 0x0000000000100000ULL,
	DIF_USELASTHISTORY        = 0x0000000000200000ULL,
	DIF_MASKEDIT              = 0x0000000000400000ULL,
	DIF_SELECTONENTRY         = 0x0000000000800000ULL,
	DIF_3STATE                = 0x0000000000800000ULL,
	DIF_EDITPATH              = 0x0000000001000000ULL,
	DIF_LISTWRAPMODE          = 0x0000000001000000ULL,
	DIF_NOAUTOCOMPLETE        = 0x0000000002000000ULL,
	DIF_LISTAUTOHIGHLIGHT     = 0x0000000002000000ULL,
	DIF_LISTNOCLOSE           = 0x0000000004000000ULL,
#ifdef FAR_USE_INTERNALS
	DIF_AUTOMATION            = 0x0000000008000000ULL,
#endif // END FAR_USE_INTERNALS
	DIF_HIDDEN                = 0x0000000010000000ULL,
	DIF_READONLY              = 0x0000000020000000ULL,
	DIF_NOFOCUS               = 0x0000000040000000ULL,
	DIF_DISABLE               = 0x0000000080000000ULL,
	DIF_DEFAULTBUTTON         = 0x0000000100000000ULL,
	DIF_FOCUS                 = 0x0000000200000000ULL;

enum FARMESSAGE
{
	DM_FIRST=0,
	DM_CLOSE,
	DM_ENABLE,
	DM_ENABLEREDRAW,
	DM_GETDLGDATA,
	DM_GETDLGITEM,
	DM_GETDLGRECT,
	DM_GETTEXT,
	DM_GETTEXTLENGTH,
	DM_KEY,
	DM_MOVEDIALOG,
	DM_SETDLGDATA,
	DM_SETDLGITEM,
	DM_SETFOCUS,
	DM_REDRAW,
	DM_SETTEXT,
	DM_SETMAXTEXTLENGTH,
	DM_SHOWDIALOG,
	DM_GETFOCUS,
	DM_GETCURSORPOS,
	DM_SETCURSORPOS,
	DM_GETTEXTPTR,
	DM_SETTEXTPTR,
	DM_SHOWITEM,
	DM_ADDHISTORY,

	DM_GETCHECK,
	DM_SETCHECK,
	DM_SET3STATE,

	DM_LISTSORT,
	DM_LISTGETITEM,
	DM_LISTGETCURPOS,
	DM_LISTSETCURPOS,
	DM_LISTDELETE,
	DM_LISTADD,
	DM_LISTADDSTR,
	DM_LISTUPDATE,
	DM_LISTINSERT,
	DM_LISTFINDSTRING,
	DM_LISTINFO,
	DM_LISTGETDATA,
	DM_LISTSETDATA,
	DM_LISTSETTITLES,
	DM_LISTGETTITLES,

	DM_RESIZEDIALOG,
	DM_SETITEMPOSITION,

	DM_GETDROPDOWNOPENED,
	DM_SETDROPDOWNOPENED,

	DM_SETHISTORY,

	DM_GETITEMPOSITION,
	DM_SETMOUSEEVENTNOTIFY,

	DM_EDITUNCHANGEDFLAG,

	DM_GETITEMDATA,
	DM_SETITEMDATA,

	DM_LISTSET,
	DM_LISTSETMOUSEREACTION,

	DM_GETCURSORSIZE,
	DM_SETCURSORSIZE,

	DM_LISTGETDATASIZE,

	DM_GETSELECTION,
	DM_SETSELECTION,

	DM_GETEDITPOSITION,
	DM_SETEDITPOSITION,

	DM_SETCOMBOBOXEVENT,
	DM_GETCOMBOBOXEVENT,

	DM_GETCONSTTEXTPTR,
	DM_GETDLGITEMSHORT,
	DM_SETDLGITEMSHORT,

	DM_GETDIALOGINFO,

	DN_FIRST=0x1000,
	DN_BTNCLICK,
	DN_CTLCOLORDIALOG,
	DN_CTLCOLORDLGITEM,
	DN_CTLCOLORDLGLIST,
	DN_DRAWDIALOG,
	DN_DRAWDLGITEM,
	DN_EDITCHANGE,
	DN_ENTERIDLE,
	DN_GOTFOCUS,
	DN_HELP,
	DN_HOTKEY,
	DN_INITDIALOG,
	DN_KILLFOCUS,
	DN_LISTCHANGE,
	DN_DRAGGED,
	DN_RESIZECONSOLE,
	DN_DRAWDIALOGDONE,
	DN_LISTHOTKEY,
	DN_INPUT,
	DN_CONTROLINPUT,
	DN_CLOSE,

	DM_USER=0x4000,

#ifdef FAR_USE_INTERNALS
	DM_KILLSAVESCREEN=DN_FIRST-1,
	DM_ALLKEYMODE=DN_FIRST-2,
	DN_ACTIVATEAPP=DM_USER-1,
#endif // END FAR_USE_INTERNALS
};

enum FARCHECKEDSTATE
{
	BSTATE_UNCHECKED = 0,
	BSTATE_CHECKED   = 1,
	BSTATE_3STATE    = 2,
	BSTATE_TOGGLE    = 3,
};

enum FARLISTMOUSEREACTIONTYPE
{
	LMRT_ONLYFOCUS   = 0,
	LMRT_ALWAYS      = 1,
	LMRT_NEVER       = 2,
};

enum FARCOMBOBOXEVENTTYPE
{
	CBET_KEY         = 0x00000001,
	CBET_MOUSE       = 0x00000002,
};

typedef unsigned __int64 LISTITEMFLAGS;
static const LISTITEMFLAGS
	LIF_NONE               = 0,
	LIF_SELECTED           = 0x0000000000010000ULL,
	LIF_CHECKED            = 0x0000000000020000ULL,
	LIF_SEPARATOR          = 0x0000000000040000ULL,
	LIF_DISABLE            = 0x0000000000080000ULL,
	LIF_GRAYED             = 0x0000000000100000ULL,
	LIF_HIDDEN             = 0x0000000000200000ULL,
	LIF_DELETEUSERDATA     = 0x0000000080000000ULL;

struct FarListItem
{
	LISTITEMFLAGS Flags;
	const wchar_t *Text;
	DWORD Reserved[3];
};

struct FarListUpdate
{
	int Index;
	struct FarListItem Item;
};

struct FarListInsert
{
	int Index;
	struct FarListItem Item;
};

struct FarListGetItem
{
	int ItemIndex;
	struct FarListItem Item;
};

struct FarListPos
{
	int SelectPos;
	int TopPos;
};

typedef unsigned __int64 FARLISTFINDFLAGS;
static const FARLISTFINDFLAGS
	LIFIND_NONE       = 0,
	LIFIND_EXACTMATCH = 0x0000000000000001ULL;


struct FarListFind
{
	int StartIndex;
	const wchar_t *Pattern;
	FARLISTFINDFLAGS Flags;
	DWORD Reserved;
};

struct FarListDelete
{
	int StartIndex;
	int Count;
};

typedef unsigned __int64 FARLISTINFOFLAGS;
static const FARLISTINFOFLAGS
	LINFO_NONE                  = 0,
	LINFO_SHOWNOBOX             = 0x0000000000000400ULL,
	LINFO_AUTOHIGHLIGHT         = 0x0000000000000800ULL,
	LINFO_REVERSEHIGHLIGHT      = 0x0000000000001000ULL,
	LINFO_WRAPMODE              = 0x0000000000008000ULL,
	LINFO_SHOWAMPERSAND         = 0x0000000000010000ULL;

struct FarListInfo
{
	FARLISTINFOFLAGS Flags;
	int ItemsNumber;
	int SelectPos;
	int TopPos;
	int MaxHeight;
	int MaxLength;
	DWORD Reserved[6];
};

struct FarListItemData
{
	int   Index;
	int   DataSize;
	void *Data;
	DWORD Reserved;
};

struct FarList
{
	int ItemsNumber;
	struct FarListItem *Items;
};

struct FarListTitles
{
	int   TitleLen;
	const wchar_t *Title;
	int   BottomLen;
	const wchar_t *Bottom;
};

struct FarListColors
{
	unsigned __int64 Flags;
	DWORD  Reserved;
	int    ColorCount;
	LPBYTE Colors;
};

struct FarDialogItem
{
	FARDIALOGITEMTYPES Type;
	int X1,Y1,X2,Y2;
	union
	{
		DWORD_PTR Reserved;
		int Selected;
		struct FarList *ListItems;
		CHAR_INFO *VBuf;
	}
#ifndef __cplusplus
	Param
#endif
	;
	const wchar_t *History;
	const wchar_t *Mask;
	FARDIALOGITEMFLAGS Flags;
	LONG_PTR UserParam;

	const wchar_t *PtrData;
	size_t MaxLen; // terminate 0 not included (if == 0 string size is unlimited)
};

struct FarDialogItemData
{
	size_t  PtrLength;
	wchar_t *PtrData;
};

struct FarDialogEvent
{
	HANDLE hDlg;
	int Msg;
	int Param1;
	INT_PTR Param2;
	INT_PTR Result;
};

struct OpenDlgPluginData
{
	HANDLE hDlg;
};

struct DialogInfo
{
	size_t StructSize;
	GUID Id;
	GUID Owner;
};

#define Dlg_RedrawDialog(Info,hDlg)            Info.SendDlgMessage(hDlg,DM_REDRAW,0,0)

#define Dlg_GetDlgData(Info,hDlg)              Info.SendDlgMessage(hDlg,DM_GETDLGDATA,0,0)
#define Dlg_SetDlgData(Info,hDlg,Data)         Info.SendDlgMessage(hDlg,DM_SETDLGDATA,0,(INT_PTR)Data)

#define Dlg_GetDlgItemData(Info,hDlg,ID)       Info.SendDlgMessage(hDlg,DM_GETITEMDATA,0,0)
#define Dlg_SetDlgItemData(Info,hDlg,ID,Data)  Info.SendDlgMessage(hDlg,DM_SETITEMDATA,0,(INT_PTR)Data)

#define DlgItem_GetFocus(Info,hDlg)            Info.SendDlgMessage(hDlg,DM_GETFOCUS,0,0)
#define DlgItem_SetFocus(Info,hDlg,ID)         Info.SendDlgMessage(hDlg,DM_SETFOCUS,ID,0)
#define DlgItem_Enable(Info,hDlg,ID)           Info.SendDlgMessage(hDlg,DM_ENABLE,ID,TRUE)
#define DlgItem_Disable(Info,hDlg,ID)          Info.SendDlgMessage(hDlg,DM_ENABLE,ID,FALSE)
#define DlgItem_IsEnable(Info,hDlg,ID)         Info.SendDlgMessage(hDlg,DM_ENABLE,ID,-1)
#define DlgItem_SetText(Info,hDlg,ID,Str)      Info.SendDlgMessage(hDlg,DM_SETTEXTPTR,ID,(INT_PTR)Str)

#define DlgItem_GetCheck(Info,hDlg,ID)         Info.SendDlgMessage(hDlg,DM_GETCHECK,ID,0)
#define DlgItem_SetCheck(Info,hDlg,ID,State)   Info.SendDlgMessage(hDlg,DM_SETCHECK,ID,State)

#define DlgEdit_AddHistory(Info,hDlg,ID,Str)   Info.SendDlgMessage(hDlg,DM_ADDHISTORY,ID,(INT_PTR)Str)

#define DlgList_AddString(Info,hDlg,ID,Str)    Info.SendDlgMessage(hDlg,DM_LISTADDSTR,ID,(INT_PTR)Str)
#define DlgList_GetCurPos(Info,hDlg,ID)        Info.SendDlgMessage(hDlg,DM_LISTGETCURPOS,ID,0)
#define DlgList_SetCurPos(Info,hDlg,ID,NewPos) {struct FarListPos LPos={NewPos,-1};Info.SendDlgMessage(hDlg,DM_LISTSETCURPOS,ID,(INT_PTR)&LPos);}
#define DlgList_ClearList(Info,hDlg,ID)        Info.SendDlgMessage(hDlg,DM_LISTDELETE,ID,0)
#define DlgList_DeleteItem(Info,hDlg,ID,Index) {struct FarListDelete FLDItem={Index,1}; Info.SendDlgMessage(hDlg,DM_LISTDELETE,ID,(INT_PTR)&FLDItem);}
#define DlgList_SortUp(Info,hDlg,ID)           Info.SendDlgMessage(hDlg,DM_LISTSORT,ID,0)
#define DlgList_SortDown(Info,hDlg,ID)         Info.SendDlgMessage(hDlg,DM_LISTSORT,ID,1)
#define DlgList_GetItemData(Info,hDlg,ID,Index)          Info.SendDlgMessage(hDlg,DM_LISTGETDATA,ID,Index)
#define DlgList_SetItemStrAsData(Info,hDlg,ID,Index,Str) {struct FarListItemData FLID{Index,0,Str,0}; Info.SendDlgMessage(hDlg,DM_LISTSETDATA,ID,(INT_PTR)&FLID);}

typedef unsigned __int64 FARDIALOGFLAGS;
static const FARDIALOGFLAGS
	FDLG_WARNING             = 0x0000000000000001ULL,
	FDLG_SMALLDIALOG         = 0x0000000000000002ULL,
	FDLG_NODRAWSHADOW        = 0x0000000000000004ULL,
	FDLG_NODRAWPANEL         = 0x0000000000000008ULL,
	FDLG_KEEPCONSOLETITLE    = 0x0000000000000010ULL,
#ifdef FAR_USE_INTERNALS
	FDLG_NONMODAL            = 0x0000000000000020ULL,
#endif // END FAR_USE_INTERNALS
	FDLG_NONE                = 0;

typedef INT_PTR(WINAPI *FARWINDOWPROC)(
    HANDLE   hDlg,
    int Msg,
    int      Param1,
    INT_PTR Param2
);

typedef INT_PTR(WINAPI *FARAPISENDDLGMESSAGE)(
    HANDLE   hDlg,
    int Msg,
    int      Param1,
    INT_PTR Param2
);

typedef INT_PTR(WINAPI *FARAPIDEFDLGPROC)(
    HANDLE   hDlg,
    int Msg,
    int      Param1,
    INT_PTR Param2
);

typedef HANDLE(WINAPI *FARAPIDIALOGINIT)(
    const GUID*           PluginId,
    const GUID*           Id,
    int                   X1,
    int                   Y1,
    int                   X2,
    int                   Y2,
    const wchar_t        *HelpTopic,
    struct FarDialogItem *Item,
    unsigned int          ItemsNumber,
    DWORD                 Reserved,
    FARDIALOGFLAGS        Flags,
    FARWINDOWPROC         DlgProc,
    INT_PTR               Param
);

typedef int (WINAPI *FARAPIDIALOGRUN)(
    HANDLE hDlg
);

typedef void (WINAPI *FARAPIDIALOGFREE)(
    HANDLE hDlg
);

struct FarKey
{
    WORD VirtualKeyCode;
    DWORD ControlKeyState;
};

typedef unsigned __int64 MENUITEMFLAGS;
static const MENUITEMFLAGS
	MIF_SELECTED   = 0x000000000010000ULL,
	MIF_CHECKED    = 0x000000000020000ULL,
	MIF_SEPARATOR  = 0x000000000040000ULL,
	MIF_DISABLE    = 0x000000000080000ULL,
	MIF_GRAYED     = 0x000000000100000ULL,
	MIF_HIDDEN     = 0x000000000200000ULL,
#ifdef FAR_USE_INTERNALS
	MIF_SUBMENU    = 0x000000000400000ULL,
#endif // END FAR_USE_INTERNALS
	MIF_NONE       = 0;

struct FarMenuItem
{
	MENUITEMFLAGS Flags;
	const wchar_t *Text;
	DWORD AccelKey;
	DWORD Reserved;
	DWORD_PTR UserData;
};

typedef unsigned __int64 FARMENUFLAGS;
static const FARMENUFLAGS
	FMENU_SHOWAMPERSAND        = 0x0000000000000001ULL,
	FMENU_WRAPMODE             = 0x0000000000000002ULL,
	FMENU_AUTOHIGHLIGHT        = 0x0000000000000004ULL,
	FMENU_REVERSEAUTOHIGHLIGHT = 0x0000000000000008ULL,
	FMENU_CHANGECONSOLETITLE   = 0x0000000000000010ULL,
#ifdef FAR_USE_INTERNALS
	FMENU_SHOWNOBOX            = 0x0000000000000020ULL,
#endif // END FAR_USE_INTERNALS
	FMENU_NONE                 = 0;

typedef int (WINAPI *FARAPIMENU)(
	const GUID*         PluginId,
    int                 X,
    int                 Y,
    int                 MaxHeight,
    FARMENUFLAGS        Flags,
    const wchar_t      *Title,
    const wchar_t      *Bottom,
    const wchar_t      *HelpTopic,
    const struct FarKey *BreakKeys,
    int                *BreakCode,
    const struct FarMenuItem *Item,
    int                 ItemsNumber
);


typedef unsigned __int64 PLUGINPANELITEMFLAGS;
static const PLUGINPANELITEMFLAGS
	PPIF_NONE                   = 0,
	PPIF_USERDATA               = 0x0000000020000000ULL,
	PPIF_SELECTED               = 0x0000000040000000ULL,
	PPIF_PROCESSDESCR           = 0x0000000080000000ULL;

struct PluginPanelItem
{
	DWORD    FileAttributes;
	FILETIME CreationTime;
	FILETIME LastAccessTime;
	FILETIME LastWriteTime;
	FILETIME ChangeTime;
	unsigned __int64 FileSize;
	unsigned __int64 PackSize;
#ifdef FAR_USE_INTERNALS
	wchar_t *FileName;
	wchar_t *AlternateFileName;
#else // ELSE FAR_USE_INTERNALS
	const wchar_t *FileName;
	const wchar_t *AlternateFileName;
#endif // END FAR_USE_INTERNALS
	PLUGINPANELITEMFLAGS Flags;
	DWORD         NumberOfLinks;
	const wchar_t *Description;
	const wchar_t *Owner;
	const wchar_t * const *CustomColumnData;
	int           CustomColumnNumber;
	DWORD_PTR     UserData;
	DWORD         CRC32;
	DWORD_PTR     Reserved[2];
};

typedef unsigned __int64 PANELINFOFLAGS;
static const PANELINFOFLAGS
	PFLAGS_NONE               = 0,
	PFLAGS_SHOWHIDDEN         = 0x0000000000000001ULL,
	PFLAGS_HIGHLIGHT          = 0x0000000000000002ULL,
	PFLAGS_REVERSESORTORDER   = 0x0000000000000004ULL,
	PFLAGS_USESORTGROUPS      = 0x0000000000000008ULL,
	PFLAGS_SELECTEDFIRST      = 0x0000000000000010ULL,
	PFLAGS_REALNAMES          = 0x0000000000000020ULL,
	PFLAGS_NUMERICSORT        = 0x0000000000000040ULL,
	PFLAGS_PANELLEFT          = 0x0000000000000080ULL,
	PFLAGS_DIRECTORIESFIRST   = 0x0000000000000100ULL,
	PFLAGS_USECRC32           = 0x0000000000000200ULL,
	PFLAGS_CASESENSITIVESORT  = 0x0000000000000400ULL,
	PFLAGS_PLUGIN             = 0x0000000000000800ULL,
	PFLAGS_VISIBLE            = 0x0000000000001000ULL,
	PFLAGS_FOCUS              = 0x0000000000002000ULL,
	PFLAGS_ALTERNATIVENAMES   = 0x0000000000004000ULL;

enum PANELINFOTYPE
{
	PTYPE_FILEPANEL,
	PTYPE_TREEPANEL,
	PTYPE_QVIEWPANEL,
	PTYPE_INFOPANEL
};

enum OPENPANELINFO_SORTMODES
{
	SM_DEFAULT                   =  0,
	SM_UNSORTED                  =  1,
	SM_NAME                      =  2,
	SM_EXT                       =  3,
	SM_MTIME                     =  4,
	SM_CTIME                     =  5,
	SM_ATIME                     =  6,
	SM_SIZE                      =  7,
	SM_DESCR                     =  8,
	SM_OWNER                     =  9,
	SM_COMPRESSEDSIZE            = 10,
	SM_NUMLINKS                  = 11,
	SM_NUMSTREAMS                = 12,
	SM_STREAMSSIZE               = 13,
	SM_FULLNAME                  = 14,
	SM_CHTIME                    = 15,
};

struct PanelInfo
{
	size_t StructSize;
	GUID OwnerGuid;
	HANDLE PluginHandle;
	PANELINFOTYPE PanelType;
	RECT PanelRect;
	int ItemsNumber;
	int SelectedItemsNumber;
	int CurrentItem;
	int TopPanelItem;
	int ViewMode;
	OPENPANELINFO_SORTMODES SortMode;
	PANELINFOFLAGS Flags;
	DWORD Reserved;
};


struct PanelRedrawInfo
{
	int CurrentItem;
	int TopPanelItem;
};

struct CmdLineSelect
{
	int SelStart;
	int SelEnd;
};

#define PANEL_NONE    ((HANDLE)(-1))
#define PANEL_ACTIVE  ((HANDLE)(-1))
#define PANEL_PASSIVE ((HANDLE)(-2))

enum FILE_CONTROL_COMMANDS
{
	FCTL_CLOSEPANEL,
	FCTL_GETPANELINFO,
	FCTL_UPDATEPANEL,
	FCTL_REDRAWPANEL,
	FCTL_GETCMDLINE,
	FCTL_SETCMDLINE,
	FCTL_SETSELECTION,
	FCTL_SETVIEWMODE,
	FCTL_INSERTCMDLINE,
	FCTL_SETUSERSCREEN,
	FCTL_SETPANELDIR,
	FCTL_SETCMDLINEPOS,
	FCTL_GETCMDLINEPOS,
	FCTL_SETSORTMODE,
	FCTL_SETSORTORDER,
	FCTL_SETCMDLINESELECTION,
	FCTL_GETCMDLINESELECTION,
	FCTL_CHECKPANELSEXIST,
	FCTL_SETNUMERICSORT,
	FCTL_GETUSERSCREEN,
	FCTL_ISACTIVEPANEL,
	FCTL_GETPANELITEM,
	FCTL_GETSELECTEDPANELITEM,
	FCTL_GETCURRENTPANELITEM,
	FCTL_GETPANELDIR,
	FCTL_GETCOLUMNTYPES,
	FCTL_GETCOLUMNWIDTHS,
	FCTL_BEGINSELECTION,
	FCTL_ENDSELECTION,
	FCTL_CLEARSELECTION,
	FCTL_SETDIRECTORIESFIRST,
	FCTL_GETPANELFORMAT,
	FCTL_GETPANELHOSTFILE,
	FCTL_SETCASESENSITIVESORT,
};

typedef void (WINAPI *FARAPITEXT)(
    int X,
    int Y,
    int Color,
    const wchar_t *Str
);

typedef HANDLE(WINAPI *FARAPISAVESCREEN)(int X1, int Y1, int X2, int Y2);

typedef void (WINAPI *FARAPIRESTORESCREEN)(HANDLE hScreen);


typedef int (WINAPI *FARAPIGETDIRLIST)(
    const wchar_t *Dir,
    struct PluginPanelItem **pPanelItem,
    int *pItemsNumber
);

typedef int (WINAPI *FARAPIGETPLUGINDIRLIST)(
    const GUID* PluginId,
    HANDLE hPanel,
    const wchar_t *Dir,
    struct PluginPanelItem **pPanelItem,
    int *pItemsNumber
);

typedef void (WINAPI *FARAPIFREEDIRLIST)(struct PluginPanelItem *PanelItem, int nItemsNumber);
typedef void (WINAPI *FARAPIFREEPLUGINDIRLIST)(struct PluginPanelItem *PanelItem, int nItemsNumber);

typedef unsigned __int64 VIEWER_FLAGS;
static const VIEWER_FLAGS
	VF_NONE                  = 0,
	VF_NONMODAL              = 0x0000000000000001ULL,
	VF_DELETEONCLOSE         = 0x0000000000000002ULL,
	VF_ENABLE_F6             = 0x0000000000000004ULL,
	VF_DISABLEHISTORY        = 0x0000000000000008ULL,
	VF_IMMEDIATERETURN       = 0x0000000000000100ULL,
	VF_DELETEONLYFILEONCLOSE = 0x0000000000000200ULL;

typedef int (WINAPI *FARAPIVIEWER)(
    const wchar_t *FileName,
    const wchar_t *Title,
    int X1,
    int Y1,
    int X2,
    int Y2,
    VIEWER_FLAGS Flags,
    UINT CodePage
);

typedef unsigned __int64 EDITOR_FLAGS;
static const EDITOR_FLAGS
	EF_NONMODAL              = 0x0000000000000001ULL,
	EF_CREATENEW             = 0x0000000000000002ULL,
	EF_ENABLE_F6             = 0x0000000000000004ULL,
	EF_DISABLEHISTORY        = 0x0000000000000008ULL,
	EF_DELETEONCLOSE         = 0x0000000000000010ULL,
	EF_IMMEDIATERETURN       = 0x0000000000000100ULL,
	EF_DELETEONLYFILEONCLOSE = 0x0000000000000200ULL,
#ifdef FAR_USE_INTERNALS
	EF_LOCKED                = 0x0000000000000400ULL,

	EF_OPENMODE_MASK         = 0x00000000F0000000ULL,
	EF_OPENMODE_NEWIFOPEN    = 0x0000000010000000ULL,
	EF_OPENMODE_USEEXISTING  = 0x0000000020000000ULL,
	EF_OPENMODE_BREAKIFOPEN  = 0x0000000030000000ULL,
	EF_OPENMODE_RELOADIFOPEN = 0x0000000040000000ULL,
#endif // END FAR_USE_INTERNALS
#ifdef FAR_USE_INTERNALS
	EF_SERVICEREGION         = 0x0000000000001000ULL,
#endif // END FAR_USE_INTERNALS
	EN_NONE                  = 0;

enum EDITOR_EXITCODE
{
	EEC_OPEN_ERROR          = 0,
	EEC_MODIFIED            = 1,
	EEC_NOT_MODIFIED        = 2,
	EEC_LOADING_INTERRUPTED = 3,
#ifdef FAR_USE_INTERNALS
	EEC_OPENED_EXISTING     = 4,
	EEC_ALREADY_EXISTS      = 5,
	EEC_OPEN_NEWINSTANCE    = 6,
	EEC_RELOAD              = 7,
#endif // END FAR_USE_INTERNALS
};

typedef int (WINAPI *FARAPIEDITOR)(
    const wchar_t *FileName,
    const wchar_t *Title,
    int X1,
    int Y1,
    int X2,
    int Y2,
    EDITOR_FLAGS Flags,
    int StartLine,
    int StartChar,
    UINT CodePage
);

typedef const wchar_t*(WINAPI *FARAPIGETMSG)(
    const GUID* PluginId,
    int MsgId
);

typedef unsigned __int64 FARHELPFLAGS;
static const FARHELPFLAGS
	FHELP_NONE        = 0,
	FHELP_NOSHOWERROR = 0x0000000080000000ULL,
	FHELP_SELFHELP    = 0x0000000000000000ULL,
	FHELP_FARHELP     = 0x0000000000000001ULL,
	FHELP_CUSTOMFILE  = 0x0000000000000002ULL,
	FHELP_CUSTOMPATH  = 0x0000000000000004ULL,
	FHELP_USECONTENTS = 0x0000000040000000ULL;

typedef BOOL (WINAPI *FARAPISHOWHELP)(
    const wchar_t *ModuleName,
    const wchar_t *Topic,
    FARHELPFLAGS Flags
);

enum ADVANCED_CONTROL_COMMANDS
{
	ACTL_GETFARVERSION,
	ACTL_GETSYSWORDDIV,
	ACTL_WAITKEY,
	ACTL_GETCOLOR,
	ACTL_GETARRAYCOLOR,
	ACTL_EJECTMEDIA,
	ACTL_GETWINDOWINFO,
	ACTL_GETWINDOWCOUNT,
	ACTL_SETCURRENTWINDOW,
	ACTL_COMMIT,
	ACTL_GETFARHWND,
	ACTL_GETSYSTEMSETTINGS,
	ACTL_GETPANELSETTINGS,
	ACTL_GETINTERFACESETTINGS,
	ACTL_GETCONFIRMATIONS,
	ACTL_GETDESCSETTINGS,
	ACTL_SETARRAYCOLOR,
	ACTL_GETPLUGINMAXREADDATA,
	ACTL_GETDIALOGSETTINGS,
	ACTL_REDRAWALL,
	ACTL_SYNCHRO,
	ACTL_SETPROGRESSSTATE,
	ACTL_SETPROGRESSVALUE,
	ACTL_QUIT,
	ACTL_GETFARRECT,
	ACTL_GETCURSORPOS,
	ACTL_SETCURSORPOS,
	ACTL_PROGRESSNOTIFY,
	ACTL_GETWINDOWTYPE,

#ifdef FAR_USE_INTERNALS
	ACTL_REMOVEMEDIA,
	ACTL_GETMEDIATYPE,
#endif // END FAR_USE_INTERNALS

};

enum FarSystemSettings
{
	FSS_CLEARROATTRIBUTE               = 0x00000001,
	FSS_DELETETORECYCLEBIN             = 0x00000002,
	FSS_USESYSTEMCOPYROUTINE           = 0x00000004,
	FSS_COPYFILESOPENEDFORWRITING      = 0x00000008,
	FSS_CREATEFOLDERSINUPPERCASE       = 0x00000010,
	FSS_SAVECOMMANDSHISTORY            = 0x00000020,
	FSS_SAVEFOLDERSHISTORY             = 0x00000040,
	FSS_SAVEVIEWANDEDITHISTORY         = 0x00000080,
	FSS_USEWINDOWSREGISTEREDTYPES      = 0x00000100,
	FSS_AUTOSAVESETUP                  = 0x00000200,
	FSS_SCANSYMLINK                    = 0x00000400,
};

enum FarPanelSettings
{
	FPS_SHOWHIDDENANDSYSTEMFILES       = 0x00000001,
	FPS_HIGHLIGHTFILES                 = 0x00000002,
	FPS_AUTOCHANGEFOLDER               = 0x00000004,
	FPS_SELECTFOLDERS                  = 0x00000008,
	FPS_ALLOWREVERSESORTMODES          = 0x00000010,
	FPS_SHOWCOLUMNTITLES               = 0x00000020,
	FPS_SHOWSTATUSLINE                 = 0x00000040,
	FPS_SHOWFILESTOTALINFORMATION      = 0x00000080,
	FPS_SHOWFREESIZE                   = 0x00000100,
	FPS_SHOWSCROLLBAR                  = 0x00000200,
	FPS_SHOWBACKGROUNDSCREENSNUMBER    = 0x00000400,
	FPS_SHOWSORTMODELETTER             = 0x00000800,
};

enum FarDialogSettings
{
	FDIS_HISTORYINDIALOGEDITCONTROLS    = 0x00000001,
	FDIS_PERSISTENTBLOCKSINEDITCONTROLS = 0x00000002,
	FDIS_AUTOCOMPLETEININPUTLINES       = 0x00000004,
	FDIS_BSDELETEUNCHANGEDTEXT          = 0x00000008,
	FDIS_DELREMOVESBLOCKS               = 0x00000010,
	FDIS_MOUSECLICKOUTSIDECLOSESDIALOG  = 0x00000020,
};

enum FarInterfaceSettings
{
	FIS_CLOCKINPANELS                  = 0x00000001,
	FIS_CLOCKINVIEWERANDEDITOR         = 0x00000002,
	FIS_MOUSE                          = 0x00000004,
	FIS_SHOWKEYBAR                     = 0x00000008,
	FIS_ALWAYSSHOWMENUBAR              = 0x00000010,
	FIS_SHOWTOTALCOPYPROGRESSINDICATOR = 0x00000100,
	FIS_SHOWCOPYINGTIMEINFO            = 0x00000200,
	FIS_USECTRLPGUPTOCHANGEDRIVE       = 0x00000800,
	FIS_SHOWTOTALDELPROGRESSINDICATOR  = 0x00001000,
};

enum FarConfirmationsSettings
{
	FCS_COPYOVERWRITE                  = 0x00000001,
	FCS_MOVEOVERWRITE                  = 0x00000002,
	FCS_DRAGANDDROP                    = 0x00000004,
	FCS_DELETE                         = 0x00000008,
	FCS_DELETENONEMPTYFOLDERS          = 0x00000010,
	FCS_INTERRUPTOPERATION             = 0x00000020,
	FCS_DISCONNECTNETWORKDRIVE         = 0x00000040,
	FCS_RELOADEDITEDFILE               = 0x00000080,
	FCS_CLEARHISTORYLIST               = 0x00000100,
	FCS_EXIT                           = 0x00000200,
	FCS_OVERWRITEDELETEROFILES         = 0x00000400,
};

enum FarDescriptionSettings
{
	FDS_UPDATEALWAYS                   = 0x00000001,
	FDS_UPDATEIFDISPLAYED              = 0x00000002,
	FDS_SETHIDDEN                      = 0x00000004,
	FDS_UPDATEREADONLY                 = 0x00000008,
};

typedef unsigned __int64 FAREJECTMEDIAFLAGS;
static const FAREJECTMEDIAFLAGS
	EJECT_NO_MESSAGE                    = 0x0000000000000001ULL,
	EJECT_LOAD_MEDIA                    = 0x0000000000000002ULL,
#ifdef FAR_USE_INTERNALS
	EJECT_NOTIFY_AFTERREMOVE            = 0x0000000000000004ULL,
	EJECT_READY                         = 0x0000000080000000ULL,
#endif // END FAR_USE_INTERNALS
	EJECT_NONE                          = 0;

struct ActlEjectMedia
{
	DWORD Letter;
	FAREJECTMEDIAFLAGS Flags;
};

#ifdef FAR_USE_INTERNALS
enum FARMEDIATYPE
{
	FMT_DRIVE_ERROR                =  -1,
	FMT_DRIVE_UNKNOWN              =  DRIVE_UNKNOWN,
	FMT_DRIVE_NO_ROOT_DIR          =  DRIVE_NO_ROOT_DIR,
	FMT_DRIVE_REMOVABLE            =  DRIVE_REMOVABLE,
	FMT_DRIVE_FIXED                =  DRIVE_FIXED,
	FMT_DRIVE_REMOTE               =  DRIVE_REMOTE,
	FMT_DRIVE_CDROM                =  DRIVE_CDROM,
	FMT_DRIVE_RAMDISK              =  DRIVE_RAMDISK,
	FMT_DRIVE_SUBSTITUTE           =  15,
	FMT_DRIVE_REMOTE_NOT_CONNECTED =  16,
	FMT_DRIVE_CD_RW                =  18,
	FMT_DRIVE_CD_RWDVD             =  19,
	FMT_DRIVE_DVD_ROM              =  20,
	FMT_DRIVE_DVD_RW               =  21,
	FMT_DRIVE_DVD_RAM              =  22,
	FMT_DRIVE_USBDRIVE             =  40,
	FMT_DRIVE_NOT_INIT             = 255,
};

typedef unsigned __int64 FARMEDIATYPEFLAGS;
static const FARMEDIATYPEFLAGS
	MEDIATYPE_NONE                      = 0,
	MEDIATYPE_NODETECTCDROM             = 0x0000000080000000ULL;

struct ActlMediaType
{
	DWORD Letter;
	FARMEDIATYPEFLAGS Flags;
	DWORD Reserved[2];
};
#endif // END FAR_USE_INTERNALS


enum FAR_MACRO_CONTROL_COMMANDS
{
	MCTL_LOADALL           = 0,
	MCTL_SAVEALL           = 1,
	MCTL_SENDSTRING        = 2,
	MCTL_GETSTATE          = 5,
	MCTL_GETAREA           = 6,
};

typedef unsigned __int64 FARKEYMACROFLAGS;
static const FARKEYMACROFLAGS
	KMFLAGS_NONE                = 0,
	KMFLAGS_DISABLEOUTPUT       = 0x0000000000000001,
	KMFLAGS_NOSENDKEYSTOPLUGINS = 0x0000000000000002,
	KMFLAGS_REG_MULTI_SZ        = 0x0000000000100000,
	KMFLAGS_SILENTCHECK         = 0x0000000000000001;

enum FARMACROSENDSTRINGCOMMAND
{
	MSSC_POST              =0,
#ifdef FAR_USE_INTERNALS
	MSSC_EXEC              =1,
#endif // END FAR_USE_INTERNALS
	MSSC_CHECK             =2,
};

enum FARMACROAREA
{
	MACROAREA_OTHER             = 0,
	MACROAREA_SHELL             = 1,
	MACROAREA_VIEWER            = 2,
	MACROAREA_EDITOR            = 3,
	MACROAREA_DIALOG            = 4,
	MACROAREA_SEARCH            = 5,
	MACROAREA_DISKS             = 6,
	MACROAREA_MAINMENU          = 7,
	MACROAREA_MENU              = 8,
	MACROAREA_HELP              = 9,
	MACROAREA_INFOPANEL         =10,
	MACROAREA_QVIEWPANEL        =11,
	MACROAREA_TREEPANEL         =12,
	MACROAREA_FINDFOLDER        =13,
	MACROAREA_USERMENU          =14,
	MACROAREA_AUTOCOMPLETION    =15,
};

enum FARMACROSTATE
{
	MACROSTATE_NOMACRO          = 0,
	MACROSTATE_EXECUTING        = 1,
	MACROSTATE_EXECUTING_COMMON = 2,
	MACROSTATE_RECORDING        = 3,
	MACROSTATE_RECORDING_COMMON = 4,
};

enum FARMACROPARSEERRORCODE
{
	MPEC_SUCCESS                = 0,
	MPEC_UNRECOGNIZED_KEYWORD   = 1,
	MPEC_UNRECOGNIZED_FUNCTION  = 2,
	MPEC_FUNC_PARAM             = 3,
	MPEC_NOT_EXPECTED_ELSE      = 4,
	MPEC_NOT_EXPECTED_END       = 5,
	MPEC_UNEXPECTED_EOS         = 6,
	MPEC_EXPECTED_TOKEN         = 7,
	MPEC_BAD_HEX_CONTROL_CHAR   = 8,
	MPEC_BAD_CONTROL_CHAR       = 9,
	MPEC_VAR_EXPECTED           =10,
	MPEC_EXPR_EXPECTED          =11,
	MPEC_ZEROLENGTHMACRO        =12,
	MPEC_INTPARSERERROR         =13,
	MPEC_CONTINUE_OTL           =14,
};

struct MacroParseResult
{
	size_t StructSize;
	DWORD ErrCode;
	COORD ErrPos;
	const wchar_t *ErrSrc;
};


struct MacroSendMacroText
{
	size_t StructSize;
	FARKEYMACROFLAGS Flags;
	DWORD AKey;
	const wchar_t *SequenceText;
};

struct MacroCheckMacroText
{
	union
	{
		struct MacroSendMacroText Text;
		struct MacroParseResult   Result;
	}
#ifndef __cplusplus
	Check
#endif
	;
};

#ifdef FAR_USE_INTERNALS
#if defined(PROCPLUGINMACROFUNC)
enum FARMACROVARTYPE
{
	FMVT_INTEGER                = 0,
	FMVT_STRING                 = 1,
	FMVT_DOUBLE                 = 2,
};

struct FarMacroValue
{
	FARMACROVARTYPE type;
	union
	{
		__int64  i;
		double   d;
		const wchar_t *s;
	}
#ifndef __cplusplus
	Value
#endif
	;
};

struct FarMacroFunction
{
	unsigned __int64 Flags;
	const wchar_t *Name;
	int nParam;
	int oParam;
	const wchar_t *Syntax;
	const wchar_t *Description;
};
#endif
#endif // END FAR_USE_INTERNALS

typedef unsigned __int64 FARSETCOLORFLAGS;
static const FARSETCOLORFLAGS

	FSETCLR_NONE                   = 0,
	FSETCLR_REDRAW                 = 0x0000000000000001ULL;

struct FarSetColors
{
	FARSETCOLORFLAGS Flags;
	int StartIndex;
	int ColorCount;
	LPBYTE Colors;
};

enum WINDOWINFO_TYPE
{
#ifdef FAR_USE_INTERNALS
	WTYPE_VIRTUAL,
	// ??????? ?? ???????? ???????????????? ?????????
	// WTYPE_* ? MODALTYPE_* (frame.hpp)!!!
	// (? ?? ???? ??????? ???? ???????????, ???? ???????? ?? ????????? ;)
#endif // END FAR_USE_INTERNALS
	WTYPE_PANELS=1,
	WTYPE_VIEWER,
	WTYPE_EDITOR,
	WTYPE_DIALOG,
	WTYPE_VMENU,
	WTYPE_HELP,
#ifdef FAR_USE_INTERNALS
	WTYPE_COMBOBOX,
	WTYPE_FINDFOLDER,
	WTYPE_USER,
#endif // END FAR_USE_INTERNALS
};

typedef unsigned __int64 WINDOWINFO_FLAGS;
static const WINDOWINFO_FLAGS
	WIF_MODIFIED = 0x0000000000000001ULL,
	WIF_CURRENT  = 0x0000000000000002ULL;

struct WindowInfo
{
	size_t StructSize;
	INT_PTR Id;
	int  Pos;
	WINDOWINFO_TYPE Type;
	WINDOWINFO_FLAGS Flags;
	wchar_t *TypeName;
	int TypeNameSize;
	wchar_t *Name;
	int NameSize;
};

struct WindowType
{
	size_t StructSize;
	long Type;
};

enum PROGRESSTATE
{
	PS_NOPROGRESS   =0x0,
	PS_INDETERMINATE=0x1,
	PS_NORMAL       =0x2,
	PS_ERROR        =0x4,
	PS_PAUSED       =0x8,
};

struct ProgressValue
{
	unsigned __int64 Completed;
	unsigned __int64 Total;
};

enum VIEWER_CONTROL_COMMANDS
{
	VCTL_GETINFO,
	VCTL_QUIT,
	VCTL_REDRAW,
	VCTL_SETKEYBAR,
	VCTL_SETPOSITION,
	VCTL_SELECT,
	VCTL_SETMODE,
};

typedef unsigned __int64 VIEWER_OPTIONS;
static const VIEWER_OPTIONS
	VOPT_NONE               = 0x0000000000000000ULL,
	VOPT_SAVEFILEPOSITION   = 0x0000000000000001ULL,
	VOPT_AUTODETECTCODEPAGE = 0x0000000000000002ULL;

enum VIEWER_SETMODE_TYPES
{
	VSMT_HEX,
	VSMT_WRAP,
	VSMT_WORDWRAP,
};

typedef unsigned __int64 VIEWER_SETMODEFLAGS_TYPES;
static const VIEWER_SETMODEFLAGS_TYPES
	VSMFL_REDRAW    = 0x0000000000000001ULL;

struct ViewerSetMode
{
	VIEWER_SETMODE_TYPES Type;
	union
	{
		int iParam;
		wchar_t *wszParam;
	}
#ifndef __cplusplus
	Param
#endif
	;
	VIEWER_SETMODEFLAGS_TYPES Flags;
	DWORD Reserved;
};

struct ViewerSelect
{
	__int64 BlockStartPos;
	int     BlockLen;
};

typedef unsigned __int64 VIEWER_SETPOS_FLAGS;
static const VIEWER_SETPOS_FLAGS
	VSP_NOREDRAW    = 0x0000000000000001ULL,
	VSP_PERCENT     = 0x0000000000000002ULL,
	VSP_RELATIVE    = 0x0000000000000004ULL,
	VSP_NORETNEWPOS = 0x0000000000000008ULL;

struct ViewerSetPosition
{
	VIEWER_SETPOS_FLAGS Flags;
	__int64 StartPos;
	__int64 LeftPos;
};

struct ViewerMode
{
	UINT CodePage;
	int Wrap;
	int WordWrap;
	int Hex;
	DWORD Reserved[4];
};

struct ViewerInfo
{
	size_t StructSize;
	int    ViewerID;
	const wchar_t *FileName;
	__int64 FileSize;
	__int64 FilePos;
	int    WindowSizeX;
	int    WindowSizeY;
	VIEWER_OPTIONS  Options;
	int    TabSize;
	struct ViewerMode CurMode;
	__int64 LeftPos;
};

enum VIEWER_EVENTS
{
	VE_READ       =0,
	VE_CLOSE      =1,

	VE_GOTFOCUS   =6,
	VE_KILLFOCUS  =7,
};


enum EDITOR_EVENTS
{
	EE_READ       =0,
	EE_SAVE       =1,
	EE_REDRAW     =2,
	EE_CLOSE      =3,

	EE_GOTFOCUS   =6,
	EE_KILLFOCUS  =7,
};

enum DIALOG_EVENTS
{
	DE_DLGPROCINIT    =0,
	DE_DEFDLGPROCINIT =1,
	DE_DLGPROCEND     =2,
};

enum SYNCHRO_EVENTS
{
	SE_COMMONSYNCHRO  =0,
};

#define EEREDRAW_ALL    (void*)0
#define EEREDRAW_CHANGE (void*)1
#define EEREDRAW_LINE   (void*)2

enum EDITOR_CONTROL_COMMANDS
{
	ECTL_GETSTRING,
	ECTL_SETSTRING,
	ECTL_INSERTSTRING,
	ECTL_DELETESTRING,
	ECTL_DELETECHAR,
	ECTL_INSERTTEXT,
	ECTL_GETINFO,
	ECTL_SETPOSITION,
	ECTL_SELECT,
	ECTL_REDRAW,
	ECTL_TABTOREAL,
	ECTL_REALTOTAB,
	ECTL_EXPANDTABS,
	ECTL_SETTITLE,
	ECTL_READINPUT,
	ECTL_PROCESSINPUT,
	ECTL_ADDCOLOR,
	ECTL_GETCOLOR,
	ECTL_SAVEFILE,
	ECTL_QUIT,
	ECTL_SETKEYBAR,
	ECTL_PROCESSKEY,
	ECTL_SETPARAM,
	ECTL_GETBOOKMARKS,
	ECTL_TURNOFFMARKINGBLOCK,
	ECTL_DELETEBLOCK,
	ECTL_ADDSTACKBOOKMARK,
	ECTL_PREVSTACKBOOKMARK,
	ECTL_NEXTSTACKBOOKMARK,
	ECTL_CLEARSTACKBOOKMARKS,
	ECTL_DELETESTACKBOOKMARK,
	ECTL_GETSTACKBOOKMARKS,
	ECTL_UNDOREDO,
	ECTL_GETFILENAME,
#ifdef FAR_USE_INTERNALS
	ECTL_SERVICEREGION,
#endif // END FAR_USE_INTERNALS
};

enum EDITOR_SETPARAMETER_TYPES
{
	ESPT_TABSIZE,
	ESPT_EXPANDTABS,
	ESPT_AUTOINDENT,
	ESPT_CURSORBEYONDEOL,
	ESPT_CHARCODEBASE,
	ESPT_CODEPAGE,
	ESPT_SAVEFILEPOSITION,
	ESPT_LOCKMODE,
	ESPT_SETWORDDIV,
	ESPT_GETWORDDIV,
	ESPT_SHOWWHITESPACE,
	ESPT_SETBOM,
};

#ifdef FAR_USE_INTERNALS
struct EditorServiceRegion
{
	int Command;
	unsigned __int64 Flags;
};
#endif // END FAR_USE_INTERNALS


struct EditorSetParameter
{
	EDITOR_SETPARAMETER_TYPES Type;
	union
	{
		int iParam;
		wchar_t *wszParam;
		DWORD Reserved1;
	}
#ifndef __cplusplus
	Param
#endif
	;
	unsigned __int64 Flags;
	DWORD Size;
};


enum EDITOR_UNDOREDO_COMMANDS
{
	EUR_BEGIN,
	EUR_END,
	EUR_UNDO,
	EUR_REDO
};


struct EditorUndoRedo
{
	EDITOR_UNDOREDO_COMMANDS Command;
	DWORD_PTR Reserved[3];
};

struct EditorGetString
{
	int StringNumber;
#ifdef FAR_USE_INTERNALS
	wchar_t *StringText;
	wchar_t *StringEOL;
#else // ELSE FAR_USE_INTERNALS
	const wchar_t *StringText;
	const wchar_t *StringEOL;
#endif // END FAR_USE_INTERNALS
	int StringLength;
	int SelStart;
	int SelEnd;
};


struct EditorSetString
{
	int StringNumber;
	const wchar_t *StringText;
	const wchar_t *StringEOL;
	int StringLength;
};

enum EXPAND_TABS
{
	EXPAND_NOTABS,
	EXPAND_ALLTABS,
	EXPAND_NEWTABS
};


enum EDITOR_OPTIONS
{
	EOPT_EXPANDALLTABS     = 0x00000001,
	EOPT_PERSISTENTBLOCKS  = 0x00000002,
	EOPT_DELREMOVESBLOCKS  = 0x00000004,
	EOPT_AUTOINDENT        = 0x00000008,
	EOPT_SAVEFILEPOSITION  = 0x00000010,
	EOPT_AUTODETECTCODEPAGE= 0x00000020,
	EOPT_CURSORBEYONDEOL   = 0x00000040,
	EOPT_EXPANDONLYNEWTABS = 0x00000080,
	EOPT_SHOWWHITESPACE    = 0x00000100,
	EOPT_BOM               = 0x00000200,
};


enum EDITOR_BLOCK_TYPES
{
	BTYPE_NONE,
	BTYPE_STREAM,
	BTYPE_COLUMN
};

enum EDITOR_CURRENTSTATE
{
	ECSTATE_MODIFIED       = 0x00000001,
	ECSTATE_SAVED          = 0x00000002,
	ECSTATE_LOCKED         = 0x00000004,
};


struct EditorInfo
{
	int EditorID;
	int WindowSizeX;
	int WindowSizeY;
	int TotalLines;
	int CurLine;
	int CurPos;
	int CurTabPos;
	int TopScreenLine;
	int LeftPos;
	int Overtype;
	int BlockType;
	int BlockStartLine;
	DWORD Options;
	int TabSize;
	int BookMarkCount;
	DWORD CurState;
	UINT CodePage;
	DWORD Reserved[5];
};

struct EditorBookMarks
{
	long *Line;
	long *Cursor;
	long *ScreenLine;
	long *LeftPos;
	DWORD Reserved[4];
};

struct EditorSetPosition
{
	int CurLine;
	int CurPos;
	int CurTabPos;
	int TopScreenLine;
	int LeftPos;
	int Overtype;
};


struct EditorSelect
{
	int BlockType;
	int BlockStartLine;
	int BlockStartPos;
	int BlockWidth;
	int BlockHeight;
};


struct EditorConvertPos
{
	int StringNumber;
	int SrcPos;
	int DestPos;
};


typedef unsigned __int64 EDITORCOLORFLAGS;
static const EDITORCOLORFLAGS
	ECF_TAB1 = 0x0000000000000001ULL;

struct EditorColor
{
	size_t StructSize;
	int StringNumber;
	int ColorItem;
	int StartPos;
	int EndPos;
	EDITORCOLORFLAGS Flags;
	FarColor Color;
};

struct EditorSaveFile
{
	const wchar_t *FileName;
	const wchar_t *FileEOL;
	UINT CodePage;
};

typedef unsigned __int64 INPUTBOXFLAGS;
static const INPUTBOXFLAGS
	FIB_ENABLEEMPTY      = 0x0000000000000001ULL,
	FIB_PASSWORD         = 0x0000000000000002ULL,
	FIB_EXPANDENV        = 0x0000000000000004ULL,
	FIB_NOUSELASTHISTORY = 0x0000000000000008ULL,
	FIB_BUTTONS          = 0x0000000000000010ULL,
	FIB_NOAMPERSAND      = 0x0000000000000020ULL,
	FIB_EDITPATH         = 0x0000000000000040ULL,
#ifdef FAR_USE_INTERNALS
	FIB_CHECKBOX         = 0x0000000000010000ULL,
#endif // END FAR_USE_INTERNALS
	FIB_NONE             = 0;

typedef int (WINAPI *FARAPIINPUTBOX)(
    const GUID* PluginId,
    const wchar_t *Title,
    const wchar_t *SubTitle,
    const wchar_t *HistoryName,
    const wchar_t *SrcText,
    wchar_t *DestText,
    int   DestLength,
    const wchar_t *HelpTopic,
    INPUTBOXFLAGS Flags
);

enum FAR_PLUGINS_CONTROL_COMMANDS
{
	PCTL_LOADPLUGIN         = 0,
	PCTL_UNLOADPLUGIN       = 1,
	PCTL_FORCEDLOADPLUGIN   = 2,
};

enum FAR_PLUGIN_LOAD_TYPE
{
	PLT_PATH = 0,
};

enum FAR_FILE_FILTER_CONTROL_COMMANDS
{
	FFCTL_CREATEFILEFILTER = 0,
	FFCTL_FREEFILEFILTER,
	FFCTL_OPENFILTERSMENU,
	FFCTL_STARTINGTOFILTER,
	FFCTL_ISFILEINFILTER,
};

enum FAR_FILE_FILTER_TYPE
{
	FFT_PANEL = 0,
	FFT_FINDFILE,
	FFT_COPY,
	FFT_SELECT,
	FFT_CUSTOM,
};

enum FAR_REGEXP_CONTROL_COMMANDS
{
	RECTL_CREATE=0,
	RECTL_FREE,
	RECTL_COMPILE,
	RECTL_OPTIMIZE,
	RECTL_MATCHEX,
	RECTL_SEARCHEX,
	RECTL_BRACKETSCOUNT
};

struct RegExpMatch
{
	int start,end;
};

struct RegExpSearch
{
	const wchar_t* Text;
	int Position;
	int Length;
	struct RegExpMatch* Match;
	int Count;
	void* Reserved;
};

enum FAR_SETTINGS_CONTROL_COMMANDS
{
	SCTL_CREATE=0,
	SCTL_FREE,
	SCTL_SET,
	SCTL_GET,
	SCTL_ENUM,
	SCTL_DELETE,
	SCTL_SUBKEY
};

enum FARSETTINGSTYPES
{
	FST_UNKNOWN,
	FST_SUBKEY,
	FST_QWORD,
	FST_STRING,
	FST_DATA
};

struct FarSettingsCreate
{
	size_t StructSize;
	GUID Guid;
	HANDLE Handle;
};

struct FarSettingsItem
{
	size_t Root;
	const wchar_t* Name;
	FARSETTINGSTYPES Type;
	union
	{
		unsigned __int64 Number;
		const wchar_t* String;
		struct
		{
			size_t Size;
			const void* Data;
		} Data;
	}
#ifndef __cplusplus
	Value
#endif
	;
};

struct FarSettingsName
{
	const wchar_t* Name;
	FARSETTINGSTYPES Type;
};

struct FarSettingsEnum
{
	size_t Root;
	size_t Count;
	const FarSettingsName* Items;
};

struct FarSettingsValue
{
	size_t Root;
	const wchar_t* Value;
};

typedef int (WINAPI *FARAPICONTROL)(
    HANDLE hPanel,
    FILE_CONTROL_COMMANDS Command,
    int Param1,
    INT_PTR Param2
);

typedef INT_PTR(WINAPI *FARAPIADVCONTROL)(
    const GUID* PluginId,
    ADVANCED_CONTROL_COMMANDS Command,
    void *Param
);

typedef int (WINAPI *FARAPIVIEWERCONTROL)(
    int ViewerID,
    VIEWER_CONTROL_COMMANDS Command,
    int Param1,
    INT_PTR Param2
);

typedef int (WINAPI *FARAPIEDITORCONTROL)(
    int EditorID,
    EDITOR_CONTROL_COMMANDS Command,
    int Param1,
    INT_PTR Param2
);

typedef int (WINAPI *FARAPIMACROCONTROL)(
    HANDLE hHandle,
    FAR_MACRO_CONTROL_COMMANDS Command,
    int Param1,
    INT_PTR Param2
);

typedef int (WINAPI *FARAPIPLUGINSCONTROL)(
    HANDLE hHandle,
    FAR_PLUGINS_CONTROL_COMMANDS Command,
    int Param1,
    INT_PTR Param2
);

typedef int (WINAPI *FARAPIFILEFILTERCONTROL)(
    HANDLE hHandle,
    FAR_FILE_FILTER_CONTROL_COMMANDS Command,
    int Param1,
    INT_PTR Param2
);

typedef int (WINAPI *FARAPIREGEXPCONTROL)(
    HANDLE hHandle,
    FAR_REGEXP_CONTROL_COMMANDS Command,
    int Param1,
    INT_PTR Param2
);

typedef int (WINAPI *FARAPISETTINGSCONTROL)(
    HANDLE hHandle,
    FAR_SETTINGS_CONTROL_COMMANDS Command,
    int Param1,
    INT_PTR Param2
);

// <C&C++>
typedef int (WINAPIV *FARSTDSPRINTF)(wchar_t *Buffer,const wchar_t *Format,...);
typedef int (WINAPIV *FARSTDSNPRINTF)(wchar_t *Buffer,size_t Sizebuf,const wchar_t *Format,...);
typedef int (WINAPIV *FARSTDSSCANF)(const wchar_t *Buffer, const wchar_t *Format,...);
// </C&C++>
typedef void (WINAPI *FARSTDQSORT)(void *base, size_t nelem, size_t width, int (__cdecl *fcmp)(const void *, const void *));
typedef void (WINAPI *FARSTDQSORTEX)(void *base, size_t nelem, size_t width, int (__cdecl *fcmp)(const void *, const void *,void *userparam),void *userparam);
typedef void   *(WINAPI *FARSTDBSEARCH)(const void *key, const void *base, size_t nelem, size_t width, int (__cdecl *fcmp)(const void *, const void *));
typedef int (WINAPI *FARSTDGETFILEOWNER)(const wchar_t *Computer,const wchar_t *Name,wchar_t *Owner,int Size);
typedef int (WINAPI *FARSTDGETNUMBEROFLINKS)(const wchar_t *Name);
typedef int (WINAPI *FARSTDATOI)(const wchar_t *s);
typedef __int64(WINAPI *FARSTDATOI64)(const wchar_t *s);
typedef wchar_t   *(WINAPI *FARSTDITOA64)(__int64 value, wchar_t *string, int radix);
typedef wchar_t   *(WINAPI *FARSTDITOA)(int value, wchar_t *string, int radix);
typedef wchar_t   *(WINAPI *FARSTDLTRIM)(wchar_t *Str);
typedef wchar_t   *(WINAPI *FARSTDRTRIM)(wchar_t *Str);
typedef wchar_t   *(WINAPI *FARSTDTRIM)(wchar_t *Str);
typedef wchar_t   *(WINAPI *FARSTDTRUNCSTR)(wchar_t *Str,int MaxLength);
typedef wchar_t   *(WINAPI *FARSTDTRUNCPATHSTR)(wchar_t *Str,int MaxLength);
typedef wchar_t   *(WINAPI *FARSTDQUOTESPACEONLY)(wchar_t *Str);
typedef const wchar_t*(WINAPI *FARSTDPOINTTONAME)(const wchar_t *Path);
typedef int (WINAPI *FARSTDGETPATHROOT)(const wchar_t *Path,wchar_t *Root, int DestSize);
typedef BOOL (WINAPI *FARSTDADDENDSLASH)(wchar_t *Path);
typedef int (WINAPI *FARSTDCOPYTOCLIPBOARD)(const wchar_t *Data);
typedef wchar_t *(WINAPI *FARSTDPASTEFROMCLIPBOARD)(void);
typedef int (WINAPI *FARSTDINPUTRECORDTOKEY)(const INPUT_RECORD *r);
typedef int (WINAPI *FARSTDKEYTOINPUTRECORD)(int Key,INPUT_RECORD *r);
typedef int (WINAPI *FARSTDLOCALISLOWER)(wchar_t Ch);
typedef int (WINAPI *FARSTDLOCALISUPPER)(wchar_t Ch);
typedef int (WINAPI *FARSTDLOCALISALPHA)(wchar_t Ch);
typedef int (WINAPI *FARSTDLOCALISALPHANUM)(wchar_t Ch);
typedef wchar_t (WINAPI *FARSTDLOCALUPPER)(wchar_t LowerChar);
typedef wchar_t (WINAPI *FARSTDLOCALLOWER)(wchar_t UpperChar);
typedef void (WINAPI *FARSTDLOCALUPPERBUF)(wchar_t *Buf,int Length);
typedef void (WINAPI *FARSTDLOCALLOWERBUF)(wchar_t *Buf,int Length);
typedef void (WINAPI *FARSTDLOCALSTRUPR)(wchar_t *s1);
typedef void (WINAPI *FARSTDLOCALSTRLWR)(wchar_t *s1);
typedef int (WINAPI *FARSTDLOCALSTRICMP)(const wchar_t *s1,const wchar_t *s2);
typedef int (WINAPI *FARSTDLOCALSTRNICMP)(const wchar_t *s1,const wchar_t *s2,int n);

typedef unsigned __int64 PROCESSNAME_FLAGS;
static const PROCESSNAME_FLAGS
	PN_CMPNAME      = 0x0000000000000000ULL,
	PN_CMPNAMELIST  = 0x0000000000010000ULL,
	PN_GENERATENAME = 0x0000000000020000ULL,
	PN_SKIPPATH     = 0x0000000001000000ULL;

typedef int (WINAPI *FARSTDPROCESSNAME)(const wchar_t *param1, wchar_t *param2, DWORD size, PROCESSNAME_FLAGS flags);

typedef void (WINAPI *FARSTDUNQUOTE)(wchar_t *Str);

typedef unsigned __int64 XLAT_FLAGS;
static const XLAT_FLAGS
	XLAT_SWITCHKEYBLAYOUT  = 0x0000000000000001ULL,
	XLAT_SWITCHKEYBBEEP    = 0x0000000000000002ULL,
	XLAT_USEKEYBLAYOUTNAME = 0x0000000000000004ULL,
	XLAT_CONVERTALLCMDLINE = 0x0000000000010000ULL;


typedef size_t (WINAPI *FARSTDKEYTOKEYNAME)(int Key,wchar_t *KeyText,size_t Size);

typedef wchar_t*(WINAPI *FARSTDXLAT)(wchar_t *Line,int StartPos,int EndPos,XLAT_FLAGS Flags);

typedef int (WINAPI *FARSTDKEYNAMETOKEY)(const wchar_t *Name);

typedef int (WINAPI *FRSUSERFUNC)(
    const struct PluginPanelItem *FData,
    const wchar_t *FullName,
    void *Param
);

typedef unsigned __int64 FRSMODE;
static const FRSMODE
	FRS_RETUPDIR             = 0x0000000000000001ULL,
	FRS_RECUR                = 0x0000000000000002ULL,
	FRS_SCANSYMLINK          = 0x0000000000000004ULL;

typedef void (WINAPI *FARSTDRECURSIVESEARCH)(const wchar_t *InitDir,const wchar_t *Mask,FRSUSERFUNC Func,FRSMODE Flags,void *Param);
typedef int (WINAPI *FARSTDMKTEMP)(wchar_t *Dest, DWORD size, const wchar_t *Prefix);
typedef void (WINAPI *FARSTDDELETEBUFFER)(void *Buffer);

enum LINK_TYPE
{
	LINK_HARDLINK         = 1,
	LINK_JUNCTION         = 2,
	LINK_VOLMOUNT         = 3,
	LINK_SYMLINKFILE      = 4,
	LINK_SYMLINKDIR       = 5,
	LINK_SYMLINK          = 6,
};

typedef unsigned __int64 MKLINK_FLAGS;
static const MKLINK_FLAGS
	MLF_NONE             = 0,
	MLF_SHOWERRMSG       = 0x0000000000010000ULL,
	MLF_DONOTUPDATEPANEL = 0x0000000000020000ULL;

typedef int (WINAPI *FARSTDMKLINK)(const wchar_t *Src,const wchar_t *Dest,LINK_TYPE Type, MKLINK_FLAGS Flags);
typedef int (WINAPI *FARGETREPARSEPOINTINFO)(const wchar_t *Src, wchar_t *Dest,int DestSize);

enum CONVERTPATHMODES
{
	CPM_FULL,
	CPM_REAL,
	CPM_NATIVE,
};

typedef int (WINAPI *FARCONVERTPATH)(CONVERTPATHMODES Mode, const wchar_t *Src, wchar_t *Dest, int DestSize);

typedef DWORD (WINAPI *FARGETCURRENTDIRECTORY)(DWORD Size,wchar_t* Buffer);

typedef struct FarStandardFunctions
{
	size_t StructSize;

	FARSTDATOI                 atoi;
	FARSTDATOI64               atoi64;
	FARSTDITOA                 itoa;
	FARSTDITOA64               itoa64;
	// <C&C++>
	FARSTDSPRINTF              sprintf;
	FARSTDSSCANF               sscanf;
	// </C&C++>
	FARSTDQSORT                qsort;
	FARSTDBSEARCH              bsearch;
	FARSTDQSORTEX              qsortex;
	// <C&C++>
	FARSTDSNPRINTF             snprintf;
	// </C&C++>

	DWORD_PTR                  Reserved[8];

	FARSTDLOCALISLOWER         LIsLower;
	FARSTDLOCALISUPPER         LIsUpper;
	FARSTDLOCALISALPHA         LIsAlpha;
	FARSTDLOCALISALPHANUM      LIsAlphanum;
	FARSTDLOCALUPPER           LUpper;
	FARSTDLOCALLOWER           LLower;
	FARSTDLOCALUPPERBUF        LUpperBuf;
	FARSTDLOCALLOWERBUF        LLowerBuf;
	FARSTDLOCALSTRUPR          LStrupr;
	FARSTDLOCALSTRLWR          LStrlwr;
	FARSTDLOCALSTRICMP         LStricmp;
	FARSTDLOCALSTRNICMP        LStrnicmp;

	FARSTDUNQUOTE              Unquote;
	FARSTDLTRIM                LTrim;
	FARSTDRTRIM                RTrim;
	FARSTDTRIM                 Trim;
	FARSTDTRUNCSTR             TruncStr;
	FARSTDTRUNCPATHSTR         TruncPathStr;
	FARSTDQUOTESPACEONLY       QuoteSpaceOnly;
	FARSTDPOINTTONAME          PointToName;
	FARSTDGETPATHROOT          GetPathRoot;
	FARSTDADDENDSLASH          AddEndSlash;
	FARSTDCOPYTOCLIPBOARD      CopyToClipboard;
	FARSTDPASTEFROMCLIPBOARD   PasteFromClipboard;
	FARSTDKEYTOKEYNAME         FarKeyToName;
	FARSTDKEYNAMETOKEY         FarNameToKey;
	FARSTDINPUTRECORDTOKEY     FarInputRecordToKey;
	FARSTDKEYTOINPUTRECORD     FarKeyToInputRecord;
	FARSTDXLAT                 XLat;
	FARSTDGETFILEOWNER         GetFileOwner;
	FARSTDGETNUMBEROFLINKS     GetNumberOfLinks;
	FARSTDRECURSIVESEARCH      FarRecursiveSearch;
	FARSTDMKTEMP               MkTemp;
	FARSTDDELETEBUFFER         DeleteBuffer;
	FARSTDPROCESSNAME          ProcessName;
	FARSTDMKLINK               MkLink;
	FARCONVERTPATH             ConvertPath;
	FARGETREPARSEPOINTINFO     GetReparsePointInfo;
	FARGETCURRENTDIRECTORY     GetCurrentDirectory;
} FARSTANDARDFUNCTIONS;

struct PluginStartupInfo
{
	size_t StructSize;
	const wchar_t *ModuleName;
	FARAPIMENU             Menu;
	FARAPIMESSAGE          Message;
	FARAPIGETMSG           GetMsg;
	FARAPICONTROL          Control;
	FARAPISAVESCREEN       SaveScreen;
	FARAPIRESTORESCREEN    RestoreScreen;
	FARAPIGETDIRLIST       GetDirList;
	FARAPIGETPLUGINDIRLIST GetPluginDirList;
	FARAPIFREEDIRLIST      FreeDirList;
	FARAPIFREEPLUGINDIRLIST FreePluginDirList;
	FARAPIVIEWER           Viewer;
	FARAPIEDITOR           Editor;
	FARAPITEXT             Text;
	FARAPIEDITORCONTROL    EditorControl;

	FARSTANDARDFUNCTIONS  *FSF;

	FARAPISHOWHELP         ShowHelp;
	FARAPIADVCONTROL       AdvControl;
	FARAPIINPUTBOX         InputBox;
	FARAPIDIALOGINIT       DialogInit;
	FARAPIDIALOGRUN        DialogRun;
	FARAPIDIALOGFREE       DialogFree;

	FARAPISENDDLGMESSAGE   SendDlgMessage;
	FARAPIDEFDLGPROC       DefDlgProc;
	DWORD_PTR              Reserved;
	FARAPIVIEWERCONTROL    ViewerControl;
	FARAPIPLUGINSCONTROL   PluginsControl;
	FARAPIFILEFILTERCONTROL FileFilterControl;
	FARAPIREGEXPCONTROL    RegExpControl;
	FARAPIMACROCONTROL     MacroControl;
	FARAPISETTINGSCONTROL  SettingsControl;
};


typedef unsigned __int64 PLUGIN_FLAGS;
static const PLUGIN_FLAGS
	PF_NONE           = 0,
	PF_PRELOAD        = 0x0000000000000001ULL,
	PF_DISABLEPANELS  = 0x0000000000000002ULL,
	PF_EDITOR         = 0x0000000000000004ULL,
	PF_VIEWER         = 0x0000000000000008ULL,
	PF_FULLCMDLINE    = 0x0000000000000010ULL,
	PF_DIALOG         = 0x0000000000000020ULL;

struct PluginMenuItem
{
	const GUID *Guids;
	const wchar_t * const *Strings;
	int Count;
};

struct GlobalInfo
{
	size_t StructSize;
	DWORD MinFarVersion;
	DWORD Version;
	GUID Guid;
	const wchar_t *Title;
	const wchar_t *Description;
	const wchar_t *Author;
};

struct PluginInfo
{
	size_t StructSize;
	PLUGIN_FLAGS Flags;
	struct PluginMenuItem DiskMenu;
	struct PluginMenuItem PluginMenu;
	struct PluginMenuItem PluginConfig;
	const wchar_t *CommandPrefix;
#ifdef FAR_USE_INTERNALS
#if defined(PROCPLUGINMACROFUNC)
	int MacroFunctionNumber;
	const FarMacroFunction *MacroFunctions;
#endif
#endif // END FAR_USE_INTERNALS
};



struct InfoPanelLine
{
	const wchar_t *Text;
	const wchar_t *Data;
	int  Separator;
};

typedef unsigned __int64 PANELMODE_FLAGS;
static const PANELMODE_FLAGS
	PMFLAGS_FULLSCREEN      = 0x0000000000000001ULL,
	PMFLAGS_DETAILEDSTATUS  = 0x0000000000000002ULL,
	PMFLAGS_ALIGNEXTENSIONS = 0x0000000000000004ULL,
	PMFLAGS_CASECONVERSION  = 0x0000000000000008ULL;

struct PanelMode
{
	size_t StructSize;
	const wchar_t *ColumnTypes;
	const wchar_t *ColumnWidths;
	const wchar_t * const *ColumnTitles;
	const wchar_t *StatusColumnTypes;
	const wchar_t *StatusColumnWidths;
	PANELMODE_FLAGS Flags;
};

typedef unsigned __int64 OPENPANELINFO_FLAGS;
static const OPENPANELINFO_FLAGS
	OPIF_NONE                    = 0,
	OPIF_DISABLEFILTER           = 0x0000000000000001ULL,
	OPIF_DISABLESORTGROUPS       = 0x0000000000000002ULL,
	OPIF_DISABLEHIGHLIGHTING     = 0x0000000000000004ULL,
	OPIF_ADDDOTS                 = 0x0000000000000008ULL,
	OPIF_RAWSELECTION            = 0x0000000000000010ULL,
	OPIF_REALNAMES               = 0x0000000000000020ULL,
	OPIF_SHOWNAMESONLY           = 0x0000000000000040ULL,
	OPIF_SHOWRIGHTALIGNNAMES     = 0x0000000000000080ULL,
	OPIF_SHOWPRESERVECASE        = 0x0000000000000100ULL,
	OPIF_COMPAREFATTIME          = 0x0000000000000400ULL,
	OPIF_EXTERNALGET             = 0x0000000000000800ULL,
	OPIF_EXTERNALPUT             = 0x0000000000001000ULL,
	OPIF_EXTERNALDELETE          = 0x0000000000002000ULL,
	OPIF_EXTERNALMKDIR           = 0x0000000000004000ULL,
	OPIF_USEATTRHIGHLIGHTING     = 0x0000000000008000ULL,
	OPIF_USECRC32                = 0x0000000000010000ULL,
	OPIF_USEFREESIZE             = 0x0000000000020000ULL;

struct KeyBarLabel
{
	struct FarKey Key;
	const wchar_t *Text;
	const wchar_t *LongText;
};

struct KeyBarTitles
{
	int CountLabels;
	struct KeyBarLabel *Labels;
};

typedef unsigned __int64 OPERATION_MODES;
static const OPERATION_MODES
	OPM_NONE       =0,
	OPM_SILENT     =0x0000000000000001ULL,
	OPM_FIND       =0x0000000000000002ULL,
	OPM_VIEW       =0x0000000000000004ULL,
	OPM_EDIT       =0x0000000000000008ULL,
	OPM_TOPLEVEL   =0x0000000000000010ULL,
	OPM_DESCR      =0x0000000000000020ULL,
	OPM_QUICKVIEW  =0x0000000000000040ULL,
	OPM_PGDN       =0x0000000000000080ULL;

struct OpenPanelInfo
{
	size_t                       StructSize;
	OPENPANELINFO_FLAGS          Flags;
	const wchar_t               *HostFile;
	const wchar_t               *CurDir;
	const wchar_t               *Format;
	const wchar_t               *PanelTitle;
	const struct InfoPanelLine  *InfoLines;
	int                          InfoLinesNumber;
	const wchar_t * const       *DescrFiles;
	int                          DescrFilesNumber;
	const struct PanelMode      *PanelModesArray;
	int                          PanelModesNumber;
	int                          StartPanelMode;
	OPENPANELINFO_SORTMODES     StartSortMode;
	int                          StartSortOrder;
	const struct KeyBarTitles   *KeyBar;
	const wchar_t               *ShortcutData;
	unsigned __int64             FreeSize;
};

struct AnalyseData
{
	size_t          StructSize;
	const wchar_t  *FileName;
	void           *Buffer;
	size_t          BufferSize;
	OPERATION_MODES OpMode;
};

enum OPENPANEL_OPENFROM
{
	OPEN_FROM_MASK          = 0x000000FF,

	OPEN_LEFTDISKMENU       = 0,
	OPEN_PLUGINSMENU        = 1,
	OPEN_FINDLIST           = 2,
	OPEN_SHORTCUT           = 3,
	OPEN_COMMANDLINE        = 4,
	OPEN_EDITOR             = 5,
	OPEN_VIEWER             = 6,
	OPEN_FILEPANEL          = 7,
	OPEN_DIALOG             = 8,
	OPEN_ANALYSE            = 9,
	OPEN_RIGHTDISKMENU      = 10,

	OPEN_FROMMACRO_MASK     = 0x000F0000,

	OPEN_FROMMACRO          = 0x00010000,
	OPEN_FROMMACROSTRING    = 0x00020000,
};

#ifdef FAR_USE_INTERNALS
enum FAR_PKF_FLAGS
{
	PKF_CONTROL     = 0x00000001,
	PKF_ALT         = 0x00000002,
	PKF_SHIFT       = 0x00000004,
	PKF_PREPROCESS  = 0x00080000, // for "Key", function ProcessKey()
};
#endif // END FAR_USE_INTERNALS

enum FAR_EVENTS
{
	FE_CHANGEVIEWMODE =0,
	FE_REDRAW         =1,
	FE_IDLE           =2,
	FE_CLOSE          =3,
	FE_BREAK          =4,
	FE_COMMAND        =5,

	FE_GOTFOCUS       =6,
	FE_KILLFOCUS      =7,
};

#ifdef __cplusplus
extern "C"
{
#endif
// Exported Functions

	int    WINAPI AnalyseW(const struct AnalyseData *Data);
	void   WINAPI ClosePanelW(HANDLE hPanel);
	int    WINAPI CompareW(HANDLE hPanel,const struct PluginPanelItem *Item1,const struct PluginPanelItem *Item2,OPENPANELINFO_SORTMODES Mode);
	int    WINAPI ConfigureW(const GUID* Guid);
	int    WINAPI DeleteFilesW(HANDLE hPanel,struct PluginPanelItem *PanelItem,int ItemsNumber,OPERATION_MODES OpMode);
	void   WINAPI ExitFARW(void);
	void   WINAPI FreeFindDataW(HANDLE hPanel,struct PluginPanelItem *PanelItem,int ItemsNumber);
	void   WINAPI FreeVirtualFindDataW(HANDLE hPanel,struct PluginPanelItem *PanelItem,int ItemsNumber);
	int    WINAPI GetFilesW(HANDLE hPanel,struct PluginPanelItem *PanelItem,int ItemsNumber,int Move,const wchar_t **DestPath,OPERATION_MODES OpMode);
	int    WINAPI GetFindDataW(HANDLE hPanel,struct PluginPanelItem **pPanelItem,int *pItemsNumber,OPERATION_MODES OpMode);
	void   WINAPI GetGlobalInfoW(struct GlobalInfo *Info);
	void   WINAPI GetOpenPanelInfoW(HANDLE hPanel,struct OpenPanelInfo *Info);
	void   WINAPI GetPluginInfoW(struct PluginInfo *Info);
	int    WINAPI GetVirtualFindDataW(HANDLE hPanel,struct PluginPanelItem **pPanelItem,int *pItemsNumber,const wchar_t *Path);
	int    WINAPI MakeDirectoryW(HANDLE hPanel,const wchar_t **Name,OPERATION_MODES OpMode);
	HANDLE WINAPI OpenPanelW(OPENPANEL_OPENFROM OpenFrom,const GUID* Guid,INT_PTR Data);
	int    WINAPI ProcessDialogEventW(int Event,void *Param);
	int    WINAPI ProcessEditorEventW(int Event,void *Param);
	int    WINAPI ProcessEditorInputW(const INPUT_RECORD *Rec);
	int    WINAPI ProcessEventW(HANDLE hPanel,int Event,void *Param);
	int    WINAPI ProcessHostFileW(HANDLE hPanel,struct PluginPanelItem *PanelItem,int ItemsNumber,OPERATION_MODES OpMode);
	int    WINAPI ProcessKeyW(HANDLE hPanel,const INPUT_RECORD *Rec);
#ifdef FAR_USE_INTERNALS
	#if defined(PROCPLUGINMACROFUNC)
	int    WINAPI ProcessMacroFuncW(const wchar_t *Name, const FarMacroValue *Params, int nParams, FarMacroValue **Results, int *nResults);
	#endif
#endif // END FAR_USE_INTERNALS
	int    WINAPI ProcessSynchroEventW(int Event,void *Param);
	int    WINAPI ProcessViewerEventW(int Event,void *Param);
	int    WINAPI PutFilesW(HANDLE hPanel,struct PluginPanelItem *PanelItem,int ItemsNumber,int Move,const wchar_t *SrcPath,OPERATION_MODES OpMode);
	int    WINAPI SetDirectoryW(HANDLE hPanel,const wchar_t *Dir,OPERATION_MODES OpMode);
	int    WINAPI SetFindListW(HANDLE hPanel,const struct PluginPanelItem *PanelItem,int ItemsNumber);
	void   WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info);

#ifdef __cplusplus
};
#endif

#endif /* RC_INVOKED */

#endif /* __PLUGIN_HPP__ */
