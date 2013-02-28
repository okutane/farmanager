/*
keybar.cpp

Keybar
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

#include "keybar.hpp"
#include "colors.hpp"
#include "keyboard.hpp"
#include "keys.hpp"
#include "manager.hpp"
#include "syslog.hpp"
#include "language.hpp"
#include "interf.hpp"
#include "config.hpp"
#include "configdb.hpp"

KeyBar::KeyBar():
	Owner(nullptr),
	AltState(0),
	CtrlState(0),
	ShiftState(0),
	CustomLabelsReaded(false)
{
	_OT(SysLog(L"[%p] KeyBar::KeyBar()", this));
}

void KeyBar::DisplayObject()
{
	GotoXY(X1,Y1);
	AltState=CtrlState=ShiftState=0;
	int KeyWidth=(X2-X1-1)/12;

	if (KeyWidth<8)
		KeyWidth=8;

	int LabelWidth=KeyWidth-2;

	for (size_t i=0; i<KEY_COUNT; i++)
	{
		if (WhereX()+LabelWidth>=X2)
			break;

		SetColor(COL_KEYBARNUM);
		Global->FS << i+1;
		SetColor(COL_KEYBARTEXT);
		const wchar_t *Label=L"";

		if (IntKeyState.ShiftPressed)
		{
			ShiftState=IntKeyState.ShiftPressed;

			if (IntKeyState.CtrlPressed)
			{
				CtrlState=IntKeyState.CtrlPressed;

				if (!IntKeyState.AltPressed) // Ctrl-Alt-Shift - ��� ������ ������ :-)
				{
					Label = Items[KBL_CTRLSHIFT][i].Title;
				}
				else if (!(Global->Opt->CASRule&1) || !(Global->Opt->CASRule&2))
				{
					Label = Items[KBL_CTRLALTSHIFT][i].Title;
				}
			}
			else if (IntKeyState.AltPressed)
			{
				Label = Items[KBL_ALTSHIFT][i].Title;

				AltState=IntKeyState.AltPressed;
			}
			else
			{
				Label = Items[KBL_SHIFT][i].Title;
			}
		}
		else if (IntKeyState.CtrlPressed)
		{
			CtrlState=IntKeyState.CtrlPressed;

			if (IntKeyState.AltPressed)
			{
				Label = Items[KBL_CTRLALT][i].Title;

				AltState=IntKeyState.AltPressed;
			}
			else
			{
				Label = Items[KBL_CTRL][i].Title;
			}
		}
		else if (IntKeyState.AltPressed)
		{
			AltState=IntKeyState.AltPressed;

			Label = Items[KBL_ALT][i].Title;
		}
		else
		{
			Label = Items[KBL_MAIN][i].Title;
		}

		if (!Label)
			Label=L"";

		string strLabel=Label;

		if (strLabel.Contains(L'|'))
		{
			auto LabelList(StringToList(Label, STLF_NOTRIM|STLF_NOUNQUOTE, L"|"));
			if(!LabelList.empty())
			{
				string strLabelTest, strLabel2;
				strLabel = LabelList.front();
				LabelList.pop_front();
				std::for_each(CONST_RANGE(LabelList, Label2)
				{
					strLabelTest=strLabel;
					strLabelTest += Label2;
					if (strLabelTest.GetLength() <= static_cast<size_t>(LabelWidth))
						if (Label2.GetLength() > strLabel2.GetLength())
							strLabel2 = Label2;
				});

				strLabel+=strLabel2;
			}
		}

		Global->FS << fmt::LeftAlign()<<fmt::ExactWidth(LabelWidth)<<strLabel;

		if (i<KEY_COUNT-1)
		{
			SetColor(COL_KEYBARBACKGROUND);
			Text(L" ");
		}
	}

	int Width=X2-WhereX()+1;

	if (Width>0)
	{
		SetColor(COL_KEYBARTEXT);
		Global->FS << fmt::MinWidth(Width)<<L"";
	}
}

void KeyBar::ClearKeyTitles(bool Custom)
{
	std::for_each(RANGE(Items, i)
	{
		std::for_each(RANGE(i, j)
		{
			(Custom? j.CustomTitle : j.Title).Clear();
		});
	});
}

void KeyBar::SetLabels(LNGID StartIndex)
{
	std::for_each(RANGE(Items, Group)
	{
		std::for_each(RANGE(Group, i)
		{
			i.Title = MSG(StartIndex++);
		});
	});
}

void KeyBar::SetCustomLabels(const wchar_t *Area)
{
	if (!CustomLabelsReaded || StrCmpI(strLanguage, Global->Opt->strLanguage) || StrCmpI(CustomArea, Area))
	{
		DWORD Index=0;
		string strRegName;
		string strValue;
		string strValueName;

		strLanguage = Global->Opt->strLanguage.Get();
		CustomArea = Area;
		strRegName=L"KeyBarLabels.";
		strRegName+=strLanguage + L"." + Area;

		ClearKeyTitles(true);

		while (Global->Db->GeneralCfg()->EnumValues(strRegName,Index++,strValueName,strValue))
		{
			DWORD Key=KeyNameToKey(strValueName);
			DWORD Key0=Key&(~KEY_CTRLMASK);
			DWORD Ctrl=Key&KEY_CTRLMASK;

			if (Key0 >= KEY_F1 && Key0 <= KEY_F24)
			{
				size_t J;
				static DWORD Area[][2]=
				{
					{ KBL_MAIN,         0 },
					{ KBL_SHIFT,        KEY_SHIFT },
					{ KBL_CTRL,         KEY_CTRL },
					{ KBL_ALT,          KEY_ALT },
					{ KBL_CTRLSHIFT,    KEY_CTRL|KEY_SHIFT },
					{ KBL_ALTSHIFT,     KEY_ALT|KEY_SHIFT },
					{ KBL_CTRLALT,      KEY_CTRL|KEY_ALT },
					{ KBL_CTRLALTSHIFT, KEY_CTRL|KEY_ALT|KEY_SHIFT },
				};

				for (J=0; J < ARRAYSIZE(Area); ++J)
					if (Area[J][1] == Ctrl)
						break;

				if (J <= ARRAYSIZE(Area))
				{
					Items[Area[J][0]][Key0-KEY_F1].CustomTitle = strValue;
				}
			}
		}

		CustomLabelsReaded=TRUE;
	}

	std::for_each(RANGE(Items, Group)
	{
		std::for_each(RANGE(Group, i)
		{
			if (i.CustomTitle && *i.CustomTitle)
			{
				i.Title = i.CustomTitle;
			}
		});
	});
}

// ��������� ������ Label
void KeyBar::Change(int Group,const wchar_t *NewStr,int Pos)
{
	if (NewStr)
	{
		Items[Group][Pos].Title = NewStr;
	}
}


int KeyBar::ProcessKey(int Key)
{
	switch (Key)
	{
		case KEY_KILLFOCUS:
		case KEY_GOTFOCUS:
			RedrawIfChanged();
			return TRUE;
	}

	return FALSE;
}

int KeyBar::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	INPUT_RECORD rec;
	int Key;

	if (!IsVisible())
		return FALSE;

	if (!(MouseEvent->dwButtonState & 3) || MouseEvent->dwEventFlags)
		return FALSE;

	if (MouseEvent->dwMousePosition.X<X1 || MouseEvent->dwMousePosition.X>X2 ||
	        MouseEvent->dwMousePosition.Y!=Y1)
		return FALSE;

	int KeyWidth=(X2-X1-1)/12;

	if (KeyWidth<8)
		KeyWidth=8;

	int X=MouseEvent->dwMousePosition.X-X1;

	if (X<KeyWidth*9)
		Key=X/KeyWidth;
	else
		Key=9+(X-KeyWidth*9)/(KeyWidth+1);

	for (;;)
	{
		GetInputRecord(&rec);

		if (rec.EventType==MOUSE_EVENT && !(rec.Event.MouseEvent.dwButtonState & 3))
			break;
	}

	if (rec.Event.MouseEvent.dwMousePosition.X<X1 ||
	        rec.Event.MouseEvent.dwMousePosition.X>X2 ||
	        rec.Event.MouseEvent.dwMousePosition.Y!=Y1)
		return FALSE;

	int NewKey,NewX=MouseEvent->dwMousePosition.X-X1;

	if (NewX<KeyWidth*9)
		NewKey=NewX/KeyWidth;
	else
		NewKey=9+(NewX-KeyWidth*9)/(KeyWidth+1);

	if (Key!=NewKey)
		return FALSE;

	if (Key>11)
		Key=11;

	if (MouseEvent->dwControlKeyState & (RIGHT_ALT_PRESSED|LEFT_ALT_PRESSED) ||
	        (MouseEvent->dwButtonState & RIGHTMOST_BUTTON_PRESSED))
	{
		if (MouseEvent->dwControlKeyState & SHIFT_PRESSED)
			Key+=KEY_ALTSHIFTF1;
		else if (MouseEvent->dwControlKeyState & (RIGHT_CTRL_PRESSED|LEFT_CTRL_PRESSED))
			Key+=KEY_CTRLALTF1;
		else
			Key+=KEY_ALTF1;
	}
	else if (MouseEvent->dwControlKeyState & (RIGHT_CTRL_PRESSED|LEFT_CTRL_PRESSED))
	{
		if (MouseEvent->dwControlKeyState & SHIFT_PRESSED)
			Key+=KEY_CTRLSHIFTF1;
		else
			Key+=KEY_CTRLF1;
	}
	else if (MouseEvent->dwControlKeyState & SHIFT_PRESSED)
		Key+=KEY_SHIFTF1;
	else
		Key+=KEY_F1;

	//if (Owner)
	//Owner->ProcessKey(Key);
	FrameManager->ProcessKey(Key);
	return TRUE;
}


void KeyBar::RedrawIfChanged()
{
	if (IntKeyState.ShiftPressed!=ShiftState ||
	        IntKeyState.CtrlPressed!=CtrlState ||
	        IntKeyState.AltPressed!=AltState)
	{
		//_SVS("KeyBar::RedrawIfChanged()");
		Redraw();
	}
}

size_t KeyBar::Change(const KeyBarTitles *Kbt)
{
	size_t Result=0;

	if (!Kbt)
		return Result;

	static DWORD Groups[]=
	{
		0,KBL_MAIN,
		KEY_SHIFT,KBL_SHIFT,
		KEY_CTRL,KBL_CTRL,
		KEY_ALT,KBL_ALT,
		KEY_CTRL|KEY_SHIFT,KBL_CTRLSHIFT,
		KEY_ALT|KEY_SHIFT,KBL_ALTSHIFT,
		KEY_CTRL|KEY_ALT,KBL_CTRLALT,
		KEY_CTRL|KEY_ALT|KEY_SHIFT,KBL_CTRLALTSHIFT,
	};

	for (size_t I = 0; I < Kbt->CountLabels; ++I)
	{

		WORD Pos=Kbt->Labels[I].Key.VirtualKeyCode;
		DWORD Shift=0,Flags=Kbt->Labels[I].Key.ControlKeyState;
		if(Flags&(LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)) Shift|=KEY_CTRL;
		if(Flags&(LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED)) Shift|=KEY_ALT;
		if(Flags&SHIFT_PRESSED) Shift|=KEY_SHIFT;

		if (Pos >= VK_F1 && Pos <= VK_F24)
		{
			for (unsigned J=0; J < ARRAYSIZE(Groups); J+=2)
			{
				if (Groups[J] == Shift)
				{
					Change(Groups[J+1],Kbt->Labels[I].Text,Pos-VK_F1);
					Result++;
					break;
				}
			}
		}
	}

	return Result;
}
