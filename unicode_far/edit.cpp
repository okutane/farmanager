/*
edit.cpp

���������� ��������� ������ ��������������
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

#include "edit.hpp"
#include "global.hpp"
#include "fn.hpp"
#include "plugin.hpp"
#include "macroopcode.hpp"
#include "keys.hpp"
#include "editor.hpp"
#include "ctrlobj.hpp"
#include "filepanels.hpp"
#include "filelist.hpp"
#include "panel.hpp"
#include "scrbuf.hpp"

static int EditEncodeDisabled=0;
static int Recurse=0;

enum {EOL_NONE,EOL_CR,EOL_LF,EOL_CRLF,EOL_CRCRLF};
static const wchar_t *EOL_TYPE_CHARS[]={L"",L"\r",L"\n",L"\r\n",L"\r\r\n"};

#define EDMASK_ANY   L'X' // ��������� ������� � ������ ����� ����� ������;
#define EDMASK_DSS   L'#' // ��������� ������� � ������ ����� �����, ������ � ���� ������;
#define EDMASK_DIGIT L'9' // ��������� ������� � ������ ����� ������ �����;
#define EDMASK_ALPHA L'A' // ��������� ������� � ������ ����� ������ �����.
#define EDMASK_HEX   L'H' // ��������� ������� � ������ ����� ����������������� �������.


Edit::Edit(ScreenObject *pOwner)
{
	SetOwner (pOwner);

	m_next = NULL;
	m_prev = NULL;

	Str=(wchar_t*) xf_malloc(sizeof(wchar_t));
	StrSize=0;

	WordDiv=Opt.strWordDiv;

	*Str=0;

	Mask=NULL;
	PrevCurPos=0;

	CurPos=0;
	CursorPos=0;
	CursorSize=-1;
	TableSet=NULL;
	LeftPos=0;
	MaxLength=-1;
	SelStart=-1;
	SelEnd=0;
	Flags.Set(FEDITLINE_EDITBEYONDEND);
	Color=F_LIGHTGRAY|B_BLACK;
	SelColor=F_WHITE|B_BLACK;

	ColorUnChanged=COL_DIALOGEDITUNCHANGED;

	EndType=EOL_NONE;
	ColorList=NULL;
	ColorCount=0;

	TabSize=Opt.EdOpt.TabSize;

	TabExpandMode = EXPAND_NOTABS;

	Flags.Change(FEDITLINE_DELREMOVESBLOCKS,Opt.EdOpt.DelRemovesBlocks);
	Flags.Change(FEDITLINE_PERSISTENTBLOCKS,Opt.EdOpt.PersistentBlocks);

	m_codepage = 0; //BUGBUG
}


Edit::~Edit()
{
  if (ColorList)
    xf_free (ColorList);
  if (Mask)
    xf_free(Mask);
  if(Str)
    xf_free(Str);
}


void Edit::SetCodePage (int codepage)
{
	if ( codepage != m_codepage )
    {
    	//m_codepage = codepage;

    	int length = WideCharToMultiByte (m_codepage, 0, Str, StrSize, NULL, 0, NULL, NULL);

        char *decoded = (char*)xf_malloc (length);

    	if ( !decoded )
    		return;

    	WideCharToMultiByte (m_codepage, 0, Str, StrSize, decoded, length, NULL, NULL);

    	int length2 = MultiByteToWideChar (codepage, 0, decoded, length, NULL, 0);

    	wchar_t *encoded = (wchar_t*)xf_malloc ((length2+1)*sizeof (wchar_t));

    	if ( !encoded )
        {
        	xf_free (decoded);
        	return;
        }

        memset (encoded, 0, (length2+1)*sizeof (wchar_t));

		MultiByteToWideChar (codepage, 0, decoded, length, encoded, length2);

		xf_free (decoded);
		xf_free (Str);

		Str = encoded;
		StrSize = length2;

		m_codepage = codepage;
    }
}

int Edit::GetCodePage ()
{
	return m_codepage;
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
  int Value=(PrevCurPos>CurPos)?-1:1;
  CurPos=GetNextCursorPos(CurPos,Value);

  FastShow();

  /*
     - ������ ����� �*�� � ������ ������� w9x ��� ��� �������
       ��������� ������� ���� ������ ���������� "��������"
       ������� ���.
  */
  if (WinVer.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS)
  {
    ::SetCursorType(TRUE,99);
    ::SetCursorType(TRUE,CursorSize);
  }

  /* $ 26.07.2000 tran
     ��� DropDownBox ������ ���������
     �� ���� ���� - ���������� �� �� ����� ������� ����� */
  if (Flags.Check(FEDITLINE_DROPDOWNBOX))
    ::SetCursorType(0,10);
  else
  {
    if (Flags.Check(FEDITLINE_OVERTYPE))
    {
      int NewCursorSize=IsWindowed()?
       (Opt.CursorSize[2]?Opt.CursorSize[2]:99):
       (Opt.CursorSize[3]?Opt.CursorSize[3]:99);
      ::SetCursorType(1,CursorSize==-1?NewCursorSize:CursorSize);
    }
    else
      ::SetCursorType(1,CursorSize);
  }
  MoveCursor(X1+CursorPos-LeftPos,Y1);
}


void Edit::SetCursorType(int Visible,int Size)
{
  Flags.Change(FEDITLINE_CURSORVISIBLE,Visible);
  CursorSize=Size;
  ::SetCursorType(Visible,Size);
}

void Edit::GetCursorType(int &Visible,int &Size)
{
  Visible=Flags.Check(FEDITLINE_CURSORVISIBLE);
  Size=CursorSize;
}

//   ���������� ������ ��������� ������� � ������ � ������ Mask.
int Edit::GetNextCursorPos(int Position,int Where)
{
  int Result=Position;

  if (Mask && *Mask && (Where==-1 || Where==1))
  {
    int i;
    int PosChanged=FALSE;
    int MaskLen=StrLength(Mask);
    for (i=Position;i<MaskLen && i>=0;i+=Where)
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
      for (i=Position;i>=0;i--)
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
      for (i=Position;i<MaskLen;i++)
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
  int EditLength=ObjWidth;
  if (!Flags.Check(FEDITLINE_EDITBEYONDEND) && CurPos>StrSize && StrSize>=0)
    CurPos=StrSize;
  if (MaxLength!=-1)
  {
    if (StrSize>MaxLength)
    {
      Str[MaxLength]=0;
      StrSize=MaxLength;
    }
    if (CurPos>MaxLength-1)
      CurPos=MaxLength>0 ? (MaxLength-1):0;
  }
  int TabCurPos=GetTabCurPos();
  /* $ 31.07.2001 KM
    ! ��� ���������� ������� ����������� ������
      � ������ �������.
  */
  if (!Flags.Check(FEDITLINE_DROPDOWNBOX))
  {
    if (TabCurPos-LeftPos>EditLength-1)
      LeftPos=TabCurPos-EditLength+1;
    if (TabCurPos<LeftPos)
      LeftPos=TabCurPos;
  }
  GotoXY(X1,Y1);
  int TabSelStart=(SelStart==-1) ? -1:RealPosToTab(SelStart);
  int TabSelEnd=(SelEnd<0) ? -1:RealPosToTab(SelEnd);

  /* $ 17.08.2000 KM
     ���� ���� �����, ������� ���������� ������, �� ����
     ��� "����������" ������� � �����, �� ���������� ����������
     ������ ��������� �������������� � Str
  */
  if (Mask && *Mask)
    RefreshStrByMask();

  wchar_t *OutStrTmp=new wchar_t[EditLength+1];
  if (!OutStrTmp)
    return;
  wchar_t *OutStr=new wchar_t[EditLength+1];
  if (!OutStr)
  {
    delete[] OutStrTmp;
    return;
  }

  CursorPos=TabCurPos;
  if (TabCurPos-LeftPos>EditLength-1)
    LeftPos=TabCurPos-EditLength+1;
  int RealLeftPos=TabPosToReal(LeftPos);
  int OutStrLength=min(EditLength,StrSize-RealLeftPos);
  if (OutStrLength < 0)
  {
    OutStrLength=0;
  }
  else
    wmemcpy(OutStrTmp,Str+RealLeftPos,OutStrLength);

  //if (TableSet) BUGBUG
    //DecodeString(OutStrTmp,(unsigned char*)TableSet->DecodeTable,OutStrLength);

  {
    wchar_t *p=OutStrTmp;
    wchar_t *e=OutStrTmp+OutStrLength;
    for (OutStrLength=0; OutStrLength<EditLength && p<e; p++)
    {
      if (*p == L'\t')
      {
        int S=TabSize-((LeftPos+OutStrLength) % TabSize);
        for (int i=0; i<S && OutStrLength<EditLength; i++,OutStrLength++)
          OutStr[OutStrLength]=L' ';
      }
      else
      {
        if (*p == 0)
          OutStr[OutStrLength]=L' ';
        else
          OutStr[OutStrLength]=*p;
        OutStrLength++;
      }
    }

    if (Flags.Check(FEDITLINE_PASSWORDMODE))
      wmemset(OutStr,L'*',OutStrLength);
  }
  OutStr[OutStrLength]=0;

  SetColor(Color);

  if (TabSelStart==-1)
  {
    if (Flags.Check(FEDITLINE_CLEARFLAG))
    {
      SetColor(ColorUnChanged);
      if (Mask && *Mask)
        OutStrLength=StrLength(RemoveTrailingSpaces(OutStr));
      mprintf(L"%-*.*s",OutStrLength,OutStrLength,OutStr);
      SetColor(Color);
      int BlankLength=EditLength-OutStrLength;
      if (BlankLength > 0)
        mprintf(L"%*s",BlankLength,L"");
    }
    else
      mprintf(L"%-*.*s",EditLength,EditLength,OutStr);
  }
  else
  {
    if ((TabSelStart-=LeftPos)<0)
      TabSelStart=0;
    int AllString=(TabSelEnd==-1);
    if (AllString)
      TabSelEnd=EditLength;
    else
      if ((TabSelEnd-=LeftPos)<0)
        TabSelEnd=0;
    wmemset(OutStr+OutStrLength,L' ',EditLength-OutStrLength);
    OutStr[EditLength]=0;
    /* $ 24.08.2000 SVS
       ! � DropDowList`� ��������� �� ������ ��������� - �� ��� ������� �����
         ���� ���� ������ ������
    */
    if (TabSelStart>=EditLength /*|| !AllString && TabSelStart>=StrSize*/ ||
        TabSelEnd<TabSelStart)
    {
      if(Flags.Check(FEDITLINE_DROPDOWNBOX))
      {
        SetColor(SelColor);
        mprintf(L"%*s",X2-X1+1,OutStr);
      }
      else
        Text(OutStr);
    }
    else
    {
      mprintf(L"%.*s",TabSelStart,OutStr);
      SetColor(SelColor);
      if(!Flags.Check(FEDITLINE_DROPDOWNBOX))
      {
        mprintf(L"%.*s",TabSelEnd-TabSelStart,OutStr+TabSelStart);
        if (TabSelEnd<EditLength)
        {
          //SetColor(Flags.Check(FEDITLINE_CLEARFLAG) ? SelColor:Color);
          SetColor(Color);
          Text(OutStr+TabSelEnd);
        }
      }
      else
      {
        mprintf(L"%*s",X2-X1+1,OutStr);
      }
    }
  }

  delete[] OutStr;
  delete[] OutStrTmp;

  /* $ 26.07.2000 tran
     ��� ����-���� ����� ��� �� ����� */
  if ( !Flags.Check(FEDITLINE_DROPDOWNBOX) )
      ApplyColor();
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
    string strPluginModule, strPluginFile, strPluginData;

    if (GetShortcutFolder(Key,&strPathName,&strPluginModule,&strPluginFile,&strPluginData))
      RetCode=TRUE;
  }
  else // ����/�����?
  {
    RetCode=_MakePath1(Key,strPathName,L"");
  }

  // ���� ���-���� ����������, ������ ��� � ������� (PathName)
  if(RetCode)
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

//    if(TableSet)
//       EncodeString(PathName,(unsigned char*)TableSet->EncodeTable,strlen(PathName)); //BUGBUG

    InsertString(strPathName);

    Flags.Clear(FEDITLINE_CLEARFLAG);
  }

  return RetCode;
}


__int64 Edit::VMProcess(int OpCode,void *vParam,__int64 iParam)
{
  switch(OpCode)
  {
    case MCODE_C_EMPTY:
      return (__int64)(GetLength()==0);
    case MCODE_C_SELECTED:
      return (__int64)(SelStart != -1 && SelStart < SelEnd);
    case MCODE_C_EOF:
      return (__int64)(CurPos >= StrSize);
    case MCODE_C_BOF:
      return (__int64)(CurPos==0);
    case MCODE_V_ITEMCOUNT:
      return (__int64)StrSize;
    case MCODE_V_CURPOS:
      return (__int64)(CursorPos+1);
  }
  return _i64(0);
}

int Edit::ProcessKey(int Key)
{
  int I;
  switch(Key)
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
      Key=KEY_CTRLINS;
      break;
    case KEY_CTRLV:
      Key=KEY_SHIFTINS;
      break;
    case KEY_CTRLX:
      Key=KEY_SHIFTDEL;
      break;
  }

  int PrevSelStart=-1,PrevSelEnd=0;

  if ( !Flags.Check(FEDITLINE_DROPDOWNBOX) && Key==KEY_CTRLL )
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
  if (((Key==KEY_BS || Key==KEY_DEL || Key==KEY_NUMDEL) && Flags.Check(FEDITLINE_DELREMOVESBLOCKS) || Key==KEY_CTRLD) &&
      !Flags.Check(FEDITLINE_EDITORMODE) && SelStart!=-1 && SelStart<SelEnd)
  {
    DeleteBlock();
    Show();
    return(TRUE);
  }

  int _Macro_IsExecuting=CtrlObject->Macro.IsExecuting();
  // $ 04.07.2000 IG - ��������� ��������� �� ������ ������� (00025.edit.cpp.txt)
  if (!ShiftPressed && (!_Macro_IsExecuting || IsNavKey(Key) && _Macro_IsExecuting) &&
      !IsShiftKey(Key) && !Recurse &&
      Key!=KEY_SHIFT && Key!=KEY_CTRL && Key!=KEY_ALT &&
      Key!=KEY_RCTRL && Key!=KEY_RALT && Key!=KEY_NONE &&
      Key!=KEY_KILLFOCUS && Key != KEY_GOTFOCUS &&
      ((Key&(~0xFF000000)) != KEY_LWIN && (Key&(~0xFF000000)) != KEY_RWIN && (Key&(~0xFF000000)) != KEY_APPS)
     )
  {
    Flags.Clear(FEDITLINE_MARKINGBLOCK); // ���... � ��� ����� ������ ����?

    if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS) && !(Key==KEY_CTRLINS || Key==KEY_CTRLNUMPAD0) &&
        !(Key==KEY_SHIFTDEL||Key==KEY_SHIFTNUMDEL||Key==KEY_SHIFTDECIMAL) && !Flags.Check(FEDITLINE_EDITORMODE) && Key != KEY_CTRLQ &&
        !(Key == KEY_SHIFTINS || Key == KEY_SHIFTNUMPAD0)) //Key != KEY_SHIFTINS) //??
    {
      /* $ 12.11.2002 DJ
         ����� ����������, ���� ������ �� ����������?
      */
      if (SelStart != -1 || SelEnd != 0)
      {
        PrevSelStart=SelStart;
        PrevSelEnd=SelEnd;
        Select(-1,0);
        Show();
      }
    }

  }

//  if (!EditEncodeDisabled && Key<256 && TableSet && !ReturnAltValue) BUGBUG
//    Key=TableSet->EncodeTable[Key];

  /* $ 11.09.2000 SVS
     ���� Opt.DlgEULBsClear = 1, �� BS � �������� ��� UnChanged ������
     ������� ����� ������ �����, ��� � Del
  */
  if (((Opt.Dialogs.EULBsClear && Key==KEY_BS) || Key==KEY_DEL || Key==KEY_NUMDEL) &&
     Flags.Check(FEDITLINE_CLEARFLAG) && CurPos>=StrSize)
    Key=KEY_CTRLY;

  /* $ 15.09.2000 SVS
     Bug - �������� ������� ������ -> Shift-Del ������� ��� ������
           ��� ������ ���� ������ ��� UnChanged ���������
  */
  if((Key == KEY_SHIFTDEL || Key == KEY_SHIFTNUMDEL || Key == KEY_SHIFTDECIMAL) && Flags.Check(FEDITLINE_CLEARFLAG) && CurPos>=StrSize && SelStart==-1)
  {
    SelStart=0;
    SelEnd=StrSize;
  }

  if (Flags.Check(FEDITLINE_CLEARFLAG) && (Key<256 && Key!=KEY_BS || Key==KEY_CTRLBRACKET ||
      Key==KEY_CTRLBACKBRACKET || Key==KEY_CTRLSHIFTBRACKET ||
      Key==KEY_CTRLSHIFTBACKBRACKET || Key==KEY_SHIFTENTER || Key==KEY_SHIFTNUMENTER))
  {
    LeftPos=0;
    SetString(L"");
    Show();
  }

  // ����� - ����� ������� ������� �����/������
  if(ProcessInsPath(Key,PrevSelStart,PrevSelEnd))
  {
    Show();
    return TRUE;
  }

  if (Key!=KEY_NONE && Key!=KEY_IDLE && Key!=KEY_SHIFTINS && Key!=KEY_SHIFTNUMPAD0 && Key!=KEY_CTRLINS &&
      (Key<KEY_F1 || Key>KEY_F12) && Key!=KEY_ALT && Key!=KEY_SHIFT &&
      Key!=KEY_CTRL && Key!=KEY_RALT && Key!=KEY_RCTRL &&
      (Key<KEY_ALT_BASE || Key>=KEY_ALT_BASE+256) &&
      !(/*Key<KEY_MACRO_BASE && Key>KEY_MACRO_ENDBASE ||*/ Key>=KEY_OP_BASE && Key <=KEY_OP_ENDBASE) && Key!=KEY_CTRLQ)
  {
    Flags.Clear(FEDITLINE_CLEARFLAG);
    Show();
  }


  switch(Key)
  {
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
      return(TRUE);
    }

    case KEY_SHIFTRIGHT: case KEY_SHIFTNUMPAD6:
    {
      if (!Flags.Check(FEDITLINE_MARKINGBLOCK))
      {
        Select(-1,0);
        Flags.Set(FEDITLINE_MARKINGBLOCK);
      }
      if (SelStart!=-1 && SelEnd==-1 || SelEnd>CurPos)
      {
        if (CurPos+1==SelEnd)
          Select(-1,0);
        else
          Select(CurPos+1,SelEnd);
      }
      else
        AddSelect(CurPos,CurPos+1);
      RecurseProcessKey(KEY_RIGHT);
      return(TRUE);
    }

    case KEY_CTRLSHIFTLEFT: case KEY_CTRLSHIFTNUMPAD4:
    {
      if (CurPos>StrSize)
      {
        PrevCurPos=CurPos;
        CurPos=StrSize;
      }

      if (CurPos>0)
        RecurseProcessKey(KEY_SHIFTLEFT);

      /* $ 12.01.2004 IS
         ��� ��������� � WordDiv ���������� IsWordDiv, � �� strchr, �.�.
         ������� ��������� ����� ���������� �� ��������� WordDiv (������� OEM)
      */
      while (CurPos>0 && !(!IsWordDiv(TableSet, WordDiv, Str[CurPos]) &&
             IsWordDiv(TableSet, WordDiv,Str[CurPos-1]) && !IsSpace(Str[CurPos])))
      {
        if (!IsSpace(Str[CurPos]) && (IsSpace(Str[CurPos-1]) ||
             IsWordDiv(TableSet, WordDiv, Str[CurPos-1])))
          break;
        RecurseProcessKey(KEY_SHIFTLEFT);
      }
      Show();
      return(TRUE);
    }

    case KEY_CTRLSHIFTRIGHT: case KEY_CTRLSHIFTNUMPAD6:
    {
      if (CurPos>=StrSize)
        return(FALSE);
      RecurseProcessKey(KEY_SHIFTRIGHT);

      while (CurPos<StrSize && !(IsWordDiv(TableSet, WordDiv, Str[CurPos]) &&
             !IsWordDiv(TableSet, WordDiv, Str[CurPos-1])))
      {
        if (!IsSpace(Str[CurPos]) && (IsSpace(Str[CurPos-1]) || IsWordDiv(TableSet, WordDiv, Str[CurPos-1])))
          break;
        RecurseProcessKey(KEY_SHIFTRIGHT);
        if (MaxLength!=-1 && CurPos==MaxLength-1)
          break;
      }
      Show();
      return(TRUE);
    }

    case KEY_SHIFTHOME:  case KEY_SHIFTNUMPAD7:
    {
      Lock ();
      while (CurPos>0)
        RecurseProcessKey(KEY_SHIFTLEFT);
      Unlock ();
      Show();
      return(TRUE);
    }

    case KEY_SHIFTEND:  case KEY_SHIFTNUMPAD1:
    {
      Lock ();

      int Len;

      if (Mask && *Mask)
      {
        wchar_t *ShortStr=new wchar_t[StrSize+1];
        if (ShortStr==NULL)
          return FALSE;

        wcsncpy(ShortStr,Str,StrSize);
        Len=StrLength(RemoveTrailingSpaces(ShortStr));
        delete[] ShortStr;
      }
      else
        Len=StrSize;

      int LastCurPos=CurPos;
      while (CurPos<Len/*StrSize*/)
      {
        RecurseProcessKey(KEY_SHIFTRIGHT);
        if(LastCurPos==CurPos)break;
        LastCurPos=CurPos;
      }

      Unlock ();
      Show();
      return(TRUE);
    }

    case KEY_BS:
    {
      if (CurPos<=0)
        return(FALSE);
      PrevCurPos=CurPos;
      CurPos--;
      if (CurPos<=LeftPos)
      {
        LeftPos-=15;
        if (LeftPos<0)
          LeftPos=0;
      }
      if (!RecurseProcessKey(KEY_DEL))
        Show();
      return(TRUE);
    }

    case KEY_CTRLSHIFTBS:
    {
      int i;
      for (i=CurPos;i>=0;i--)
        RecurseProcessKey(KEY_BS);
      Show();
      return(TRUE);
    }

    case KEY_CTRLBS:
    {
      if (CurPos>StrSize)
      {
        PrevCurPos=CurPos;
        CurPos=StrSize;
      }
      Lock ();
//      while (CurPos>0 && IsSpace(Str[CurPos-1]))
//        RecurseProcessKey(KEY_BS);
      while (1)
      {
        int StopDelete=FALSE;
        if (CurPos>1 && IsSpace(Str[CurPos-1])!=IsSpace(Str[CurPos-2]))
          StopDelete=TRUE;
        RecurseProcessKey(KEY_BS);
        if (CurPos==0 || StopDelete)
          break;
        if (IsWordDiv(TableSet, WordDiv,Str[CurPos-1]))
          break;
      }
      Unlock ();
      Show();
      return(TRUE);
    }

    case KEY_CTRLQ:
    {
      Lock ();
      if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS) && (SelStart != -1 || Flags.Check(FEDITLINE_CLEARFLAG)))
        RecurseProcessKey(KEY_DEL);
      ProcessCtrlQ();
      Unlock ();
      Show();
      return(TRUE);
    }

    case KEY_OP_SELWORD:
    {
      int OldCurPos=CurPos;
      PrevSelStart=SelStart;
      PrevSelEnd=SelEnd;

#if defined(MOUSEKEY)
      if(CurPos >= SelStart && CurPos <= SelEnd)
      { // �������� ��� ������ ��� ��������� ������� �����
        Select(0,StrSize);
      }
      else
#endif
      {
        int SStart, SEnd;
        CalcWordFromString(Str,CurPos,&SStart,&SEnd,TableSet,WordDiv); // TableSet --> UseDecodeTable?&TableSet:NULL
        Select(SStart,SEnd+(SEnd < StrSize?1:0));
      }
      CurPos=OldCurPos; // ���������� �������
      Show();
      return TRUE;
    }

    case KEY_OP_DATE:
    case KEY_OP_PLAINTEXT:
    {
      if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS))
      {
        if(SelStart != -1 || Flags.Check(FEDITLINE_CLEARFLAG)) // BugZ#1053 - ���������� � $Text
          RecurseProcessKey(KEY_DEL);
      }
      const wchar_t *S = eStackAsString();
      if(Key == KEY_OP_DATE)
        ProcessInsDate(S);
      else
        ProcessInsPlainText(S);
      Show();
      return TRUE;
    }

    case KEY_CTRLT:
    case KEY_CTRLDEL:
    case KEY_CTRLNUMDEL:
    case KEY_CTRLDECIMAL:
    {
      if (CurPos>=StrSize)
        return(FALSE);
      Lock ();
//      while (CurPos<StrSize && IsSpace(Str[CurPos]))
//        RecurseProcessKey(KEY_DEL);
      if (Mask && *Mask)
      {
        int MaskLen=StrLength(Mask);
        int ptr=CurPos;
        while(ptr<MaskLen)
        {
          ptr++;
          if (!CheckCharMask(Mask[ptr]) ||
             (IsSpace(Str[ptr]) && !IsSpace(Str[ptr+1])) ||
             (IsWordDiv(TableSet, WordDiv, Str[ptr])))
            break;
        }
        for (int i=0;i<ptr-CurPos;i++)
          RecurseProcessKey(KEY_DEL);
      }
      else
      {
        while (1)
        {
          int StopDelete=FALSE;
          if (CurPos<StrSize-1 && IsSpace(Str[CurPos]) && !IsSpace(Str[CurPos+1]))
            StopDelete=TRUE;
          RecurseProcessKey(KEY_DEL);
          if (CurPos>=StrSize || StopDelete)
            break;
          if (IsWordDiv(TableSet, WordDiv, Str[CurPos]))
            break;
        }
      }
      Unlock ();
      Show();
      return(TRUE);
    }

    case KEY_CTRLY:
    {
      if (Flags.Check(FEDITLINE_READONLY|FEDITLINE_DROPDOWNBOX))
        return (TRUE);
      PrevCurPos=CurPos;
      LeftPos=CurPos=0;
      *Str=0;
      StrSize=0;
      Str=(wchar_t *)xf_realloc(Str,1*sizeof(wchar_t));
      Select(-1,0);
      Show();
      return(TRUE);
    }

    case KEY_CTRLK:
    {
      if (Flags.Check(FEDITLINE_READONLY|FEDITLINE_DROPDOWNBOX))
        return (TRUE);
      if (CurPos>=StrSize)
        return(FALSE);
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
      Str=(wchar_t *)xf_realloc(Str,(StrSize+1)*sizeof (wchar_t));
      Show();
      return(TRUE);
    }

    case KEY_HOME:        case KEY_NUMPAD7:
    case KEY_CTRLHOME:    case KEY_CTRLNUMPAD7:
    {
      PrevCurPos=CurPos;
      CurPos=0;
      Show();
      return(TRUE);
    }

    case KEY_END:         case KEY_NUMPAD1:
    case KEY_CTRLEND:     case KEY_CTRLNUMPAD1:
    {
      PrevCurPos=CurPos;

      if (Mask && *Mask)
      {
        wchar_t *ShortStr=new wchar_t[StrSize+1];
        if (ShortStr==NULL)
          return FALSE;
        wcsncpy(ShortStr,Str,StrSize);
        CurPos=StrLength(RemoveTrailingSpaces(ShortStr));
        delete[] ShortStr;
      }
      else
        CurPos=StrSize;

      Show();
      return(TRUE);
    }

    case KEY_LEFT:        case KEY_NUMPAD4:
    case KEY_CTRLS:
    {
      if (CurPos>0)
      {
        PrevCurPos=CurPos;
        CurPos--;
        Show();
      }
      return(TRUE);
    }

    case KEY_RIGHT:       case KEY_NUMPAD6:
    case KEY_CTRLD:
    {
      PrevCurPos=CurPos;

      if (Mask && *Mask)
      {
        wchar_t *ShortStr=new wchar_t[StrSize+1];
        if (ShortStr==NULL)
          return FALSE;
        wcsncpy(ShortStr,Str,StrSize);
        int Len=StrLength(RemoveTrailingSpaces(ShortStr));
        delete[] ShortStr;
        if (Len>CurPos)
          CurPos++;
      }
      else
        CurPos++;

      Show();
      return(TRUE);
    }

    case KEY_INS:         case KEY_NUMPAD0:
    {
      Flags.Swap(FEDITLINE_OVERTYPE);
      Show();
      return(TRUE);
    }

    case KEY_NUMDEL:
    case KEY_DEL:
    {
      if (Flags.Check(FEDITLINE_READONLY|FEDITLINE_DROPDOWNBOX))
        return (TRUE);
      if (CurPos>=StrSize)
        return(FALSE);
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
      if (Mask && *Mask)
      {
        int MaskLen=StrLength(Mask);
        int i,j;
        for (i=CurPos,j=CurPos;i<MaskLen;i++)
        {
          if (CheckCharMask(Mask[i+1]))
          {
            while(!CheckCharMask(Mask[j]) && j<MaskLen)
              j++;
            Str[j]=Str[i+1];
            j++;
          }
        }
        Str[j]=' ';
      }
      else
      {
        wmemmove(Str+CurPos,Str+CurPos+1,StrSize-CurPos);
        StrSize--;
        Str=(wchar_t *)xf_realloc(Str,(StrSize+1)*sizeof (wchar_t));
      }

      Show();
      return(TRUE);
    }

    case KEY_CTRLLEFT:  case KEY_CTRLNUMPAD4:
    {
      PrevCurPos=CurPos;
      if (CurPos>StrSize)
        CurPos=StrSize;
      if (CurPos>0)
        CurPos--;
      while (CurPos>0 && !(!IsWordDiv(TableSet, WordDiv, Str[CurPos]) &&
             IsWordDiv(TableSet, WordDiv, Str[CurPos-1]) && !IsSpace(Str[CurPos])))
      {
        if (!IsSpace(Str[CurPos]) && IsSpace(Str[CurPos-1]))
          break;
        CurPos--;
      }
      Show();
      return(TRUE);
    }

    case KEY_CTRLRIGHT:   case KEY_CTRLNUMPAD6:
    {
      if (CurPos>=StrSize)
        return(FALSE);
      PrevCurPos=CurPos;

      int Len;
      if (Mask && *Mask)
      {
        wchar_t *ShortStr=new wchar_t[StrSize+1];
        if (ShortStr==NULL)
          return FALSE;
        wcsncpy(ShortStr,Str,StrSize);
        Len=StrLength(RemoveTrailingSpaces(ShortStr));
        delete[] ShortStr;
        if (Len>CurPos)
          CurPos++;
      }
      else
      {
        Len=StrSize;
        CurPos++;
      }

      while (CurPos<Len/*StrSize*/ && !(IsWordDiv(TableSet, WordDiv,Str[CurPos]) &&
             !IsWordDiv(TableSet, WordDiv, Str[CurPos-1])))
      {
        if (!IsSpace(Str[CurPos]) && IsSpace(Str[CurPos-1]))
          break;
        CurPos++;
      }
      Show();
      return(TRUE);
    }

    case KEY_SHIFTNUMDEL:
    case KEY_SHIFTDECIMAL:
    case KEY_SHIFTDEL:
    {
      if (SelStart==-1 || SelStart>=SelEnd)
        return(FALSE);
      RecurseProcessKey(KEY_CTRLINS);
      DeleteBlock();
      Show();
      return(TRUE);
    }

    case KEY_CTRLINS:     case KEY_CTRLNUMPAD0:
    {
      if (!Flags.Check(FEDITLINE_PASSWORDMODE))
        if (SelStart==-1 || SelStart>=SelEnd)
        {
          if (Mask && *Mask)
          {
            wchar_t *ShortStr=new wchar_t[StrSize+1];
            if (ShortStr==NULL)
              return FALSE;
            wcsncpy(ShortStr,Str,StrSize);
            RemoveTrailingSpaces(ShortStr);
            CopyToClipboard(ShortStr);
            delete[] ShortStr;
          }
          else
            CopyToClipboard(Str);
        }
        else
          if (SelEnd<=StrSize) // TODO: ���� � ������ ������� �������� "StrSize &&", �� �������� ��� "Ctrl-Ins � ������ ������ ������� ��������"
          {
            int Ch=Str[SelEnd];
            Str[SelEnd]=0;
            CopyToClipboard(Str+SelStart);
            Str[SelEnd]=Ch;
          }
      return(TRUE);
    }

    case KEY_SHIFTINS:    case KEY_SHIFTNUMPAD0:
    {
        wchar_t *ClipText=NULL;

        if (MaxLength==-1)
            ClipText=PasteFromClipboard();
        else
            ClipText=PasteFromClipboardEx(MaxLength);
        /* tran $ */
        if (ClipText==NULL)
          return(TRUE);
        if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS)){
          DeleteBlock();
        }

        for (I=StrLength(Str)-1;I>=0 && IsEol(Str[I]);I--)
          Str[I]=0;
        for (I=0;ClipText[I];I++)
          if (IsEol(ClipText[I]))
          {
            if (IsEol(ClipText[I+1]))
              wmemmove(&ClipText[I],&ClipText[I+1],StrLength(&ClipText[I+1])+1);
            if (ClipText[I+1]==0)
              ClipText[I]=0;
            else
              ClipText[I]=L' ';
          }

        if (Flags.Check(FEDITLINE_CLEARFLAG))
        {
          LeftPos=0;
          SetString(ClipText);
          Flags.Clear(FEDITLINE_CLEARFLAG);
        }
        else
          InsertString(ClipText);
        if(ClipText)
          delete[] ClipText;
        Show();
      return(TRUE);
    }

    case KEY_SHIFTTAB:
    {
        PrevCurPos=CurPos;
        CursorPos-=(CursorPos-1) % TabSize+1;
        SetTabCurPos(CursorPos);
        Show();
      return(TRUE);
    }

    case KEY_SHIFTSPACE:
      Key = KEY_SPACE;

    default:
    {
//      _D(SysLog(L"Key=0x%08X",Key));

      if (Key==KEY_NONE || Key==KEY_IDLE || Key==KEY_ENTER || Key==KEY_NUMENTER || Key>=65536 )
        break;
      if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS))
      {
        if (PrevSelStart!=-1)
        {
          SelStart=PrevSelStart;
          SelEnd=PrevSelEnd;
        }
        DeleteBlock();
      }

      if (InsertKey(Key))
        Show();

      return(TRUE);
    }
  }
  return(FALSE);
}

// ��������� Ctrl-Q
int Edit::ProcessCtrlQ(void)
{
  INPUT_RECORD rec;
  DWORD Key;

  while (1)
  {
    Key=GetInputRecord(&rec);
    if (Key!=KEY_NONE && Key!=KEY_IDLE && rec.Event.KeyEvent.uChar.AsciiChar)
      break;

    if(Key==KEY_CONSOLE_BUFFER_RESIZE)
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

int Edit::ProcessInsDate(const wchar_t *Fmt)
{
  string strTStr;

  if(MkStrFTime(strTStr,Fmt))
  {
    InsertString(strTStr);
    return TRUE;
  }
  return FALSE;
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
  wchar_t *NewStr;

  if (Flags.Check(FEDITLINE_READONLY|FEDITLINE_DROPDOWNBOX))
    return (TRUE);

  if (Key==KEY_TAB && Flags.Check(FEDITLINE_OVERTYPE))
  {
    PrevCurPos=CurPos;
    CursorPos+=TabSize - (CursorPos % TabSize);
    SetTabCurPos(CursorPos);

    return(TRUE);
  }
  if (Mask && *Mask)
  {
    int MaskLen=StrLength(Mask);
    if (CurPos<MaskLen)
    {
      if (KeyMatchedMask(Key))
      {
        if (!Flags.Check(FEDITLINE_OVERTYPE))
        {
          int i=MaskLen-1;
          while(!CheckCharMask(Mask[i]) && i>CurPos)
            i--;

          for (int j=i;i>CurPos;i--)
          {
            if (CheckCharMask(Mask[i]))
            {
              while(!CheckCharMask(Mask[j-1]))
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
        PrevCurPos=CurPos;
        Str[CurPos++]=Key;
      }
      else
      {
        // ����� ������� ��� "����� ������ �� �����", �������� ��� SetAttr - ������ '.'
        ;// char *Ptr=strchr(Mask+CurPos,Key);
      }
    }
    else if (CurPos<StrSize)
    {
      PrevCurPos=CurPos;
      Str[CurPos++]=Key;
    }
  }
  else
  {
    if(MaxLength == -1 || StrSize < MaxLength)
    {
      if (CurPos>=StrSize)
      {
        if ((NewStr=(wchar_t *)xf_realloc(Str,(CurPos+2)*sizeof (wchar_t)))==NULL)
          return(FALSE);
        Str=NewStr;
        swprintf(&Str[StrSize],L"%*s",CurPos-StrSize,L"");
        //memset(Str+StrSize,' ',CurPos-StrSize);Str[CurPos+1]=0;
        StrSize=CurPos+1;
      }
      else
        if (!Flags.Check(FEDITLINE_OVERTYPE))
          StrSize++;


      if (Key==KEY_TAB && (TabExpandMode==EXPAND_NEWTABS || TabExpandMode==EXPAND_ALLTABS))
      {
        StrSize--;
        InsertTab();
        return TRUE;
      }

      if ((NewStr=(wchar_t *)xf_realloc(Str,(StrSize+1)*sizeof (wchar_t)))==NULL)
        return(TRUE);
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
      PrevCurPos=CurPos;
      Str[CurPos++]=Key;
    }
    else if (Flags.Check(FEDITLINE_OVERTYPE))
    {
      if(CurPos < StrSize)
      {
        PrevCurPos=CurPos;
        Str[CurPos++]=Key;
      }
    }
    else
      MessageBeep(MB_ICONHAND);
  }
  Str[StrSize]=0;
  return(TRUE);
}

void Edit::SetObjectColor(int Color,int SelColor,int ColorUnChanged)
{
  Edit::Color=Color;
  Edit::SelColor=SelColor;
  Edit::ColorUnChanged=ColorUnChanged;
}


void Edit::GetString(wchar_t *Str,int MaxSize)
{
    xwcsncpy(Str, Edit::Str,MaxSize-1);
    Str[MaxSize-1]=0;
}

void Edit::GetString(string &strStr)
{
    strStr = Edit::Str;
}


const wchar_t* Edit::GetStringAddrW()
{
    return Str;
}



void Edit::SetString(const wchar_t *Str)
{
  if ( Flags.Check(FEDITLINE_READONLY) )
    return;
  Select(-1,0);
  SetBinaryString(Str,StrLength(Str));
}

void Edit::SetEOL(const wchar_t *EOL)
{
  EndType=EOL_NONE;
  if ( EOL && *EOL )
  {
    if (EOL[0]==L'\r')
      if (EOL[1]==L'\n')
        EndType=EOL_CRLF;
      else if (EOL[1]==L'\r' && EOL[2]==L'\n')
        EndType=EOL_CRCRLF;
      else
        EndType=EOL_CR;
    else
      if (EOL[0]==L'\n')
        EndType=EOL_LF;
  }

}

const wchar_t *Edit::GetEOL(void)
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
  if ( Flags.Check(FEDITLINE_READONLY) )
    return;

  // ��������� ������������ �������, ���� ��������� MaxLength
  if(MaxLength != -1 && Length > MaxLength)
  {
    Length=MaxLength; // ??
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
  if (Mask && *Mask)
  {
    RefreshStrByMask(TRUE);

    int maskLen=StrLength(Mask);
    for (int i=0,j=0;j<maskLen && j<Length;)
    {
      if (CheckCharMask(Mask[i]))
      {
        int goLoop=FALSE;
        if (KeyMatchedMask(Str[j]))
          InsertKey(Str[j]);
        else
          goLoop=TRUE;
        j++;
        if (goLoop) continue;
      }
      else
      {
        PrevCurPos=CurPos;
        CurPos++;
      }
      i++;
    }

    /* ����� ���������� ������� (*Str==0), �.�. ��� ������� ������
       ������ �������� ����� ����� SetBinaryString("",0)
       �.�. ����� ������� �� ���������� "�������������" ������ � ������
    */
    RefreshStrByMask(*Str==0);
  }
  else
  {
    wchar_t *NewStr=(wchar_t *)xf_realloc(Edit::Str,(Length+1)*sizeof (wchar_t));
    if (NewStr==NULL)
      return;
    Edit::Str=NewStr;
    StrSize=Length;
    wmemcpy(Edit::Str,Str,Length);
    Edit::Str[Length]=0;

    if ( TabExpandMode == EXPAND_ALLTABS )
      ReplaceTabs ();
    PrevCurPos=CurPos;
    CurPos=StrSize;
  }
}

void Edit::GetBinaryString(const wchar_t **Str,const wchar_t **EOL,int &Length)
{
    *Str=Edit::Str;

    if (EOL!=NULL)
        *EOL=EOL_TYPE_CHARS[EndType];

    Length=StrSize; //???
}

int Edit::GetSelString(wchar_t *Str, int MaxSize)
{
  if (SelStart==-1 || SelEnd!=-1 && SelEnd<=SelStart ||
      SelStart>=StrSize)
  {
    *Str=0;
    return(FALSE);
  }

  int CopyLength;
  if (SelEnd==-1)
    CopyLength=MaxSize-1;
  else
    CopyLength=Min(MaxSize-1,SelEnd-SelStart);

  wcsncpy(Str,Edit::Str+SelStart,CopyLength);
  Str[CopyLength]=0;

  return(TRUE);
}

int Edit::GetSelString (string &strStr)
{
  if (SelStart==-1 || SelEnd!=-1 && SelEnd<=SelStart ||
      SelStart>=StrSize)
  {
    strStr = L"";
    return(FALSE);
  }

  int CopyLength;

  CopyLength=SelEnd-SelStart; //??? BUGBUG

  wchar_t *lpwszStr = strStr.GetBuffer (CopyLength);

  wcsncpy(lpwszStr,Edit::Str+SelStart,CopyLength);
  lpwszStr[CopyLength]=0;

  strStr.ReleaseBuffer ();

  return(TRUE);
}



void Edit::InsertString(const wchar_t *Str)
{
  if (Flags.Check(FEDITLINE_READONLY|FEDITLINE_DROPDOWNBOX))
    return;

  Select(-1,0);
  InsertBinaryString(Str,StrLength(Str));
}


void Edit::InsertBinaryString(const wchar_t *Str,int Length)
{
  wchar_t *NewStr;

  if (Flags.Check(FEDITLINE_READONLY|FEDITLINE_DROPDOWNBOX))
    return;

  Flags.Clear(FEDITLINE_CLEARFLAG);

  if (Mask && *Mask)
  {
    int Pos=CurPos;
    int MaskLen=StrLength(Mask);
    if (Pos<MaskLen)
    {
      //_SVS(SysLog(L"InsertBinaryString ==> Str='%s' (Length=%d) Mask='%s'",Str,Length,Mask+Pos));
      int StrLen=(MaskLen-Pos>Length)?Length:MaskLen-Pos;
      /* $ 15.11.2000 KM
         ������� ����������� ��� ���������� ������ PasteFromClipboard
         � ������ � ������
      */
      for (int i=Pos,j=0;j<StrLen+Pos;)
      {
        if (CheckCharMask(Mask[i]))
        {
          int goLoop=FALSE;
          if (j < Length && KeyMatchedMask(Str[j]))
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
          PrevCurPos=CurPos;
          CurPos++;
        }
        i++;
      }
    }
    RefreshStrByMask();
    //_SVS(SysLog(L"InsertBinaryString ==> Edit::Str='%s'",Edit::Str));
  }
  else
  {
    if(MaxLength != -1 && StrSize+Length > MaxLength)
    {
      // ��������� ������������ �������, ���� ��������� MaxLength
      if(StrSize < MaxLength)
      {
        Length=MaxLength-StrSize;
      }
    }

    if(MaxLength == -1 || StrSize+Length <= MaxLength)
    {
      if (CurPos>StrSize)
      {
        if ((NewStr=(wchar_t *)xf_realloc(Edit::Str,(CurPos+1)*sizeof (wchar_t)))==NULL)
          return;
        Edit::Str=NewStr;
        swprintf(&Edit::Str[StrSize],L"%*s",CurPos-StrSize,L"");
        //memset(Edit::Str+StrSize,' ',CurPos-StrSize);Edit::Str[CurPos+1]=0;
        StrSize=CurPos;
      }

      int TmpSize=StrSize-CurPos;
      wchar_t *TmpStr=new wchar_t[TmpSize+16];
      if(!TmpStr)
        return;

      wmemcpy(TmpStr,&Edit::Str[CurPos],TmpSize);

      StrSize+=Length;
      if ((NewStr=(wchar_t *)xf_realloc(Edit::Str,(StrSize+1)*sizeof (wchar_t)))==NULL)
      {
        delete[] TmpStr;
        return;
      }
      Edit::Str=NewStr;
      wmemcpy(&Edit::Str[CurPos],Str,Length);
      PrevCurPos=CurPos;
      CurPos+=Length;
      wmemcpy(Edit::Str+CurPos,TmpStr,TmpSize);
      Edit::Str[StrSize]=0;
      delete[] TmpStr;

      if ( TabExpandMode == EXPAND_ALLTABS )
        ReplaceTabs();
    }
    else
      MessageBeep(MB_ICONHAND);
  }
}


int Edit::GetLength()
{
  return(StrSize);
}


// ������� ��������� ����� ����� � ������ Edit
void Edit::SetInputMask(const wchar_t *InputMask)
{
  if (Mask)
    xf_free(Mask);

  if (InputMask && *InputMask)
  {
    if((Mask=_wcsdup(InputMask)) == NULL)
      return;
    RefreshStrByMask(TRUE);
  }
  else
    Mask=NULL;
}


// ������� ���������� ��������� ������ ����� �� ����������� Mask
void Edit::RefreshStrByMask(int InitMode)
{
  int i;
  if (Mask && *Mask)
  {
    int MaskLen=StrLength(Mask);
    if (StrSize!=MaskLen)
    {
      wchar_t *NewStr=(wchar_t *)xf_realloc(Str,(MaskLen+1)*sizeof (wchar_t));
      if (NewStr==NULL)
        return;
      Str=NewStr;
      for (i=StrSize;i<MaskLen;i++)
        Str[i]=L' ';
      StrSize=MaxLength=MaskLen;
      Str[StrSize]=0;
    }
    for (i=0;i<MaskLen;i++)
    {
      if (InitMode)
        Str[i]=L' ';
      if (!CheckCharMask(Mask[i]))
        Str[i]=Mask[i];
    }
  }
}


int Edit::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
  if ((MouseEvent->dwButtonState & 3)==0)
    return(FALSE);
  if (MouseEvent->dwMousePosition.X<X1 || MouseEvent->dwMousePosition.X>X2 ||
      MouseEvent->dwMousePosition.Y!=Y1)
    return(FALSE);
  //SetClearFlag(0); // ����� ������ ��� ��������� � ������ �����-������?
  SetTabCurPos(MouseEvent->dwMousePosition.X - X1 + LeftPos);

  if (!Flags.Check(FEDITLINE_PERSISTENTBLOCKS))
    Select(-1,0);

  Show();
  return(TRUE);
}


/* $ 03.08.2000 KM
   ������� ������� �������� ��-�� �������������
   ���������� ������ ����� ����.
*/
int Edit::Search(const wchar_t *Str,int Position,int Case,int WholeWords,int Reverse)
{
/*  int I,J,Length=strlen(Str);
  if (Reverse)
  {
    Position--;
    if (Position>=StrSize)
      Position=StrSize-1;
    if (Position<0)
      return(FALSE);
  }
  if (Position<StrSize && *Str)
    for (I=Position;Reverse && I>=0 || !Reverse && I<StrSize;Reverse ? I--:I++)
    {
      for (J=0;;J++)
      {
        if (Str[J]==0)
        {
          CurPos=I;
          return(TRUE);
        }
        if (WholeWords)
        {
          int ChLeft,ChRight;
          int locResultLeft=FALSE;
          int locResultRight=FALSE;

          ChLeft=(TableSet==NULL) ? Edit::Str[I-1]:TableSet->DecodeTable[Edit::Str[I-1]];
          if (I>0)
            locResultLeft=(IsSpace(ChLeft) || strchr(WordDiv,ChLeft)!=NULL);
          else
            locResultLeft=TRUE;
          if (I+Length<StrSize)
          {
            ChRight=(TableSet==NULL) ? Edit::Str[I+Length]:TableSet->DecodeTable[Edit::Str[I+Length]];
            locResultRight=(IsSpace(ChRight) || strchr(WordDiv,ChRight)!=NULL);
          }
          else
            locResultRight=TRUE;
          if (!locResultLeft || !locResultRight)
            break;
        }
        int Ch=(TableSet==NULL) ? Edit::Str[I+J]:TableSet->DecodeTable[Edit::Str[I+J]];
        if (Case)
        {
          if (Ch!=Str[J])
            break;
        }
        else
        {
          if (LocalUpper(Ch)!=LocalUpper(Str[J]))
            break;
        }
      }
    }*/ //BUGBUG
  return(FALSE);
}

void Edit::InsertTab()
{
  wchar_t *TabPtr;
  int Pos,S;
  if ( Flags.Check(FEDITLINE_READONLY) )
    return;

  Pos=CurPos;
  S=TabSize-(Pos % TabSize);

  if(SelStart!=-1)
  {
    if(Pos<=SelStart)
    {
      SelStart+=S-(Pos==SelStart?0:1);
    }
    if(SelEnd!=-1 && Pos<SelEnd)
    {
      SelEnd+=S;
    }
  }

  int PrevStrSize=StrSize;
  StrSize+=S;

  CurPos+=S;

  Str=(wchar_t *)xf_realloc(Str,(StrSize+1)*sizeof (wchar_t));

  TabPtr=Str+Pos;

  wmemmove(TabPtr+S,TabPtr,PrevStrSize-Pos);
  wmemset(TabPtr,L' ',S);

  Str[StrSize]=0;
}


void Edit::ReplaceTabs()
{
  wchar_t *TabPtr;
  int Pos=0,S;
  if ( Flags.Check(FEDITLINE_READONLY) )
    return;

  while ((TabPtr=(wchar_t *)wmemchr(Str+Pos,L'\t',StrSize-Pos))!=NULL)
  {
    Pos=(int)(TabPtr-Str);
    S=TabSize-((int)(TabPtr-Str) % TabSize);
    if(SelStart!=-1)
    {
      if(Pos<=SelStart)
      {
        SelStart+=S-(Pos==SelStart?0:1);
      }
      if(SelEnd!=-1 && Pos<SelEnd)
      {
        SelEnd+=S-1;
      }
    }
    int PrevStrSize=StrSize;
    StrSize+=S-1;
    if (CurPos>Pos)
      CurPos+=S-1;
    Str=(wchar_t *)xf_realloc(Str,(StrSize+1)*sizeof (wchar_t));
    TabPtr=Str+Pos;
    wmemmove(TabPtr+S,TabPtr+1,PrevStrSize-Pos);
    wmemset(TabPtr,L' ',S);
    Str[StrSize]=0;
  }
}


int Edit::GetTabCurPos()
{
  return(RealPosToTab(CurPos));
}


void Edit::SetTabCurPos(int NewPos)
{
  int Pos;

  if (Mask && *Mask)
  {
    wchar_t *ShortStr=new wchar_t[StrSize+1];
    if (ShortStr==NULL)
      return;
    wcsncpy(ShortStr,Str,StrSize);
    Pos=StrLength(RemoveTrailingSpaces(ShortStr));
    delete[] ShortStr;
    if (NewPos>Pos)
      NewPos=Pos;
  }

  CurPos=TabPosToReal(NewPos);
}


int Edit::RealPosToTab(int Pos)
{
  int TabPos,I;

  if ( (TabExpandMode == EXPAND_ALLTABS) || wmemchr(Str,L'\t',StrSize)==NULL)
    return(Pos);


  /* $ 10.10.2004 KM
     ����� ����������� Bug #1122 �������� ��� � ��������������
     ����� �� ������� ������ � ��������� ��� �������������
     Cursor beyond end of line.
  */
  for (TabPos=0,I=0;I<Pos && ((Flags.Check(FEDITLINE_EDITBEYONDEND))?TRUE:Str[I]);I++)
  {
    if (I>=StrSize)
    {
      TabPos+=Pos-I;
      break;
    }
    if (Str[I]==L'\t')
      TabPos+=TabSize - (TabPos % TabSize);
    else
      TabPos++;
  }
  return(TabPos);
}


int Edit::TabPosToReal(int Pos)
{
  int TabPos,I;

  if ( (TabExpandMode == EXPAND_ALLTABS) || wmemchr(Str,L'\t',StrSize)==NULL)
    return(Pos);


  /* $ 10.10.2004 KM
     ����� ����������� Bug #1122 �������� ��� � ��������������
     ����� �� ������� ������ � ��������� ��� �������������
     Cursor beyond end of line.
  */
  for (TabPos=0,I=0;TabPos<Pos && ((Flags.Check(FEDITLINE_EDITBEYONDEND))?TRUE:Str[I]);I++)
  {
    if (I>StrSize)
    {
      I+=Pos-TabPos;
      break;
    }
    if (Str[I]==L'\t')
    {
      int NewTabPos=TabPos+TabSize - (TabPos % TabSize);
      if (NewTabPos>Pos)
        break;
      TabPos=NewTabPos;
    }
    else
      TabPos++;
  }
  return(I);
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
  if (End==-1 || End>SelEnd && SelEnd!=-1)
    SelEnd=End;
  if (SelEnd>StrSize)
    SelEnd=StrSize;
  if (SelEnd<SelStart && SelEnd!=-1)
  {
    SelStart=-1;
    SelEnd=0;
  }
}


void Edit::GetSelection(int &Start,int &End)
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


void Edit::GetRealSelection(int &Start,int &End)
{
  Start=SelStart;
  End=SelEnd;
}


void Edit::DisableEncode(int Disable)
{
  EditEncodeDisabled=Disable;
}


void Edit::SetTables(struct CharTableSet *TableSet)
{
  Edit::TableSet=TableSet;
};

void Edit::DeleteBlock()
{
  if (Flags.Check(FEDITLINE_READONLY|FEDITLINE_DROPDOWNBOX))
    return;

  if (SelStart==-1 || SelStart>=SelEnd)
    return;

  PrevCurPos=CurPos;
  if (Mask && *Mask)
  {
    for (int i=SelStart;i<SelEnd;i++)
    {
      if (CheckCharMask(Mask[i]))
        Str[i]=L' ';
    }
    CurPos=SelStart;
  }
  else
  {
    int From=SelStart,To=SelEnd;
    if(From>StrSize)From=StrSize;
    if(To>StrSize)To=StrSize;
    wmemmove(Str+From,Str+To,StrSize-To+1);
    StrSize-=To-From;
    if (CurPos>From)
      if (CurPos<To)
        CurPos=From;
      else
        CurPos-=To-From;
    Str=(wchar_t *)xf_realloc(Str,(StrSize+1)*sizeof (wchar_t));
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
}


void Edit::AddColor(struct ColorItem *col)
{
  if ((ColorCount & 15)==0)
    ColorList=(ColorItem *)xf_realloc(ColorList,(ColorCount+16)*sizeof(*ColorList));
  ColorList[ColorCount++]=*col;
}


int Edit::DeleteColor(int ColorPos)
{
  int Src;
  if (ColorCount==0)
    return(FALSE);
  int Dest=0;
  for (Src=0;Src<ColorCount;Src++)
    if (ColorPos!=-1 && ColorList[Src].StartPos!=ColorPos)
    {
      if (Dest!=Src)
        ColorList[Dest]=ColorList[Src];
      Dest++;
    }
  int DelCount=ColorCount-Dest;
  ColorCount=Dest;
  if (ColorCount==0)
  {
    xf_free (ColorList);
    ColorList=NULL;
  }
  return(DelCount!=0);
}


int Edit::GetColor(struct ColorItem *col,int Item)
{
  if (Item >= ColorCount)
    return(FALSE);
  *col=ColorList[Item];
  return(TRUE);
}


void Edit::ApplyColor()
{
  int Col,I,SelColor0;

  for (Col=0;Col<ColorCount;Col++)
  {
    struct ColorItem *CurItem=ColorList+Col;
    int Attr=CurItem->Color;
    int Length=CurItem->EndPos-CurItem->StartPos+1;
    if(CurItem->StartPos+Length >= StrSize)
      Length=StrSize-CurItem->StartPos;

    int Start=RealPosToTab(CurItem->StartPos)-LeftPos;
    int LengthFind=CurItem->StartPos+Length >= StrSize?StrSize-CurItem->StartPos+1:Length;
    int CorrectPos=0;

    if(Attr&ECF_TAB1)
      Attr&=~ECF_TAB1;
    else
      CorrectPos=LengthFind > 0 && CurItem->StartPos < StrSize && wmemchr(Str+CurItem->StartPos,L'\t',LengthFind)?1:0;

    int End=RealPosToTab(CurItem->EndPos+CorrectPos)-LeftPos;

    CHAR_INFO TextData[1024];
    if (Start<=X2 && End>=X1)
    {
      if (Start<X1)
        Start=X1;

      if (End>X2)
        End=X2;

      Length=End-Start+1;

      if(Length < X2)
        Length-=CorrectPos;

      if (Length > 0 && Length < sizeof(TextData))
      {
        ScrBuf.Read(Start,Y1,End,Y1,TextData,sizeof(TextData));

        SelColor0=SelColor;

        if(SelColor >= COL_FIRSTPALETTECOLOR)
          SelColor0=Palette[SelColor-COL_FIRSTPALETTECOLOR];

        for (I=0;I < Length;I++)
          if (TextData[I].Attributes != SelColor0)
            TextData[I].Attributes=Attr;

        ScrBuf.Write(Start,Y1,TextData,Length);
      }
    }
  }
}

/* $ 24.09.2000 SVS $
  ������� Xlat - ������������� �� �������� QWERTY <-> ������
*/
void Edit::Xlat(BOOL All)
{
  //   ��� CmdLine - ���� ��� ���������, ����������� ��� ������
  if(All && SelStart == -1 && SelEnd == 0)
  {
    ::Xlat(Str,0,StrLength(Str),TableSet,Opt.XLat.Flags);
    Show();
    return;
  }

  if(SelStart != -1 && SelStart != SelEnd)
  {
    if(SelEnd == -1)
      SelEnd=StrLength(Str);
    ::Xlat(Str,SelStart,SelEnd,TableSet,Opt.XLat.Flags);
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
   int start=CurPos, end, StrSize=StrLength(Str);
   BOOL DoXlat=TRUE;

   /* $ 12.01.2004 IS
      ��� ��������� � WordDiv ���������� IsWordDiv, � �� strchr, �.�.
      ������� ��������� ����� ���������� �� ��������� WordDiv (������� OEM)
   */
   if(IsWordDiv(TableSet,Opt.XLat.strWordDivForXlat,Str[start]))
   {
      if(start) start--;
      DoXlat=(!IsWordDiv(TableSet,Opt.XLat.strWordDivForXlat,Str[start]));
   }

   if(DoXlat)
   {
    while(start>=0 && !IsWordDiv(TableSet,Opt.XLat.strWordDivForXlat,Str[start]))
      start--;
    start++;
    end=start+1;
    while(end<StrSize && !IsWordDiv(TableSet,Opt.XLat.strWordDivForXlat,Str[end]))
      end++;
    ::Xlat(Str,start,end,TableSet,Opt.XLat.Flags);
    Show();
   }
  }
}


/* $ 15.11.2000 KM
   ���������: �������� �� ������ � �����������
   �������� ��������, ������������ ������
*/
int Edit::KeyMatchedMask(int Key)
{
  int Inserted=FALSE;
  if (Mask[CurPos]==EDMASK_ANY)
    Inserted=TRUE;
  else if (Mask[CurPos]==EDMASK_DSS && (iswdigit(Key) || Key==L' ' || Key==L'-'))
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
  return (Chr==EDMASK_ANY || Chr==EDMASK_DIGIT || Chr==EDMASK_DSS || Chr==EDMASK_ALPHA || Chr==EDMASK_HEX)?TRUE:FALSE;
}

void Edit::SetDialogParent(DWORD Sets)
{
  if((Sets&(FEDITLINE_PARENT_SINGLELINE|FEDITLINE_PARENT_MULTILINE)) == (FEDITLINE_PARENT_SINGLELINE|FEDITLINE_PARENT_MULTILINE) ||
     (Sets&(FEDITLINE_PARENT_SINGLELINE|FEDITLINE_PARENT_MULTILINE)) == 0)
    Flags.Clear(FEDITLINE_PARENT_SINGLELINE|FEDITLINE_PARENT_MULTILINE);
  else if(Sets&FEDITLINE_PARENT_SINGLELINE)
  {
    Flags.Clear(FEDITLINE_PARENT_MULTILINE);
    Flags.Set(FEDITLINE_PARENT_SINGLELINE);
  }
  else if(Sets&FEDITLINE_PARENT_MULTILINE)
  {
    Flags.Clear(FEDITLINE_PARENT_SINGLELINE);
    Flags.Set(FEDITLINE_PARENT_MULTILINE);
  }
}


SystemCPEncoder::SystemCPEncoder (int nCodePage)
{
	m_nCodePage = nCodePage;
	m_nRefCount = 1;

	m_strName.Format (L"codepage - %d", m_nCodePage);
}

SystemCPEncoder::~SystemCPEncoder ()
{
}

int __stdcall SystemCPEncoder::AddRef ()
{
	return ++m_nRefCount;
}

int __stdcall SystemCPEncoder::Release ()
{
	if ( --m_nRefCount == 0 )
	{
		delete this;
		return 0;
	}

	return m_nRefCount;
}

const wchar_t* __stdcall SystemCPEncoder::GetName()
{
	return (const wchar_t*)m_strName;
}

int __stdcall SystemCPEncoder::Encode (
		const char *lpString,
		int nLength,
		wchar_t *lpwszResult,
		int nResultLength
		)
{
	int length = MultiByteToWideChar (m_nCodePage, 0, lpString, nLength, NULL, 0);

	if ( lpwszResult )
		length = MultiByteToWideChar (m_nCodePage, 0, lpString, nLength, lpwszResult, nResultLength);

	return length;
}

int __stdcall SystemCPEncoder::Decode (
		const wchar_t *lpwszString,
		int nLength,
		char *lpResult,
		int nResultLength
		)
{
	int length = WideCharToMultiByte (m_nCodePage, 0, lpwszString, nLength, NULL, 0, NULL, NULL);

	if ( lpResult )
		length = WideCharToMultiByte (m_nCodePage, 0, lpwszString, nLength, lpResult, nResultLength, NULL, NULL);

	return length;
}

int __stdcall SystemCPEncoder::Transcode (
		const wchar_t *lpwszString,
		int nLength,
		ICPEncoder *pFrom,
		wchar_t *lpwszResult,
		int nResultLength
		)
{
	int length = pFrom->Decode (lpwszString, nLength, NULL, 0);

	char *lpDecoded = (char *)xf_malloc (length);

	if ( lpDecoded )
    {
		pFrom->Decode (lpwszString, nLength, lpDecoded, length);

		length = Encode (lpDecoded, length, NULL, 0);

		if ( lpwszResult )
			length = Encode (lpDecoded, length, lpwszResult, nResultLength);

		xf_free (lpDecoded);

		return length;
	}

	return -1;
}
