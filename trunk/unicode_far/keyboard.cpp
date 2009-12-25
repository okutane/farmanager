/*
keyboard.cpp

�������, ������� ��������� � ���������
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

#include "keyboard.hpp"
#include "keys.hpp"
#include "farqueue.hpp"
#include "lang.hpp"
#include "ctrlobj.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "cmdline.hpp"
#include "grabber.hpp"
#include "manager.hpp"
#include "scrbuf.hpp"
#include "savescr.hpp"
#include "lockscrn.hpp"
#include "imports.hpp"
#include "TPreRedrawFunc.hpp"
#include "syslog.hpp"
#include "interf.hpp"
#include "iswind.hpp"
#include "registry.hpp"
#include "message.hpp"
#include "config.hpp"
#include "scrsaver.hpp"
#include "strmix.hpp"
#include "synchro.hpp"
#include "constitle.hpp"

/* start ���������� ���������� */

// "��������������" ������� ����� ������
FarQueue<DWORD> *KeyQueue=NULL;
int AltPressed=0,CtrlPressed=0,ShiftPressed=0;
int RightAltPressed=0,RightCtrlPressed=0,RightShiftPressed=0;
DWORD MouseButtonState=0,PrevMouseButtonState=0;
int PrevLButtonPressed=0, PrevRButtonPressed=0, PrevMButtonPressed=0;
SHORT PrevMouseX=0,PrevMouseY=0,MouseX=0,MouseY=0;
int PreMouseEventFlags=0,MouseEventFlags=0;
// ������ ��� ��� ���� Alt-������?
int ReturnAltValue=0;

/* end ���������� ���������� */

static SHORT KeyToVKey[WCHAR_MAX];
static WCHAR VKeyToASCII[0x200];

static unsigned int AltValue=0;
static int KeyCodeForALT_LastPressed=0;

static MOUSE_EVENT_RECORD lastMOUSE_EVENT_RECORD;
static int ShiftPressedLast=FALSE,AltPressedLast=FALSE,CtrlPressedLast=FALSE;
static BOOL IsKeyCASPressed=FALSE; // CtrlAltShift - ������ ��� ���?

static int RightShiftPressedLast=FALSE,RightAltPressedLast=FALSE,RightCtrlPressedLast=FALSE;
static BOOL IsKeyRCASPressed=FALSE; // Right CtrlAltShift - ������ ��� ���?

static clock_t PressedLastTime,KeyPressedLastTime;
static int ShiftState=0;
#if defined(DETECT_ALT_ENTER)
static int AltEnter=-1;
#endif
static int LastShiftEnterPressed=FALSE;

/* ----------------------------------------------------------------- */
static struct TTable_KeyToVK
{
	int Key;
	int VK;
} Table_KeyToVK[]=
{
//   {KEY_PGUP,          VK_PRIOR},
//   {KEY_PGDN,          VK_NEXT},
//   {KEY_END,           VK_END},
//   {KEY_HOME,          VK_HOME},
//   {KEY_LEFT,          VK_LEFT},
//   {KEY_UP,            VK_UP},
//   {KEY_RIGHT,         VK_RIGHT},
//   {KEY_DOWN,          VK_DOWN},
//   {KEY_INS,           VK_INSERT},
//   {KEY_DEL,           VK_DELETE},
//   {KEY_LWIN,          VK_LWIN},
//   {KEY_RWIN,          VK_RWIN},
//   {KEY_APPS,          VK_APPS},
//   {KEY_MULTIPLY,      VK_MULTIPLY},
//   {KEY_ADD,           VK_ADD},
//   {KEY_SUBTRACT,      VK_SUBTRACT},
//   {KEY_DIVIDE,        VK_DIVIDE},
//   {KEY_F1,            VK_F1},
//   {KEY_F2,            VK_F2},
//   {KEY_F3,            VK_F3},
//   {KEY_F4,            VK_F4},
//   {KEY_F5,            VK_F5},
//   {KEY_F6,            VK_F6},
//   {KEY_F7,            VK_F7},
//   {KEY_F8,            VK_F8},
//   {KEY_F9,            VK_F9},
//   {KEY_F10,           VK_F10},
//   {KEY_F11,           VK_F11},
//   {KEY_F12,           VK_F12},
	{KEY_BREAK,         VK_CANCEL},
	{KEY_BS,            VK_BACK},
	{KEY_TAB,           VK_TAB},
	{KEY_ENTER,         VK_RETURN},
	{KEY_NUMENTER,      VK_RETURN}, //????
	{KEY_ESC,           VK_ESCAPE},
	{KEY_SPACE,         VK_SPACE},
	{KEY_NUMPAD5,       VK_CLEAR},
};


struct TFKey3
{
	DWORD Key;
	int   Len;
	const wchar_t *Name;
	const wchar_t *UName;
};

static TFKey3 FKeys1[]=
{
	{ KEY_RCTRLALTSHIFTRELEASE,24, L"RightCtrlAltShiftRelease", L"RIGHTCTRLALTSHIFTRELEASE"},
	{ KEY_RCTRLALTSHIFTPRESS,  22, L"RightCtrlAltShiftPress", L"RIGHTCTRLALTSHIFTPRESS"},
	{ KEY_CTRLALTSHIFTRELEASE, 19, L"CtrlAltShiftRelease", L"CTRLALTSHIFTRELEASE"},
	{ KEY_CTRLALTSHIFTPRESS,   17, L"CtrlAltShiftPress", L"CTRLALTSHIFTPRESS"},
	{ KEY_LAUNCH_MEDIA_SELECT, 17, L"LaunchMediaSelect", L"LAUNCHMEDIASELECT"},
	{ KEY_BROWSER_FAVORITES,   16, L"BrowserFavorites", L"BROWSERFAVORITES"},
	{ KEY_MEDIA_PREV_TRACK,    14, L"MediaPrevTrack", L"MEDIAPREVTRACK"},
	{ KEY_MEDIA_PLAY_PAUSE,    14, L"MediaPlayPause", L"MEDIAPLAYPAUSE"},
	{ KEY_MEDIA_NEXT_TRACK,    14, L"MediaNextTrack", L"MEDIANEXTTRACK"},
	{ KEY_BROWSER_REFRESH,     14, L"BrowserRefresh", L"BROWSERREFRESH"},
	{ KEY_BROWSER_FORWARD,     14, L"BrowserForward", L"BROWSERFORWARD"},
	//{ KEY_HP_COMMUNITIES,      13, L"HPCommunities", L"HPCOMMUNITIES"},
	{ KEY_BROWSER_SEARCH,      13, L"BrowserSearch", L"BROWSERSEARCH"},
	{ KEY_MSWHEEL_RIGHT,       12, L"MsWheelRight", L"MSWHEELRIGHT"},
#if 0
	{ KEY_MSM1DBLCLICK,        12, L"MsM1DblClick", L"MSM1DBLCLICK"},
	{ KEY_MSM2DBLCLICK,        12, L"MsM2DblClick", L"MSM2DBLCLICK"},
	{ KEY_MSM3DBLCLICK,        12, L"MsM3DblClick", L"MSM3DBLCLICK"},
	{ KEY_MSLDBLCLICK,         11, L"MsLDblClick", L"MSLDBLCLICK"},
	{ KEY_MSRDBLCLICK,         11, L"MsRDblClick", L"MSRDBLCLICK"},
#endif
	{ KEY_MSWHEEL_DOWN,        11, L"MsWheelDown", L"MSWHEELDOWN"},
	{ KEY_MSWHEEL_LEFT,        11, L"MsWheelLeft", L"MSWHEELLEFT"},
	//{ KEY_AC_BOOKMARKS,        11, L"ACBookmarks", L"ACBOOKMARKS"},
	{ KEY_BROWSER_STOP,        11, L"BrowserStop", L"BROWSERSTOP"},
	{ KEY_BROWSER_HOME,        11, L"BrowserHome", L"BROWSERHOME"},
	{ KEY_BROWSER_BACK,        11, L"BrowserBack", L"BROWSERBACK"},
	{ KEY_VOLUME_MUTE,         10, L"VolumeMute", L"VOLUMEMUTE"},
	{ KEY_VOLUME_DOWN,         10, L"VolumeDown", L"VOLUMEDOWN"},
	{ KEY_SCROLLLOCK,          10, L"ScrollLock", L"SCROLLLOCK"},
	{ KEY_LAUNCH_MAIL,         10, L"LaunchMail", L"LAUNCHMAIL"},
	{ KEY_LAUNCH_APP2,         10, L"LaunchApp2", L"LAUNCHAPP2"},
	{ KEY_LAUNCH_APP1,         10, L"LaunchApp1", L"LAUNCHAPP1"},
	//{ KEY_HP_INTERNET,         10, L"HPInternet", L"HPINTERNET"},
	//{ KEY_AC_FORWARD,           9, L"ACForward", L"ACFORWARD"},
	//{ KEY_AC_REFRESH,           9, L"ACRefresh", L"ACREFRESH"},
	{ KEY_MSWHEEL_UP,           9, L"MsWheelUp", L"MSWHEELUP"},
	{ KEY_MEDIA_STOP,           9, L"MediaStop", L"MEDIASTOP"},
	{ KEY_BACKSLASH,            9, L"BackSlash", L"BACKSLASH"},
	//{ KEY_HP_MEETING,           9, L"HPMeeting", L"HPMEETING"},
	{ KEY_MSM1CLICK,            9, L"MsM1Click", L"MSM1CLICK"},
	{ KEY_MSM2CLICK,            9, L"MsM2Click", L"MSM2CLICK"},
	{ KEY_MSM3CLICK,            9, L"MsM3Click", L"MSM3CLICK"},
	{ KEY_MSLCLICK,             8, L"MsLClick", L"MSLCLICK"},
	{ KEY_MSRCLICK,             8, L"MsRClick", L"MSRCLICK"},
	//{ KEY_HP_MARKET,            8, L"HPMarket", L"HPMARKET"},
	{ KEY_VOLUME_UP,            8, L"VolumeUp", L"VOLUMEUP"},
	{ KEY_SUBTRACT,             8, L"Subtract", L"SUBTRACT"},
	{ KEY_NUMENTER,             8, L"NumEnter", L"NUMENTER"},
	{ KEY_MULTIPLY,             8, L"Multiply", L"MULTIPLY"},
	{ KEY_CAPSLOCK,             8, L"CapsLock", L"CAPSLOCK"},
	{ KEY_PRNTSCRN,             8, L"PrntScrn", L"PRNTSCRN"},
	{ KEY_NUMLOCK,              7, L"NumLock", L"NUMLOCK"},
	{ KEY_DECIMAL,              7, L"Decimal", L"DECIMAL"},
	{ KEY_STANDBY,              7, L"Standby", L"STANDBY"},
	//{ KEY_HP_SEARCH,            8, L"HPSearch", L"HPSEARCH"},
	//{ KEY_HP_HOME,              6, L"HPHome", L"HPHOME"},
	//{ KEY_HP_MAIL,              6, L"HPMail", L"HPMAIL"},
	//{ KEY_HP_NEWS,              6, L"HPNews", L"HPNEWS"},
	//{ KEY_AC_BACK,              6, L"ACBack", L"ACBACK"},
	//{ KEY_AC_STOP,              6, L"ACStop", L"ACSTOP"},
	{ KEY_DIVIDE,               6, L"Divide", L"DIVIDE"},
	{ KEY_NUMDEL,               6, L"NumDel", L"NUMDEL"},
	{ KEY_SPACE,                5, L"Space", L"SPACE"},
	{ KEY_RIGHT,                5, L"Right", L"RIGHT"},
	{ KEY_PAUSE,                5, L"Pause", L"PAUSE"},
	{ KEY_ENTER,                5, L"Enter", L"ENTER"},
	{ KEY_CLEAR,                5, L"Clear", L"CLEAR"},
	{ KEY_BREAK,                5, L"Break", L"BREAK"},
	{ KEY_PGUP,                 4, L"PgUp", L"PGUP"},
	{ KEY_PGDN,                 4, L"PgDn", L"PGDN"},
	{ KEY_LEFT,                 4, L"Left", L"LEFT"},
	{ KEY_HOME,                 4, L"Home", L"HOME"},
	{ KEY_DOWN,                 4, L"Down", L"DOWN"},
	{ KEY_APPS,                 4, L"Apps", L"APPS"},
	{ KEY_RWIN,                 4 ,L"RWin", L"RWIN"},
	{ KEY_NUMPAD9,              4 ,L"Num9", L"NUM9"},
	{ KEY_NUMPAD8,              4 ,L"Num8", L"NUM8"},
	{ KEY_NUMPAD7,              4 ,L"Num7", L"NUM7"},
	{ KEY_NUMPAD6,              4 ,L"Num6", L"NUM6"},
	{ KEY_NUMPAD5,              4, L"Num5", L"NUM5"},
	{ KEY_NUMPAD4,              4 ,L"Num4", L"NUM4"},
	{ KEY_NUMPAD3,              4 ,L"Num3", L"NUM3"},
	{ KEY_NUMPAD2,              4 ,L"Num2", L"NUM2"},
	{ KEY_NUMPAD1,              4 ,L"Num1", L"NUM1"},
	{ KEY_NUMPAD0,              4 ,L"Num0", L"NUM0"},
	{ KEY_LWIN,                 4 ,L"LWin", L"LWIN"},
	{ KEY_TAB,                  3, L"Tab", L"TAB"},
	{ KEY_INS,                  3, L"Ins", L"INS"},
	{ KEY_F10,                  3, L"F10", L"F10"},
	{ KEY_F11,                  3, L"F11", L"F11"},
	{ KEY_F12,                  3, L"F12", L"F12"},
	{ KEY_F13,                  3, L"F13", L"F13"},
	{ KEY_F14,                  3, L"F14", L"F14"},
	{ KEY_F15,                  3, L"F15", L"F15"},
	{ KEY_F16,                  3, L"F16", L"F16"},
	{ KEY_F17,                  3, L"F17", L"F17"},
	{ KEY_F18,                  3, L"F18", L"F18"},
	{ KEY_F19,                  3, L"F19", L"F19"},
	{ KEY_F20,                  3, L"F20", L"F20"},
	{ KEY_F21,                  3, L"F21", L"F21"},
	{ KEY_F22,                  3, L"F22", L"F22"},
	{ KEY_F23,                  3, L"F23", L"F23"},
	{ KEY_F24,                  3, L"F24", L"F24"},
	{ KEY_ESC,                  3, L"Esc", L"ESC"},
	{ KEY_END,                  3, L"End", L"END"},
	{ KEY_DEL,                  3, L"Del", L"DEL"},
	{ KEY_ADD,                  3, L"Add", L"ADD"},
	{ KEY_UP,                   2, L"Up", L"UP"},
	{ KEY_F9,                   2, L"F9", L"F9"},
	{ KEY_F8,                   2, L"F8", L"F8"},
	{ KEY_F7,                   2, L"F7", L"F7"},
	{ KEY_F6,                   2, L"F6", L"F6"},
	{ KEY_F5,                   2, L"F5", L"F5"},
	{ KEY_F4,                   2, L"F4", L"F4"},
	{ KEY_F3,                   2, L"F3", L"F3"},
	{ KEY_F2,                   2, L"F2", L"F2"},
	{ KEY_F1,                   2, L"F1", L"F1"},
	{ KEY_BS,                   2, L"BS", L"BS"},
	{ KEY_BACKBRACKET,          1, L"]",  L"]"},
	{ KEY_QUOTE,                1, L"\"",  L"\""},
	{ KEY_BRACKET,              1, L"[",  L"["},
	{ KEY_COLON,                1, L":",  L":"},
	{ KEY_SEMICOLON,            1, L";",  L";"},
	{ KEY_SLASH,                1, L"/",  L"/"},
	{ KEY_DOT,                  1, L".",  L"."},
	{ KEY_COMMA,                1, L",",  L","},
};

static TFKey3 ModifKeyName[]=
{
	{ KEY_RCTRL  ,5 ,L"RCtrl", L"RCTRL"},
	{ KEY_SHIFT  ,5 ,L"Shift", L"SHIFT"},
	{ KEY_CTRL   ,4 ,L"Ctrl", L"CTRL"},
	{ KEY_RALT   ,4 ,L"RAlt", L"RALT"},
	{ KEY_ALT    ,3 ,L"Alt", L"ALT"},
	{ KEY_M_SPEC ,4 ,L"Spec", L"SPEC"},
	{ KEY_M_OEM  ,3 ,L"Oem", L"OEM"},
//  { KEY_LCTRL  ,5 ,L"LCtrl", L"LCTRL"},
//  { KEY_LALT   ,4 ,L"LAlt", L"LALT"},
//  { KEY_LSHIFT ,6 ,L"LShift", L"LSHIFT"},
//  { KEY_RSHIFT ,6 ,L"RShift", L"RSHIFT"},
};

#if defined(SYSLOG)
static TFKey3 SpecKeyName[]=
{
	{ KEY_CONSOLE_BUFFER_RESIZE,19, L"ConsoleBufferResize", L"CONSOLEBUFFERRESIZE"},
	{ KEY_LOCKSCREEN           ,10, L"LockScreen", L"LOCKSCREEN"},
	{ KEY_OP_SELWORD           ,10, L"OP_SelWord", L"OP_SELWORD"},
	{ KEY_KILLFOCUS             ,9, L"KillFocus", L"KILLFOCUS"},
	{ KEY_GOTFOCUS              ,8, L"GotFocus", L"GOTFOCUS"},
	{ KEY_DRAGCOPY             , 8, L"DragCopy", L"DRAGCOPY"},
	{ KEY_DRAGMOVE             , 8, L"DragMove", L"DRAGMOVE"},
	{ KEY_OP_DATE              , 7, L"OP_Date", L"OP_DATE"},
	{ KEY_OP_PLAINTEXT         , 7, L"OP_Text", L"OP_TEXT"},
	{ KEY_OP_XLAT              , 7, L"OP_Xlat", L"OP_XLAT"},
	{ KEY_NONE                 , 4, L"None", L"NONE"},
	{ KEY_IDLE                 , 4, L"Idle", L"IDLE"},
};
#endif

/* ----------------------------------------------------------------- */

/*
   ������������� ������� ������.
   �������� ������ ����� CopyGlobalSettings, ������ ��� ������ ����� GetRegKey
   ������� ���������� ������.
*/
void InitKeysArray()
{
	HKL Layout[10];
	int LayoutNumber=GetKeyboardLayoutList(countof(Layout),Layout); // ���������� 0! � telnet

	if (LayoutNumber==0)
	{
		HKEY hk=NULL;

		if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Keyboard Layout\\Preload", 0, KEY_READ, &hk)==ERROR_SUCCESS)
		{
			DWORD dwType, dwIndex, dwDataSize, dwValueSize, dwKeyb;
			wchar_t SData[16], SValue[16];

			for (dwIndex=0; dwIndex < (int)countof(Layout); dwIndex++)
			{
				dwValueSize=16;
				dwDataSize=16*sizeof(wchar_t);

				if (ERROR_SUCCESS==RegEnumValue(hk, dwIndex, SValue, &dwValueSize, NULL, &dwType,(LPBYTE)SData, &dwDataSize))
				{
					if (dwType == REG_SZ && isdigit(SValue[0]) &&
					        (isdigit(SData[0]) || (SData[0] >= L'a' && SData[0] <= L'f') || (SData[0] >= L'A' && SData[0] <= L'F')))
					{
						wchar_t *endptr=NULL;
						dwKeyb=wcstoul(SData, &endptr, 16); // SData=="00000419"

						if (dwKeyb)
						{
							if (dwKeyb <= 0xFFFF)
								dwKeyb |= (dwKeyb << 16);

							Layout[LayoutNumber++] = (HKL)((DWORD_PTR)dwKeyb);
						}
					}
				}
				else
					break;
			}

			RegCloseKey(hk);
		}
	}

	memset(KeyToVKey,0,sizeof(KeyToVKey));
	memset(VKeyToASCII,0,sizeof(VKeyToASCII));

	if (LayoutNumber && LayoutNumber < (int)countof(Layout))
	{
		BYTE KeyState[0x100]={0};
		WCHAR buf[1];

		//KeyToVKey - ������������ ���� ��������� ���� ��� ������� ��� ���� � ���� ������ �� �����
		//*********
		//��� ��� ������� ����������� ����������� ����� ����� ����������� �� �������,
		//�� ������� ���� ��� �� ����� ������������ �������� ��� ������ ������� ���������
		//�� ������� ��������� ������� - ����� �����, ������ ������ ����������� ���� ���������
		//VKs � ShiftVKs � ��������� ������� ��������� �� ���� ���������� � ����� ��:
		//���� ������ VK ������� � ��� �� ������ ������ �� ����������� ����� ������ ��� ������
		//��������� ������� ������� ���� ������
		//
		for (BYTE j=0; j<2; j++)
		{
			KeyState[VK_SHIFT]=j*0x80;

			for (int i=0; i<LayoutNumber; i++)
			{
				for (int VK=0; VK<256; VK++)
				{
					if (ToUnicodeEx(LOBYTE(VK),0,KeyState,buf,1,0,Layout[i]) > 0)
					{
						if (!KeyToVKey[buf[0]])
							KeyToVKey[buf[0]] = VK + j*0x100;
					}
				}
			}
		}

		//VKeyToASCII - ������������ ������ � KeyToVKey ���� ��������� ���. ������ �� US-ASCII
		//***********
		//���� ����������� ������ -> VK ������ �������� �����������
		//VK -> ������� � ����� ������ 0x80, �.�. ������ US-ASCII �������
		for (WCHAR i=1, x=0; i < 0x80; i++)
		{
			x = KeyToVKey[i];

			if (x && !VKeyToASCII[x])
				VKeyToASCII[x]=Upper(i);
		}
	}

	//_SVS(SysLogDump(L"KeyToKey calculate",0,KeyToKey,sizeof(KeyToKey),NULL));
	//unsigned char KeyToKeyMap[256];
	//if(GetRegKey(L"System",L"KeyToKeyMap",KeyToKeyMap,KeyToKey,sizeof(KeyToKeyMap)))
	//memcpy(KeyToKey,KeyToKeyMap,sizeof(KeyToKey));
	//_SVS(SysLogDump("KeyToKey readed",0,KeyToKey,sizeof(KeyToKey),NULL));
}

//���������� ���� Key � CompareKey ��� ���� � �� �� ������� � ������ ����������
bool KeyToKeyLayoutCompare(int Key, int CompareKey)
{
	_KEYMACRO(CleverSysLog Clev(L"KeyToKeyLayoutCompare()"));
	_KEYMACRO(SysLog(L"Param: Key=%08X",Key));
	Key = KeyToVKey[Key&0xFFFF]&0xFF;
	CompareKey = KeyToVKey[CompareKey&0xFFFF]&0xFF;

	if (Key != 0 && Key == CompareKey)
		return true;

	return false;
}

//������ ������� ��������� Eng ���������� Key
int KeyToKeyLayout(int Key)
{
	_KEYMACRO(CleverSysLog Clev(L"KeyToKeyLayout()"));
	_KEYMACRO(SysLog(L"Param: Key=%08X",Key));
	int VK = KeyToVKey[Key&0xFFFF];

	if (VK && VKeyToASCII[VK])
		return VKeyToASCII[VK];

	return Key;
}

/*
  State:
    -1 get state
     0 off
     1 on
     2 flip
*/
int SetFLockState(UINT vkKey, int State)
{
	/*
	   VK_NUMLOCK (90)
	   VK_SCROLL (91)
	   VK_CAPITAL (14)
	*/
	UINT ExKey=(vkKey==VK_CAPITAL?0:KEYEVENTF_EXTENDEDKEY);

	switch (vkKey)
	{
		case VK_NUMLOCK:
		case VK_CAPITAL:
		case VK_SCROLL:
			break;
		default:
			return -1;
	}

	short oldState=GetKeyState(vkKey);

	if (State >= 0)
	{
		//if (State == 2 || (State==1 && !(keyState[vkKey] & 1)) || (!State && (keyState[vkKey] & 1)) )
		if (State == 2 || (State==1 && !oldState) || (!State && oldState))
		{
			keybd_event(vkKey, 0, ExKey, 0);
			keybd_event(vkKey, 0, ExKey | KEYEVENTF_KEYUP, 0);
		}
	}

	return (int)(WORD)oldState;
}

int WINAPI InputRecordToKey(const INPUT_RECORD *r)
{
	if (r)
	{
		INPUT_RECORD Rec=*r; // ����!, �.�. ������ CalcKeyCode
		//   ��������� INPUT_RECORD ��������������!
		return (int)CalcKeyCode(&Rec,FALSE);
	}

	return KEY_NONE;
}


DWORD IsMouseButtonPressed()
{
	INPUT_RECORD rec;

	if (PeekInputRecord(&rec))
		GetInputRecord(&rec);

	Sleep(1);
	return MouseButtonState;
}

static DWORD KeyMsClick2ButtonState(DWORD Key,DWORD& Event)
{
	Event=0;
#if 0

	switch (Key)
	{
		case KEY_MSM1DBLCLICK:
		case KEY_MSM2DBLCLICK:
		case KEY_MSM3DBLCLICK:
		case KEY_MSLDBLCLICK:
		case KEY_MSRDBLCLICK:
			Event=MOUSE_MOVED;
	}

#endif

	switch (Key)
	{
		case KEY_MSLCLICK:
			return FROM_LEFT_1ST_BUTTON_PRESSED;
		case KEY_MSM1CLICK:
			return FROM_LEFT_2ND_BUTTON_PRESSED;
		case KEY_MSM2CLICK:
			return FROM_LEFT_3RD_BUTTON_PRESSED;
		case KEY_MSM3CLICK:
			return FROM_LEFT_4TH_BUTTON_PRESSED;
		case KEY_MSRCLICK:
			return RIGHTMOST_BUTTON_PRESSED;
	}

	return 0;
}

DWORD GetInputRecord(INPUT_RECORD *rec,bool ExcludeMacro,bool ProcessMouse)
{
	static int LastEventIdle=FALSE;
	DWORD ReadCount;
	DWORD LoopCount=0,CalcKey;
	DWORD ReadKey=0;
	int NotMacros=FALSE;
	static int LastMsClickMacroKey=0;
	PluginSynchroManager.Process();

	if (!ExcludeMacro && CtrlObject && CtrlObject->Cp())
	{
//     _KEYMACRO(CleverSysLog SL(L"GetInputRecord()"));
		int VirtKey,ControlState;
		CtrlObject->Macro.RunStartMacro();
		int MacroKey=CtrlObject->Macro.GetKey();

		if (MacroKey)
		{
			DWORD EventState,MsClickKey;

			if ((MsClickKey=KeyMsClick2ButtonState(MacroKey,EventState)) != 0)
			{
				// ������! ��� ������� ������� ������ �������� MOUSE_EVENT, ��������������� _����������_ ������� ����.
				rec->EventType=MOUSE_EVENT;
				memcpy(&rec->Event.MouseEvent,&lastMOUSE_EVENT_RECORD,sizeof(MOUSE_EVENT_RECORD));
				rec->Event.MouseEvent.dwButtonState=MsClickKey;
				rec->Event.MouseEvent.dwEventFlags=EventState;
				LastMsClickMacroKey=MacroKey;
				return MacroKey;
			}
			else
			{
				// ���� ���������� ������� ������� - ������� ��������� ������ Drag
				if (KeyMsClick2ButtonState(LastMsClickMacroKey,EventState))
				{
					LastMsClickMacroKey=0;
					Panel::EndDrag();
				}

				ScrBuf.Flush();
				TranslateKeyToVK(MacroKey,VirtKey,ControlState,rec);
				rec->EventType=((((unsigned int)MacroKey >= KEY_MACRO_BASE && (unsigned int)MacroKey <= KEY_MACRO_ENDBASE) || ((unsigned int)MacroKey>=KEY_OP_BASE && (unsigned int)MacroKey <=KEY_OP_ENDBASE)) || (MacroKey&(~0xFF000000)) >= KEY_END_FKEY)?0:FARMACRO_KEY_EVENT;

				if (!(MacroKey&KEY_SHIFT))
					ShiftPressed=0;

				//_KEYMACRO(SysLog(L"MacroKey1 =%s",_FARKEY_ToName(MacroKey)));
				// memset(rec,0,sizeof(*rec));
				return(MacroKey);
			}
		}
	}

	if (KeyQueue && KeyQueue->Peek())
	{
		CalcKey=KeyQueue->Get();
		NotMacros=CalcKey&0x80000000?1:0;
		CalcKey&=~0x80000000;

		//???
		if (!ExcludeMacro && CtrlObject && CtrlObject->Macro.IsRecording() && (CalcKey == (KEY_ALT|KEY_NUMPAD0) || CalcKey == (KEY_ALT|KEY_INS)))
		{
			if (CtrlObject->Macro.ProcessKey(CalcKey))
			{
				RunGraber();
				rec->EventType=0;
				CalcKey=KEY_NONE;
			}

			return(CalcKey);
		}

		if (!NotMacros)
		{
			_KEYMACRO(CleverSysLog Clev(L"CALL(1) CtrlObject->Macro.ProcessKey()"));

			if (!ExcludeMacro && CtrlObject!=NULL && CtrlObject->Macro.ProcessKey(CalcKey))
			{
				rec->EventType=0;
				CalcKey=KEY_NONE;
			}
		}

		return(CalcKey);
	}

	int EnableShowTime=Opt.Clock && (WaitInMainLoop || (CtrlObject!=NULL &&
	                                 CtrlObject->Macro.GetMode()==MACRO_SEARCH));

	if (EnableShowTime)
		ShowTime(1);

	ScrBuf.Flush();

	if (!LastEventIdle)
		StartIdleTime=clock();

	LastEventIdle=FALSE;
	SetFarConsoleMode();
	BOOL ZoomedState=IsZoomed(GetConsoleWindow());
	BOOL IconicState=IsIconic(GetConsoleWindow());

	for (;;)
	{
		// "�������" �� ������������/�������������� ���� �������
		if (ZoomedState!=IsZoomed(GetConsoleWindow()) && IconicState==IsIconic(GetConsoleWindow()))
		{
			ZoomedState=!ZoomedState;
			ChangeVideoMode(ZoomedState);
		}

		PeekConsoleInput(GetStdHandle(STD_INPUT_HANDLE),rec,1,&ReadCount);

		/* $ 26.04.2001 VVM
		   ! ����� ������� �������� */
		if (ReadCount!=0)
		{
			//cheat for flock
			if (rec->EventType==KEY_EVENT&&rec->Event.KeyEvent.wVirtualScanCode==0&&(rec->Event.KeyEvent.wVirtualKeyCode==VK_NUMLOCK||rec->Event.KeyEvent.wVirtualKeyCode==VK_CAPITAL||rec->Event.KeyEvent.wVirtualKeyCode==VK_SCROLL))
			{
				INPUT_RECORD pinp;
				DWORD nread;
				ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &pinp, 1, &nread);
				continue;
			}

#if defined(DETECT_ALT_ENTER)
			/*
			   Windowed -> FullScreen
			     KEY_EVENT_RECORD:   Dn, Vk="VK_MENU"    FarAltEnter:  0
			     FOCUS_EVENT_RECORD: FALSE               FarAltEnter:  1
			       FOCUS_EVENT_RECORD: TRUE                FarAltEnter:  1
			       FOCUS_EVENT_RECORD: TRUE                FarAltEnter:  1

			   FullScreen -> Windowed
			     KEY_EVENT_RECORD:   Dn, Vk="VK_MENU"    FarAltEnter:  1
			     WINDOW_BUFFER_SIZE_RECORD: Size=[W,H]   FarAltEnter:  0
			       FOCUS_EVENT_RECORD: FALSE               FarAltEnter:  0
			       FOCUS_EVENT_RECORD: TRUE                FarAltEnter:  0
			*/

			if (rec->EventType==KEY_EVENT && rec->Event.KeyEvent.wVirtualKeyCode == VK_MENU && rec->Event.KeyEvent.bKeyDown)
			{
				AltEnter=1;
			}
			else if (AltEnter &&
			         (rec->EventType==FOCUS_EVENT && !rec->Event.FocusEvent.bSetFocus ||
			          rec->EventType==WINDOW_BUFFER_SIZE_EVENT) &&
			         PrevFarAltEnterMode != FarAltEnter(FAR_CONSOLE_GET_MODE))
			{
				CONSOLE_SCREEN_BUFFER_INFO csbi;
				GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE),&csbi);
				_SVS(SysLog(L"AltEnter >>> dwSize={%d,%d} srWindow={%d,%d,%d,%d} dwMaximumWindowSize={%d,%d}  ScrX=%d (%d) ScrY=%d (%d)",csbi.dwSize.X,csbi.dwSize.Y,csbi.srWindow.Left, csbi.srWindow.Top,csbi.srWindow.Right,csbi.srWindow.Bottom,csbi.dwMaximumWindowSize.X,csbi.dwMaximumWindowSize.Y,ScrX,PrevScrX,ScrY,PrevScrY));

				if (rec->EventType==FOCUS_EVENT)
				{
					DWORD ReadCount2;
					INPUT_RECORD TmpRec2;
					TmpRec2.EventType=WINDOW_BUFFER_SIZE_EVENT;
					TmpRec2.Event.WindowBufferSizeEvent.dwSize.X=csbi.dwSize.X;
					TmpRec2.Event.WindowBufferSizeEvent.dwSize.Y=csbi.dwSize.Y;
					WriteConsoleInput(GetStdHandle(STD_INPUT_HANDLE),&TmpRec2,1,&ReadCount2); // ������ ����� ������!
				}
				else
					AltEnter=1;

				//PrevFarAltEnterMode=FarAltEnter(FAR_CONSOLE_GET_MODE);
				/*
				  DWORD ReadCount2,ReadCount3;
				  GetNumberOfConsoleInputEvents(GetStdHandle(STD_INPUT_HANDLE),&ReadCount2);
				  if(ReadCount2 >= 3)
				  {
				    INPUT_RECORD TmpRec2[2];
				    ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE),TmpRec2,2,&ReadCount3); // ������ 2, ������ ��������� �����
				  }
				*/
			}
			else
				AltEnter=0;

#endif

			// ��� ����� ����� ������ � ������� ���������� ����� - ��������� �� �������� - � ��� ��������
			// ... ����� ��������� � ������� ��������� ����� ������������
			if (rec->EventType==KEY_EVENT && IsProcessAssignMacroKey)
			{
				if (rec->Event.KeyEvent.wVirtualKeyCode == VK_SHIFT)
				{
					if (rec->Event.KeyEvent.dwControlKeyState&ENHANCED_KEY)
					{
						/*
						Left Right ��������� ����������� �������
						Dn, 1, Vk=0x0010, Scan=0x002A Ctrl=0x00000030 (caSa - cecN)
						Dn, 1, Vk=0x0025, Scan=0x004B Ctrl=0x00000130 (caSa - cEcN)
						Up, 1, Vk=0x0025, Scan=0x004B Ctrl=0x00000130 (caSa - cEcN)
						Dn, 1, Vk=0x0027, Scan=0x004D Ctrl=0x00000130 (caSa - cEcN)
						Up, 1, Vk=0x0027, Scan=0x004D Ctrl=0x00000130 (caSa - cEcN)
						Up, 1, Vk=0x0010, Scan=0x002A Ctrl=0x00000020 (casa - cecN)
						*/
						INPUT_RECORD pinp;
						DWORD nread;
						ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &pinp, 1, &nread);
						continue;
					}

					/* ��������� �����, �.�.
					NumLock=ON Shift-Numpad1
					   Dn, 1, Vk=0x0010, Scan=0x002A Ctrl=0x00000030 (caSa - cecN)
					   Dn, 1, Vk=0x0023, Scan=0x004F Ctrl=0x00000020 (casa - cecN)
					   Up, 1, Vk=0x0023, Scan=0x004F Ctrl=0x00000020 (casa - cecN)
					>>>Dn, 1, Vk=0x0010, Scan=0x002A Ctrl=0x00000030 (caSa - cecN)
					   Up, 1, Vk=0x0010, Scan=0x002A Ctrl=0x00000020 (casa - cecN)
					����� ��������� ������ ����
					*/
					if (rec->Event.KeyEvent.bKeyDown)
					{
						if (!ShiftState)
							ShiftState=TRUE;
						else // ����� ������ �� �������... ���� ����� ������ ����
						{
							INPUT_RECORD pinp;
							DWORD nread;
							ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &pinp, 1, &nread);
							continue;
						}
					}
					else if (!rec->Event.KeyEvent.bKeyDown)
						ShiftState=FALSE;
				}
			}

			// // _SVS(INPUT_RECORD_DumpBuffer());
#if 0

			if (rec->EventType==KEY_EVENT)
			{
				// ����� ���������� ���������� ������ �������
				DWORD ReadCount2;
				GetNumberOfConsoleInputEvents(GetStdHandle(STD_INPUT_HANDLE),&ReadCount2);

				// ���� �� ���������� �����, �� ���������� ��� �� ������� KEY_EVENT
				if (ReadCount2 > 1)
				{
					INPUT_RECORD *TmpRec=(INPUT_RECORD*)xf_malloc(sizeof(INPUT_RECORD)*ReadCount2);

					if (TmpRec)
					{
						DWORD ReadCount3;
						INPUT_RECORD TmpRec2;
						PeekConsoleInput(GetStdHandle(STD_INPUT_HANDLE),TmpRec,ReadCount2,&ReadCount3);

						for (int I=0; I < ReadCount2; ++I)
						{
							if (TmpRec[I].EventType!=KEY_EVENT)
								break;

							// // _SVS(SysLog(L"%d> %s",I,_INPUT_RECORD_Dump(rec)));
							ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE),&TmpRec2,1,&ReadCount3);

							if (TmpRec[I].Event.KeyEvent.bKeyDown==1)
							{
								if (TmpRec[I].Event.KeyEvent.uChar.AsciiChar != 0)
									WriteInput(TmpRec[I].Event.KeyEvent.uChar.AsciiChar,0);
							}
							else if (TmpRec[I].Event.KeyEvent.wVirtualKeyCode==0x12)
							{
								if (TmpRec[I].Event.KeyEvent.uChar.AsciiChar != 0)
									WriteInput(TmpRec[I].Event.KeyEvent.uChar.AsciiChar,0);
							}
						}

						// ��������� ������
						xf_free(TmpRec);
						return KEY_NONE;
					}
				}
			}

#endif
			break;
		}

		ScrBuf.Flush();
		Sleep(1);

		// ��������� �������� �������� ������������ ����
		if (Opt.Mouse) // � ����� �� ��� �������???
			SetFarConsoleMode();

		if (CloseFAR)
		{
//      CloseFAR=FALSE;
			/* $ 30.08.2001 IS
			   ��� �������������� �������� ���� �������� ����� ���� ��� ��, ��� � ���
			   ������� �� F10 � �������, ������ �� ����������� ������������� ��������,
			   ���� ��� ��������.
			*/
			if (!Opt.CloseConsoleRule)
				FrameManager->IsAnyFrameModified(TRUE);
			else
				FrameManager->ExitMainLoop(FALSE);

			return KEY_NONE;
		}

		if ((LoopCount & 15)==0)
		{
			clock_t CurTime=clock();
			clock_t TimeAfterExec=CurTime-StartExecTime;

			if (EnableShowTime)
				ShowTime(0);

			if (WaitInMainLoop)
			{
				if (Opt.InactivityExit && Opt.InactivityExitTime>0 &&
				        CurTime-StartIdleTime>Opt.InactivityExitTime*60000 &&
				        FrameManager->GetFrameCount()==1)
				{
					FrameManager->ExitMainLoop(FALSE);
					return(KEY_NONE);
				}

				if ((LoopCount & 63)==0)
				{
					static int Reenter=0;

					if (!Reenter)
					{
						Reenter++;
						SHORT X,Y;
						GetRealCursorPos(X,Y);

						if (X==0 && Y==ScrY && CtrlObject->CmdLine->IsVisible())
						{
							for (;;)
							{
								INPUT_RECORD tmprec;
								int Key=GetInputRecord(&tmprec);

								if ((DWORD)Key==KEY_NONE || ((DWORD)Key!=KEY_SHIFT && tmprec.Event.KeyEvent.bKeyDown))
									break;
							}

							CtrlObject->Cp()->SetScreenPosition();
							ScrBuf.ResetShadow();
							ScrBuf.Flush();
						}

						Reenter--;
					}

					static int UpdateReenter=0;

					if (!UpdateReenter && CurTime-KeyPressedLastTime>700)
					{
						UpdateReenter=TRUE;
						CtrlObject->Cp()->LeftPanel->UpdateIfChanged(UIC_UPDATE_NORMAL);
						CtrlObject->Cp()->RightPanel->UpdateIfChanged(UIC_UPDATE_NORMAL);
						UpdateReenter=FALSE;
					}
				}
			}

			if (StartExecTime!=0 && TimeAfterExec>2000)
			{
				StartExecTime=0;

				if (!IsWindowed() && !Opt.Mouse)
				{
					SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),ENABLE_WINDOW_INPUT|ENABLE_MOUSE_INPUT);
					SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),ENABLE_WINDOW_INPUT);
				}

				ConsoleTitle::SetFarTitle(NULL);//LastFarTitle);
			}

			if (Opt.ScreenSaver && Opt.ScreenSaverTime>0 &&
			        CurTime-StartIdleTime>Opt.ScreenSaverTime*60000)
				if (!ScreenSaver(WaitInMainLoop))
					return(KEY_NONE);

			if (!WaitInMainLoop && LoopCount==64)
			{
				LastEventIdle=TRUE;
				memset(rec,0,sizeof(*rec));
				rec->EventType=KEY_EVENT;
				return(KEY_IDLE);
			}
		}

		if (PluginSynchroManager.Process())
		{
			memset(rec,0,sizeof(*rec));
			return KEY_NONE;
		}

		LoopCount++;
	} // while (1)

	clock_t CurClock=clock();

	if (rec->EventType==FOCUS_EVENT)
	{
		/* $ 28.04.2001 VVM
		  + �� ������ ���������� ���� ����� ������, �� � ��������� ������ */
		ShiftPressed=RightShiftPressedLast=ShiftPressedLast=FALSE;
		CtrlPressed=CtrlPressedLast=RightCtrlPressedLast=FALSE;
		AltPressed=AltPressedLast=RightAltPressedLast=FALSE;
		MouseButtonState=0;
		ShiftState=FALSE;
		PressedLastTime=0;
		ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE),rec,1,&ReadCount);
		CalcKey=rec->Event.FocusEvent.bSetFocus?KEY_GOTFOCUS:KEY_KILLFOCUS;
		memset(rec,0,sizeof(*rec)); // ����� � ProcessEditorInput ����� ���� �������� - ������ ����� ����������
		rec->EventType=KEY_EVENT;

		//���� ������ ��� ����� ���������� � ��������� ������� � �.�. ����� ������ ������
		if (CalcKey == KEY_GOTFOCUS)
			RestoreConsoleWindowInfo();
		else
			SaveConsoleWindowInfo();

		return CalcKey;
	}

	if (rec->EventType==KEY_EVENT)
	{
		/* ��������� �����, �.�.
		NumLock=ON Shift-Numpad1
		   Dn, 1, Vk=0x0010, Scan=0x002A Ctrl=0x00000030 (caSa - cecN)
		   Dn, 1, Vk=0x0023, Scan=0x004F Ctrl=0x00000020 (casa - cecN)
		   Up, 1, Vk=0x0023, Scan=0x004F Ctrl=0x00000020 (casa - cecN)
		>>>Dn, 1, Vk=0x0010, Scan=0x002A Ctrl=0x00000030 (caSa - cecN)
		   Up, 1, Vk=0x0010, Scan=0x002A Ctrl=0x00000020 (casa - cecN)
		����� ��������� ������ ����
		*/
		/*
		    if(rec->Event.KeyEvent.wVirtualKeyCode == VK_SHIFT)
		    {
		      if(rec->Event.KeyEvent.bKeyDown)
		      {
		        if(!ShiftState)
		          ShiftState=TRUE;
		        else // ����� ������ �� �������... ���� ����� ������ ����
		        {
		          INPUT_RECORD pinp;
		          DWORD nread;
		          ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &pinp, 1, &nread);
		          return KEY_NONE;
		        }
		      }
		      else if(!rec->Event.KeyEvent.bKeyDown)
		        ShiftState=FALSE;
		    }

		    if((rec->Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED) == 0 && ShiftState)
		      rec->Event.KeyEvent.dwControlKeyState|=SHIFT_PRESSED;
		*/
//_SVS(if(rec->EventType==KEY_EVENT)SysLog(L"%s",_INPUT_RECORD_Dump(rec)));
		DWORD CtrlState=rec->Event.KeyEvent.dwControlKeyState;

//_SVS(if(rec->EventType==KEY_EVENT)SysLog(L"[%d] if(rec->EventType==KEY_EVENT) >>> %s",__LINE__,_INPUT_RECORD_Dump(rec)));
		if (CtrlObject && CtrlObject->Macro.IsRecording())
		{
			static WORD PrevVKKeyCode=0; // NumLock+Cursor
			WORD PrevVKKeyCode2=PrevVKKeyCode;
			PrevVKKeyCode=rec->Event.KeyEvent.wVirtualKeyCode;

			/* 1.07.2001 KM
			  ��� ���������� Shift-Enter � ������� ����������
			  ������� Shift ����� ���������� ������.
			*/
			if ((PrevVKKeyCode2==VK_SHIFT && PrevVKKeyCode==VK_RETURN &&
			        rec->Event.KeyEvent.bKeyDown) ||
			        (PrevVKKeyCode2==VK_RETURN && PrevVKKeyCode==VK_SHIFT &&
			         !rec->Event.KeyEvent.bKeyDown))
			{
				if (PrevVKKeyCode2 != VK_SHIFT)
				{
					INPUT_RECORD pinp;
					DWORD nread;
					// ������ �� �������...
					ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &pinp, 1, &nread);
					return KEY_NONE;
				}
			}
		}

		if (AltPressed && (CtrlState & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED))==0)
			DetectWindowedMode();

		CtrlPressed=(CtrlState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED));
		AltPressed=(CtrlState & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED));
		ShiftPressed=(CtrlState & SHIFT_PRESSED);
		RightCtrlPressed=(CtrlState & RIGHT_CTRL_PRESSED);
		RightAltPressed=(CtrlState & RIGHT_ALT_PRESSED);
		RightShiftPressed=(CtrlState & SHIFT_PRESSED); //???
		KeyPressedLastTime=CurClock;

		/* $ 24.08.2000 SVS
		   + ���������� �� ������� KEY_CTRLALTSHIFTRELEASE
		*/
		if (IsKeyCASPressed && (Opt.CASRule&1) && (!CtrlPressed || !AltPressed || !ShiftPressed))
		{
			IsKeyCASPressed=FALSE;
			return KEY_CTRLALTSHIFTRELEASE;
		}

		if (IsKeyRCASPressed && (Opt.CASRule&2) && (!RightCtrlPressed || !RightAltPressed || !ShiftPressed))
		{
			IsKeyRCASPressed=FALSE;
			return KEY_RCTRLALTSHIFTRELEASE;
		}
	}

//_SVS(if(rec->EventType==KEY_EVENT)SysLog(L"[%d] if(rec->EventType==KEY_EVENT) >>> %s",__LINE__,_INPUT_RECORD_Dump(rec)));
	ReturnAltValue=FALSE;
	CalcKey=CalcKeyCode(rec,TRUE,&NotMacros);
	/*
	  if(CtrlObject && CtrlObject->Macro.IsRecording() && (CalcKey == (KEY_ALT|KEY_NUMPAD0) || CalcKey == (KEY_ALT|KEY_INS)))
	  {
	    if(CtrlObject->Macro.ProcessKey(CalcKey))
	    {
	      RunGraber();
	      rec->EventType=0;
	      CalcKey=KEY_NONE;
	    }
	    return(CalcKey);
	  }
	*/

//_SVS(SysLog(L"1) CalcKey=%s",_FARKEY_ToName(CalcKey)));
	if (ReturnAltValue && !NotMacros)
	{
		_KEYMACRO(CleverSysLog Clev(L"CALL(2) CtrlObject->Macro.ProcessKey()"));

		if (CtrlObject!=NULL && CtrlObject->Macro.ProcessKey(CalcKey))
		{
			rec->EventType=0;
			CalcKey=KEY_NONE;
		}

		return(CalcKey);
	}

	ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE),rec,1,&ReadCount);
#if 0
	ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE),rec,1,&ReadCount);

	// ��� ����� ����� ������ � ������� ���������� ����� - ��������� �� �������� - � ��� ��������
	// ... ����� ��������� � ������� ��������� ����� ������������
	/* ��������� �����, �.�.
	NumLock=ON Shift-Numpad1
	   Dn, 1, Vk=0x0010, Scan=0x002A Ctrl=0x00000030 (caSa - cecN)
	   Dn, 1, Vk=0x0023, Scan=0x004F Ctrl=0x00000020 (casa - cecN)
	   Up, 1, Vk=0x0023, Scan=0x004F Ctrl=0x00000020 (casa - cecN)
	>>>Dn, 1, Vk=0x0010, Scan=0x002A Ctrl=0x00000030 (caSa - cecN)
	   Up, 1, Vk=0x0010, Scan=0x002A Ctrl=0x00000020 (casa - cecN)
	����� ��������� ������ ����
	*/
	if (rec->Event.KeyEvent.wVirtualKeyCode == VK_SHIFT &&
	        rec->EventType==KEY_EVENT &&
	        IsProcessAssignMacroKey)
	{
		if (rec->Event.KeyEvent.bKeyDown)
		{
			if (!ShiftState)
				ShiftState=TRUE;
		}
		else if (!rec->Event.KeyEvent.bKeyDown)
			ShiftState=FALSE;
	}

#endif

	if (EnableShowTime)
		ShowTime(1);

	/*& 17.05.2001 OT ��������� ������ �������, ������� �������*/
	if (rec->EventType==WINDOW_BUFFER_SIZE_EVENT)
	{
#if defined(DETECT_ALT_ENTER)
		//_SVS(CleverSysLog Clev(L""));
		//_SVS(SysLog(L"ScrX=%d (%d) ScrY=%d (%d), AltEnter=%d",ScrX,PrevScrX,ScrY,PrevScrY,AltEnter));
		//_SVS(SysLog(L"FarAltEnter -> %d",FarAltEnter(FAR_CONSOLE_GET_MODE)));
#endif
		int PScrX=ScrX;
		int PScrY=ScrY;
		//// // _SVS(SysLog(1,"GetInputRecord(WINDOW_BUFFER_SIZE_EVENT)"));
		Sleep(1);
		GetVideoMode(CurScreenBufferInfo);
#if defined(DETECT_ALT_ENTER)

//    if((PScrX == PrevScrX && PScrY == PrevScrY) && PrevFarAltEnterMode == FarAltEnter(FAR_CONSOLE_GET_MODE))
		if (PScrX+1 == CurScreenBufferInfo.dwSize.X &&
		        PScrY+1 == CurScreenBufferInfo.dwSize.Y &&
		        PScrX+1 <= CurScreenBufferInfo.dwMaximumWindowSize.X &&
		        PScrY+1 <= CurScreenBufferInfo.dwMaximumWindowSize.Y
		        && PrevFarAltEnterMode == FarAltEnter(FAR_CONSOLE_GET_MODE)
		   )
#else
		if (PScrX+1 == CurScreenBufferInfo.dwSize.X &&
		        PScrY+1 == CurScreenBufferInfo.dwSize.Y)
#endif
		{
#if defined(DETECT_ALT_ENTER)
			//_SVS(SysLog(L"return KEY_NONE"));
#endif
			return KEY_NONE;
		}
		else
		{
#if defined(DETECT_ALT_ENTER)

			//_SVS(SysLog(L"return KEY_CONSOLE_BUFFER_RESIZE ScrX=%d (%d) ScrY=%d (%d)",ScrX,PrevScrX,ScrY,PrevScrY));
			if (FarAltEnter(FAR_CONSOLE_GET_MODE) == FAR_CONSOLE_FULLSCREEN)
			{
				//_SVS(SysLog(L"call ChangeVideoMode"));
				PrevFarAltEnterMode=FarAltEnter(FAR_CONSOLE_GET_MODE);
				ChangeVideoMode(PScrY==24?50:25,80);
				GetVideoMode(CurScreenBufferInfo);
			}
			else
			{
				//_SVS(SysLog(L"PrevScrX=PScrX"));
				PrevScrX=PScrX;
				PrevScrY=PScrY;
			}

#else
			PrevScrX=PScrX;
			PrevScrY=PScrY;
			//// // _SVS(SysLog(-1,"GetInputRecord(WINDOW_BUFFER_SIZE_EVENT); return KEY_CONSOLE_BUFFER_RESIZE"));
#endif
			Sleep(1);

			if (FrameManager)
			{
#if defined(DETECT_ALT_ENTER)
				//_SVS(SysLog(L"if(FrameManager)"));
#endif
				// �������� ������ (������ ��� ������!)
				LockScreen LckScr;

				if (GlobalSaveScrPtr)
					GlobalSaveScrPtr->Discard();

				FrameManager->ResizeAllFrame();
				FrameManager->GetCurrentFrame()->Show();
				//// // _SVS(SysLog(L"PreRedrawFunc = %p",PreRedrawFunc));
				PreRedrawItem preRedrawItem=PreRedraw.Peek();

				if (preRedrawItem.PreRedrawFunc)
				{
					preRedrawItem.PreRedrawFunc();
				}
			}

			return(KEY_CONSOLE_BUFFER_RESIZE);
		}
	}

	if (rec->EventType==KEY_EVENT)
	{
		DWORD CtrlState=rec->Event.KeyEvent.dwControlKeyState;
		DWORD KeyCode=rec->Event.KeyEvent.wVirtualKeyCode;
		CtrlPressed=(CtrlState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED));
		AltPressed=(CtrlState & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED));
		RightCtrlPressed=(CtrlState & RIGHT_CTRL_PRESSED);
		RightAltPressed=(CtrlState & RIGHT_ALT_PRESSED);

		// ��� NumPad!
		if ((CalcKey&(KEY_CTRL|KEY_SHIFT|KEY_ALT|KEY_RCTRL|KEY_RALT)) == KEY_SHIFT &&
		        (CalcKey&KEY_MASKF) >= KEY_NUMPAD0 && (CalcKey&KEY_MASKF) <= KEY_NUMPAD9)
			ShiftPressed=SHIFT_PRESSED;
		else
			ShiftPressed=(CtrlState & SHIFT_PRESSED);

		if ((KeyCode==VK_F16 && ReadKey==VK_F16) || KeyCode==0)
			return(KEY_NONE);

		if (!rec->Event.KeyEvent.bKeyDown &&
		        (KeyCode==VK_SHIFT || KeyCode==VK_CONTROL || KeyCode==VK_MENU) &&
		        CurClock-PressedLastTime<500)
		{
			int Key=-1;

			if (ShiftPressedLast && KeyCode==VK_SHIFT)
			{
				if (ShiftPressedLast)
				{
					Key=KEY_SHIFT;
					//// // _SVS(SysLog(L"ShiftPressedLast, Key=KEY_SHIFT"));
				}
				else if (RightShiftPressedLast)
				{
					Key=KEY_RSHIFT;
					//// // _SVS(SysLog(L"RightShiftPressedLast, Key=KEY_RSHIFT"));
				}
			}

			if (KeyCode==VK_CONTROL)
			{
				if (CtrlPressedLast)
				{
					Key=KEY_CTRL;
					//// // _SVS(SysLog(L"CtrlPressedLast, Key=KEY_CTRL"));
				}
				else if (RightCtrlPressedLast)
				{
					Key=KEY_RCTRL;
					//// // _SVS(SysLog(L"CtrlPressedLast, Key=KEY_RCTRL"));
				}
			}

			if (KeyCode==VK_MENU)
			{
				if (AltPressedLast)
				{
					Key=KEY_ALT;
					//// // _SVS(SysLog(L"AltPressedLast, Key=KEY_ALT"));
				}
				else if (RightAltPressedLast)
				{
					Key=KEY_RALT;
					//// // _SVS(SysLog(L"RightAltPressedLast, Key=KEY_RALT"));
				}
			}

			{
				_KEYMACRO(CleverSysLog Clev(L"CALL(3) CtrlObject->Macro.ProcessKey()"));

				if (Key!=-1 && !NotMacros && CtrlObject!=NULL && CtrlObject->Macro.ProcessKey(Key))
				{
					rec->EventType=0;
					Key=KEY_NONE;
				}
			}

			if (Key!=-1)
				return(Key);
		}

		ShiftPressedLast=RightShiftPressedLast=FALSE;
		CtrlPressedLast=RightCtrlPressedLast=FALSE;
		AltPressedLast=RightAltPressedLast=FALSE;
		ShiftPressedLast=(KeyCode==VK_SHIFT && rec->Event.KeyEvent.bKeyDown) ||
		                 (KeyCode==VK_RETURN && ShiftPressed && !rec->Event.KeyEvent.bKeyDown);

		if (!ShiftPressedLast)
			if (KeyCode==VK_CONTROL && rec->Event.KeyEvent.bKeyDown)
			{
				if (CtrlState & RIGHT_CTRL_PRESSED)
				{
					RightCtrlPressedLast=TRUE;
					//// // _SVS(SysLog(L"RightCtrlPressedLast=TRUE;"));
				}
				else
				{
					CtrlPressedLast=TRUE;
					//// // _SVS(SysLog(L"CtrlPressedLast=TRUE;"));
				}
			}

		if (!ShiftPressedLast && !CtrlPressedLast && !RightCtrlPressedLast)
		{
			if (KeyCode==VK_MENU && rec->Event.KeyEvent.bKeyDown)
			{
				if (CtrlState & RIGHT_ALT_PRESSED)
				{
					RightAltPressedLast=TRUE;
				}
				else
				{
					AltPressedLast=TRUE;
				}

				PressedLastTime=CurClock;
			}
		}
		else
			PressedLastTime=CurClock;

		if (KeyCode==VK_SHIFT || KeyCode==VK_MENU || KeyCode==VK_CONTROL || KeyCode==VK_NUMLOCK || KeyCode==VK_SCROLL || KeyCode==VK_CAPITAL)
		{
			if ((KeyCode==VK_NUMLOCK || KeyCode==VK_SCROLL || KeyCode==VK_CAPITAL) &&
			        (CtrlState&(LEFT_CTRL_PRESSED|LEFT_ALT_PRESSED|SHIFT_PRESSED|RIGHT_ALT_PRESSED|RIGHT_CTRL_PRESSED))
			   )
			{
				// TODO:
				;
			}
			else
			{
				/* $ 24.08.2000 SVS
				   + ���������� �� ������� KEY_CTRLALTSHIFTPRESS
				*/
				switch (KeyCode)
				{
					case VK_SHIFT:
					case VK_MENU:
					case VK_CONTROL:

						if (!IsKeyCASPressed && CtrlPressed && AltPressed && ShiftPressed)
						{
							if (!IsKeyRCASPressed && RightCtrlPressed && RightAltPressed && RightShiftPressed)
							{
								if (Opt.CASRule&2)
								{
									IsKeyRCASPressed=TRUE;
									return (KEY_RCTRLALTSHIFTPRESS);
								}
							}
							else if (Opt.CASRule&1)
							{
								IsKeyCASPressed=TRUE;
								return (KEY_CTRLALTSHIFTPRESS);
							}
						}

						break;
					case VK_LSHIFT:
					case VK_LMENU:
					case VK_LCONTROL:

						if (!IsKeyRCASPressed && RightCtrlPressed && RightAltPressed && RightShiftPressed)
						{
							if ((Opt.CASRule&2))
							{
								IsKeyRCASPressed=TRUE;
								return (KEY_RCTRLALTSHIFTPRESS);
							}
						}

						break;
				}

				return(KEY_NONE);
			}
		}

		Panel::EndDrag();
	}

	if (rec->EventType==MOUSE_EVENT)
	{
		memcpy(&lastMOUSE_EVENT_RECORD,&rec->Event.MouseEvent,sizeof(MOUSE_EVENT_RECORD));
		PreMouseEventFlags=MouseEventFlags;
		MouseEventFlags=rec->Event.MouseEvent.dwEventFlags;
		DWORD CtrlState=rec->Event.MouseEvent.dwControlKeyState;
		KeyMacro::SetMacroConst(constMsCtrlState,(__int64)CtrlState);
		/*
		    // ������ �� ���������� ;-) �������� ����������� ������ ��� �������� �����
		    if(CtrlState != (CtrlPressed|AltPressed|ShiftPressed))
		    {
		      static INPUT_RECORD TempRec[2]={
		        {KEY_EVENT,{1,1,VK_F16,0,{0},0}},
		        {KEY_EVENT,{0,1,VK_F16,0,{0},0}}
		      };
		      DWORD WriteCount;
		      TempRec[0].Event.KeyEvent.dwControlKeyState=TempRec[1].Event.KeyEvent.dwControlKeyState=CtrlState;
					WriteConsoleInput(GetStdHandle(STD_INPUT_HANDLE),TempRec,2,&WriteCount);
		    }
		*/
		CtrlPressed=(CtrlState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED));
		AltPressed=(CtrlState & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED));
		ShiftPressed=(CtrlState & SHIFT_PRESSED);
		RightCtrlPressed=(CtrlState & RIGHT_CTRL_PRESSED);
		RightAltPressed=(CtrlState & RIGHT_ALT_PRESSED);
		RightShiftPressed=(CtrlState & SHIFT_PRESSED);
		DWORD BtnState=rec->Event.MouseEvent.dwButtonState;
		KeyMacro::SetMacroConst(constMsButton,(__int64)rec->Event.MouseEvent.dwButtonState);

		if (MouseEventFlags != MOUSE_MOVED)
		{
//// // _SVS(SysLog(L"1. CtrlState=%X PrevRButtonPressed=%d,RButtonPressed=%d",CtrlState,PrevRButtonPressed,RButtonPressed));
			PrevMouseButtonState=MouseButtonState;
		}

		MouseButtonState=BtnState;
//// // _SVS(SysLog(L"2. BtnState=%X PrevRButtonPressed=%d,RButtonPressed=%d",BtnState,PrevRButtonPressed,RButtonPressed));
		PrevMouseX=MouseX;
		PrevMouseY=MouseY;
		MouseX=rec->Event.MouseEvent.dwMousePosition.X;
		MouseY=rec->Event.MouseEvent.dwMousePosition.Y;
		KeyMacro::SetMacroConst(constMsX,(__int64)MouseX);
		KeyMacro::SetMacroConst(constMsY,(__int64)MouseY);

		/* $ 26.04.2001 VVM
		   + ��������� �������� �����. */
		if (MouseEventFlags == MOUSE_WHEELED)
		{ // ���������� ������ � ������� �� ����.�������
			short zDelta = HIWORD(rec->Event.MouseEvent.dwButtonState);
			CalcKey = (zDelta>0)?KEY_MSWHEEL_UP:KEY_MSWHEEL_DOWN;
			/* $ 27.04.2001 SVS
			   �� ���� ������ �������� ������� ��� ��������� ������, ��-�� ����
			   ������ ���� ������������ � �������� ����� ����� "ShiftMsWheelUp"
			*/
			CalcKey |= (CtrlState&SHIFT_PRESSED?KEY_SHIFT:0)|
			           (CtrlState&(LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)?KEY_CTRL:0)|
			           (CtrlState&(LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED)?KEY_ALT:0);
			/* $ 14.05.2002 VVM
			  - ������� ��� ������ ������. ����� ������ ����� (ProcessEditorInput) */
			memset(rec,0,sizeof(*rec)); // ����� � ProcessEditorInput ����� ���� �������� - ������ ����� ����������
			rec->EventType = KEY_EVENT;
		} /* if */

		// ��������� ��������������� �������� (NT>=6)
		if (MouseEventFlags == MOUSE_HWHEELED)
		{
			short zDelta = HIWORD(rec->Event.MouseEvent.dwButtonState);
			CalcKey = (zDelta>0)?KEY_MSWHEEL_RIGHT:KEY_MSWHEEL_LEFT;
			CalcKey |= (CtrlState&SHIFT_PRESSED?KEY_SHIFT:0)|
			           (CtrlState&(LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)?KEY_CTRL:0)|
			           (CtrlState&(LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED)?KEY_ALT:0);
			memset(rec,0,sizeof(*rec));
			rec->EventType = KEY_EVENT;
		} /* if */

		if (rec->EventType==MOUSE_EVENT && (!ExcludeMacro||ProcessMouse) && CtrlObject && (ProcessMouse || !(CtrlObject->Macro.IsRecording() || CtrlObject->Macro.IsExecuting())))
		{
			if (MouseEventFlags != MOUSE_MOVED)
			{
				DWORD MsCalcKey=0;
#if 0

				if (rec->Event.MouseEvent.dwButtonState&RIGHTMOST_BUTTON_PRESSED)
					MsCalcKey=(MouseEventFlags == DOUBLE_CLICK)?KEY_MSRDBLCLICK:KEY_MSRCLICK;
				else if (rec->Event.MouseEvent.dwButtonState&FROM_LEFT_1ST_BUTTON_PRESSED)
					MsCalcKey=(MouseEventFlags == DOUBLE_CLICK)?KEY_MSLDBLCLICK:KEY_MSLCLICK;
				else if (rec->Event.MouseEvent.dwButtonState&FROM_LEFT_2ND_BUTTON_PRESSED)
					MsCalcKey=(MouseEventFlags == DOUBLE_CLICK)?KEY_MSM1DBLCLICK:KEY_MSM1CLICK;
				else if (rec->Event.MouseEvent.dwButtonState&FROM_LEFT_3RD_BUTTON_PRESSED)
					MsCalcKey=(MouseEventFlags == DOUBLE_CLICK)?KEY_MSM2DBLCLICK:KEY_MSM2CLICK;
				else if (rec->Event.MouseEvent.dwButtonState&FROM_LEFT_4TH_BUTTON_PRESSED)
					MsCalcKey=(MouseEventFlags == DOUBLE_CLICK)?KEY_MSM3DBLCLICK:KEY_MSM3CLICK;

#else

				if (rec->Event.MouseEvent.dwButtonState&RIGHTMOST_BUTTON_PRESSED)
					MsCalcKey=KEY_MSRCLICK;
				else if (rec->Event.MouseEvent.dwButtonState&FROM_LEFT_1ST_BUTTON_PRESSED)
					MsCalcKey=KEY_MSLCLICK;
				else if (rec->Event.MouseEvent.dwButtonState&FROM_LEFT_2ND_BUTTON_PRESSED)
					MsCalcKey=KEY_MSM1CLICK;
				else if (rec->Event.MouseEvent.dwButtonState&FROM_LEFT_3RD_BUTTON_PRESSED)
					MsCalcKey=KEY_MSM2CLICK;
				else if (rec->Event.MouseEvent.dwButtonState&FROM_LEFT_4TH_BUTTON_PRESSED)
					MsCalcKey=KEY_MSM3CLICK;

#endif

				if (MsCalcKey)
				{
					MsCalcKey |= (CtrlState&SHIFT_PRESSED?KEY_SHIFT:0)|
					             (CtrlState&(LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)?KEY_CTRL:0)|
					             (CtrlState&(LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED)?KEY_ALT:0);

					// ��� WaitKey()
					if (ProcessMouse)
						return MsCalcKey;
					else if (CtrlObject->Macro.ProcessKey(MsCalcKey))
					{
						memset(rec,0,sizeof(*rec)); // ����� � ProcessEditorInput ����� ���� �������� - ������ ����� ����������
						return KEY_NONE;
					}
				}
			}
		}
	}

	int GrayKey=(CalcKey==KEY_ADD || CalcKey==KEY_SUBTRACT || CalcKey==KEY_MULTIPLY);

	if (ReadKey!=0 && !GrayKey)
		CalcKey=ReadKey;

	{
		_KEYMACRO(CleverSysLog Clev(L"CALL(1) CtrlObject->Macro.ProcessKey()"));

		if (!NotMacros && CtrlObject!=NULL && CtrlObject->Macro.ProcessKey(CalcKey))
		{
			rec->EventType=0;
			CalcKey=KEY_NONE;
		}
	}
	return(CalcKey);
}

DWORD PeekInputRecord(INPUT_RECORD *rec,bool ExcludeMacro)
{
	DWORD ReadCount;
	DWORD Key;
	ScrBuf.Flush();

	if (KeyQueue && (Key=KeyQueue->Peek()) != 0)
	{
		int VirtKey,ControlState;
		ReadCount=TranslateKeyToVK(Key,VirtKey,ControlState,rec)?1:0;
	}
	else if ((!ExcludeMacro) && (Key=CtrlObject->Macro.PeekKey()) != 0)
	{
		int VirtKey,ControlState;
		ReadCount=TranslateKeyToVK(Key,VirtKey,ControlState,rec)?1:0;
	}
	else
	{
		PeekConsoleInput(GetStdHandle(STD_INPUT_HANDLE),rec,1,&ReadCount);
	}

	if (ReadCount==0)
		return(0);

	return(CalcKeyCode(rec,TRUE));
}

/* $ 24.08.2000 SVS
 + �������� � ������ WaitKey - ����������� ������� ���������� �������
     ���� KeyWait = -1 - ��� � ������
*/
DWORD WaitKey(DWORD KeyWait,DWORD delayMS,bool ExcludeMacro)
{
	int Visible=0,Size=0;

	if (KeyWait == KEY_CTRLALTSHIFTRELEASE || KeyWait == KEY_RCTRLALTSHIFTRELEASE)
	{
		GetCursorType(Visible,Size);
		SetCursorType(0,10);
	}

	clock_t CheckTime=clock()+delayMS;
	DWORD Key;

	for (;;)
	{
		INPUT_RECORD rec;
		Key=KEY_NONE;

		if (PeekInputRecord(&rec,ExcludeMacro))
			Key=GetInputRecord(&rec,ExcludeMacro,true);

		if (KeyWait == (DWORD)-1)
		{
			if ((Key&(~KEY_CTRLMASK)) < KEY_END_FKEY || IS_INTERNAL_KEY_REAL(Key))
				break;
		}
		else if (Key == KeyWait)
			break;

		if (delayMS && clock() >= CheckTime)
		{
			Key=KEY_NONE;
			break;
		}

		Sleep(1);
	}

	if (KeyWait == KEY_CTRLALTSHIFTRELEASE || KeyWait == KEY_RCTRLALTSHIFTRELEASE)
		SetCursorType(Visible,Size);

	return Key;
}

int WriteInput(int Key,DWORD Flags)
{
	if (Flags&(SKEY_VK_KEYS|SKEY_IDLE))
	{
		INPUT_RECORD Rec;
		DWORD WriteCount;

		if (Flags&SKEY_IDLE)
		{
			Rec.EventType=FOCUS_EVENT;
			Rec.Event.FocusEvent.bSetFocus=TRUE;
		}
		else
		{
			Rec.EventType=KEY_EVENT;
			Rec.Event.KeyEvent.bKeyDown=1;
			Rec.Event.KeyEvent.wRepeatCount=1;
			Rec.Event.KeyEvent.wVirtualKeyCode=Key;
			Rec.Event.KeyEvent.wVirtualScanCode=MapVirtualKeyA(
			                                        Rec.Event.KeyEvent.wVirtualKeyCode, 0);

			if (Key < 0x30 || Key > 0x5A) // 0-9:;<=>?@@ A..Z  //?????
				Key=0;

			Rec.Event.KeyEvent.uChar.UnicodeChar=Rec.Event.KeyEvent.uChar.AsciiChar=Key;
			Rec.Event.KeyEvent.dwControlKeyState=0;
		}

		return WriteConsoleInput(GetStdHandle(STD_INPUT_HANDLE),&Rec,1,&WriteCount);
	}
	else if (KeyQueue)
	{
		return KeyQueue->Put(((DWORD)Key)|(Flags&SKEY_NOTMACROS?0x80000000:0));
	}
	else
		return 0;
}


int CheckForEscSilent()
{
	INPUT_RECORD rec;
	int Key;
	BOOL Processed=TRUE;
	/* TODO: �����, � ����� �� - ��, �.�.
	         �� �������� ����� ��������� CtrlObject->Macro.PeekKey() �� ESC ��� BREAK
	         �� � ���� ��� �������� - ���� �� ���� ���� ����� !!!
	*/

	// ���� � "�������"...
	if (CtrlObject->Macro.IsExecuting() != MACROMODE_NOMACRO && FrameManager->GetCurrentFrame())
	{
#if 0

		// ...�� ��� ����� ������������������ (�� Op-���)...
		if (CtrlObject->Macro.IsExecutingLastKey() && !CtrlObject->Macro.IsOpCode(CtrlObject->Macro.PeekKey()))
			CtrlObject->Macro.GetKey(); // ...�� "��������" ������
		else
			Processed=FALSE;

#else

		if (CtrlObject->Macro.IsDsableOutput())
			Processed=FALSE;

#endif
	}

	if (Processed && PeekInputRecord(&rec))
	{
		int MMode=CtrlObject->Macro.GetMode();
		CtrlObject->Macro.SetMode(MACRO_LAST); // ����� �� ����������� ������� :-)
		Key=GetInputRecord(&rec);
		CtrlObject->Macro.SetMode(MMode);

		/*
		if(Key == KEY_CONSOLE_BUFFER_RESIZE)
		{
		  // �������� ������ (������ ��� ������!)
		  LockScreen LckScr;
		  FrameManager->ResizeAllFrame();
		  FrameManager->GetCurrentFrame()->Show();
		  PreRedrawItem preRedrawItem=PreRedraw.Peek();
		  if(preRedrawItem.PreRedrawFunc)
		  {
		    preRedrawItem.PreRedrawFunc();
		  }

		}
		else
		*/
		if (Key==KEY_ESC || Key==KEY_BREAK)
			return(TRUE);
		else if (Key==KEY_ALTF9)
			FrameManager->ProcessKey(KEY_ALTF9);
	}

	if (!Processed && CtrlObject->Macro.IsExecuting() != MACROMODE_NOMACRO)
		ScrBuf.Flush();

	return(FALSE);
}

int ConfirmAbortOp()
{
//  SaveScreen SaveScr; // �����! ��������� �� ��������� ��������� �����
	BOOL rc=TRUE;
	IsProcessAssignMacroKey++; // �������� ���� �������
	// �.�. � ���� ������� ������ ������ Alt-F9!
	if (Opt.Confirm.Esc)
		rc=AbortMessage();

	IsProcessAssignMacroKey--;
	return rc;
}

/* $ 09.02.2001 IS
     ������������� ������� Esc
*/
int CheckForEsc()
{
	if (CheckForEscSilent())
		return(ConfirmAbortOp());
	else
		return(FALSE);
}

/* $ 25.07.2000 SVS
    ! ������� KeyToText ������� ���������������� - ����� � ������ FSF
*/
/* $ 01.08.2000 SVS
   ! �������������� ��������� � KeyToText - ������ ������
   Size=0 - �� ���������!
*/
static string &GetShiftKeyName(string &strName, DWORD Key,int& Len)
{
	if ((Key&KEY_RCTRL) == KEY_RCTRL)   strName += ModifKeyName[0].Name;
	else if (Key&KEY_CTRL)              strName += ModifKeyName[2].Name;

//  else if(Key&KEY_LCTRL)             strcat(Name,ModifKeyName[3].Name);

	if ((Key&KEY_RALT) == KEY_RALT)     strName += ModifKeyName[3].Name;
	else if (Key&KEY_ALT)               strName += ModifKeyName[4].Name;

//  else if(Key&KEY_LALT)    strcat(Name,ModifKeyName[6].Name);

	if (Key&KEY_SHIFT)                  strName += ModifKeyName[1].Name;

//  else if(Key&KEY_LSHIFT)  strcat(Name,ModifKeyName[0].Name);
//  else if(Key&KEY_RSHIFT)  strcat(Name,ModifKeyName[1].Name);
	if (Key&KEY_M_SPEC)                 strName += ModifKeyName[5].Name;
	else if (Key&KEY_M_OEM)             strName += ModifKeyName[6].Name;

	Len=(int)strName.GetLength();
	return strName;
}

/* $ 24.09.2000 SVS
 + ������� KeyNameToKey - ��������� ���� ������� �� �����
   ���� ��� �� ����� ��� ��� ������ - ������������ -1
   ����� � �����, �� ��������� � �������!
*/


int WINAPI KeyNameToKey(const wchar_t *Name)
{
	if (!Name || !*Name)
		return -1;

	DWORD Key=0;
//// // _SVS(SysLog(L"KeyNameToKey('%s')",Name));

	// ��� ������������?
	if (Name[0] == L'$' && Name[1])
		return -1;// KeyNameMacroToKey(Name);

	if (Name[0] == L'%' && Name[1])
		return -1;

	if (Name[1] && wcspbrk(Name,L"()")) // ���� �� ���� ������ � ����������� '(' ��� ')', �� ��� ���� �� �������!
		return -1;

//   if((Key=KeyNameMacroToKey(Name)) != (DWORD)-1)
//     return Key;
	int I, Pos;
	static string strTmpName;
	strTmpName = Name;
	strTmpName.Upper();
	int Len=(int)strTmpName.GetLength();

	// ��������� �� ���� �������������
	for (Pos=I=0; I < int(countof(ModifKeyName)); ++I)
	{
		if (wcsstr(strTmpName,ModifKeyName[I].UName) && !(Key&ModifKeyName[I].Key))
		{
			int CntReplace=ReplaceStrings(strTmpName,ModifKeyName[I].UName,L"",-1,TRUE);
			Key|=ModifKeyName[I].Key;
			Pos+=ModifKeyName[I].Len*CntReplace;
		}
	}

	//Pos=strlen(TmpName);
//// // _SVS(SysLog(L"Name=%s",Name));
	// ���� ���-�� �������� - �����������.
	if (Pos < Len)
	{
		// ������� - FKeys1
		const wchar_t* Ptr=Name+Pos;
		int PtrLen = Len-Pos;

		for (I=(int)countof(FKeys1)-1; I>=0; I--)
		{
			if (PtrLen == FKeys1[I].Len && !StrCmpI(Ptr,FKeys1[I].Name))
			{
				Key|=FKeys1[I].Key;
				Pos+=FKeys1[I].Len;
				break;
			}
		}

		if (I == -1)
		{
			if (Len == 1 || Pos == Len-1)
			{
				int Chr=Name[Pos];

				if (Key&(KEY_ALT|KEY_RCTRL|KEY_CTRL|KEY_RALT))
				{
					if (Chr > 0x7F)
						Chr=KeyToKeyLayout(Chr);

					Chr=Upper(Chr);
				}

				Key|=Chr;

				if (Chr)
					Pos++;
			}
			else if (Key & (KEY_ALT|KEY_RALT))
			{
				int K=_wtoi(Ptr);

				if (K != -1)
				{
					// ���� �������� Alt-Num
					Key=(Key|K|KEY_ALTDIGIT)&(~(KEY_ALT|KEY_RALT));
					Pos=Len;
				}
			}
			else if (Key & (KEY_M_SPEC|KEY_M_OEM))
			{
				int K=_wtoi(Ptr);

				if (K != -1)
				{
					if (Key & KEY_M_SPEC)
						Key=(Key|(K+KEY_VK_0xFF_BEGIN))&(~(KEY_M_SPEC|KEY_M_OEM));
					else if (Key & KEY_M_OEM)
						Key=(Key|(K+KEY_FKEY_BEGIN))&(~(KEY_M_SPEC|KEY_M_OEM));

					Pos=Len;
				}
			}
		}
	}

	/*
		if(!(Key&(KEY_ALT|KEY_RCTRL|KEY_CTRL|KEY_RALT|KEY_ALTDIGIT)) && (Key&KEY_SHIFT) && LocalIsalpha(Key&(~KEY_CTRLMASK)))
		{
			Key&=~KEY_SHIFT;
			Key=LocalUpper(Key);
		}
	*/
//// // _SVS(SysLog(L"Key=0x%08X (%c) => '%s'",Key,(Key?Key:' '),Name));
	return (!Key || Pos < Len)? -1: (int)Key;
}

BOOL WINAPI KeyToText(int Key0, string &strKeyText0)
{
	string strKeyText;
	string strKeyTemp;
	int I, Len;
	DWORD Key=(DWORD)Key0, FKey=(DWORD)Key0&0xFFFFFF;
	//if(Key >= KEY_MACRO_BASE && Key <= KEY_MACRO_ENDBASE)
	//  return KeyMacroToText(Key0, strKeyText0);

	if (Key&KEY_ALTDIGIT)
		strKeyText.Format(L"Alt%05d", Key&FKey);
	else
	{
		GetShiftKeyName(strKeyText,Key,Len);

		for (I=0; I<int(countof(FKeys1)); I++)
		{
			if (FKey==FKeys1[I].Key)
			{
				strKeyText += FKeys1[I].Name;
				break;
			}
		}

		if (I  == countof(FKeys1))
		{
			if (FKey >= KEY_VK_0xFF_BEGIN && FKey <= KEY_VK_0xFF_END)
			{
				strKeyTemp.Format(L"Spec%05d",FKey-KEY_VK_0xFF_BEGIN);
				strKeyText += strKeyTemp;
			}
			else if (FKey > KEY_LAUNCH_APP2 && FKey < KEY_CTRLALTSHIFTPRESS)
			{
				strKeyTemp.Format(L"Oem%05d",FKey-KEY_FKEY_BEGIN);
				strKeyText += strKeyTemp;
			}
			else
			{
#if defined(SYSLOG)

				// ���� ����� ���� ����� ������ ��� ����, ��� "�����������" ������������ ���������
				for (I=0; I<countof(SpecKeyName); I++)
					if (FKey==SpecKeyName[I].Key)
					{
						strKeyText += SpecKeyName[I].Name;
						break;
					}

				if (I  == countof(SpecKeyName))
#endif
				{
					FKey=Upper((wchar_t)Key&0xFFFF);

					wchar_t KeyText[2];
					KeyText[0]=KeyText[1] = 0;

					if (FKey >= L'A' && FKey <= L'Z')
					{
						if (Key&(KEY_RCTRL|KEY_CTRL|KEY_ALT|KEY_RCTRL)) // ??? � ���� ���� ������ ������������ ???
							KeyText[0]=(wchar_t)FKey; // ��� ������ � �������������� ����������� "��������" � ������� ��������
						else
							KeyText[0]=(wchar_t)(Key&0xFFFF);
					}
					else
						KeyText[0]=(wchar_t)Key&0xFFFF;

					strKeyText += (const wchar_t*)KeyText;
				}
			}
		}

		if (strKeyText.IsEmpty())
		{
			strKeyText0.Clear();
			return FALSE;
		}
	}

	strKeyText0 = strKeyText;
	return TRUE;
}


int TranslateKeyToVK(int Key,int &VirtKey,int &ControlState,INPUT_RECORD *Rec)
{
	int FKey  =Key&0x0003FFFF;
	int FShift=Key&0x7F000000; // ������� ��� ������������ � ������ �����!
	VirtKey=0;
	ControlState=(FShift&KEY_SHIFT?PKF_SHIFT:0)|
	             (FShift&KEY_ALT?PKF_ALT:0)|
	             (FShift&KEY_CTRL?PKF_CONTROL:0);

	bool KeyInTable=false;
	for (size_t i=0; i < countof(Table_KeyToVK); i++)
	{
		if (FKey==Table_KeyToVK[i].Key)
		{
			VirtKey=Table_KeyToVK[i].VK;
			KeyInTable=true;
			break;
		}
	}

	if (!KeyInTable)
	{
		if ((FKey>='0' && FKey<='9') || (FKey>='A' && FKey<='Z'))
			VirtKey=FKey;
		else if ((unsigned int)FKey > KEY_FKEY_BEGIN && (unsigned int)FKey < KEY_END_FKEY)
			VirtKey=FKey-KEY_FKEY_BEGIN;
		else if (FKey < 0x100)          // EXTENDED_KEY_BASE ???
			VirtKey=VkKeyScanA(FKey)&0xFF;
		else
			VirtKey=FKey;
	}

	if (Rec && VirtKey!=0)
	{
		Rec->EventType=KEY_EVENT;
		Rec->Event.KeyEvent.bKeyDown=1;
		Rec->Event.KeyEvent.wRepeatCount=1;
		Rec->Event.KeyEvent.wVirtualKeyCode=VirtKey;
		Rec->Event.KeyEvent.wVirtualScanCode = MapVirtualKeyA(
		                                           Rec->Event.KeyEvent.wVirtualKeyCode, 0);

		if (Key>255)                    // 0xFFFFF ???
			Key=0;

		Rec->Event.KeyEvent.uChar.UnicodeChar=
		    Rec->Event.KeyEvent.uChar.AsciiChar=Key;
		// ����� ������ � Shift-�������� ������, ������ ��� ControlState
		Rec->Event.KeyEvent.dwControlKeyState=
		    (FShift&KEY_SHIFT?SHIFT_PRESSED:0)|
		    (FShift&KEY_ALT?LEFT_ALT_PRESSED:0)|
		    (FShift&KEY_CTRL?LEFT_CTRL_PRESSED:0)|
		    (FShift&KEY_RALT?RIGHT_ALT_PRESSED:0)|
		    (FShift&KEY_RCTRL?RIGHT_CTRL_PRESSED:0);
	}

	return(VirtKey!=0);
}


int IsNavKey(DWORD Key)
{
	static DWORD NavKeys[][2]=
	{
		{0,KEY_CTRLC},
		{0,KEY_INS},      {0,KEY_NUMPAD0},
		{0,KEY_CTRLINS},  {0,KEY_CTRLNUMPAD0},

		{1,KEY_LEFT},     {1,KEY_NUMPAD4},
		{1,KEY_RIGHT},    {1,KEY_NUMPAD6},
		{1,KEY_HOME},     {1,KEY_NUMPAD7},
		{1,KEY_END},      {1,KEY_NUMPAD1},
		{1,KEY_UP},       {1,KEY_NUMPAD8},
		{1,KEY_DOWN},     {1,KEY_NUMPAD2},
		{1,KEY_PGUP},     {1,KEY_NUMPAD9},
		{1,KEY_PGDN},     {1,KEY_NUMPAD3},
		//!!!!!!!!!!!
	};

	for (int I=0; I < int(countof(NavKeys)); I++)
		if ((!NavKeys[I][0] && Key==NavKeys[I][1]) ||
		        (NavKeys[I][0] && (Key&0x00FFFFFF)==(NavKeys[I][1]&0x00FFFFFF)))
			return TRUE;

	return FALSE;
}

int IsShiftKey(DWORD Key)
{
	static DWORD ShiftKeys[]=
	{
		KEY_SHIFTLEFT,          KEY_SHIFTNUMPAD4,
		KEY_SHIFTRIGHT,         KEY_SHIFTNUMPAD6,
		KEY_SHIFTHOME,          KEY_SHIFTNUMPAD7,
		KEY_SHIFTEND,           KEY_SHIFTNUMPAD1,
		KEY_SHIFTUP,            KEY_SHIFTNUMPAD8,
		KEY_SHIFTDOWN,          KEY_SHIFTNUMPAD2,
		KEY_SHIFTPGUP,          KEY_SHIFTNUMPAD9,
		KEY_SHIFTPGDN,          KEY_SHIFTNUMPAD3,
		KEY_CTRLSHIFTHOME,      KEY_CTRLSHIFTNUMPAD7,
		KEY_CTRLSHIFTPGUP,      KEY_CTRLSHIFTNUMPAD9,
		KEY_CTRLSHIFTEND,       KEY_CTRLSHIFTNUMPAD1,
		KEY_CTRLSHIFTPGDN,      KEY_CTRLSHIFTNUMPAD3,
		KEY_CTRLSHIFTLEFT,      KEY_CTRLSHIFTNUMPAD4,
		KEY_CTRLSHIFTRIGHT,     KEY_CTRLSHIFTNUMPAD6,
		KEY_ALTSHIFTDOWN,       KEY_ALTSHIFTNUMPAD2,
		KEY_ALTSHIFTLEFT,       KEY_ALTSHIFTNUMPAD4,
		KEY_ALTSHIFTRIGHT,      KEY_ALTSHIFTNUMPAD6,
		KEY_ALTSHIFTUP,         KEY_ALTSHIFTNUMPAD8,
		KEY_ALTSHIFTEND,        KEY_ALTSHIFTNUMPAD1,
		KEY_ALTSHIFTHOME,       KEY_ALTSHIFTNUMPAD7,
		KEY_ALTSHIFTPGDN,       KEY_ALTSHIFTNUMPAD3,
		KEY_ALTSHIFTPGUP,       KEY_ALTSHIFTNUMPAD9,
		KEY_CTRLALTPGUP,        KEY_CTRLALTNUMPAD9,
		KEY_CTRLALTHOME,        KEY_CTRLALTNUMPAD7,
		KEY_CTRLALTPGDN,        KEY_CTRLALTNUMPAD2,
		KEY_CTRLALTEND,         KEY_CTRLALTNUMPAD1,
		KEY_CTRLALTLEFT,        KEY_CTRLALTNUMPAD4,
		KEY_CTRLALTRIGHT,       KEY_CTRLALTNUMPAD6,
		KEY_ALTUP,
		KEY_ALTLEFT,
		KEY_ALTDOWN,
		KEY_ALTRIGHT,
		KEY_ALTHOME,
		KEY_ALTEND,
		KEY_ALTPGUP,
		KEY_ALTPGDN,
		KEY_ALT,
		KEY_CTRL,
	};

	for (int I=0; I<int(countof(ShiftKeys)); I++)
		if (Key==ShiftKeys[I])
			return TRUE;

	return FALSE;
}


// GetAsyncKeyState(VK_RSHIFT)
DWORD CalcKeyCode(INPUT_RECORD *rec,int RealKey,int *NotMacros)
{
//_SVS(CleverSysLog Clev(L"CalcKeyCode"));
//_SVS(SysLog(L"CalcKeyCode -> %s| RealKey=%d  *NotMacros=%d",_INPUT_RECORD_Dump(rec),RealKey,(NotMacros?*NotMacros:0)));
	UINT CtrlState=rec->Event.KeyEvent.dwControlKeyState;
	UINT ScanCode=rec->Event.KeyEvent.wVirtualScanCode;
	UINT KeyCode=rec->Event.KeyEvent.wVirtualKeyCode;
	WCHAR Char=rec->Event.KeyEvent.uChar.UnicodeChar;
	//// // _SVS(if(KeyCode == VK_DECIMAL || KeyCode == VK_DELETE) SysLog(L"CalcKeyCode -> CtrlState=%04X KeyCode=%s ScanCode=%08X AsciiChar=%02X ShiftPressed=%d ShiftPressedLast=%d",CtrlState,_VK_KEY_ToName(KeyCode), ScanCode, Char.AsciiChar,ShiftPressed,ShiftPressedLast));

	if (NotMacros)
		*NotMacros=CtrlState&0x80000000?TRUE:FALSE;

//  CtrlState&=~0x80000000;

	if (!(rec->EventType==KEY_EVENT || rec->EventType == FARMACRO_KEY_EVENT))
		return(KEY_NONE);

	if (!RealKey)
	{
		CtrlPressed=(CtrlState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED));
		AltPressed=(CtrlState & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED));
		ShiftPressed=(CtrlState & SHIFT_PRESSED);
		RightCtrlPressed=(CtrlState & RIGHT_CTRL_PRESSED);
		RightAltPressed=(CtrlState & RIGHT_ALT_PRESSED);
		RightShiftPressed=(CtrlState & SHIFT_PRESSED);
	}

	DWORD Modif=(CtrlPressed?KEY_CTRL:0)|(AltPressed?KEY_ALT:0)|(ShiftPressed?KEY_SHIFT:0);

	if (rec->Event.KeyEvent.wVirtualKeyCode >= 0xFF && RealKey)
	{
		//VK_?=0x00FF, Scan=0x0013 uChar=[U=' ' (0x0000): A=' ' (0x00)] Ctrl=0x00000120 (casac - EcNs)
		//VK_?=0x00FF, Scan=0x0014 uChar=[U=' ' (0x0000): A=' ' (0x00)] Ctrl=0x00000120 (casac - EcNs)
		//VK_?=0x00FF, Scan=0x0015 uChar=[U=' ' (0x0000): A=' ' (0x00)] Ctrl=0x00000120 (casac - EcNs)
		//VK_?=0x00FF, Scan=0x001A uChar=[U=' ' (0x0000): A=' ' (0x00)] Ctrl=0x00000120 (casac - EcNs)
		//VK_?=0x00FF, Scan=0x001B uChar=[U=' ' (0x0000): A=' ' (0x00)] Ctrl=0x00000120 (casac - EcNs)
		//VK_?=0x00FF, Scan=0x001E uChar=[U=' ' (0x0000): A=' ' (0x00)] Ctrl=0x00000120 (casac - EcNs)
		//VK_?=0x00FF, Scan=0x001F uChar=[U=' ' (0x0000): A=' ' (0x00)] Ctrl=0x00000120 (casac - EcNs)
		//VK_?=0x00FF, Scan=0x0023 uChar=[U=' ' (0x0000): A=' ' (0x00)] Ctrl=0x00000120 (casac - EcNs)
		if (!rec->Event.KeyEvent.bKeyDown && (CtrlState&(ENHANCED_KEY|NUMLOCK_ON)))
			return Modif|(KEY_VK_0xFF_BEGIN+ScanCode);

		return KEY_IDLE;
	}

	static DWORD Time=0;

	if (!AltValue)
	{
		Time=GetTickCount();
	}

	if (!rec->Event.KeyEvent.bKeyDown)
	{
		KeyCodeForALT_LastPressed=0;

		if (KeyCode==VK_MENU && AltValue!=0)
		{
			//FlushInputBuffer();//???
			INPUT_RECORD TempRec;
			DWORD ReadCount;
			ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE),&TempRec,1,&ReadCount);
			ReturnAltValue=TRUE;
			//_SVS(SysLog(L"0 AltNumPad -> AltValue=0x%0X CtrlState=%X",AltValue,CtrlState));
			AltValue&=0xFFFF;
			/*
			� �������������� �� ���������� / ������� ������ � �������, �� ������� ����� '�':

			1. ���������� Alt:
			bKeyDown=TRUE,  wRepeatCount=1, wVirtualKeyCode=VK_MENU,    UnicodeChar=0,    dwControlKeyState=LEFT_ALT_PRESSED

			2. ����� numpad-������� �������� ��� ������� � OEM, ���� �� ���� �������, ��� 63 ('?'), ���� �� �������:
			bKeyDown=TRUE,  wRepeatCount=1, wVirtualKeyCode=VK_NUMPAD2, UnicodeChar=0,    dwControlKeyState=LEFT_ALT_PRESSED
			bKeyDown=FALSE, wRepeatCount=1, wVirtualKeyCode=VK_NUMPAD2, UnicodeChar=0,    dwControlKeyState=LEFT_ALT_PRESSED
			bKeyDown=TRUE,  wRepeatCount=1, wVirtualKeyCode=VK_NUMPAD3, UnicodeChar=0,    dwControlKeyState=LEFT_ALT_PRESSED
			bKeyDown=FALSE, wRepeatCount=1, wVirtualKeyCode=VK_NUMPAD3, UnicodeChar=0,    dwControlKeyState=LEFT_ALT_PRESSED
			bKeyDown=TRUE,  wRepeatCount=1, wVirtualKeyCode=VK_NUMPAD5, UnicodeChar=0,    dwControlKeyState=LEFT_ALT_PRESSED
			bKeyDown=FALSE, wRepeatCount=1, wVirtualKeyCode=VK_NUMPAD5, UnicodeChar=0,    dwControlKeyState=LEFT_ALT_PRESSED

			3. ���������� Alt, ��� ���� � uChar.UnicodeChar ����� �������� ������:
			bKeyDown=FALSE, wRepeatCount=1, wVirtualKeyCode=VK_MENU,    UnicodeChar=1099, dwControlKeyState=0

			������ ��� ����� ������: ���� rec->Event.KeyEvent.uChar.UnicodeChar �� ���� - ���� ���, � �� ��, ��� �� ����� ����������� Alt ������.
			*/

			if (rec->Event.KeyEvent.uChar.UnicodeChar)
			{
				// BUGBUG: � Windows 7 Event.KeyEvent.uChar.UnicodeChar _������_ ��������, �� ������ �� ������ ���, ��� ����.
				// ������� �������, ��� ���� �������� ����� ��������� �� ��������� 50 ��, �� ��� ��������������� ��� D&D ��� ������� ����������,
				// ����� - ������ ����.
				if (GetTickCount()-Time<50)
				{
					AltValue=rec->Event.KeyEvent.uChar.UnicodeChar;
				}
			}
			else
			{
				rec->Event.KeyEvent.uChar.UnicodeChar=static_cast<WCHAR>(AltValue);
			}

			//// // _SVS(SysLog(L"KeyCode==VK_MENU -> AltValue=%X (%c)",AltValue,AltValue));
			return(AltValue);
		}
		else
			return(KEY_NONE);
	}

	if ((CtrlState & 9)==9)
	{
		if (Char)
			return Char;
		else
			CtrlPressed=0;
	}

	if (KeyCode==VK_MENU)
		AltValue=0;

	if (Char && !CtrlPressed && !AltPressed)
	{
		if (KeyCode==VK_OEM_3 && !Opt.UseVk_oem_x)
			return(ShiftPressed ? '~':'`');

		if (KeyCode==VK_OEM_7 && !Opt.UseVk_oem_x)
			return(ShiftPressed ? '"':'\'');
	}

	if (Char<L' ' && (CtrlPressed || AltPressed))
	{
		switch (KeyCode)
		{
			case VK_OEM_COMMA:
				Char=L',';
				break;
			case VK_OEM_PERIOD:
				Char=L'.';
				break;
			case VK_OEM_4:

				if (!Opt.UseVk_oem_x)
					Char=L'[';

				break;
			case VK_OEM_5:

				//Char.AsciiChar=ScanCode==0x29?0x15:'\\'; //???
				if (!Opt.UseVk_oem_x)
					Char=L'\\';

				break;
			case VK_OEM_6:

				if (!Opt.UseVk_oem_x)
					Char=L']';

				break;
			case VK_OEM_7:

				if (!Opt.UseVk_oem_x)
					Char=L'\"';

				break;
		}
	}

	/* $ 24.08.2000 SVS
	   "������������ 100 �����" :-)
	*/
	if (CtrlPressed && AltPressed && ShiftPressed)
	{
		switch (KeyCode)
		{
			case VK_SHIFT:
			case VK_MENU:
			case VK_CONTROL:
			{
				if (RightCtrlPressed && RightAltPressed && RightShiftPressed)
				{
					if ((Opt.CASRule&2))
						return (IsKeyRCASPressed?KEY_RCTRLALTSHIFTPRESS:KEY_RCTRLALTSHIFTRELEASE);
				}
				else if (Opt.CASRule&1)
					return (IsKeyCASPressed?KEY_CTRLALTSHIFTPRESS:KEY_CTRLALTSHIFTRELEASE);
			}
		}
	}

	if (RightCtrlPressed && RightAltPressed && RightShiftPressed)
	{
		switch (KeyCode)
		{
			case VK_RSHIFT:
			case VK_RMENU:
			case VK_RCONTROL:

				if (Opt.CASRule&2)
					return (IsKeyRCASPressed?KEY_RCTRLALTSHIFTPRESS:KEY_RCTRLALTSHIFTRELEASE);

				break;
		}
	}

	if (KeyCode>=VK_F1 && KeyCode<=VK_F24)
//    return(Modif+KEY_F1+((KeyCode-VK_F1)<<8));
		return(Modif+KEY_F1+((KeyCode-VK_F1)));

	int NotShift=!CtrlPressed && !AltPressed && !ShiftPressed;

	if (AltPressed && !CtrlPressed && !ShiftPressed)
	{
#if 0

		if (AltValue==0 && (CtrlObject->Macro.IsRecording() == MACROMODE_NOMACRO || !Opt.UseNumPad))
		{
			// VK_INSERT  = 0x2D       AS-0 = 0x2D
			// VK_NUMPAD0 = 0x60       A-0  = 0x60
			/*
			  � �������� �� ��� ������� - ���, ��� � ����� ��������,
			  ������ ��� ������� ���� � �����.
			*/
			if (//(CtrlState&NUMLOCK_ON)  && KeyCode==VK_NUMPAD0 && !(CtrlState&ENHANCED_KEY) ||
			    (Opt.UseNumPad && KeyCode==VK_INSERT && (CtrlState&ENHANCED_KEY)) ||
			    (!Opt.UseNumPad && (KeyCode==VK_INSERT || KeyCode==VK_NUMPAD0))
			)
			{   // CtrlObject->Macro.IsRecording()
				//// // _SVS(SysLog(L"IsProcessAssignMacroKey=%d",IsProcessAssignMacroKey));
				if (IsProcessAssignMacroKey && Opt.UseNumPad)
				{
					return KEY_INS|KEY_ALT;
				}
				else
				{
					RunGraber();
				}

				return(KEY_NONE);
			}
		}

#else

		if (AltValue==0)
		{
			if (KeyCode==VK_INSERT || KeyCode==VK_NUMPAD0)
			{
				if (CtrlObject && CtrlObject->Macro.IsRecording())
					CtrlObject->Macro.ProcessKey(KEY_INS|KEY_ALT);

				RunGraber();
				return(KEY_NONE);
			}
		}

#endif

		//// // _SVS(SysLog(L"1 AltNumPad -> CalcKeyCode -> KeyCode=%s  ScanCode=0x%0X AltValue=0x%0X CtrlState=%X GetAsyncKeyState(VK_SHIFT)=%X",_VK_KEY_ToName(KeyCode),ScanCode,AltValue,CtrlState,GetAsyncKeyState(VK_SHIFT)));
		if ((CtrlState & ENHANCED_KEY)==0
		        //(CtrlState&NUMLOCK_ON) && KeyCode >= VK_NUMPAD0 && KeyCode <= VK_NUMPAD9 ||
		        // !(CtrlState&NUMLOCK_ON) && KeyCode < VK_NUMPAD0
		   )
		{
			//// // _SVS(SysLog(L"2 AltNumPad -> CalcKeyCode -> KeyCode=%s  ScanCode=0x%0X AltValue=0x%0X CtrlState=%X GetAsyncKeyState(VK_SHIFT)=%X",_VK_KEY_ToName(KeyCode),ScanCode,AltValue,CtrlState,GetAsyncKeyState(VK_SHIFT)));
			static unsigned int ScanCodes[]={82,79,80,81,75,76,77,71,72,73};

			for (int I=0; I<int(countof(ScanCodes)); I++)
			{
				if (ScanCodes[I]==ScanCode)
				{
					if (RealKey && (unsigned int)KeyCodeForALT_LastPressed != KeyCode)
					{
						AltValue=AltValue*10+I;
						KeyCodeForALT_LastPressed=KeyCode;
					}

//          _SVS(SysLog(L"AltNumPad -> AltValue=0x%0X CtrlState=%X",AltValue,CtrlState));
					if (AltValue!=0)
						return(KEY_NONE);
				}
			}
		}
	}

	/*
	NumLock=Off
	  Down
	    CtrlState=0100 KeyCode=0028 ScanCode=00000050 AsciiChar=00         ENHANCED_KEY
	    CtrlState=0100 KeyCode=0028 ScanCode=00000050 AsciiChar=00
	  Num2
	    CtrlState=0000 KeyCode=0028 ScanCode=00000050 AsciiChar=00
	    CtrlState=0000 KeyCode=0028 ScanCode=00000050 AsciiChar=00

	  Ctrl-8
	    CtrlState=0008 KeyCode=0026 ScanCode=00000048 AsciiChar=00
	  Ctrl-Shift-8               ^^!!!
	    CtrlState=0018 KeyCode=0026 ScanCode=00000048 AsciiChar=00

	------------------------------------------------------------------------
	NumLock=On

	  Down
	    CtrlState=0120 KeyCode=0028 ScanCode=00000050 AsciiChar=00         ENHANCED_KEY
	    CtrlState=0120 KeyCode=0028 ScanCode=00000050 AsciiChar=00
	  Num2
	    CtrlState=0020 KeyCode=0062 ScanCode=00000050 AsciiChar=32
	    CtrlState=0020 KeyCode=0062 ScanCode=00000050 AsciiChar=32

	  Ctrl-8
	    CtrlState=0028 KeyCode=0068 ScanCode=00000048 AsciiChar=00
	  Ctrl-Shift-8               ^^!!!
	    CtrlState=0028 KeyCode=0026 ScanCode=00000048 AsciiChar=00
	*/

	/* ------------------------------------------------------------- */
	switch (KeyCode)
	{
		case VK_INSERT:
		case VK_NUMPAD0:

			if (CtrlState&ENHANCED_KEY)
			{
				return(Modif|KEY_INS);
			}
			else if ((CtrlState&NUMLOCK_ON) && NotShift && KeyCode == VK_NUMPAD0)
				return '0';

			return Modif|(Opt.UseNumPad?KEY_NUMPAD0:KEY_INS);
		case VK_DOWN:
		case VK_NUMPAD2:

			if (CtrlState&ENHANCED_KEY)
			{
				return(Modif|KEY_DOWN);
			}
			else if ((CtrlState&NUMLOCK_ON) && NotShift && KeyCode == VK_NUMPAD2)
				return '2';

			return Modif|(Opt.UseNumPad?KEY_NUMPAD2:KEY_DOWN);
		case VK_LEFT:
		case VK_NUMPAD4:

			if (CtrlState&ENHANCED_KEY)
			{
				return(Modif|KEY_LEFT);
			}
			else if ((CtrlState&NUMLOCK_ON) && NotShift && KeyCode == VK_NUMPAD4)
				return '4';

			return Modif|(Opt.UseNumPad?KEY_NUMPAD4:KEY_LEFT);
		case VK_RIGHT:
		case VK_NUMPAD6:

			if (CtrlState&ENHANCED_KEY)
			{
				return(Modif|KEY_RIGHT);
			}
			else if ((CtrlState&NUMLOCK_ON) && NotShift && KeyCode == VK_NUMPAD6)
				return '6';

			return Modif|(Opt.UseNumPad?KEY_NUMPAD6:KEY_RIGHT);
		case VK_UP:
		case VK_NUMPAD8:

			if (CtrlState&ENHANCED_KEY)
			{
				return(Modif|KEY_UP);
			}
			else if ((CtrlState&NUMLOCK_ON) && NotShift && KeyCode == VK_NUMPAD8)
				return '8';

			return Modif|(Opt.UseNumPad?KEY_NUMPAD8:KEY_UP);
		case VK_END:
		case VK_NUMPAD1:

			if (CtrlState&ENHANCED_KEY)
			{
				return(Modif|KEY_END);
			}
			else if ((CtrlState&NUMLOCK_ON) && NotShift && KeyCode == VK_NUMPAD1)
				return '1';

			return Modif|(Opt.UseNumPad?KEY_NUMPAD1:KEY_END);
		case VK_HOME:
		case VK_NUMPAD7:

			if (CtrlState&ENHANCED_KEY)
			{
				return(Modif|KEY_HOME);
			}
			else if ((CtrlState&NUMLOCK_ON) && NotShift && KeyCode == VK_NUMPAD7)
				return '7';

			return Modif|(Opt.UseNumPad?KEY_NUMPAD7:KEY_HOME);
		case VK_NEXT:
		case VK_NUMPAD3:

			if (CtrlState&ENHANCED_KEY)
			{
				return(Modif|KEY_PGDN);
			}
			else if ((CtrlState&NUMLOCK_ON) && NotShift && KeyCode == VK_NUMPAD3)
				return '3';

			return Modif|(Opt.UseNumPad?KEY_NUMPAD3:KEY_PGDN);
		case VK_PRIOR:
		case VK_NUMPAD9:

			if (CtrlState&ENHANCED_KEY)
			{
				return(Modif|KEY_PGUP);
			}
			else if ((CtrlState&NUMLOCK_ON) && NotShift && KeyCode == VK_NUMPAD9)
				return '9';

			return Modif|(Opt.UseNumPad?KEY_NUMPAD9:KEY_PGUP);
		case VK_CLEAR:
		case VK_NUMPAD5:

			if (CtrlState&ENHANCED_KEY)
			{
				return(Modif|KEY_NUMPAD5);
			}
			else if ((CtrlState&NUMLOCK_ON) && NotShift && KeyCode == VK_NUMPAD5)
				return '5';

			return Modif|KEY_NUMPAD5;
		case VK_DELETE:
		case VK_DECIMAL:

			if (CtrlState&ENHANCED_KEY)
			{
				return (Modif|KEY_DEL);
			}
			else if ((CtrlState&NUMLOCK_ON) && NotShift && KeyCode == VK_DECIMAL)
				return KEY_DECIMAL;

			return Modif|(Opt.UseNumPad?KEY_NUMDEL:KEY_DEL);
	}

	switch (KeyCode)
	{
		case VK_RETURN:
			//  !!!!!!!!!!!!! - ���� "!ShiftPressed", �� Shift-F4 Shift-Enter, ��
			//                  �������� Shift...
//_SVS(SysLog(L"ShiftPressed=%d RealKey=%d !ShiftPressedLast=%d !CtrlPressed=%d !AltPressed=%d (%d)",ShiftPressed,RealKey,ShiftPressedLast,CtrlPressed,AltPressed,(ShiftPressed && RealKey && !ShiftPressedLast && !CtrlPressed && !AltPressed)));
#if 0

			if (ShiftPressed && RealKey && !ShiftPressedLast && !CtrlPressed && !AltPressed && !LastShiftEnterPressed)
				return(KEY_ENTER);

			LastShiftEnterPressed=Modif&KEY_SHIFT?TRUE:FALSE;
			return(Modif|KEY_ENTER);
#else

			if (ShiftPressed && RealKey && !ShiftPressedLast && !CtrlPressed && !AltPressed && !LastShiftEnterPressed)
				return(Opt.UseNumPad && (CtrlState&ENHANCED_KEY)?KEY_NUMENTER:KEY_ENTER);

			LastShiftEnterPressed=Modif&KEY_SHIFT?TRUE:FALSE;
			return(Modif|(Opt.UseNumPad && (CtrlState&ENHANCED_KEY)?KEY_NUMENTER:KEY_ENTER));
#endif
		case VK_BROWSER_BACK:
			return Modif|KEY_BROWSER_BACK;
		case VK_BROWSER_FORWARD:
			return Modif|KEY_BROWSER_FORWARD;
		case VK_BROWSER_REFRESH:
			return Modif|KEY_BROWSER_REFRESH;
		case VK_BROWSER_STOP:
			return Modif|KEY_BROWSER_STOP;
		case VK_BROWSER_SEARCH:
			return Modif|KEY_BROWSER_SEARCH;
		case VK_BROWSER_FAVORITES:
			return Modif|KEY_BROWSER_FAVORITES;
		case VK_BROWSER_HOME:
			return Modif|KEY_BROWSER_HOME;
		case VK_VOLUME_MUTE:
			return Modif|KEY_VOLUME_MUTE;
		case VK_VOLUME_DOWN:
			return Modif|KEY_VOLUME_DOWN;
		case VK_VOLUME_UP:
			return Modif|KEY_VOLUME_UP;
		case VK_MEDIA_NEXT_TRACK:
			return Modif|KEY_MEDIA_NEXT_TRACK;
		case VK_MEDIA_PREV_TRACK:
			return Modif|KEY_MEDIA_PREV_TRACK;
		case VK_MEDIA_STOP:
			return Modif|KEY_MEDIA_STOP;
		case VK_MEDIA_PLAY_PAUSE:
			return Modif|KEY_MEDIA_PLAY_PAUSE;
		case VK_LAUNCH_MAIL:
			return Modif|KEY_LAUNCH_MAIL;
		case VK_LAUNCH_MEDIA_SELECT:
			return Modif|KEY_LAUNCH_MEDIA_SELECT;
		case VK_LAUNCH_APP1:
			return Modif|KEY_LAUNCH_APP1;
		case VK_LAUNCH_APP2:
			return Modif|KEY_LAUNCH_APP2;
		case VK_APPS:
			return(Modif|KEY_APPS);
		case VK_LWIN:
			return(Modif|KEY_LWIN);
		case VK_RWIN:
			return(Modif|KEY_RWIN);
		case VK_BACK:
			return(Modif|KEY_BS);
		case VK_SPACE:

			if (Char == L' ' || !Char)
				return(Modif|KEY_SPACE);

			return Char;
		case VK_TAB:
			return(Modif|KEY_TAB);
		case VK_ADD:
			return(Modif|KEY_ADD);
		case VK_SUBTRACT:
			return(Modif|KEY_SUBTRACT);
		case VK_ESCAPE:
			return(Modif|KEY_ESC);
	}

	switch (KeyCode)
	{
		case VK_CAPITAL:
			return(Modif|KEY_CAPSLOCK);
		case VK_NUMLOCK:
			return(Modif|KEY_NUMLOCK);
		case VK_SCROLL:
			return(Modif|KEY_SCROLLLOCK);
	}

	/* ------------------------------------------------------------- */
	if (CtrlPressed && AltPressed && ShiftPressed)
	{

		_SVS(if (KeyCode!=VK_CONTROL && KeyCode!=VK_MENU) SysLog(L"CtrlAltShift -> |%s|%s|",_VK_KEY_ToName(KeyCode),_INPUT_RECORD_Dump(rec)));

		if (KeyCode>='A' && KeyCode<='Z')
			return((KEY_SHIFT|KEY_CTRL|KEY_ALT)+KeyCode);

		if (Opt.ShiftsKeyRules) //???
			switch (KeyCode)
			{
				case VK_OEM_3:
					return(KEY_SHIFT+KEY_CTRL+KEY_ALT+'`');
				case VK_OEM_MINUS:
					return(KEY_SHIFT+KEY_CTRL+KEY_ALT+'-');
				case VK_OEM_PLUS:
					return(KEY_SHIFT+KEY_CTRL+KEY_ALT+'=');
				case VK_OEM_5:
					return(KEY_SHIFT+KEY_CTRL+KEY_ALT+KEY_BACKSLASH);
				case VK_OEM_6:
					return(KEY_SHIFT+KEY_CTRL+KEY_ALT+KEY_BACKBRACKET);
				case VK_OEM_4:
					return(KEY_SHIFT+KEY_CTRL+KEY_ALT+KEY_BRACKET);
				case VK_OEM_7:
					return(KEY_SHIFT+KEY_CTRL+KEY_ALT+'\'');
				case VK_OEM_1:
					return(KEY_SHIFT+KEY_CTRL+KEY_ALT+KEY_SEMICOLON);
				case VK_OEM_2:
					return(KEY_SHIFT+KEY_CTRL+KEY_ALT+KEY_SLASH);
				case VK_OEM_PERIOD:
					return(KEY_SHIFT+KEY_CTRL+KEY_ALT+KEY_DOT);
				case VK_OEM_COMMA:
					return(KEY_SHIFT+KEY_CTRL+KEY_ALT+KEY_COMMA);
			}

		switch (KeyCode)
		{
			case VK_DIVIDE:
				return(KEY_SHIFT|KEY_CTRLALT|KEY_DIVIDE);
			case VK_MULTIPLY:
				return(KEY_SHIFT|KEY_CTRLALT|KEY_MULTIPLY);
			case VK_CANCEL:
				return(KEY_SHIFT|KEY_CTRLALT|KEY_PAUSE);
			case VK_SLEEP:
				return KEY_SHIFT|KEY_CTRLALT|KEY_STANDBY;
			case VK_SNAPSHOT:
				return KEY_SHIFT|KEY_CTRLALT|KEY_PRNTSCRN;
		}

		if (Char)
			return KEY_SHIFT|KEY_CTRL|KEY_ALT|Char;

		if (!RealKey && (KeyCode==VK_CONTROL || KeyCode==VK_MENU))
			return(KEY_NONE);

		if (KeyCode)
			return((KEY_SHIFT|KEY_CTRL|KEY_ALT)+KeyCode);
	}

	/* ------------------------------------------------------------- */
	if (CtrlPressed && AltPressed)
	{

		_SVS(if (KeyCode!=VK_CONTROL && KeyCode!=VK_MENU) SysLog(L"CtrlAlt -> |%s|%s|",_VK_KEY_ToName(KeyCode),_INPUT_RECORD_Dump(rec)));

		if (KeyCode>='A' && KeyCode<='Z')
			return((KEY_CTRL|KEY_ALT)+KeyCode);

		if (Opt.ShiftsKeyRules) //???
			switch (KeyCode)
			{
				case VK_OEM_3:
					return(KEY_CTRL+KEY_ALT+'`');
				case VK_OEM_MINUS:
					return(KEY_CTRL+KEY_ALT+'-');
				case VK_OEM_PLUS:
					return(KEY_CTRL+KEY_ALT+'=');
				case VK_OEM_5:
					return(KEY_CTRL+KEY_ALT+KEY_BACKSLASH);
				case VK_OEM_6:
					return(KEY_CTRL+KEY_ALT+KEY_BACKBRACKET);
				case VK_OEM_4:
					return(KEY_CTRL+KEY_ALT+KEY_BRACKET);
				case VK_OEM_7:
					return(KEY_CTRL+KEY_ALT+'\'');
				case VK_OEM_1:
					return(KEY_CTRL+KEY_ALT+KEY_SEMICOLON);
				case VK_OEM_2:
					return(KEY_CTRL+KEY_ALT+KEY_SLASH);
				case VK_OEM_PERIOD:
					return(KEY_CTRL+KEY_ALT+KEY_DOT);
				case VK_OEM_COMMA:
					return(KEY_CTRL+KEY_ALT+KEY_COMMA);
			}

		switch (KeyCode)
		{
			case VK_DIVIDE:
				return(KEY_CTRLALT|KEY_DIVIDE);
			case VK_MULTIPLY:
				return(KEY_CTRLALT|KEY_MULTIPLY);
				// KEY_EVENT_RECORD: Dn, 1, Vk="VK_CANCEL" [3/0x0003], Scan=0x0046 uChar=[U=' ' (0x0000): A=' ' (0x00)] Ctrl=0x0000014A (CAsac - EcnS)
				//case VK_PAUSE:
			case VK_CANCEL: // Ctrl-Alt-Pause

				if (!ShiftPressed && (CtrlState&ENHANCED_KEY))
					return KEY_CTRLALT|KEY_PAUSE;

				return KEY_NONE;
			case VK_SLEEP:
				return KEY_CTRLALT|KEY_STANDBY;
			case VK_SNAPSHOT:
				return KEY_CTRLALT|KEY_PRNTSCRN;
		}

		if (Char)
			return KEY_CTRL|KEY_ALT|Char;

		if (!RealKey && (KeyCode==VK_CONTROL || KeyCode==VK_MENU))
			return(KEY_NONE);

		if (KeyCode)
			return((KEY_CTRL|KEY_ALT)+KeyCode);
	}

	/* ------------------------------------------------------------- */
	if (AltPressed && ShiftPressed)
	{

		_SVS(if (KeyCode!=VK_MENU && KeyCode!=VK_SHIFT) SysLog(L"AltShift -> |%s|%s|",_VK_KEY_ToName(KeyCode),_INPUT_RECORD_Dump(rec)));

		if (KeyCode>='0' && KeyCode<='9')
		{
			if (WaitInFastFind > 0 &&
			        CtrlObject->Macro.GetCurRecord(NULL,NULL) < MACROMODE_RECORDING &&
			        CtrlObject->Macro.GetIndex(KEY_ALTSHIFT0+KeyCode-'0',-1) == -1)
			{
				return KEY_ALT|KEY_SHIFT|Char;
			}
			else
				return(KEY_ALTSHIFT0+KeyCode-'0');
		}

		if (!WaitInMainLoop && KeyCode>='A' && KeyCode<='Z')
			return(KEY_ALTSHIFT+KeyCode);

		if (Opt.ShiftsKeyRules) //???
			switch (KeyCode)
			{
				case VK_OEM_3:
					return(KEY_ALT+KEY_SHIFT+'`');
				case VK_OEM_MINUS:
					return(KEY_ALT+KEY_SHIFT+'_');
				case VK_OEM_PLUS:
					return(KEY_ALT+KEY_SHIFT+'=');
				case VK_OEM_5:
					return(KEY_ALT+KEY_SHIFT+KEY_BACKSLASH);
				case VK_OEM_6:
					return(KEY_ALT+KEY_SHIFT+KEY_BACKBRACKET);
				case VK_OEM_4:
					return(KEY_ALT+KEY_SHIFT+KEY_BRACKET);
				case VK_OEM_7:
					return(KEY_ALT+KEY_SHIFT+'\'');
				case VK_OEM_1:
					return(KEY_ALT+KEY_SHIFT+KEY_SEMICOLON);
				case VK_OEM_2:
					//if(WaitInFastFind)
					//  return(KEY_ALT+KEY_SHIFT+'?');
					//else
					return(KEY_ALT+KEY_SHIFT+KEY_SLASH);
				case VK_OEM_PERIOD:
					return(KEY_ALT+KEY_SHIFT+KEY_DOT);
				case VK_OEM_COMMA:
					return(KEY_ALT+KEY_SHIFT+KEY_COMMA);
			}

		switch (KeyCode)
		{
			case VK_DIVIDE:
				//if(WaitInFastFind)
				//  return(KEY_ALT+KEY_SHIFT+'/');
				//else
				return(KEY_ALTSHIFT|KEY_DIVIDE);
			case VK_MULTIPLY:
				//if(WaitInFastFind)
				//{
				//  return(KEY_ALT+KEY_SHIFT+'*');
				//}
				//else
				return(KEY_ALTSHIFT|KEY_MULTIPLY);
			case VK_PAUSE:
				return(KEY_ALTSHIFT|KEY_PAUSE);
			case VK_SLEEP:
				return KEY_ALTSHIFT|KEY_STANDBY;
			case VK_SNAPSHOT:
				return KEY_ALTSHIFT|KEY_PRNTSCRN;
		}

		if (Char)
			return KEY_ALT|KEY_SHIFT|Char;

		if (!RealKey && (KeyCode==VK_MENU || KeyCode==VK_SHIFT))
			return(KEY_NONE);

		if (KeyCode)
			return(KEY_ALT+KEY_SHIFT+KeyCode);
	}

	/* ------------------------------------------------------------- */
	if (CtrlPressed && ShiftPressed)
	{

		_SVS(if (KeyCode!=VK_CONTROL && KeyCode!=VK_SHIFT) SysLog(L"CtrlShift -> |%s|%s|",_VK_KEY_ToName(KeyCode),_INPUT_RECORD_Dump(rec)));

		if (KeyCode>='0' && KeyCode<='9')
			return(KEY_CTRLSHIFT0+KeyCode-'0');

		if (KeyCode>='A' && KeyCode<='Z')
			return(KEY_CTRLSHIFTA+KeyCode-'A');

		switch (KeyCode)
		{
			case VK_OEM_PERIOD:
				return(KEY_CTRLSHIFTDOT);
			case VK_OEM_4:
				return(KEY_CTRLSHIFTBRACKET);
			case VK_OEM_6:
				return(KEY_CTRLSHIFTBACKBRACKET);
			case VK_OEM_2:
				return(KEY_CTRLSHIFTSLASH);
			case VK_OEM_5:
				return(KEY_CTRLSHIFTBACKSLASH);
			case VK_DIVIDE:
				return(KEY_CTRLSHIFT|KEY_DIVIDE);
			case VK_MULTIPLY:
				return(KEY_CTRLSHIFT|KEY_MULTIPLY);
			case VK_SLEEP:
				return KEY_CTRLSHIFT|KEY_STANDBY;
			case VK_SNAPSHOT:
				return KEY_CTRLSHIFT|KEY_PRNTSCRN;
		}

		if (Opt.ShiftsKeyRules) //???
			switch (KeyCode)
			{
				case VK_OEM_3:
					return(KEY_CTRL+KEY_SHIFT+'`');
				case VK_OEM_MINUS:
					return(KEY_CTRL+KEY_SHIFT+'-');
				case VK_OEM_PLUS:
					return(KEY_CTRL+KEY_SHIFT+'=');
				case VK_OEM_7:
					return(KEY_CTRL+KEY_SHIFT+'\'');
				case VK_OEM_1:
					return(KEY_CTRL+KEY_SHIFT+KEY_SEMICOLON);
				case VK_OEM_COMMA:
					return(KEY_CTRL+KEY_SHIFT+KEY_COMMA);
			}

		if (Char)
			return KEY_CTRL|KEY_SHIFT|Char;

		if (!RealKey && (KeyCode==VK_CONTROL || KeyCode==VK_SHIFT))
			return(KEY_NONE);

		if (KeyCode)
			return((KEY_CTRL|KEY_SHIFT)+KeyCode);
	}

	/* ------------------------------------------------------------- */
	if ((CtrlState & RIGHT_CTRL_PRESSED)==RIGHT_CTRL_PRESSED)
	{
		if (KeyCode>='0' && KeyCode<='9')
			return(KEY_RCTRL0+KeyCode-'0');
	}

	/* ------------------------------------------------------------- */
	if (!CtrlPressed && !AltPressed && !ShiftPressed)
	{
		switch (KeyCode)
		{
			case VK_DIVIDE:
				return(KEY_DIVIDE);
			case VK_CANCEL:
				return(KEY_BREAK);
			case VK_MULTIPLY:
				return(KEY_MULTIPLY);
			case VK_PAUSE:
				return(KEY_PAUSE);
			case VK_SLEEP:
				return KEY_STANDBY;
			case VK_SNAPSHOT:
				return KEY_PRNTSCRN;
		}
	}

	/* ------------------------------------------------------------- */
	if (CtrlPressed)
	{

		_SVS(if (KeyCode!=VK_CONTROL) SysLog(L"Ctrl -> |%s|%s|",_VK_KEY_ToName(KeyCode),_INPUT_RECORD_Dump(rec)));

		if (KeyCode>='0' && KeyCode<='9')
			return(KEY_CTRL0+KeyCode-'0');

		if (KeyCode>='A' && KeyCode<='Z')
			return(KEY_CTRL+KeyCode);

		switch (KeyCode)
		{
			case VK_OEM_COMMA:
				return(KEY_CTRLCOMMA);
			case VK_OEM_PERIOD:
				return(KEY_CTRLDOT);
			case VK_OEM_2:
				return(KEY_CTRLSLASH);
			case VK_OEM_4:
				return(KEY_CTRLBRACKET);
			case VK_OEM_5:
				return(KEY_CTRLBACKSLASH);
			case VK_OEM_6:
				return(KEY_CTRLBACKBRACKET);
			case VK_OEM_7:
				return(KEY_CTRL+'\''); // KEY_QUOTE
			case VK_MULTIPLY:
				return(KEY_CTRL|KEY_MULTIPLY);
			case VK_DIVIDE:
				return(KEY_CTRL|KEY_DIVIDE);
			case VK_PAUSE:

				if (CtrlState&ENHANCED_KEY)
					return KEY_CTRL|KEY_NUMLOCK;

				return(KEY_BREAK);
			case VK_SLEEP:
				return KEY_CTRL|KEY_STANDBY;
			case VK_SNAPSHOT:
				return KEY_CTRL|KEY_PRNTSCRN;
		}

		if (Opt.ShiftsKeyRules) //???
			switch (KeyCode)
			{
				case VK_OEM_3:
					return(KEY_CTRL+'`');
				case VK_OEM_MINUS:
					return(KEY_CTRL+'-');
				case VK_OEM_PLUS:
					return(KEY_CTRL+'=');
				case VK_OEM_1:
					return(KEY_CTRL+KEY_SEMICOLON);
			}

		if (KeyCode)
		{
			if (!RealKey && KeyCode==VK_CONTROL)
				return(KEY_NONE);

			return(KEY_CTRL+KeyCode);
		}
	}

	/* ------------------------------------------------------------- */
	if (AltPressed)
	{

		_SVS(if (KeyCode!=VK_MENU) SysLog(L"Alt -> |%s|%s|",_VK_KEY_ToName(KeyCode),_INPUT_RECORD_Dump(rec)));

		if (Opt.ShiftsKeyRules) //???
			switch (KeyCode)
			{
				case VK_OEM_3:
					return(KEY_ALT+'`');
				case VK_OEM_MINUS:
					//if(WaitInFastFind)
					//  return(KEY_ALT+KEY_SHIFT+'_');
					//else
					return(KEY_ALT+'-');
				case VK_OEM_PLUS:
					return(KEY_ALT+'=');
				case VK_OEM_5:
					return(KEY_ALT+KEY_BACKSLASH);
				case VK_OEM_6:
					return(KEY_ALT+KEY_BACKBRACKET);
				case VK_OEM_4:
					return(KEY_ALT+KEY_BRACKET);
				case VK_OEM_7:
					return(KEY_ALT+'\'');
				case VK_OEM_1:
					return(KEY_ALT+KEY_SEMICOLON);
				case VK_OEM_2:
					return(KEY_ALT+KEY_SLASH);
			}

		switch (KeyCode)
		{
			case VK_OEM_COMMA:
				return(KEY_ALTCOMMA);
			case VK_OEM_PERIOD:
				return(KEY_ALTDOT);
			case VK_DIVIDE:
				//if(WaitInFastFind)
				//  return(KEY_ALT+KEY_SHIFT+'/');
				//else
				return(KEY_ALT|KEY_DIVIDE);
			case VK_MULTIPLY:
//        if(WaitInFastFind)
//          return(KEY_ALT+KEY_SHIFT+'*');
//        else
				return(KEY_ALT|KEY_MULTIPLY);
			case VK_PAUSE:
				return(KEY_ALT+KEY_PAUSE);
			case VK_SLEEP:
				return KEY_ALT|KEY_STANDBY;
			case VK_SNAPSHOT:
				return KEY_ALT|KEY_PRNTSCRN;
		}

		if (Char)
		{
			if (!Opt.ShiftsKeyRules || WaitInFastFind > 0)
				return KEY_ALT|Upper(Char);
			else if (WaitInMainLoop)
				return KEY_ALT|Char;
		}

		if (!RealKey && KeyCode==VK_MENU)
			return(KEY_NONE);

		return(KEY_ALT+KeyCode);
	}

	if (ShiftPressed)
	{

		_SVS(if (KeyCode!=VK_SHIFT) SysLog(L"Shift -> |%s|%s|",_VK_KEY_ToName(KeyCode),_INPUT_RECORD_Dump(rec)));

		switch (KeyCode)
		{
			case VK_DIVIDE:
				return(KEY_SHIFT|KEY_DIVIDE);
			case VK_MULTIPLY:
				return(KEY_SHIFT|KEY_MULTIPLY);
			case VK_PAUSE:
				return(KEY_SHIFT|KEY_PAUSE);
			case VK_SLEEP:
				return KEY_SHIFT|KEY_STANDBY;
			case VK_SNAPSHOT:
				return KEY_SHIFT|KEY_PRNTSCRN;
		}
	}

	return Char?Char:KEY_NONE;
}
