/*
edit.cpp

������ ���������
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

#include "edit.hpp"
#include "keyboard.hpp"
#include "macroopcode.hpp"
#include "ctrlobj.hpp"
#include "plugins.hpp"
#include "scrbuf.hpp"
#include "interf.hpp"
#include "clipboard.hpp"
#include "xlat.hpp"
#include "shortcuts.hpp"
#include "pathmix.hpp"
#include "panelmix.hpp"
#include "manager.hpp"
#include "fileedit.hpp"


const GUID& ColorItem::GetOwner() const
{
	return *reinterpret_cast<const GUID*>(Owner);
}

void ColorItem::SetOwner(const GUID& Value)
{
	Owner = &*Global->Sets->GuidSet.emplace(Value).first;
}

const FarColor& ColorItem::GetColor() const
{
	return *reinterpret_cast<const FarColor*>(Color);
}

void ColorItem::SetColor(const FarColor& Value)
{
	Color = &*Global->Sets->ColorSet.emplace(Value).first;
}

static int Recurse=0;

enum {EOL_NONE,EOL_CR,EOL_LF,EOL_CRLF,EOL_CRCRLF};
static const wchar_t *EOL_TYPE_CHARS[]={L"",L"\r",L"\n",L"\r\n",L"\r\r\n"};

static const wchar_t EDMASK_ANY    = L'X'; // ��������� ������� � ������ ����� ����� ������;
static const wchar_t EDMASK_DSS    = L'#'; // ��������� ������� � ������ ����� �����, ������ � ���� ������;
static const wchar_t EDMASK_DIGIT  = L'9'; // ��������� ������� � ������ ����� ������ �����;
static const wchar_t EDMASK_DIGITS = L'N'; // ��������� ������� � ������ ����� ������ ����� � �������;
static const wchar_t EDMASK_ALPHA  = L'A'; // ��������� ������� � ������ ����� ������ �����.
static const wchar_t EDMASK_HEX    = L'H'; // ��������� ������� � ������ ����� ����������������� �������.

Edit::Edit(ScreenObject *pOwner, bool bAllocateData):
	Str(bAllocateData ? static_cast<wchar_t*>(xf_malloc(sizeof(wchar_t))) : nullptr),
	StrSize(0),
	CurPos(0),
	m_next(nullptr),
	m_prev(nullptr),
	ColorList(nullptr),
	ColorCount(0),
	MaxColorCount(0),
	SelStart(-1),
	SelEnd(0),
	LeftPos(0),
	EndType(EOL_NONE)
{
	SetOwner(pOwner);

	if (bAllocateData)
		*Str=0;

	Flags.Set(FEDITLINE_EDITBEYONDEND);
	Flags.Change(FEDITLINE_DELREMOVESBLOCKS,Global->Opt->EdOpt.DelRemovesBlocks);
	Flags.Change(FEDITLINE_PERSISTENTBLOCKS,Global->Opt->EdOpt.PersistentBlocks);
	Flags.Change(FEDITLINE_SHOWWHITESPACE,Global->Opt->EdOpt.ShowWhiteSpace!=0);
	Flags.Change(FEDITLINE_SHOWLINEBREAK,Global->Opt->EdOpt.ShowWhiteSpace==1);
}

Edit::~Edit()
{
	if (ColorList)
		xf_free(ColorList);

	if (Str)
		xf_free(Str);
}

void Edit::DisplayObject()
{
	if (Flags.Check(FEDITLINE_DROPDOWNBOX))
	{
		Flags.Clear(FEDITLINE_CLEARFLAG);  // ��� ����-���� ��� �� ����� �������� unchanged text
		SelStart=0;
		SelEnd=StrSize; // � ����� ������� ��� ��� �������� -
		//    ���� �� ���������� �� ������� Edit
	}

	//   ���������� ������ ��������� ������� � ������ � ������ Mask.
	int Value=(GetPrevCurPos()>CurPos)?-1:1;
	CurPos=GetNextCursorPos(CurPos,Value);
	FastShow();

	/* $ 26.07.2000 tran
	   ��� DropDownBox ������ ���������
	   �� ���� ���� - ���������� �� �� ����� ������� ����� */
	if (Flags.Check(FEDITLINE_DROPDOWNBOX))
		::SetCursorType(0,10);
	else
	{
		if (Flags.Check(FEDITLINE_OVERTYPE))
		{
			int NewCursorSize=IsConsoleFullscreen()?
			                  (Global->Opt->CursorSize[3]?(int)Global->Opt->CursorSize[3]:99):
					                  (Global->Opt->CursorSize[2]?(int)Global->Opt->CursorSize[2]:99);
			::SetCursorType(1,GetCursorSize()==-1?NewCursorSize:GetCursorSize());
		}
		else
{
			int NewCursorSize=IsConsoleFullscreen()?
			                  (Global->Opt->CursorSize[1]?(int)Global->Opt->CursorSize[1]:10):
					                  (Global->Opt->CursorSize[0]?(int)Global->Opt->CursorSize[0]:10);
			::SetCursorType(1,GetCursorSize()==-1?NewCursorSize:GetCursorSize());
		}
	}

	MoveCursor(X1+GetLineCursorPos()-LeftPos,Y1);
}

void Edit::SetCursorType(bool Visible, DWORD Size)
{
	Flags.Change(FEDITLINE_CURSORVISIBLE,Visible);
	SetCursorSize(Size);
	::SetCursorType(Visible,Size);
}

void Edit::GetCursorType(bool& Visible, DWORD& Size)
{
	Visible=Flags.Check(FEDITLINE_CURSORVISIBLE)!=FALSE;
	Size = GetCursorSize();
}

//   ���������� ������ ��������� ������� � ������ � ������ Mask.
int Edit::GetNextCursorPos(int Position,int Where)
{
	int Result = Position;
	auto Mask = GetInputMask();

	if (!Mask.empty() && (Where==-1 || Where==1))
	{
		int PosChanged=FALSE;
		int MaskLen = static_cast<int>(Mask.size());

		for (int i=Position; i<MaskLen && i>=0; i+=Where)
		{
			if (CheckCharMask(Mask[i]))
			{
				Result=i;
				PosChanged=TRUE;
				break;
			}
		}

		if (!PosChanged)
		{
			for (int i=Position; i>=0; i--)
			{
				if (CheckCharMask(Mask[i]))
				{
					Result=i;
					PosChanged=TRUE;
					break;
				}
			}
		}

		if (!PosChanged)
		{
			for (int i=Position; i<MaskLen; i++)
			{
				if (CheckCharMask(Mask[i]))
				{
					Result=i;
					break;
				}
			}
		}
	}

	return Result;
}

void Edit::FastShow()
{
	const size_t EditLength=ObjWidth();

	if (!Flags.Check(FEDITLINE_EDITBEYONDEND) && CurPos>StrSize && StrSize>=0)
		CurPos=StrSize;

	if (GetMaxLength()!=-1)
	{
		if (StrSize>GetMaxLength())
		{
			Str[GetMaxLength()]=0;
			StrSize=GetMaxLength();
		}

		if (CurPos>GetMaxLength()-1)
			CurPos=GetMaxLength()>0 ? (GetMaxLength()-1):0;
	}

	int TabCurPos=GetTabCurPos();

	/* $ 31.07.2001 KM
	  ! ��� ���������� ������� ����������� ������
	    � ������ �������.
	*/
	int UnfixedLeftPos = LeftPos;
	if (!Flags.Check(FEDITLINE_DROPDOWNBOX))
	{
		FixLeftPos(TabCurPos);
	}

	GotoXY(X1,Y1);
	int TabSelStart=(SelStart==-1) ? -1:RealPosToTab(SelStart);
	int TabSelEnd=(SelEnd<0) ? -1:RealPosToTab(SelEnd);

	/* $ 17.08.2000 KM
	   ���� ���� �����, ������� ���������� ������, �� ����
	   ��� "����������" ������� � �����, �� ���������� ����������
	   ������ ��������� �������������� � Str
	*/
	auto Mask = GetInputMask();
	if (!Mask.empty())
		RefreshStrByMask();

	string OutStr, OutStrTmp;
	OutStr.reserve(EditLength);
	OutStrTmp.reserve(EditLength);

	SetLineCursorPos(TabCurPos);
	int RealLeftPos=TabPosToReal(LeftPos);

	OutStrTmp.assign(Str + RealLeftPos, std::max(0, std::min(static_cast<int>(EditLength), StrSize-RealLeftPos)));

	{
		auto TrailingSpaces = OutStrTmp.cend();
		if (Flags.Check(FEDITLINE_PARENT_SINGLELINE|FEDITLINE_PARENT_MULTILINE) && Mask.empty() && !OutStrTmp.empty())
		{
			TrailingSpaces = std::find_if_not(OutStrTmp.crbegin(), OutStrTmp.crend(), [](wchar_t i) { return IsSpace(i);}).base();
		}

		for (auto i = OutStrTmp.begin(); OutStr.size() < EditLength && i != OutStrTmp.end(); ++i)
		{
			if ((Flags.Check(FEDITLINE_SHOWWHITESPACE) && Flags.Check(FEDITLINE_EDITORMODE)) || i >= TrailingSpaces)
			{
				if (*i==L' ') // *p==L'\xA0' ==> NO-BREAK SPACE
				{
					*i=L'\xB7';
				}
			}

			if (*i == L'\t')
			{
				int S=GetTabSize()-((UnfixedLeftPos+OutStr.size()) % GetTabSize());
				OutStr.push_back((((Flags.Check(FEDITLINE_SHOWWHITESPACE) && Flags.Check(FEDITLINE_EDITORMODE)) || i >= TrailingSpaces) && (!OutStr.empty() || S==GetTabSize()))?L'\x2192':L' ');
				for (int i=1; i<S && OutStr.size() < EditLength; ++i)
				{
					OutStr.push_back(L' ');
				}
			}
			else
			{
				if (!*i)
					OutStr.push_back(L' ');
				else
					OutStr.push_back(*i);
			}
		}

		if (Flags.Check(FEDITLINE_PASSWORDMODE))
			OutStr.assign(OutStr.size(), L'*');

		if (Flags.Check(FEDITLINE_SHOWLINEBREAK) && Flags.Check(FEDITLINE_EDITORMODE) && (StrSize >= RealLeftPos) && (OutStr.size() < EditLength))
		{
			switch(EndType)
			{
			case EOL_CR:
				OutStr.push_back(Oem2Unicode[13]);
				break;
			case EOL_LF:
				OutStr.push_back(Oem2Unicode[10]);
				break;
			case EOL_CRLF:
				OutStr.push_back(Oem2Unicode[13]);
				if(OutStr.size() < EditLength)
				{
					OutStr.push_back(Oem2Unicode[10]);
				}
				break;
			case EOL_CRCRLF:
				OutStr.push_back(Oem2Unicode[13]);
				if(OutStr.size() < EditLength)
				{
					OutStr.push_back(Oem2Unicode[13]);
					if(OutStr.size() < EditLength)
					{
						OutStr.push_back(Oem2Unicode[10]);
					}
				}
				break;
			}
		}

		if(!m_next && Flags.Check(FEDITLINE_SHOWWHITESPACE) && Flags.Check(FEDITLINE_EDITORMODE) && (StrSize >= RealLeftPos) && (OutStr.size() < EditLength))
		{
			OutStr.push_back(L'\x25a1');
		}
	}

	SetColor(GetNormalColor());

	if (TabSelStart==-1)
	{
		if (Flags.Check(FEDITLINE_CLEARFLAG))
		{
			SetColor(GetUnchangedColor());

			if (!Mask.empty())
				RemoveTrailingSpaces(OutStr);

			Global->FS << fmt::LeftAlign() << OutStr;
			SetColor(GetNormalColor());
			size_t BlankLength=EditLength-OutStr.size();

			if (BlankLength > 0)
			{
				Global->FS << fmt::MinWidth(BlankLength)<<L"";
			}
		}
		else
		{
			Global->FS << fmt::LeftAlign()<<fmt::ExactWidth(EditLength)<<OutStr;
		}
	}
	else
	{
		if ((TabSelStart-=LeftPos)<0)
			TabSelStart=0;

		int AllString=(TabSelEnd==-1);

		if (AllString)
			TabSelEnd=static_cast<int>(EditLength);
		else if ((TabSelEnd-=LeftPos)<0)
			TabSelEnd=0;

		OutStr.append(EditLength - OutStr.size(), L' ');

		/* $ 24.08.2000 SVS
		   ! � DropDowList`� ��������� �� ������ ��������� - �� ��� ������� �����
		     ���� ���� ������ ������
		*/
		if (TabSelStart>=static_cast<int>(EditLength) /*|| !AllString && TabSelStart>=StrSize*/ ||
		        TabSelEnd<TabSelStart)
		{
			if (Flags.Check(FEDITLINE_DROPDOWNBOX))
			{
				SetColor(GetSelectedColor());
				Global->FS << fmt::MinWidth(X2-X1+1)<<OutStr;
			}
			else
				Text(OutStr);
		}
		else
		{
			Global->FS << fmt::MaxWidth(TabSelStart)<<OutStr;
			SetColor(GetSelectedColor());

			if (!Flags.Check(FEDITLINE_DROPDOWNBOX))
			{
				Global->FS << fmt::MaxWidth(TabSelEnd-TabSelStart) << OutStr.data() + TabSelStart;

				if (TabSelEnd<static_cast<int>(EditLength))
				{
					//SetColor(Flags.Check(FEDITLINE_CLEARFLAG) ? SelColor:Color);
					SetColor(GetNormalColor());
					Text(OutStr.data()+TabSelEnd);
				}
			}
			else
			{
				Global->FS << fmt::MinWidth(X2-X1+1)<<OutStr;
			}
		}
	}

	/* $ 26.07.2000 tran
	   ��� ����-���� ����� ��� �� ����� */
	if (!Flags.Check(FEDITLINE_DROPDOWNBOX))
		ApplyColor(GetSelectedColor());
}

int Edit::RecurseProcessKey(int Key)
{
	Recurse++;
	int RetCode=ProcessKey(Key);
	Recurse--;
	return(RetCode);
}

// ������� ������� ������ ��������� - �� ��������� �� ���� ������
int Edit::ProcessInsPath(int Key,int PrevSelStart,int PrevSelEnd)
{
	int RetCode=FALSE;
	string strPathName;

	if (Key>=KEY_RCTRL0 && Key<=KEY_RCTRL9) // ��������?
	{
		if (Shortcuts().Get(Key-KEY_RCTRL0,&strPathName,nullptr,nullptr,nullptr))
			RetCode=TRUE;
	}
	else // ����/�����?
	{
		RetCode=_MakePath1(Key,strPathName,L"");
	}

	// ���� ���-���� ����������, ������ ��� � ������� (PathName)
	if (RetCode)
	{
		if (Flags.Check(FEDITLINE_CLEARFLAG))
		{
			LeftPos=0;
			SetString(L"");
		}

		if (PrevSelStart!=-1)
		{
			SelStart=PrevSelStart;
			SelEnd=PrevSelEnd;
		}

		if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS))
			DeleteBlock();

		InsertString(strPathName);
		Flags.Clear(FEDITLINE_CLEARFLAG);
	}

	return RetCode;
}

__int64 Edit::VMProcess(int OpCode,void *vParam,__int64 iParam)
{
	switch (OpCode)
	{
		case MCODE_C_EMPTY:
			return !GetLength();
		case MCODE_C_SELECTED:
			return SelStart != -1 && SelStart < SelEnd;
		case MCODE_C_EOF:
			return CurPos >= StrSize;
		case MCODE_C_BOF:
			return !CurPos;
		case MCODE_V_ITEMCOUNT:
			return StrSize;
		case MCODE_V_CURPOS:
			return GetLineCursorPos()+1;
		case MCODE_F_EDITOR_SEL:
		{
			int Action=(int)((intptr_t)vParam);

			switch (Action)
			{
				case 0:  // Get Param
				{
					switch (iParam)
					{
						case 0:  // return FirstLine
						case 2:  // return LastLine
							return IsSelection()?1:0;
						case 1:  // return FirstPos
							return IsSelection()?SelStart+1:0;
						case 3:  // return LastPos
							return IsSelection()?SelEnd:0;
						case 4: // return block type (0=nothing 1=stream, 2=column)
							return IsSelection()?1:0;
					}

					break;
				}
				case 1:  // Set Pos
				{
					if (IsSelection())
					{
						switch (iParam)
						{
							case 0: // begin block (FirstLine & FirstPos)
							case 1: // end block (LastLine & LastPos)
							{
								SetTabCurPos(iParam?SelEnd:SelStart);
								Show();
								return 1;
							}
						}
					}

					break;
				}
				case 2: // Set Stream Selection Edge
				case 3: // Set Column Selection Edge
				{
					switch (iParam)
					{
						case 0:  // selection start
						{
							SetMacroSelectionStart(GetTabCurPos());
							return 1;
						}
						case 1:  // selection finish
						{
							if (GetMacroSelectionStart() != -1)
							{
								if (GetMacroSelectionStart() != GetTabCurPos())
									Select(GetMacroSelectionStart(),GetTabCurPos());
								else
									Select(-1,0);

								Show();
								SetMacroSelectionStart(-1);
								return 1;
							}

							return 0;
						}
					}

					break;
				}
				case 4: // UnMark sel block
				{
					Select(-1,0);
					SetMacroSelectionStart(-1);
					Show();
					return 1;
				}
			}

			break;
		}
	}

	return 0;
}

int Edit::ProcessKey(int Key)
{
	auto Mask = GetInputMask();
	switch (Key)
	{
		case KEY_ADD:
			Key=L'+';
			break;
		case KEY_SUBTRACT:
			Key=L'-';
			break;
		case KEY_MULTIPLY:
			Key=L'*';
			break;
		case KEY_DIVIDE:
			Key=L'/';
			break;
		case KEY_DECIMAL:
			Key=L'.';
			break;
		case KEY_CTRLC:
		case KEY_RCTRLC:
			Key=KEY_CTRLINS;
			break;
		case KEY_CTRLV:
		case KEY_RCTRLV:
			Key=KEY_SHIFTINS;
			break;
		case KEY_CTRLX:
		case KEY_RCTRLX:
			Key=KEY_SHIFTDEL;
			break;
	}

	int PrevSelStart=-1,PrevSelEnd=0;

	if (!Flags.Check(FEDITLINE_DROPDOWNBOX) && (Key==KEY_CTRLL || Key==KEY_RCTRLL))
	{
		Flags.Swap(FEDITLINE_READONLY);
	}

	/* $ 26.07.2000 SVS
	   Bugs #??
	     � ������� ����� ��� ���������� ����� �������� BS � ������
	     ���������� �������� ����� (��� � ���������) ��������:
	       - ������ ����� �������� ������
	       - ��������� ����� �����
	*/
	if ((((Key==KEY_BS || Key==KEY_DEL || Key==KEY_NUMDEL) && Flags.Check(FEDITLINE_DELREMOVESBLOCKS)) || Key==KEY_CTRLD || Key==KEY_RCTRLD) &&
	        !Flags.Check(FEDITLINE_EDITORMODE) && SelStart!=-1 && SelStart<SelEnd)
	{
		DeleteBlock();
		Show();
		return TRUE;
	}

	int _Macro_IsExecuting=Global->CtrlObject->Macro.IsExecuting();

	// $ 04.07.2000 IG - ��������� ��������� �� ������ ������� (00025.edit.cpp.txt)
	if (!IntKeyState.ShiftPressed && (!_Macro_IsExecuting || (IsNavKey(Key) && _Macro_IsExecuting)) &&
	        !IsShiftKey(Key) && !Recurse &&
	        Key!=KEY_SHIFT && Key!=KEY_CTRL && Key!=KEY_ALT &&
	        Key!=KEY_RCTRL && Key!=KEY_RALT && Key!=KEY_NONE &&
	        Key!=KEY_INS &&
	        Key!=KEY_KILLFOCUS && Key != KEY_GOTFOCUS &&
	        ((Key&(~KEY_CTRLMASK)) != KEY_LWIN && (Key&(~KEY_CTRLMASK)) != KEY_RWIN && (Key&(~KEY_CTRLMASK)) != KEY_APPS)
	   )
	{
		Flags.Clear(FEDITLINE_MARKINGBLOCK); // ���... � ��� ����� ������ ����?

		if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS) && !(Key==KEY_CTRLINS || Key==KEY_RCTRLINS || Key==KEY_CTRLNUMPAD0 || Key==KEY_RCTRLNUMPAD0) &&
		        !(Key==KEY_SHIFTDEL||Key==KEY_SHIFTNUMDEL||Key==KEY_SHIFTDECIMAL) && !Flags.Check(FEDITLINE_EDITORMODE) &&
		        (Key != KEY_CTRLQ && Key != KEY_RCTRLQ) &&
		        !(Key == KEY_SHIFTINS || Key == KEY_SHIFTNUMPAD0)) //Key != KEY_SHIFTINS) //??
		{
			/* $ 12.11.2002 DJ
			   ����� ����������, ���� ������ �� ����������?
			*/
			if (SelStart != -1 || SelEnd )
			{
				PrevSelStart=SelStart;
				PrevSelEnd=SelEnd;
				Select(-1,0);
				Show();
			}
		}
	}

	/* $ 11.09.2000 SVS
	   ���� Global->Opt->DlgEULBsClear = 1, �� BS � �������� ��� UnChanged ������
	   ������� ����� ������ �����, ��� � Del
	*/
	if (((Global->Opt->Dialogs.EULBsClear && Key==KEY_BS) || Key==KEY_DEL || Key==KEY_NUMDEL) &&
	        Flags.Check(FEDITLINE_CLEARFLAG) && CurPos>=StrSize)
		Key=KEY_CTRLY;

	/* $ 15.09.2000 SVS
	   Bug - �������� ������� ������ -> Shift-Del ������� ��� ������
	         ��� ������ ���� ������ ��� UnChanged ���������
	*/
	if ((Key == KEY_SHIFTDEL || Key == KEY_SHIFTNUMDEL || Key == KEY_SHIFTDECIMAL) && Flags.Check(FEDITLINE_CLEARFLAG) && CurPos>=StrSize && SelStart==-1)
	{
		SelStart=0;
		SelEnd=StrSize;
	}

	if (Flags.Check(FEDITLINE_CLEARFLAG) && ((Key <= 0xFFFF && Key!=KEY_BS) || Key==KEY_CTRLBRACKET || Key==KEY_RCTRLBRACKET ||
	        Key==KEY_CTRLBACKBRACKET || Key==KEY_RCTRLBACKBRACKET || Key==KEY_CTRLSHIFTBRACKET || Key==KEY_RCTRLSHIFTBRACKET ||
	        Key==KEY_CTRLSHIFTBACKBRACKET || Key==KEY_RCTRLSHIFTBACKBRACKET || Key==KEY_SHIFTENTER || Key==KEY_SHIFTNUMENTER))
	{
		LeftPos=0;
		SetString(L"");
		Show();
	}

	// ����� - ����� ������� ������� �����/������
	if (ProcessInsPath(Key,PrevSelStart,PrevSelEnd))
	{
		Show();
		return TRUE;
	}

	if (Key!=KEY_NONE && Key!=KEY_IDLE && Key!=KEY_SHIFTINS && Key!=KEY_SHIFTNUMPAD0 && (Key!=KEY_CTRLINS && Key!=KEY_RCTRLINS) &&
	        ((unsigned int)Key<KEY_F1 || (unsigned int)Key>KEY_F12) && Key!=KEY_ALT && Key!=KEY_SHIFT &&
	        Key!=KEY_CTRL && Key!=KEY_RALT && Key!=KEY_RCTRL &&
	        !((Key>=KEY_ALT_BASE && Key <= KEY_ALT_BASE+0xFFFF) || (Key>=KEY_RALT_BASE && Key <= KEY_RALT_BASE+0xFFFF)) && // ???? 256 ???
	        !(((unsigned int)Key>=KEY_MACRO_BASE && (unsigned int)Key<=KEY_MACRO_ENDBASE) || ((unsigned int)Key>=KEY_OP_BASE && (unsigned int)Key <=KEY_OP_ENDBASE)) &&
	        (Key!=KEY_CTRLQ && Key!=KEY_RCTRLQ))
	{
		Flags.Clear(FEDITLINE_CLEARFLAG);
		Show();
	}

	switch (Key)
	{
		case KEY_CTRLA: case KEY_RCTRLA:
			{
				Select(0, GetLength());
				Show();
			}
			break;
		case KEY_SHIFTLEFT: case KEY_SHIFTNUMPAD4:
		{
			if (CurPos>0)
			{
				RecurseProcessKey(KEY_LEFT);

				if (!Flags.Check(FEDITLINE_MARKINGBLOCK))
				{
					Select(-1,0);
					Flags.Set(FEDITLINE_MARKINGBLOCK);
				}

				if (SelStart!=-1 && SelStart<=CurPos)
					Select(SelStart,CurPos);
				else
				{
					int EndPos=CurPos+1;
					int NewStartPos=CurPos;

					if (EndPos>StrSize)
						EndPos=StrSize;

					if (NewStartPos>StrSize)
						NewStartPos=StrSize;

					AddSelect(NewStartPos,EndPos);
				}

				Show();
			}

			return TRUE;
		}
		case KEY_SHIFTRIGHT: case KEY_SHIFTNUMPAD6:
		{
			if (!Flags.Check(FEDITLINE_MARKINGBLOCK))
			{
				Select(-1,0);
				Flags.Set(FEDITLINE_MARKINGBLOCK);
			}

			if ((SelStart!=-1 && SelEnd==-1) || SelEnd>CurPos)
			{
				if (CurPos+1==SelEnd)
					Select(-1,0);
				else
					Select(CurPos+1,SelEnd);
			}
			else
				AddSelect(CurPos,CurPos+1);

			RecurseProcessKey(KEY_RIGHT);
			return TRUE;
		}
		case KEY_CTRLSHIFTLEFT:  case KEY_CTRLSHIFTNUMPAD4:
		case KEY_RCTRLSHIFTLEFT: case KEY_RCTRLSHIFTNUMPAD4:
		{
			if (CurPos>StrSize)
			{
				SetPrevCurPos(CurPos);
				CurPos=StrSize;
			}

			if (CurPos>0)
				RecurseProcessKey(KEY_SHIFTLEFT);

			while (CurPos>0 && !(!IsWordDiv(WordDiv(), Str[CurPos]) &&
			                     IsWordDiv(WordDiv(),Str[CurPos-1]) && !IsSpace(Str[CurPos])))
			{
				if (!IsSpace(Str[CurPos]) && (IsSpace(Str[CurPos-1]) ||
				                              IsWordDiv(WordDiv(), Str[CurPos-1])))
					break;

				RecurseProcessKey(KEY_SHIFTLEFT);
			}

			Show();
			return TRUE;
		}
		case KEY_CTRLSHIFTRIGHT:  case KEY_CTRLSHIFTNUMPAD6:
		case KEY_RCTRLSHIFTRIGHT: case KEY_RCTRLSHIFTNUMPAD6:
		{
			if (CurPos>=StrSize)
				return FALSE;

			RecurseProcessKey(KEY_SHIFTRIGHT);

			while (CurPos<StrSize && !(IsWordDiv(WordDiv(), Str[CurPos]) &&
			                           !IsWordDiv(WordDiv(), Str[CurPos-1])))
			{
				if (!IsSpace(Str[CurPos]) && (IsSpace(Str[CurPos-1]) || IsWordDiv(WordDiv(), Str[CurPos-1])))
					break;

				RecurseProcessKey(KEY_SHIFTRIGHT);

				if (GetMaxLength()!=-1 && CurPos==GetMaxLength()-1)
					break;
			}

			Show();
			return TRUE;
		}
		case KEY_SHIFTHOME:  case KEY_SHIFTNUMPAD7:
		{
			Lock();

			while (CurPos>0)
				RecurseProcessKey(KEY_SHIFTLEFT);

			Unlock();
			Show();
			return TRUE;
		}
		case KEY_SHIFTEND:  case KEY_SHIFTNUMPAD1:
		{
			Lock();
			int Len;

			if (!Mask.empty())
			{
				wchar_t_ptr ShortStr(StrSize + 1);

				if (!ShortStr)
					return FALSE;

				xwcsncpy(ShortStr.get(), Str, StrSize + 1);
				Len = StrLength(RemoveTrailingSpaces(ShortStr.get()));
			}
			else
				Len=StrSize;

			int LastCurPos=CurPos;

			while (CurPos<Len/*StrSize*/)
			{
				RecurseProcessKey(KEY_SHIFTRIGHT);

				if (LastCurPos==CurPos)break;

				LastCurPos=CurPos;
			}

			Unlock();
			Show();
			return TRUE;
		}
		case KEY_BS:
		{
			if (CurPos<=0)
				return FALSE;

			SetPrevCurPos(CurPos);
			CurPos--;

			if (CurPos<=LeftPos)
			{
				LeftPos-=15;

				if (LeftPos<0)
					LeftPos=0;
			}

			if (!RecurseProcessKey(KEY_DEL))
				Show();

			return TRUE;
		}
		case KEY_CTRLSHIFTBS:
		case KEY_RCTRLSHIFTBS:
		{
			DisableCallback();

			// BUGBUG
			for (int i=CurPos; i>=0; i--)
			{
				RecurseProcessKey(KEY_BS);
			}
			RevertCallback();
			Changed(true);
			Show();
			return TRUE;
		}
		case KEY_CTRLBS:
		case KEY_RCTRLBS:
		{
			if (CurPos>StrSize)
			{
				SetPrevCurPos(CurPos);
				CurPos=StrSize;
			}

			Lock();

			DisableCallback();

			// BUGBUG
			for (;;)
			{
				int StopDelete=FALSE;

				if (CurPos>1 && IsSpace(Str[CurPos-1])!=IsSpace(Str[CurPos-2]))
					StopDelete=TRUE;

				RecurseProcessKey(KEY_BS);

				if (!CurPos || StopDelete)
					break;

				if (IsWordDiv(WordDiv(),Str[CurPos-1]))
					break;
			}

			Unlock();
			RevertCallback();
			Changed(true);
			Show();
			return TRUE;
		}
		case KEY_CTRLQ:
		case KEY_RCTRLQ:
		{
			Lock();

			if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS) && (SelStart != -1 || Flags.Check(FEDITLINE_CLEARFLAG)))
				RecurseProcessKey(KEY_DEL);

			ProcessCtrlQ();
			Unlock();
			Show();
			return TRUE;
		}
		case KEY_OP_SELWORD:
		{
			int OldCurPos=CurPos;
			PrevSelStart=SelStart;
			PrevSelEnd=SelEnd;
#if defined(MOUSEKEY)

			if (CurPos >= SelStart && CurPos <= SelEnd)
			{ // �������� ��� ������ ��� ��������� ������� �����
				Select(0,StrSize);
			}
			else
#endif
			{
				int SStart, SEnd;

				if (CalcWordFromString(Str,CurPos,&SStart,&SEnd,WordDiv()))
					Select(SStart,SEnd+(SEnd < StrSize?1:0));
			}

			CurPos=OldCurPos; // ���������� �������
			Show();
			return TRUE;
		}
		case KEY_OP_PLAINTEXT:
		{
			if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS))
			{
				if (SelStart != -1 || Flags.Check(FEDITLINE_CLEARFLAG)) // BugZ#1053 - ���������� � $Text
					RecurseProcessKey(KEY_DEL);
			}

			ProcessInsPlainText(Global->CtrlObject->Macro.eStackAsString());

			Show();
			return TRUE;
		}
		case KEY_CTRLT:
		case KEY_CTRLDEL:
		case KEY_CTRLNUMDEL:
		case KEY_CTRLDECIMAL:
		case KEY_RCTRLT:
		case KEY_RCTRLDEL:
		case KEY_RCTRLNUMDEL:
		case KEY_RCTRLDECIMAL:
		{
			if (CurPos>=StrSize)
				return FALSE;

			Lock();
			DisableCallback();
			if (!Mask.empty())
			{
				int MaskLen = static_cast<int>(Mask.size());
				int ptr=CurPos;

				while (ptr<MaskLen)
				{
					ptr++;

					if (!CheckCharMask(Mask[ptr]) ||
					        (IsSpace(Str[ptr]) && !IsSpace(Str[ptr+1])) ||
					        (IsWordDiv(WordDiv(), Str[ptr])))
						break;
				}

				// BUGBUG
				for (int i=0; i<ptr-CurPos; i++)
					RecurseProcessKey(KEY_DEL);
			}
			else
			{
				for (;;)
				{
					int StopDelete=FALSE;

					if (CurPos<StrSize-1 && IsSpace(Str[CurPos]) && !IsSpace(Str[CurPos+1]))
						StopDelete=TRUE;

					RecurseProcessKey(KEY_DEL);

					if (CurPos>=StrSize || StopDelete)
						break;

					if (IsWordDiv(WordDiv(), Str[CurPos]))
						break;
				}
			}

			Unlock();
			RevertCallback();
			Changed(true);
			Show();
			return TRUE;
		}
		case KEY_CTRLY:
		case KEY_RCTRLY:
		{
			if (Flags.Check(FEDITLINE_READONLY|FEDITLINE_DROPDOWNBOX))
				return (TRUE);

			SetPrevCurPos(CurPos);
			LeftPos=CurPos=0;
			*Str=0;
			StrSize=0;
			Str=(wchar_t *)xf_realloc(Str,1*sizeof(wchar_t));
			Select(-1,0);
			Changed();
			Show();
			return TRUE;
		}
		case KEY_CTRLK:
		case KEY_RCTRLK:
		{
			if (Flags.Check(FEDITLINE_READONLY|FEDITLINE_DROPDOWNBOX))
				return (TRUE);

			if (CurPos>=StrSize)
				return FALSE;

			if (!Flags.Check(FEDITLINE_EDITBEYONDEND))
			{
				if (CurPos<SelEnd)
					SelEnd=CurPos;

				if (SelEnd<SelStart && SelEnd!=-1)
				{
					SelEnd=0;
					SelStart=-1;
				}
			}

			Str[CurPos]=0;
			StrSize=CurPos;
			Str=(wchar_t *)xf_realloc(Str,(StrSize+1)*sizeof(wchar_t));
			Changed();
			Show();
			return TRUE;
		}
		case KEY_HOME:        case KEY_NUMPAD7:
		case KEY_CTRLHOME:    case KEY_CTRLNUMPAD7:
		case KEY_RCTRLHOME:   case KEY_RCTRLNUMPAD7:
		{
			SetPrevCurPos(CurPos);
			CurPos=0;
			Show();
			return TRUE;
		}
		case KEY_END:           case KEY_NUMPAD1:
		case KEY_CTRLEND:       case KEY_CTRLNUMPAD1:
		case KEY_RCTRLEND:      case KEY_RCTRLNUMPAD1:
		case KEY_CTRLSHIFTEND:  case KEY_CTRLSHIFTNUMPAD1:
		case KEY_RCTRLSHIFTEND: case KEY_RCTRLSHIFTNUMPAD1:
		{
			SetPrevCurPos(CurPos);

			if (!Mask.empty())
			{
				wchar_t_ptr ShortStr(StrSize + 1);

				if (!ShortStr)
					return FALSE;

				xwcsncpy(ShortStr.get(), Str, StrSize + 1);
				CurPos=StrLength(RemoveTrailingSpaces(ShortStr.get()));
			}
			else
				CurPos=StrSize;

			Show();
			return TRUE;
		}
		case KEY_LEFT:        case KEY_NUMPAD4:        case KEY_MSWHEEL_LEFT:
		case KEY_CTRLS:       case KEY_RCTRLS:
		{
			if (CurPos>0)
			{
				SetPrevCurPos(CurPos);
				CurPos--;
				Show();
			}

			return TRUE;
		}
		case KEY_RIGHT:       case KEY_NUMPAD6:        case KEY_MSWHEEL_RIGHT:
		case KEY_CTRLD:       case KEY_RCTRLD:
		{
			SetPrevCurPos(CurPos);

			if (!Mask.empty())
			{
				wchar_t_ptr ShortStr(StrSize + 1);

				if (!ShortStr)
					return FALSE;

				xwcsncpy(ShortStr.get(), Str, StrSize+1);
				int Len=StrLength(RemoveTrailingSpaces(ShortStr.get()));

				if (Len>CurPos)
					CurPos++;
			}
			else
				CurPos++;

			Show();
			return TRUE;
		}
		case KEY_INS:         case KEY_NUMPAD0:
		{
			Flags.Swap(FEDITLINE_OVERTYPE);
			Show();
			return TRUE;
		}
		case KEY_NUMDEL:
		case KEY_DEL:
		{
			if (Flags.Check(FEDITLINE_READONLY|FEDITLINE_DROPDOWNBOX))
				return (TRUE);

			if (CurPos>=StrSize)
				return FALSE;

			if (SelStart!=-1)
			{
				if (SelEnd!=-1 && CurPos<SelEnd)
					SelEnd--;

				if (CurPos<SelStart)
					SelStart--;

				if (SelEnd!=-1 && SelEnd<=SelStart)
				{
					SelStart=-1;
					SelEnd=0;
				}
			}

			if (!Mask.empty())
			{
				int MaskLen = static_cast<int>(Mask.size());
				int i,j;
				for (i=CurPos,j=CurPos; i<MaskLen; i++)
				{
					if (CheckCharMask(Mask[i+1]))
					{
						while (!CheckCharMask(Mask[j]) && j<MaskLen)
							j++;

						Str[j]=Str[i+1];
						j++;
					}
				}

				Str[j]=L' ';
			}
			else
			{
				wmemmove(Str+CurPos,Str+CurPos+1,StrSize-CurPos);
				StrSize--;
				Str=(wchar_t *)xf_realloc(Str,(StrSize+1)*sizeof(wchar_t));
			}

			Changed(true);
			Show();
			return TRUE;
		}
		case KEY_CTRLLEFT:  case KEY_CTRLNUMPAD4:
		case KEY_RCTRLLEFT: case KEY_RCTRLNUMPAD4:
		{
			SetPrevCurPos(CurPos);

			if (CurPos>StrSize)
				CurPos=StrSize;

			if (CurPos>0)
				CurPos--;

			while (CurPos>0 && !(!IsWordDiv(WordDiv(), Str[CurPos]) &&
			                     IsWordDiv(WordDiv(), Str[CurPos-1]) && !IsSpace(Str[CurPos])))
			{
				if (!IsSpace(Str[CurPos]) && IsSpace(Str[CurPos-1]))
					break;

				CurPos--;
			}

			Show();
			return TRUE;
		}
		case KEY_CTRLRIGHT:   case KEY_CTRLNUMPAD6:
		case KEY_RCTRLRIGHT:  case KEY_RCTRLNUMPAD6:
		{
			if (CurPos>=StrSize)
				return FALSE;

			SetPrevCurPos(CurPos);
			int Len;

			if (!Mask.empty())
			{
				wchar_t_ptr ShortStr(StrSize+1);

				if (!ShortStr)
					return FALSE;

				xwcsncpy(ShortStr.get(),Str,StrSize+1);
				Len=StrLength(RemoveTrailingSpaces(ShortStr.get()));

				if (Len>CurPos)
					CurPos++;
			}
			else
			{
				Len=StrSize;
				CurPos++;
			}

			while (CurPos<Len/*StrSize*/ && !(IsWordDiv(WordDiv(),Str[CurPos]) &&
			                                  !IsWordDiv(WordDiv(), Str[CurPos-1])))
			{
				if (!IsSpace(Str[CurPos]) && IsSpace(Str[CurPos-1]))
					break;

				CurPos++;
			}

			Show();
			return TRUE;
		}
		case KEY_SHIFTNUMDEL:
		case KEY_SHIFTDECIMAL:
		case KEY_SHIFTDEL:
		{
			if (SelStart==-1 || SelStart>=SelEnd)
				return FALSE;

			RecurseProcessKey(KEY_CTRLINS);
			DeleteBlock();
			Show();
			return TRUE;
		}
		case KEY_CTRLINS:     case KEY_CTRLNUMPAD0:
		case KEY_RCTRLINS:    case KEY_RCTRLNUMPAD0:
		{
			if (!Flags.Check(FEDITLINE_PASSWORDMODE))
			{
				if (SelStart==-1 || SelStart>=SelEnd)
				{
					if (!Mask.empty())
					{
						wchar_t_ptr ShortStr(StrSize + 1);

						if (!ShortStr)
							return FALSE;

						xwcsncpy(ShortStr.get(),Str,StrSize+1);
						RemoveTrailingSpaces(ShortStr.get());
						SetClipboard(ShortStr.get());
					}
					else
					{
						SetClipboard(Str);
					}
				}
				else if (SelEnd<=StrSize) // TODO: ���� � ������ ������� �������� "StrSize &&", �� �������� ��� "Ctrl-Ins � ������ ������ ������� ��������"
				{
					int Ch=Str[SelEnd];
					Str[SelEnd]=0;
					SetClipboard(Str+SelStart);
					Str[SelEnd]=Ch;
				}
			}

			return TRUE;
		}
		case KEY_SHIFTINS:    case KEY_SHIFTNUMPAD0:
		{
			string ClipText;

			if (GetMaxLength()==-1)
			{
				if (!GetClipboard(ClipText))
					return TRUE;
			}
			else
			{
				if (!GetClipboardEx(GetMaxLength(), ClipText))
					return TRUE;
			}

			if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS))
			{
				DisableCallback();
				DeleteBlock();
				RevertCallback();
			}

			for (int i=StrLength(Str)-1; i>=0 && IsEol(Str[i]); i--)
				Str[i]=0;

			for (size_t i=0; ClipText.data()[i]; i++)
			{
				if (IsEol(ClipText.data()[i]))
				{
					if (IsEol(ClipText.data()[i+1]))
						ClipText.erase(i, 1);

					if (i+1 == ClipText.size())
						ClipText.resize(i);
					else
						ClipText[i] = L' ';
				}
			}

			if (Flags.Check(FEDITLINE_CLEARFLAG))
			{
				LeftPos=0;
				Flags.Clear(FEDITLINE_CLEARFLAG);
				SetString(ClipText.data());
			}
			else
			{
				InsertString(ClipText.data());
			}

			Show();
			return TRUE;
		}
		case KEY_SHIFTTAB:
		{
			SetPrevCurPos(CurPos);
			int Pos = GetLineCursorPos();
			SetLineCursorPos(Pos-((Pos-1) % GetTabSize()+1));

			if (GetLineCursorPos()<0) SetLineCursorPos(0); //CursorPos=0,TabSize=1 case

			SetTabCurPos(GetLineCursorPos());
			Show();
			return TRUE;
		}
		case KEY_SHIFTSPACE:
			Key = KEY_SPACE;
		default:
		{
//      _D(SysLog(L"Key=0x%08X",Key));
			if (Key==KEY_NONE || Key==KEY_IDLE || Key==KEY_ENTER || Key==KEY_NUMENTER || Key>=65536)
				break;

			if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS))
			{
				if (PrevSelStart!=-1)
				{
					SelStart=PrevSelStart;
					SelEnd=PrevSelEnd;
				}
				DisableCallback();
				DeleteBlock();
				RevertCallback();
			}

			if (InsertKey(Key))
			{
				int CurWindowType = FrameManager->GetCurrentFrame()->GetType();
				if (CurWindowType == MODALTYPE_DIALOG || CurWindowType == MODALTYPE_PANELS)
				{
					Show();
				}
			}
			return TRUE;
		}
	}

	return FALSE;
}

// ��������� Ctrl-Q
int Edit::ProcessCtrlQ()
{
	INPUT_RECORD rec;
	for (;;)
	{
		DWORD Key=GetInputRecord(&rec);

		if (Key!=KEY_NONE && Key!=KEY_IDLE && rec.Event.KeyEvent.uChar.AsciiChar)
			break;

		if (Key==KEY_CONSOLE_BUFFER_RESIZE)
		{
//      int Dis=EditOutDisabled;
//      EditOutDisabled=0;
			Show();
//      EditOutDisabled=Dis;
		}
	}

	/*
	  EditOutDisabled++;
	  if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS))
	  {
	    DeleteBlock();
	  }
	  else
	    Flags.Clear(FEDITLINE_CLEARFLAG);
	  EditOutDisabled--;
	*/
	return InsertKey(rec.Event.KeyEvent.uChar.AsciiChar);
}

int Edit::ProcessInsPlainText(const wchar_t *str)
{
	if (*str)
	{
		InsertString(str);
		return TRUE;
	}

	return FALSE;
}

int Edit::InsertKey(int Key)
{
	bool changed=false;
	wchar_t *NewStr;

	if (Flags.Check(FEDITLINE_READONLY|FEDITLINE_DROPDOWNBOX))
		return (TRUE);

	if (Key==KEY_TAB && Flags.Check(FEDITLINE_OVERTYPE))
	{
		SetPrevCurPos(CurPos);
		int Pos = GetLineCursorPos();
		SetLineCursorPos(Pos + (GetTabSize() - (Pos % GetTabSize())));
		SetTabCurPos(GetLineCursorPos());
		return TRUE;
	}

	auto Mask = GetInputMask();
	if (!Mask.empty())
	{
		int MaskLen = static_cast<int>(Mask.size());

		if (CurPos<MaskLen)
		{
			if (KeyMatchedMask(Key, Mask))
			{
				if (!Flags.Check(FEDITLINE_OVERTYPE))
				{
					int i=MaskLen-1;

					while (!CheckCharMask(Mask[i]) && i>CurPos)
						i--;

					for (int j=i; i>CurPos; i--)
					{
						if (CheckCharMask(Mask[i]))
						{
							while (!CheckCharMask(Mask[j-1]))
							{
								if (j<=CurPos)
									break;

								j--;
							}

							Str[i]=Str[j-1];
							j--;
						}
					}
				}

				SetPrevCurPos(CurPos);
				Str[CurPos++]=Key;
				changed=true;
			}
			else
			{
				// ����� ������� ��� "����� ������ �� �����", �������� ��� SetAttr - ������ '.'
				;// char *Ptr=strchr(Mask+CurPos,Key);
			}
		}
		else if (CurPos<StrSize)
		{
			SetPrevCurPos(CurPos);
			Str[CurPos++]=Key;
			changed=true;
		}
	}
	else
	{
		if (GetMaxLength() == -1 || StrSize < GetMaxLength())
		{
			if (CurPos>=StrSize)
			{
				if (!(NewStr=(wchar_t *)xf_realloc(Str,(CurPos+2)*sizeof(wchar_t))))
					return FALSE;

				Str=NewStr;
				_snwprintf(&Str[StrSize],CurPos+2,L"%*s",CurPos-StrSize,L"");
				//memset(Str+StrSize,' ',CurPos-StrSize);Str[CurPos+1]=0;
				StrSize=CurPos+1;
			}
			else if (!Flags.Check(FEDITLINE_OVERTYPE))
				StrSize++;

			if (Key==KEY_TAB && (GetTabExpandMode()==EXPAND_NEWTABS || GetTabExpandMode()==EXPAND_ALLTABS))
			{
				StrSize--;
				InsertTab();
				return TRUE;
			}

			if (!(NewStr=(wchar_t *)xf_realloc(Str,(StrSize+1)*sizeof(wchar_t))))
				return TRUE;

			Str=NewStr;

			if (!Flags.Check(FEDITLINE_OVERTYPE))
			{
				wmemmove(Str+CurPos+1,Str+CurPos,StrSize-CurPos);

				if (SelStart!=-1)
				{
					if (SelEnd!=-1 && CurPos<SelEnd)
						SelEnd++;

					if (CurPos<SelStart)
						SelStart++;
				}
			}

			SetPrevCurPos(CurPos);
			Str[CurPos++]=Key;
			changed=true;
		}
		else if (Flags.Check(FEDITLINE_OVERTYPE))
		{
			if (CurPos < StrSize)
			{
				SetPrevCurPos(CurPos);
				Str[CurPos++]=Key;
				changed=true;
			}
		}
		else
			MessageBeep(MB_ICONHAND);
	}

	Str[StrSize]=0;

	if (changed) Changed();

	return TRUE;
}

void Edit::GetString(wchar_t *Str,int MaxSize)
{
	//xwcsncpy(Str, this->Str,MaxSize);
	wmemmove(Str,this->Str,std::min(StrSize,MaxSize-1));
	Str[std::min(StrSize,MaxSize-1)]=0;
	Str[MaxSize-1]=0;
}

void Edit::GetString(string &strStr)
{
	strStr = Str;
}

const wchar_t* Edit::GetStringAddr()
{
	return Str;
}

void  Edit::SetHiString(const string& Str)
{
	if (Flags.Check(FEDITLINE_READONLY))
		return;

	string NewStr;
	HiText2Str(NewStr, Str);
	Select(-1,0);
	SetBinaryString(NewStr.data(), static_cast<int>(NewStr.size()));
}

void Edit::SetString(const wchar_t *Str, int Length)
{
	if (Flags.Check(FEDITLINE_READONLY))
		return;

	Select(-1,0);
	SetBinaryString(Str,Length==-1? StrLength(Str) : Length);
}

void Edit::SetEOL(const wchar_t *EOL)
{
	EndType=EOL_NONE;

	if (EOL && *EOL)
	{
		if (EOL[0]==L'\r')
			if (EOL[1]==L'\n')
				EndType=EOL_CRLF;
			else if (EOL[1]==L'\r' && EOL[2]==L'\n')
				EndType=EOL_CRCRLF;
			else
				EndType=EOL_CR;
		else if (EOL[0]==L'\n')
			EndType=EOL_LF;
	}
}

const wchar_t *Edit::GetEOL()
{
	return EOL_TYPE_CHARS[EndType];
}

/* $ 25.07.2000 tran
   ����������:
   � ���� ������ DropDownBox �� ��������������
   ��� �� ���������� ������ �� SetString � �� ������ Editor
   � Dialog �� ����� �� ���������� */
void Edit::SetBinaryString(const wchar_t *Str,int Length)
{
	if (Flags.Check(FEDITLINE_READONLY))
		return;

	// ��������� ������������ �������, ���� ��������� GetMaxLength()
	if (GetMaxLength() != -1 && Length > GetMaxLength())
	{
		Length=GetMaxLength(); // ??
	}

	if (Length>0 && !Flags.Check(FEDITLINE_PARENT_SINGLELINE))
	{
		if (Str[Length-1]==L'\r')
		{
			EndType=EOL_CR;
			Length--;
		}
		else
		{
			if (Str[Length-1]==L'\n')
			{
				Length--;

				if (Length > 0 && Str[Length-1]==L'\r')
				{
					Length--;

					if (Length > 0 && Str[Length-1]==L'\r')
					{
						Length--;
						EndType=EOL_CRCRLF;
					}
					else
						EndType=EOL_CRLF;
				}
				else
					EndType=EOL_LF;
			}
			else
				EndType=EOL_NONE;
		}
	}

	CurPos=0;

	auto Mask = GetInputMask();
	if (!Mask.empty())
	{
		RefreshStrByMask(TRUE);
		int maskLen = static_cast<int>(Mask.size());

		for (int i=0,j=0; j<maskLen && j<Length;)
		{
			if (CheckCharMask(Mask[i]))
			{
				int goLoop=FALSE;

				if (KeyMatchedMask(Str[j], Mask))
					InsertKey(Str[j]);
				else
					goLoop=TRUE;

				j++;

				if (goLoop) continue;
			}
			else
			{
				SetPrevCurPos(CurPos);
				CurPos++;
			}

			i++;
		}

		/* ����� ���������� ������� (!*Str), �.�. ��� ������� ������
		   ������ �������� ����� ����� SetBinaryString("",0)
		   �.�. ����� ������� �� ���������� "�������������" ������ � ������
		*/
		RefreshStrByMask(!*Str);
	}
	else
	{
		wchar_t *NewStr=(wchar_t *)xf_realloc_nomove(this->Str,(Length+1)*sizeof(wchar_t));

		if (!NewStr)
			return;

		this->Str=NewStr;
		StrSize=Length;
		wmemcpy(this->Str,Str,Length);
		this->Str[Length]=0;

		if (GetTabExpandMode() == EXPAND_ALLTABS)
			ReplaceTabs();

		SetPrevCurPos(CurPos);
		CurPos=StrSize;
	}

	Changed();
}

void Edit::GetBinaryString(const wchar_t **Str,const wchar_t **EOL,intptr_t &Length)
{
	*Str=this->Str;

	if (EOL)
		*EOL=EOL_TYPE_CHARS[EndType];

	Length=StrSize; //???
}

int Edit::GetSelString(wchar_t *Str, int MaxSize)
{
	if (SelStart==-1 || (SelEnd!=-1 && SelEnd<=SelStart) ||
	        SelStart>=StrSize)
	{
		*Str=0;
		return FALSE;
	}

	int CopyLength;

	if (SelEnd==-1)
		CopyLength=MaxSize;
	else
		CopyLength=std::min(MaxSize,SelEnd-SelStart+1);

	xwcsncpy(Str,this->Str+SelStart,CopyLength);
	return TRUE;
}

int Edit::GetSelString(string &strStr, size_t MaxSize)
{
	if (SelStart==-1 || (SelEnd!=-1 && SelEnd<=SelStart) ||
	        SelStart>=StrSize)
	{
		strStr.clear();
		return FALSE;
	}

	size_t CopyLength;

	if (MaxSize == string::npos)
		MaxSize = StrSize;

	if (SelEnd==-1)
		CopyLength=MaxSize;
	else
		CopyLength=std::min(MaxSize, static_cast<size_t>(SelEnd-SelStart+1));

	strStr.assign(this->Str + SelStart, CopyLength);
	return TRUE;
}

void Edit::AppendString(const wchar_t *Str)
{
	int LastPos = CurPos;
	CurPos = GetLength();
	InsertString(Str);
	CurPos = LastPos;
}

void Edit::InsertString(const string& Str)
{
	if (Flags.Check(FEDITLINE_READONLY|FEDITLINE_DROPDOWNBOX))
		return;

	if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS))
		DeleteBlock();

	InsertBinaryString(Str.data(), static_cast<int>(Str.size()));
}

void Edit::InsertBinaryString(const wchar_t *Str,int Length)
{
	wchar_t *NewStr;

	if (Flags.Check(FEDITLINE_READONLY|FEDITLINE_DROPDOWNBOX))
		return;

	Flags.Clear(FEDITLINE_CLEARFLAG);

	auto Mask = GetInputMask();
	if (!Mask.empty())
	{
		int Pos=CurPos;
		int MaskLen = static_cast<int>(Mask.size());

		if (Pos<MaskLen)
		{
			//_SVS(SysLog(L"InsertBinaryString ==> Str='%s' (Length=%d) Mask='%s'",Str,Length,Mask+Pos));
			int StrLen=(MaskLen-Pos>Length)?Length:MaskLen-Pos;

			/* $ 15.11.2000 KM
			   ������� ����������� ��� ���������� ������ PasteFromClipboard
			   � ������ � ������
			*/
			for (int i=Pos,j=0; j<StrLen+Pos;)
			{
				if (CheckCharMask(Mask[i]))
				{
					int goLoop=FALSE;

					if (j < Length && KeyMatchedMask(Str[j], Mask))
					{
						InsertKey(Str[j]);
						//_SVS(SysLog(L"InsertBinaryString ==> InsertKey(Str[%d]='%c');",j,Str[j]));
					}
					else
						goLoop=TRUE;

					j++;

					if (goLoop) continue;
				}
				else
				{
					if(Mask[j] == Str[j])
					{
						j++;
					}
					SetPrevCurPos(CurPos);
					CurPos++;
				}

				i++;
			}
		}

		RefreshStrByMask();
		//_SVS(SysLog(L"InsertBinaryString ==> this->Str='%s'",this->Str));
	}
	else
	{
		if (GetMaxLength() != -1 && StrSize+Length > GetMaxLength())
		{
			// ��������� ������������ �������, ���� ��������� GetMaxLength()
			if (StrSize < GetMaxLength())
			{
				Length=GetMaxLength()-StrSize;
			}
		}

		if (GetMaxLength() == -1 || StrSize+Length <= GetMaxLength())
		{
			if (CurPos>StrSize)
			{
				if (!(NewStr=(wchar_t *)xf_realloc(this->Str,(CurPos+1)*sizeof(wchar_t))))
					return;

				this->Str=NewStr;
				_snwprintf(&this->Str[StrSize],CurPos+1,L"%*s",CurPos-StrSize,L"");
				//memset(this->Str+StrSize,' ',CurPos-StrSize);this->Str[CurPos+1]=0;
				StrSize=CurPos;
			}

			int TmpSize=StrSize-CurPos;
			wchar_t_ptr TmpStr(TmpSize + 16);

			if (!TmpStr)
				return;

			wmemcpy(TmpStr.get(), &this->Str[CurPos], TmpSize);
			StrSize+=Length;

			if (!(NewStr=(wchar_t *)xf_realloc(this->Str,(StrSize+1)*sizeof(wchar_t))))
			{
				return;
			}

			this->Str=NewStr;
			wmemcpy(&this->Str[CurPos],Str,Length);
			SetPrevCurPos(CurPos);
			CurPos+=Length;
			wmemcpy(this->Str + CurPos, TmpStr.get(), TmpSize);
			this->Str[StrSize]=0;

			if (GetTabExpandMode() == EXPAND_ALLTABS)
				ReplaceTabs();

			Changed();
		}
		else
			MessageBeep(MB_ICONHAND);
	}
}

int Edit::GetLength()
{
	return(StrSize);
}

int Edit::ProcessMouse(const MOUSE_EVENT_RECORD *MouseEvent)
{
	if (!(MouseEvent->dwButtonState & 3))
		return FALSE;

	if (MouseEvent->dwMousePosition.X<X1 || MouseEvent->dwMousePosition.X>X2 ||
	        MouseEvent->dwMousePosition.Y!=Y1)
		return FALSE;

	//SetClearFlag(0); // ����� ������ ��� ��������� � ������ �����-������?
	SetTabCurPos(MouseEvent->dwMousePosition.X - X1 + LeftPos);

	if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS))
		Select(-1,0);

	if (MouseEvent->dwButtonState&FROM_LEFT_1ST_BUTTON_PRESSED)
	{
		static int PrevDoubleClick=0;
		static COORD PrevPosition={};

		if (GetTickCount()-PrevDoubleClick<=GetDoubleClickTime() && MouseEvent->dwEventFlags!=MOUSE_MOVED &&
		        PrevPosition.X == MouseEvent->dwMousePosition.X && PrevPosition.Y == MouseEvent->dwMousePosition.Y)
		{
			Select(0,StrSize);
			PrevDoubleClick=0;
			PrevPosition.X=0;
			PrevPosition.Y=0;
		}

		if (MouseEvent->dwEventFlags==DOUBLE_CLICK)
		{
			ProcessKey(KEY_OP_SELWORD);
			PrevDoubleClick=GetTickCount();
			PrevPosition=MouseEvent->dwMousePosition;
		}
		else
		{
			PrevDoubleClick=0;
			PrevPosition.X=0;
			PrevPosition.Y=0;
		}
	}

	Show();
	return TRUE;
}

/* $ 03.08.2000 KM
   ������� ������� �������� ��-�� �������������
   ���������� ������ ����� ����.
*/
int Edit::Search(const string& Str,const string &UpperStr, const string &LowerStr, RegExp &re, RegExpMatch *pm,string& ReplaceStr,int Position,int Case,int WholeWords,int Reverse,int Regexp,int PreserveStyle, int *SearchLength)
{
	return SearchString(this->Str,this->StrSize,Str,UpperStr,LowerStr,re,pm,ReplaceStr,CurPos,Position,Case,WholeWords,Reverse,Regexp,PreserveStyle,SearchLength,WordDiv().data());
}

void Edit::InsertTab()
{
	wchar_t *TabPtr;
	int Pos,S;

	if (Flags.Check(FEDITLINE_READONLY))
		return;

	Pos=CurPos;
	S=GetTabSize()-(Pos % GetTabSize());

	if (SelStart!=-1)
	{
		if (Pos<=SelStart)
		{
			SelStart+=S-(Pos==SelStart?0:1);
		}

		if (SelEnd!=-1 && Pos<SelEnd)
		{
			SelEnd+=S;
		}
	}

	int PrevStrSize=StrSize;
	StrSize+=S;
	CurPos+=S;
	Str=(wchar_t *)xf_realloc(Str,(StrSize+1)*sizeof(wchar_t));
	TabPtr=Str+Pos;
	wmemmove(TabPtr+S,TabPtr,PrevStrSize-Pos);
	wmemset(TabPtr,L' ',S);
	Str[StrSize]=0;
	Changed();
}

bool Edit::ReplaceTabs()
{
	wchar_t *TabPtr;
	int Pos=0;

	if (Flags.Check(FEDITLINE_READONLY))
		return false;

	bool changed=false;

	while ((TabPtr = wmemchr(Str+Pos, L'\t', StrSize-Pos)))
	{
		changed=true;
		Pos=(int)(TabPtr-Str);
		int S=GetTabSize()-((int)(TabPtr-Str) % GetTabSize());

		if (SelStart!=-1)
		{
			if (Pos<=SelStart)
			{
				SelStart+=S-(Pos==SelStart?0:1);
			}

			if (SelEnd!=-1 && Pos<SelEnd)
			{
				SelEnd+=S-1;
			}
		}

		int PrevStrSize=StrSize;
		StrSize+=S-1;

		if (CurPos>Pos)
			CurPos+=S-1;

		Str=(wchar_t *)xf_realloc(Str,(StrSize+1)*sizeof(wchar_t));
		TabPtr=Str+Pos;
		wmemmove(TabPtr+S,TabPtr+1,PrevStrSize-Pos);
		wmemset(TabPtr,L' ',S);
		Str[StrSize]=0;
	}

	if (changed) Changed();
	return changed;
}

int Edit::GetTabCurPos()
{
	return(RealPosToTab(CurPos));
}

void Edit::SetTabCurPos(int NewPos)
{
	auto Mask = GetInputMask();
	if (!Mask.empty())
	{
		wchar_t_ptr ShortStr(StrSize + 1);

		if (!ShortStr)
			return;

		xwcsncpy(ShortStr.get(), Str, StrSize + 1);
		int Pos=StrLength(RemoveTrailingSpaces(ShortStr.get()));

		if (NewPos>Pos)
			NewPos=Pos;
	}

	CurPos=TabPosToReal(NewPos);
}

int Edit::RealPosToTab(int Pos)
{
	return RealPosToTab(0, 0, Pos, nullptr);
}

int Edit::RealPosToTab(int PrevLength, int PrevPos, int Pos, int* CorrectPos)
{
	// ������������� �����
	bool bCorrectPos = CorrectPos && *CorrectPos;

	if (CorrectPos)
		*CorrectPos = 0;

	// ���� � ��� ��� ���� ������������� � �������, �� ������ ��������� ����������
	if (GetTabExpandMode() == EXPAND_ALLTABS)
		return PrevLength+Pos-PrevPos;

	// ������������� �������������� ����� ���������� ���������
	int TabPos = PrevLength;

	// ���� ���������� ������� �� ������ ������, �� ����� ��� ����� ��� �
	// ��������� ����� ������ �� ����, ����� ���������� ����������
	if (PrevPos >= StrSize)
		TabPos += Pos-PrevPos;
	else
	{
		// �������� ���������� � ���������� �������
		int Index = PrevPos;

		// �������� �� ���� �������� �� ������� ������, ���� ��� ��� � �������� ������,
		// ���� �� ����� ������, ���� ������� ������ �� ��������� ������
		for (; Index < std::min(Pos, StrSize); Index++)

			// ������������ ����
			if (Str[Index] == L'\t')
			{
				// ���� ���� ������������� ������ ������������� ����� � ��� ������������
				// ��� �� �����������, �� ����������� ����� �������������� ������ �� �������
				if (bCorrectPos)
				{
					++Pos;
					*CorrectPos = 1;
					bCorrectPos = false;
				}

				// ����������� ����� ���� � ������ �������� � ������� ������� � ������
				TabPos += GetTabSize()-(TabPos%GetTabSize());
			}
		// ������������ ��� ��������� �������
			else
				TabPos++;

		// ���� ������� ��������� �� ��������� ������, �� ��� ����� ��� ����� � �� ������
		if (Pos >= StrSize)
			TabPos += Pos-Index;
	}

	return TabPos;
}

int Edit::TabPosToReal(int Pos)
{
	if (GetTabExpandMode() == EXPAND_ALLTABS)
		return Pos;

	int Index = 0;

	for (int TabPos = 0; TabPos < Pos; Index++)
	{
		if (Index > StrSize)
		{
			Index += Pos-TabPos;
			break;
		}

		if (Str[Index] == L'\t')
		{
			int NewTabPos = TabPos+GetTabSize()-(TabPos%GetTabSize());

			if (NewTabPos > Pos)
				break;

			TabPos = NewTabPos;
		}
		else
		{
			TabPos++;
		}
	}

	return Index;
}

void Edit::Select(int Start,int End)
{
	SelStart=Start;
	SelEnd=End;

	/* $ 24.06.2002 SKV
	   ���� ������ ��������� �� ������ ������, ���� ��������� �����.
	   17.09.2002 ��������� �������. ���������.
	*/
	if (SelEnd<SelStart && SelEnd!=-1)
	{
		SelStart=-1;
		SelEnd=0;
	}

	if (SelStart==-1 && SelEnd==-1)
	{
		SelStart=-1;
		SelEnd=0;
	}

//  if (SelEnd>StrSize)
//    SelEnd=StrSize;
}

void Edit::AddSelect(int Start,int End)
{
	if (Start<SelStart || SelStart==-1)
		SelStart=Start;

	if (End==-1 || (End>SelEnd && SelEnd!=-1))
		SelEnd=End;

	if (SelEnd>StrSize)
		SelEnd=StrSize;

	if (SelEnd<SelStart && SelEnd!=-1)
	{
		SelStart=-1;
		SelEnd=0;
	}
}

void Edit::GetSelection(intptr_t &Start,intptr_t &End)
{
	/* $ 17.09.2002 SKV
	  ���� ����, ��� ��� ��������� ������ OO design'�,
	  ��� ��� ��� � �������� �����.
	*/
	/*  if (SelEnd>StrSize+1)
	    SelEnd=StrSize+1;
	  if (SelStart>StrSize+1)
	    SelStart=StrSize+1;*/
	/* SKV $ */
	Start=SelStart;
	End=SelEnd;

	if (End>StrSize)
		End=-1;//StrSize;

	if (Start>StrSize)
		Start=StrSize;
}

void Edit::GetRealSelection(intptr_t &Start,intptr_t &End)
{
	Start=SelStart;
	End=SelEnd;
}

void Edit::DeleteBlock()
{
	if (Flags.Check(FEDITLINE_READONLY|FEDITLINE_DROPDOWNBOX))
		return;

	if (SelStart==-1 || SelStart>=SelEnd)
		return;

	SetPrevCurPos(CurPos);

	auto Mask = GetInputMask();
	if (!Mask.empty())
	{
		for (int i=SelStart; i<SelEnd; i++)
		{
			if (CheckCharMask(Mask[i]))
				Str[i]=L' ';
		}

		CurPos=SelStart;
	}
	else
	{
		int From=SelStart,To=SelEnd;

		if (From>StrSize)From=StrSize;

		if (To>StrSize)To=StrSize;

		wmemmove(Str+From,Str+To,StrSize-To+1);
		StrSize-=To-From;

		if (CurPos>From)
		{
			if (CurPos<To)
				CurPos=From;
			else
				CurPos-=To-From;
		}

		Str=(wchar_t *)xf_realloc(Str,(StrSize+1)*sizeof(wchar_t));
	}

	SelStart=-1;
	SelEnd=0;
	Flags.Clear(FEDITLINE_MARKINGBLOCK);

	// OT: �������� �� ������������ �������� ������ ��� �������� � �������
	if (Flags.Check((FEDITLINE_PARENT_SINGLELINE|FEDITLINE_PARENT_MULTILINE)))
	{
		if (LeftPos>CurPos)
			LeftPos=CurPos;
	}

	Changed(true);
}

void Edit::AddColor(ColorItem *col,bool skipsort)
{
	if (ColorCount==MaxColorCount)
	{
		if (ColorCount<256)
		{
			if (!(ColorCount & 15))
			{
				MaxColorCount=ColorCount+16;
				ColorList=(ColorItem *)xf_realloc(ColorList,(MaxColorCount)*sizeof(*ColorList));
			}
		}
		else if (ColorCount<2048)
		{
			if (!(ColorCount & 255))
			{
				MaxColorCount=ColorCount+256;
				ColorList=(ColorItem *)xf_realloc(ColorList,(MaxColorCount)*sizeof(*ColorList));
			}
		}
		else if (ColorCount<65536)
		{
			if (!(ColorCount & 2047))
			{
				MaxColorCount=ColorCount+2048;
				ColorList=(ColorItem *)xf_realloc(ColorList,(MaxColorCount)*sizeof(*ColorList));
			}
		}
		else if (!(ColorCount & 65535))
		{
			MaxColorCount=ColorCount+65536;
			ColorList=(ColorItem *)xf_realloc(ColorList,(MaxColorCount)*sizeof(*ColorList));
		}
	}

	#ifdef _DEBUG
	//_ASSERTE(ColorCount<MaxColorCount);
	#endif

	if (skipsort && !ColorListFlags.Check(ECLF_NEEDSORT) && ColorCount && ColorList[ColorCount-1].Priority>col->Priority)
		ColorListFlags.Set(ECLF_NEEDSORT);


	ColorList[ColorCount++]=*col;

	if (!skipsort)
	{
		std::stable_sort(ColorList, ColorList + ColorCount);
	}
}

void Edit::SortColorUnlocked()
{
	if (ColorListFlags.Check(ECLF_NEEDFREE))
	{
		ColorListFlags.Clear(ECLF_NEEDFREE);
		if (!ColorCount)
		{
			xf_free(ColorList);
			ColorList=nullptr;
			MaxColorCount = 0;
		}
	}

	if (ColorListFlags.Check(ECLF_NEEDSORT))
	{
		ColorListFlags.Clear(ECLF_NEEDSORT);
		std::stable_sort(ColorList, ColorList + ColorCount);
	}
}

int Edit::DeleteColor(int ColorPos,const GUID& Owner,bool skipfree)
{
	int Src;

	if (!ColorCount)
		return FALSE;

	int Dest=0;

	if (ColorPos!=-1)
	{
		for (Src=0; Src<ColorCount; Src++)
		{
			if ((ColorList[Src].StartPos!=ColorPos) || (Owner != ColorList[Src].GetOwner()))
			{
				if (Dest!=Src)
					ColorList[Dest]=ColorList[Src];

				Dest++;
			}
		}
	}
	else
	{
		for (Src=0; Src<ColorCount; Src++)
		{
			if (Owner != ColorList[Src].GetOwner())
			{
				if (Dest!=Src)
					ColorList[Dest]=ColorList[Src];

				Dest++;
			}
		}
	}

	int DelCount=ColorCount-Dest;
	ColorCount=Dest;

	if (!ColorCount)
	{
		if (skipfree)
		{
			ColorListFlags.Clear(ECLF_NEEDFREE);
		}
		else
		{
			xf_free(ColorList);
			ColorList=nullptr;
			MaxColorCount = 0;
		}
	}

	return(DelCount);
}

int Edit::GetColor(ColorItem *col,int Item)
{
	if (Item >= ColorCount)
		return FALSE;

	*col=ColorList[Item];
	return TRUE;
}

void Edit::ApplyColor(const FarColor& SelColor)
{
	// ��� ����������� ��������� ����������� ������� ����� ���������� �����
	int Pos = INT_MIN, TabPos = INT_MIN, TabEditorPos = INT_MIN;

	int XPos = 0;
	if(Flags.Check(FEDITLINE_EDITORMODE))
	{
		EditorInfo ei={sizeof(EditorInfo)};
		Global->CtrlObject->Plugins->GetCurEditor()->EditorControl(ECTL_GETINFO, 0, &ei);
		XPos = ei.CurTabPos - ei.LeftPos;
	}

	// ������������ �������� ��������
	for (int Col = 0; Col < ColorCount; Col++)
	{
		ColorItem *CurItem = ColorList+Col;

		// ���������� �������� � ������� ������ ������ �����
		if (CurItem->StartPos > CurItem->EndPos)
			continue;

		// �������� �������� �������� �� ���������� �� �����
		if (CurItem->StartPos-LeftPos > X2 && CurItem->EndPos-LeftPos < X1)
			continue;

		int Length = CurItem->EndPos-CurItem->StartPos+1;

		if (CurItem->StartPos+Length >= StrSize)
			Length = StrSize-CurItem->StartPos;

		// �������� ��������� �������
		int RealStart, Start;

		// ���� ���������� ������� ����� �������, �� ������ �� ���������
		// � ����� ���� ����� ����������� ��������
		if (Pos == CurItem->StartPos)
		{
			RealStart = TabPos;
			Start = TabEditorPos;
		}
		// ���� ���������� ��� ������ ��� ��� ���������� ������� ������ �������,
		// �� ���������� ���������� � ������ ������
		else if (Pos == INT_MIN || CurItem->StartPos < Pos)
		{
			RealStart = RealPosToTab(CurItem->StartPos);
			Start = RealStart-LeftPos;
		}
		// ��� ������������ ������ ���������� ������������ ���������� �������
		else
		{
			RealStart = RealPosToTab(TabPos, Pos, CurItem->StartPos, nullptr);
			Start = RealStart-LeftPos;
		}

		// ���������� ����������� �������� ��� �� ����������� ���������� �������������
		Pos = CurItem->StartPos;
		TabPos = RealStart;
		TabEditorPos = Start;

		// ���������� �������� ��������� � ������� ��������� ������� �� �������
		if (Start > X2)
			continue;

		// ������������� ������������ ����� (�����������, ���� ���������� ���� ECF_TABMARKFIRST)
		int CorrectPos = CurItem->Flags & ECF_TABMARKFIRST ? 0 : 1;

		// �������� �������� �������
		int EndPos = CurItem->EndPos;
		int RealEnd, End;

		bool TabMarkCurrent=false;

		// ������������ ������, ����� ���������� ������� ����� �������, �� ����
		// ����� �������������� ������� ����� 1
		if (Pos == EndPos)
		{
			// ���� ���������� ������ ������������ ������������ ����� � ������������
			// ������ ������ -- ��� ���, �� ������ ������ � ����� �������������,
			// ����� ������ �� ��������� � ���� ������ ��������
			if (CorrectPos && EndPos < StrSize && Str[EndPos] == L'\t')
			{
				RealEnd = RealPosToTab(TabPos, Pos, ++EndPos, nullptr);
				End = RealEnd-LeftPos;
				TabMarkCurrent = (CurItem->Flags & ECF_TABMARKCURRENT) && XPos>=Start && XPos<End;
			}
			else
			{
				RealEnd = TabPos;
				CorrectPos = 0;
				End = TabEditorPos;
			}
		}
		// ���� ���������� ������� ������ �������, �� ���������� ����������
		// � ������ ������ (� ������ ������������� ������������ �����)
		else if (EndPos < Pos)
		{
			// TODO: �������� ��� �� ����� ��������� � ������ ����� (�� ������� Mantis#0001718)
			RealEnd = RealPosToTab(0, 0, EndPos, &CorrectPos);
			EndPos += CorrectPos;
			End = RealEnd-LeftPos;
		}
		// ��� ������������ ������ ���������� ������������ ���������� ������� (� ������
		// ������������� ������������ �����)
		else
		{
			// Mantis#0001718: ���������� ECF_TABMARKFIRST �� ������ ��������� ������������
			// ��������� � ������ ���������� ����
			if (CorrectPos && EndPos < StrSize && Str[EndPos] == L'\t')
				RealEnd = RealPosToTab(TabPos, Pos, ++EndPos, nullptr);
			else
			{
				RealEnd = RealPosToTab(TabPos, Pos, EndPos, &CorrectPos);
				EndPos += CorrectPos;
			}
			End = RealEnd-LeftPos;
		}

		// ���������� ����������� �������� ��� �� ����������� ���������� �������������
		Pos = EndPos;
		TabPos = RealEnd;
		TabEditorPos = End;

		// ���������� �������� ��������� � ������� �������� ������� ������ ����� ������� ������
		if (End < X1)
			continue;

		// �������� ��������� �������� �� ������
		if (Start < X1)
			Start = X1;

		if (End > X2)
			End = X2;

		if(TabMarkCurrent)
		{
			Start = XPos;
			End = XPos+1;
		}

		// ������������� ����� ��������������� ��������
		Length = End-Start+1;

		if (Length < X2)
			Length -= CorrectPos;

		// ������������ �������, ���� ���� ��� ������������
		if (Length > 0)
		{
			Global->ScrBuf->ApplyColor(
			    Start,
			    Y1,
			    Start+Length-1,
			    Y1,
			    CurItem->GetColor(),
			    // �� ������������ ���������
			    SelColor,
			    true
			);
		}
	}
}

/* $ 24.09.2000 SVS $
  ������� Xlat - ������������� �� �������� QWERTY <-> ������
*/
void Edit::Xlat(bool All)
{
	//   ��� CmdLine - ���� ��� ���������, ����������� ��� ������
	if (All && SelStart == -1 && !SelEnd)
	{
		::Xlat(Str,0,StrLength(Str),Global->Opt->XLat.Flags);
		Changed();
		Show();
		return;
	}

	if (SelStart != -1 && SelStart != SelEnd)
	{
		if (SelEnd == -1)
			SelEnd=StrLength(Str);

		::Xlat(Str,SelStart,SelEnd,Global->Opt->XLat.Flags);
		Changed();
		Show();
	}
	/* $ 25.11.2000 IS
	 ���� ��� ���������, �� ���������� ������� �����. ����� ������������ ��
	 ������ ����������� ������ ������������.
	*/
	else
	{
		/* $ 10.12.2000 IS
		   ������������ ������ �� �����, �� ������� ����� ������, ��� �� �����, ���
		   ��������� ����� ������� ������� �� 1 ������
		*/
		int start=CurPos, StrSize=StrLength(Str);
		bool DoXlat=true;

		if (IsWordDiv(Global->Opt->XLat.strWordDivForXlat,Str[start]))
		{
			if (start) start--;

			DoXlat=(!IsWordDiv(Global->Opt->XLat.strWordDivForXlat,Str[start]));
		}

		if (DoXlat)
		{
			while (start>=0 && !IsWordDiv(Global->Opt->XLat.strWordDivForXlat,Str[start]))
				start--;

			start++;
			int end=start+1;

			while (end<StrSize && !IsWordDiv(Global->Opt->XLat.strWordDivForXlat,Str[end]))
				end++;

			::Xlat(Str,start,end,Global->Opt->XLat.Flags);
			Changed();
			Show();
		}
	}
}

/* $ 15.11.2000 KM
   ���������: �������� �� ������ � �����������
   �������� ��������, ������������ ������
*/
int Edit::KeyMatchedMask(int Key, const string& Mask)
{
	int Inserted=FALSE;

	if (Mask[CurPos]==EDMASK_ANY)
		Inserted=TRUE;
	else if (Mask[CurPos]==EDMASK_DSS && (iswdigit(Key) || Key==L' ' || Key==L'-'))
		Inserted=TRUE;
	else if (Mask[CurPos]==EDMASK_DIGITS && (iswdigit(Key) || Key==L' '))
		Inserted=TRUE;
	else if (Mask[CurPos]==EDMASK_DIGIT && (iswdigit(Key)))
		Inserted=TRUE;
	else if (Mask[CurPos]==EDMASK_ALPHA && IsAlpha(Key))
		Inserted=TRUE;
	else if (Mask[CurPos]==EDMASK_HEX && (iswdigit(Key) || (Upper(Key)>=L'A' && Upper(Key)<=L'F') || (Upper(Key)>=L'a' && Upper(Key)<=L'f')))
		Inserted=TRUE;

	return Inserted;
}

int Edit::CheckCharMask(wchar_t Chr)
{
	return Chr==EDMASK_ANY || Chr==EDMASK_DIGIT || Chr==EDMASK_DIGITS || Chr==EDMASK_DSS || Chr==EDMASK_ALPHA || Chr==EDMASK_HEX;
}

void Edit::SetDialogParent(DWORD Sets)
{
	if ((Sets&(FEDITLINE_PARENT_SINGLELINE|FEDITLINE_PARENT_MULTILINE)) == (FEDITLINE_PARENT_SINGLELINE|FEDITLINE_PARENT_MULTILINE) ||
	        !(Sets&(FEDITLINE_PARENT_SINGLELINE|FEDITLINE_PARENT_MULTILINE)))
		Flags.Clear(FEDITLINE_PARENT_SINGLELINE|FEDITLINE_PARENT_MULTILINE);
	else if (Sets&FEDITLINE_PARENT_SINGLELINE)
	{
		Flags.Clear(FEDITLINE_PARENT_MULTILINE);
		Flags.Set(FEDITLINE_PARENT_SINGLELINE);
	}
	else if (Sets&FEDITLINE_PARENT_MULTILINE)
	{
		Flags.Clear(FEDITLINE_PARENT_SINGLELINE);
		Flags.Set(FEDITLINE_PARENT_MULTILINE);
	}
}

void Edit::FixLeftPos(int TabCurPos)
{
	if (TabCurPos<0) TabCurPos=GetTabCurPos(); //�����������, ����� ��� ���� �� ������
	if (TabCurPos-LeftPos>ObjWidth()-1)
		LeftPos=TabCurPos-ObjWidth()+1;

	if (TabCurPos<LeftPos)
		LeftPos=TabCurPos;
}

const FarColor& Edit::GetNormalColor() const
{
	return static_cast<Editor*>(GetOwner())->GetNormalColor();
}

const FarColor& Edit::GetSelectedColor() const
{
	return static_cast<Editor*>(GetOwner())->GetSelectedColor();
}

const FarColor& Edit::GetUnchangedColor() const
{
	return static_cast<Editor*>(GetOwner())->GetNormalColor();
}

const int Edit::GetTabSize() const
{
	return static_cast<Editor*>(GetOwner())->GetTabSize();
}

const EXPAND_TABS Edit::GetTabExpandMode() const
{
	return static_cast<Editor*>(GetOwner())->GetConvertTabs();
}

const string& Edit::WordDiv() const
{
	return static_cast<Editor*>(GetOwner())->GetWordDiv();
}

int Edit::GetCursorSize()
{
	return -1;
}

int Edit::GetMacroSelectionStart() const
{
	return static_cast<Editor*>(GetOwner())->GetMacroSelectionStart();
}

void Edit::SetMacroSelectionStart(int Value)
{
	static_cast<Editor*>(GetOwner())->SetMacroSelectionStart(Value);
}

int Edit::GetLineCursorPos() const
{
	return static_cast<Editor*>(GetOwner())->GetLineCursorPos();
}

void Edit::SetLineCursorPos(int Value)
{
	return static_cast<Editor*>(GetOwner())->SetLineCursorPos(Value);
}
