/*
vmenu.cpp

������� ������������ ����
  � ��� ��:
    * ������ � DI_COMBOBOX
    * ������ � DI_LISTBOX
    * ...
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

#include "vmenu.hpp"
#include "keyboard.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "macroopcode.hpp"
#include "colors.hpp"
#include "chgprior.hpp"
#include "dialog.hpp"
#include "savescr.hpp"
#include "ctrlobj.hpp"
#include "manager.hpp"
#include "constitle.hpp"
#include "syslog.hpp"
#include "interf.hpp"
#include "palette.hpp"
#include "config.hpp"
#include "processname.hpp"

VMenu::VMenu(const wchar_t *Title,       // ��������� ����
             MenuDataEx *Data, // ������ ����
             int ItemCount,     // ���������� ������� ����
             int MaxHeight,     // ������������ ������
             DWORD Flags,       // ����� ScrollBar?
             FARWINDOWPROC Proc,    // ����������
             Dialog *ParentDialog
             )  // �������� ��� ListBox
{
  int I;
  SetDynamicallyBorn(false);

  VMenu::VMFlags.Set(Flags|VMENU_MOUSEREACTION);
  VMenu::VMFlags.Clear(VMENU_MOUSEDOWN);

/*& 28.05.2001 OT ��������� ����������� ����� �� ����� ������� ���� */
//  FrameFromLaunched=FrameManager->GetCurrentFrame();
//  FrameFromLaunched->LockRefresh();

  VMenu::ParentDialog=ParentDialog;
  /* $ 03.06.2001 KM
     ! ������ ��������� ��������� ����� VMENU_WRAPMODE, � ���������
       ������ ��� �������� ���� ��������� �������� _������_, ���
       �� ������ ������.
  */
  VMFlags.Set(VMENU_UPDATEREQUIRED);
  VMFlags.Clear(VMENU_SHOWAMPERSAND);
  TopPos=0;
  SaveScr=NULL;
  OldTitle=NULL;

  GetCursorType(PrevCursorVisible,PrevCursorSize);

  if(!Proc) // ������� ������ ���� ������!!!
    Proc=(FARWINDOWPROC)VMenu::DefMenuProc;
  VMenuProc=Proc;

  if (Title!=NULL)
    strTitle = Title;
  else
    strTitle = L"";

  strBottomTitle = L"";

  VMenu::Item=NULL;
  VMenu::ItemCount=0;

  VMenu::LastAddedItem = 0;

  /* $ 01.08.2000 SVS
   - Bug � ������������, ���� �������� NULL ��� Title
  */
  /* $ 30.11.2001 DJ
     �������������� ����� ���, ��� ��������� ������
  */
  MaxLength=(int)strTitle.GetLength ()+2;

  VMenu::ItemHiddenCount=0;

  MenuItemEx NewItem;
  for (I=0; I < ItemCount; I++)
  {
    NewItem.Clear ();

		if(!IsPtr(Data[I].Name))
      NewItem.strName = MSG((int)(DWORD_PTR)Data[I].Name);
    else
      NewItem.strName = Data[I].Name;
    //NewItem.AmpPos=-1;
    NewItem.AccelKey=Data[I].AccelKey;
    NewItem.Flags=Data[I].Flags;
    AddItem(&NewItem);
  }

  BoxType=DOUBLE_BOX;
  for (SelectPos=-1,I=0;I<ItemCount;I++)
  {
    int Length=(int)Item[I]->strName.GetLength();
    if (Length>MaxLength)
      MaxLength=Length;
    if ((Item[I]->Flags&LIF_SELECTED) && !(Item[I]->Flags&(LIF_DISABLE | LIF_HIDDEN)))
      SelectPos=I;
  }

  VMFlags.Clear(VMENU_SELECTPOSNONE);
  if(SelectPos < 0)
    SelectPos=SetSelectPos(0,1);
  if(SelectPos < 0)
  {
    VMFlags.Set(VMENU_SELECTPOSNONE);
    SelectPos=0;
  }

  SetMaxHeight(MaxHeight);
  /* $ 28.07.2000 SVS
     ��������� ���� �� ���������
  */
  SetColors(NULL);
  if (!VMFlags.Check(VMENU_LISTBOX) && CtrlObject!=NULL)
  {
    PrevMacroMode=CtrlObject->Macro.GetMode();
    if (PrevMacroMode!=MACRO_MAINMENU &&
        PrevMacroMode!=MACRO_DIALOG &&
        PrevMacroMode!=MACRO_USERMENU)
      CtrlObject->Macro.SetMode(MACRO_MENU);
  }

  if (!VMFlags.Check(VMENU_LISTBOX))
    FrameManager->ModalizeFrame(this);

	Used=new bool[WCHAR_MAX];
}

VMenu::~VMenu()
{
  if (!VMFlags.Check(VMENU_LISTBOX) && CtrlObject!=NULL)
    CtrlObject->Macro.SetMode(PrevMacroMode);
  Hide();
  DeleteItems();
	delete[] Used;

/*& 28.05.2001 OT ��������� ����������� ������, � ������� ����������� ��� ���� */
//  FrameFromLaunched->UnlockRefresh();
  SetCursorType(PrevCursorVisible,PrevCursorSize);

  if (!VMFlags.Check(VMENU_LISTBOX))
  {
    FrameManager->UnmodalizeFrame(this);
    FrameManager->RefreshFrame();
  }
}

void VMenu::Hide()
{
  CriticalSectionLock Lock(CS);

  ChangePriority ChPriority(THREAD_PRIORITY_NORMAL);

  if(!VMFlags.Check(VMENU_LISTBOX) && SaveScr)
  {
    delete SaveScr;
    SaveScr=NULL;
    ScreenObject::Hide();
  }

  Y2=-1;
//  X2=-1;

  VMFlags.Set(VMENU_UPDATEREQUIRED);
  if(OldTitle)
  {
    delete OldTitle;
    OldTitle=NULL;
  }
}


void VMenu::Show()
{
  CriticalSectionLock Lock(CS);

  //int OldX1 = X1, OldY1 = Y1, OldX2 = X2, OldY2 = Y2;

  if(VMFlags.Check(VMENU_LISTBOX))
  {
    if (VMFlags.Check(VMENU_SHOWNOBOX))
      BoxType=NO_BOX;
    else if (VMFlags.Check (VMENU_LISTHASFOCUS))
      BoxType=SHORT_DOUBLE_BOX;
    else
      BoxType=SHORT_SINGLE_BOX;
  }

	if(!VMFlags.Check(VMENU_LISTBOX))
	{
		int AutoCenter=FALSE,AutoHeight=FALSE;

		if(!VMFlags.Check(VMENU_COMBOBOX))
		{
			if (X1==-1)
			{
				X1=(ScrX-MaxLength-4)/2;
				AutoCenter=TRUE;
			}

			if(X1<2)
				X1=2;
			if (X2<=0)
				X2=X1+MaxLength+4+((VMFlags.Check(VMENU_COMBOBOX)||VMFlags.Check(VMENU_LISTBOX))?0:2);

			if (!AutoCenter && X2 > ScrX-4+2*(BoxType==SHORT_DOUBLE_BOX || BoxType==SHORT_SINGLE_BOX))
			{
				X1+=ScrX-4-X2;
				X2=ScrX-4;
				if (X1<2)
				{
					X1=2;
					X2=ScrX-2;
				}
			}
			if (X2>ScrX-2)
				X2=ScrX-2;

			if (Y1==-1)
			{
				if (MaxHeight!=0 && MaxHeight<GetShowItemCount()) //???
					Y1=(ScrY-MaxHeight-2)/2;
				else
					if ((Y1=(ScrY-GetShowItemCount()-2)/2)<0) // ???
						Y1=0;
				AutoHeight=TRUE;
			}
		}
		if (Y2<=0)
		{
			if (MaxHeight!=0 && MaxHeight<GetShowItemCount()) //???
				Y2=Y1+MaxHeight+1;
			else
				Y2=Y1+GetShowItemCount()+1; //???
		}
		if (Y2>ScrY)
			Y2=ScrY;
		if (AutoHeight && Y1<3 && Y2>ScrY-3)
		{
			Y1=2;
			Y2=ScrY-2;
		}
	}


  if (X2>X1 && Y2+(VMFlags.Check(VMENU_SHOWNOBOX)?1:0)>Y1)
  {
  /*  if ( (OldX1 != X1) ||
         (OldY1 != Y1) ||
         (OldX2 != X2) ||
         (OldY2 != Y2) )
       VMFlags.Clear (VMENU_DISABLEDRAWBACKGROUND);*/

    if(SelectPos == -1)
      SelectPos=SetSelectPos(0,1);
//_SVS(SysLog(L"VMenu::Show()"));
    if(!VMFlags.Check(VMENU_LISTBOX))
      ScreenObject::Show();
//      Show();
    else
    {
      VMFlags.Set(VMENU_UPDATEREQUIRED);
      DisplayObject();
    }
  }
}

/* $ 28.07.2000 SVS
   ����������� ������� � ������ VMenu::Colors[] -
      �������� ��������� �� VMenu::Colors[]
*/
void VMenu::DisplayObject()
{
  CriticalSectionLock Lock(CS);
//_SVS(SysLog(L"VMFlags&VMENU_UPDATEREQUIRED=%d",VMFlags.Check(VMENU_UPDATEREQUIRED)));
//  if (!(VMFlags&VMENU_UPDATEREQUIRED))
//    return;
  ChangePriority ChPriority(THREAD_PRIORITY_NORMAL);
  VMFlags.Clear(VMENU_UPDATEREQUIRED);
  Modal::ExitCode=-1;

  SetCursorType(0,10);

  if (!VMFlags.Check(VMENU_LISTBOX) && SaveScr==NULL)
  {
    if (!VMFlags.Check(VMENU_DISABLEDRAWBACKGROUND) && !(BoxType==SHORT_DOUBLE_BOX || BoxType==SHORT_SINGLE_BOX))
      SaveScr=new SaveScreen(X1-2,Y1-1,X2+4,Y2+2);
    else
      SaveScr=new SaveScreen(X1,Y1,X2+2,Y2+1);
  }
  if (!VMFlags.Check(VMENU_DISABLEDRAWBACKGROUND))
  {
    /* $ 23.07.2000 SVS
       ���� ��� ListBox �������
    */
    if (BoxType==SHORT_DOUBLE_BOX || BoxType==SHORT_SINGLE_BOX)
    {
      Box(X1,Y1,X2,Y2,VMenu::Colors[VMenuColorBox],BoxType);
      if(!VMFlags.Check(VMENU_LISTBOX|VMENU_ALWAYSSCROLLBAR))
      {
        MakeShadow(X1+2,Y2+1,X2+1,Y2+1);
        MakeShadow(X2+1,Y1+1,X2+2,Y2+1);
      }
    }
    else
    {
      if (BoxType!=NO_BOX)
        SetScreen(X1-2,Y1-1,X2+2,Y2+1,L' ',VMenu::Colors[VMenuColorBody]);
      else
        SetScreen(X1,Y1,X2,Y2,L' ',VMenu::Colors[VMenuColorBody]);
      if(!VMFlags.Check(VMENU_LISTBOX|VMENU_ALWAYSSCROLLBAR))
      {
        MakeShadow(X1,Y2+2,X2+3,Y2+2);
        MakeShadow(X2+3,Y1,X2+4,Y2+2);
      }
      if (BoxType!=NO_BOX)
        Box(X1,Y1,X2,Y2,VMenu::Colors[VMenuColorBox],BoxType);
    }

//    VMFlags.Set (VMENU_DISABLEDRAWBACKGROUND);
  }
  /* $ 03.06.2001 KM
     ! ������ DI_LISTBOX'� ����������� �������� ���������.
  */
  /* $ 23.02.2002 DJ
     �������� ����� ��������� �� �� ����� ���������, � �� �������� ������ ����!
  */
  if(!VMFlags.Check(VMENU_LISTBOX))
    DrawTitles();
  ShowMenu(TRUE);
}

void VMenu::DrawTitles()
{
  CriticalSectionLock Lock(CS);

  int MaxTitleLength = X2-X1-2;
  int WidthTitle;

  if ( !strTitle.IsEmpty() )
  {
    if((WidthTitle=(int)strTitle.GetLength()) > MaxTitleLength)
      WidthTitle=MaxTitleLength-1;
    GotoXY(X1+(X2-X1-1-WidthTitle)/2,Y1);
    SetColor(VMenu::Colors[VMenuColorTitle]);
    mprintf(L" %*.*s ",WidthTitle,WidthTitle,(const wchar_t*)strTitle);
  }
  if ( !strBottomTitle.IsEmpty() )
  {
    if((WidthTitle=(int)strBottomTitle.GetLength()) > MaxTitleLength)
      WidthTitle=MaxTitleLength-1;
    GotoXY(X1+(X2-X1-1-WidthTitle)/2,Y2);
    SetColor(VMenu::Colors[VMenuColorTitle]);
    mprintf(L" %*.*s ",WidthTitle,WidthTitle,(const wchar_t*)strBottomTitle);
  }
}

int VMenu::GetVisualPos(int Pos)
{
	if (ItemCount == GetShowItemCount())
		return Pos;

	if (Pos < 0 || Pos > ItemCount) //???
		return 0;

	int v=0;

	for (int i=0; i < Pos; i++)
	{
		if(!(Item[i]->Flags&LIF_HIDDEN))
			v++;
	}

	return v;
}

/* $ 28.07.2000 SVS
   ����������� ������� � ������ VMenu::Colors[] -
      �������� ��������� �� VMenu::Colors[]
*/
void VMenu::ShowMenu(int IsParent)
{
  CriticalSectionLock Lock(CS);

  string strTmpStr;

  wchar_t BoxChar[2]={0};
  int Y,I;
  /* $ 23.02.2002 DJ
     ���� � ���� ���� ������� - ��� �� ������, ��� �� ���� �������� ���!
  */
  if (/*ItemCount==0 ||*/ X2<=X1 || Y2<=Y1)
  {
    if(!(VMFlags.Check(VMENU_SHOWNOBOX) && Y2==Y1))
      return;
  }
  ChangePriority ChPriority(THREAD_PRIORITY_NORMAL);

  int VisualSelectPos = GetVisualPos(SelectPos);

  // ��������� Top`�
  if(GetVisualPos(TopPos)+GetShowItemCount() >= Y2-Y1 && VisualSelectPos == GetShowItemCount()-1)
  {
    if (GetShowItemCount() != ItemCount)
    {
      int i=TopPos;
      for (int v=0; i > 0 && v < 1; i--)
      {
        if(!(Item[i]->Flags&LIF_HIDDEN))
          v++;
      }
      TopPos=i;
    }
    else
    {
    	TopPos--;
    }
  }
  if (TopPos<0)
    TopPos=0;

  /* $ 22.07.2001 KM
   - �� �������������� ��� ����� ��� ������ ������
     ShowMenu � ���������� TRUE, ��� ������ ��������
     ��������� ����.
  */
  /* $ 21.02.2002 DJ
     � ��� �������, �� ������� ������, ����� ��������� �����!
  */
  if (VMFlags.Check(VMENU_LISTBOX))
  {
    if (VMFlags.Check(VMENU_SHOWNOBOX))
      BoxType = NO_BOX;
    else if (VMFlags.Check (VMENU_LISTHASFOCUS))
      BoxType = SHORT_DOUBLE_BOX;
    else
      BoxType = SHORT_SINGLE_BOX;
  }

  if(VMFlags.Check(VMENU_LISTBOX))
  {
    if((!IsParent || !GetShowItemCount()))
    {
      if(GetShowItemCount())
        BoxType=VMFlags.Check(VMENU_SHOWNOBOX)?NO_BOX:SHORT_SINGLE_BOX;
      SetScreen(X1,Y1,X2,Y2,L' ',VMenu::Colors[VMenuColorBody]);
    }
    if (BoxType!=NO_BOX)
      Box(X1,Y1,X2,Y2,VMenu::Colors[VMenuColorBox],BoxType);
    DrawTitles();
  }

  switch(BoxType)
  {
    case NO_BOX:
      *BoxChar=L' ';
      break;
    case SINGLE_BOX:
    case SHORT_SINGLE_BOX:
      *BoxChar=BoxSymbols[BS_V1]; // |
      break;
    case DOUBLE_BOX:
    case SHORT_DOUBLE_BOX:
      *BoxChar=BoxSymbols[BS_V2]; // ||
      break;
  }

  if (GetShowItemCount() <= 0)
    return;
  if (VisualSelectPos>=0 && VisualSelectPos < GetShowItemCount())
  {
    if(Item[SelectPos]->Flags&(LIF_DISABLE | LIF_HIDDEN))
      Item[SelectPos]->Flags&=~LIF_SELECTED;
    else
      Item[SelectPos]->Flags|=LIF_SELECTED;
  }

  /* $ 02.12.2001 KM
     ! ��������������, ���� �����, �������� "�������" �������.
  */
  if(VMFlags.Check(VMENU_AUTOHIGHLIGHT|VMENU_REVERSEHIGHLIGHT))
    AssignHighlights(VMFlags.Check(VMENU_REVERSEHIGHLIGHT));

  /* $ 21.07.2001 KM
   ! ����������� ��������� ���� � ������ VMENU_SHOWNOBOX.
  */
  if (VisualSelectPos > GetVisualPos(TopPos)+((BoxType!=NO_BOX)?Y2-Y1-2:Y2-Y1))
  {
    if (GetShowItemCount() != ItemCount)
    {
      int p=VisualSelectPos-((BoxType!=NO_BOX)?Y2-Y1-2:Y2-Y1);
      int i=0;
      for (int v=0; i < ItemCount && v < p; i++)
      {
        if(!(Item[i]->Flags&LIF_HIDDEN))
          v++;
      }
      TopPos=i;
    }
    else
    {
      TopPos=SelectPos-((BoxType!=NO_BOX)?Y2-Y1-2:Y2-Y1);
    }
  }
  if (SelectPos < TopPos)
    TopPos=SelectPos;
  if(TopPos<0)
    TopPos=0;

  for (Y=Y1+((BoxType!=NO_BOX)?1:0),I=TopPos;Y<((BoxType!=NO_BOX)?Y2:Y2+1);Y++,I++)
  {
    GotoXY(X1,Y);
    if (I < ItemCount)
    {
      if(Item[I]->Flags&LIF_HIDDEN)
      {
        Y--;
        continue;
      }
      if (Item[I]->Flags&LIF_SEPARATOR)
      {
        int SepWidth=X2-X1+1;
				wchar_t *TmpStr=strTmpStr.GetBuffer(SepWidth+1);
        wchar_t *Ptr=TmpStr+1;
				MakeSeparator(SepWidth,TmpStr,BoxType==NO_BOX?0:(BoxType==SINGLE_BOX||BoxType==SHORT_SINGLE_BOX?2:1));

        if (I>0 && I<ItemCount-1 && SepWidth>3)
          for (unsigned int J=0;Ptr[J+3]!=0;J++)
          {
						wchar_t PrevItem = (Item[I-1]->strName.GetLength()>=J) ? Item[I-1]->strName.At(J) : 0;
						wchar_t NextItem = (Item[I+1]->strName.GetLength()>=J) ? Item[I+1]->strName.At(J) : 0;

						if (PrevItem==0 && NextItem==0)
              break;

						if (PrevItem==BoxSymbols[BS_V1])
            {
              int Correction=0;
              if (!VMFlags.Check(VMENU_SHOWAMPERSAND) && wmemchr(Item[I-1]->strName,L'&',J)!=NULL)
                Correction=1;
							if (NextItem==BoxSymbols[BS_V1])
								Ptr[J-Correction+(BoxType==NO_BOX?1:2)]=BoxSymbols[BS_C_H1V1];
							else
								Ptr[J-Correction+(BoxType==NO_BOX?1:2)]=BoxSymbols[BS_B_H1V1];
            }
						else if (NextItem==BoxSymbols[BS_V1])
						{
							int Correction=0;
							if (!VMFlags.Check(VMENU_SHOWAMPERSAND) && wmemchr(Item[I+1]->strName,L'&',J)!=NULL)
								Correction=1;
							Ptr[J-Correction+(BoxType==NO_BOX?1:2)]=BoxSymbols[BS_T_H1V1];
						}
          }
        //Text(X1,Y,VMenu::Colors[VMenuColorSeparator],TmpStr); // VMenuColorBox
        SetColor(VMenu::Colors[VMenuColorSeparator]);
        BoxText(TmpStr,FALSE);

        if ( !Item[I]->strName.IsEmpty() )
        {
          int ItemWidth=(int)Item[I]->strName.GetLength();
          if(ItemWidth > X2-X1-3)  // 1 ???
            ItemWidth=X2-X1-3;
          GotoXY(X1+(X2-X1-1-ItemWidth)/2,Y);
          mprintf(L" %*.*s ",ItemWidth,ItemWidth,(const wchar_t*)Item[I]->strName);
          //??????
        }
				strTmpStr.ReleaseBuffer();
      }
      else
      {
        if (BoxType!=NO_BOX)
        {
          SetColor(VMenu::Colors[VMenuColorBox]);
          BoxText(BoxChar);
          GotoXY(X2,Y);
          BoxText(BoxChar);
        }

//        if ((Item[I].Flags&LIF_SELECTED) && !(Item[I].Flags&LIF_DISABLE))
//          SetColor(VMenu::Colors[VMenuColorSelected]);
//        else
//          SetColor(VMenu::Colors[(Item[I].Flags&LIF_DISABLE?9:3)]);

        if (BoxType!=NO_BOX)
        {
          /*
          if(Item[I]->ShowPos > 0)
          {
            Text(X1,Y,VMenu::Colors[Item[I]->Flags&LIF_SELECTED?VMenuColorHSelect:VMenuColorHilite],L"{");
          }
          */
          GotoXY(X1+1,Y);
        }
        else
          GotoXY(X1,Y);

        if ((Item[I]->Flags&LIF_SELECTED) && !(Item[I]->Flags&LIF_DISABLE))
          SetColor(VMenu::Colors[Item[I]->Flags&LIF_GRAYED?VMenuColorSelGrayed:VMenuColorSelected]);
        else
          SetColor(VMenu::Colors[Item[I]->Flags&LIF_DISABLE?VMenuColorDisabled:(Item[I]->Flags&LIF_GRAYED?VMenuColorGrayed:VMenuColorText)]);

				string strMenuLine;

				wchar_t CheckMark = L' ';
				if (Item[I]->Flags & LIF_CHECKED)
				{
					if (!(Item[I]->Flags & 0x0000FFFF))
						CheckMark = 0x221A;
					else
						CheckMark = static_cast<wchar_t>(Item[I]->Flags & 0x0000FFFF);
				}
				strMenuLine.Append(CheckMark);

				strMenuLine.Append(L' '); // left scroller (<<) placeholder

				int ShowPos = HiFindRealPos(Item[I]->strName, Item[I]->ShowPos, VMFlags.Check(VMENU_SHOWAMPERSAND));
				string strMItemPtr(Item[I]->strName.CPtr() + ShowPos);
				int strMItemPtrLen;
				if (VMFlags.Check(VMENU_SHOWAMPERSAND))
					strMItemPtrLen = static_cast<int>(strMItemPtr.GetLength());
				else
					strMItemPtrLen = HiStrlen(strMItemPtr);

				// fit menu string into available space
				if (strMItemPtrLen > GetMaxLineWidth())
				{
					strMItemPtr.SetLength(HiFindRealPos(strMItemPtr, GetMaxLineWidth(), VMFlags.Check(VMENU_SHOWAMPERSAND)));
				}

				// set highlight
				if (!VMFlags.Check(VMENU_SHOWAMPERSAND))
				{
				  int AmpPos = Item[I]->AmpPos - ShowPos;
					if((AmpPos >= 0) && (static_cast<size_t>(AmpPos) < strMItemPtr.GetLength()) && (strMItemPtr.At(AmpPos) != L'&'))
					{
						string strEnd = strMItemPtr.CPtr() + AmpPos;
						strMItemPtr.SetLength(AmpPos);
						strMItemPtr += L"&";
						strMItemPtr += strEnd;
					}
				}

				strMenuLine.Append(strMItemPtr);

				if (Item[I]->Flags & MIF_SUBMENU)
				{
					// append padding if needed
					if (strMItemPtrLen < GetMaxLineWidth())
					{
						string strPadding;
						strPadding.Format(L"%*c", GetMaxLineWidth()-strMItemPtrLen, L' ');
						strMenuLine.Append(strPadding);
					}
					strMenuLine.Append(L' '); // right scroller (>>) placeholder
					strMenuLine.Append(L'\x25BA'); // sub menu arrow
				}

				// ��������� ������ ������ ��� ������!!!
				// ��� ���������� ������������ ������!!!
				wchar_t *TmpStr = strMenuLine.GetBuffer();
				wchar_t *TabPtr;
				while ((TabPtr = wcschr(TmpStr, L'\t')))
					*TabPtr=L' ';
				strMenuLine.ReleaseBuffer();

				int Col;

				if (!(Item[I]->Flags & LIF_DISABLE))
				{
					if (Item[I]->Flags & LIF_SELECTED)
						Col = VMenu::Colors[Item[I]->Flags & LIF_GRAYED ? VMenuColorSelGrayed : VMenuColorHSelect];
					else
						Col = VMenu::Colors[Item[I]->Flags & LIF_GRAYED ? VMenuColorGrayed : VMenuColorHilite];
				}
				else
					Col = VMenu::Colors[VMenuColorDisabled];

				if (VMFlags.Check(VMENU_SHOWAMPERSAND))
					Text(strMenuLine);
				else
					HiText(strMenuLine, Col);

				// ������� ��������� ��� NO_BOX
				mprintf(L"%*s",X2-WhereX()+(BoxType==NO_BOX?1:0),L"");

				SetColor(VMenu::Colors[(Item[I]->Flags&LIF_DISABLE)?VMenuColorArrowsDisabled:(Item[I]->Flags&LIF_SELECTED?VMenuColorArrowsSelect:VMenuColorArrows)]);

				if (/*BoxType!=NO_BOX && */Item[I]->ShowPos > 0)
				{
					GotoXY(X1+2,Y);
					BoxText(L'\xab'); // '<<'
				}

				if (strMItemPtrLen > GetMaxLineWidth())
				{
					//if ((VMFlags.Check(VMENU_LISTBOX|VMENU_ALWAYSSCROLLBAR) || Opt.ShowMenuScrollbar) && (((BoxType!=NO_BOX)?Y2-Y1-1:Y2-Y1+1)<ItemCount))
					//	GotoXY(WhereX()-1,Y);
					//else
						GotoXY(X1+2+GetMaxLineWidth()+1,Y);
					BoxText(L'\xbb');// '>>'
				}
			}
		}
    else
    {
      if (BoxType!=NO_BOX)
      {
        SetColor(VMenu::Colors[VMenuColorBox]);
        BoxText(BoxChar);
        GotoXY(X2,Y);
        BoxText(BoxChar);
        GotoXY(X1+1,Y);
      }
      else
        GotoXY(X1,Y);
      SetColor(VMenu::Colors[VMenuColorText]);
                                                     // ������� ��������� ��� NO_BOX
      mprintf(L"%*s",((BoxType!=NO_BOX)?X2-X1-1:X2-X1)+(BoxType==NO_BOX?1:0),L"");
    }
  }
  /* $ 28.06.2000 tran
       ���������� �������� ���� ������� � ���� ������ ���
       ��� ������
     $ 29.06.2000 SVS
       ���������� ScrollBar � ���� ���� �������� ����� Opt.ShowMenuScrollbar
     $ 18.07.2000 SVS
       + ������ ������� scrollbar ��� DI_LISTBOX & DI_COMBOBOX � �����������
         ��� ������������� ����
  */
  if (VMFlags.Check(VMENU_LISTBOX|VMENU_ALWAYSSCROLLBAR) || Opt.ShowMenuScrollbar)
  {
		SetColor(VMenu::Colors[VMenuColorScrollBar]);
		if (BoxType!=NO_BOX)
			ScrollBarEx(X2,Y1+1,Y2-Y1-1,GetVisualPos(TopPos),GetShowItemCount());
		else
			ScrollBarEx(X2,Y1,Y2-Y1+1,GetVisualPos(TopPos),GetShowItemCount());
  }
}

BOOL VMenu::UpdateRequired()
{
  CriticalSectionLock Lock(CS);

  return ((LastAddedItem>=TopPos && LastAddedItem<(TopPos+Y2-Y1)+2) || VMFlags.Check(VMENU_UPDATEREQUIRED));

  //+1 for real diff
  //+2 for scrollbar
}

BOOL VMenu::CheckKeyHiOrAcc(DWORD Key,int Type,int Translate)
{
  CriticalSectionLock Lock(CS);

  int I;
  MenuItemEx *CurItem;

  if(VMFlags.Check(VMENU_LISTBOX)) // �� ������� �������� EndLoop ��� ���������,
    EndLoop=FALSE;                 // ����� �� ����� �������� ������ � �������� ������

  for (I=0; I < ItemCount; I++)
  {
    CurItem = Item[I];

    if(!(CurItem->Flags&(LIF_DISABLE|LIF_HIDDEN)) &&
       (
         (!Type && CurItem->AccelKey && Key == CurItem->AccelKey) ||
         (Type && Dialog::IsKeyHighlighted(CurItem->strName,Key,Translate,CurItem->AmpPos))
       )
      )
    {
      Item[SelectPos]->Flags&=~LIF_SELECTED;
      CurItem->Flags|=LIF_SELECTED;
      SelectPos=I;
      ShowMenu(TRUE);
      if(!VMenu::ParentDialog && !(Item[SelectPos]->Flags&LIF_GRAYED))
      {
        Modal::ExitCode=I;
        EndLoop=TRUE;
      }
      break;
    }
  }
  return EndLoop==TRUE;
}

int FindNextVisualPos(const wchar_t *Str, int Pos, int Direct)
{
	/*
			&&      = '&'
			&&&     = '&'
								 ^H
			&&&&    = '&&'
			&&&&&   = '&&'
								 ^H
			&&&&&&  = '&&&'
	*/
	if (Str)
	{
		if (Direct < 0)
		{
			if (!Pos || Pos == 1)
				return 0;
			if (Str[Pos-1] != L'&')
			{
				if (Str[Pos-2] == L'&')
				{
					if (Pos-3 >= 0 && Str[Pos-3] == L'&')
						return Pos-1;
					return Pos-2;
				}
				return Pos-1;
			}
			else
			{
				if (Pos-3 >= 0 && Str[Pos-3] == L'&')
				{
					return Pos-3;
				}
				return Pos-2;
			}
		}
		else
		{
			if (!Str[Pos])
				return Pos+1;
			if (Str[Pos] == L'&')
			{
				if (Str[Pos+1] == L'&' && Str[Pos+2] == L'&')
					return Pos+3;
				return Pos+2;
			}
			else
			{
				return Pos+1;
			}
		}
	}

	return 0;
}

BOOL VMenu::ShiftItemShowPos(int Pos,int Direct)
{
	int _len;
	int ItemShowPos=Item[Pos]->ShowPos;

	if(VMFlags.Check(VMENU_SHOWAMPERSAND))
		_len=StrLength(Item[Pos]->strName);
	else
		_len=HiStrlen(Item[Pos]->strName);

	if(_len < GetMaxLineWidth() ||
			(Direct < 0 && ItemShowPos==0) ||
			(Direct > 0 && ItemShowPos > _len))
		return FALSE;

  if (VMFlags.Check(VMENU_SHOWAMPERSAND))
  {
		if(Direct < 0)
			ItemShowPos--;
		else
			ItemShowPos++;
	}
	else
	{
		ItemShowPos = FindNextVisualPos(Item[Pos]->strName,ItemShowPos,Direct);
	}

	if(ItemShowPos < 0)
		ItemShowPos=0;

	if (ItemShowPos + GetMaxLineWidth() > _len)
		ItemShowPos = _len - GetMaxLineWidth();

	if(ItemShowPos != Item[Pos]->ShowPos)
	{
		Item[Pos]->ShowPos=ItemShowPos;
		VMFlags.Set(VMENU_UPDATEREQUIRED);
		return TRUE;
	}
	return FALSE;
}



__int64 VMenu::VMProcess(int OpCode,void *vParam,__int64 iParam)
{
  switch(OpCode)
  {
    case MCODE_C_EMPTY:
      return (__int64)(ItemCount<=0);
    case MCODE_C_EOF:
      return (__int64)(SelectPos==ItemCount-1);
    case MCODE_C_BOF:
      return (__int64)(SelectPos==0);
    case MCODE_C_SELECTED:
      return (__int64)(ItemCount > 0 && SelectPos >= 0);
    case MCODE_V_ITEMCOUNT:
      return (__int64)GetShowItemCount(); // ????
    case MCODE_V_CURPOS:
      return (__int64)(SelectPos+1);

    case MCODE_F_MENU_CHECKHOTKEY:
    {
      const wchar_t *str = (const wchar_t *)vParam;
      if ( *str )
        return (__int64)((DWORD)CheckHighlights((WORD)*str));
      return _i64(0);
    }

    case MCODE_F_MENU_SELECT:
    {
      const wchar_t *str = (const wchar_t *)vParam;
      if ( *str )
      {
        string strTemp;
        int Res;
        for(int I=0; I < ItemCount; ++I)
        {
					MenuItemEx *_item=GetItemPtr(I);

          if(_item->Flags&LIF_HIDDEN) //???
            continue;

          Res=0;
          RemoveExternalSpaces(HiText2Str(strTemp,_item->strName));

          const wchar_t *p;
          switch(iParam)
          {
            case 0: // full compare
              Res=StrCmpI(strTemp,str)==0;
              break;
            case 1: // begin compare
              p=StrStrI(strTemp,str);
              Res=p==strTemp;
              break;
            case 2: // end compare
              p=RevStrStrI(strTemp,str);
              Res=p!=NULL && *(p+StrLength(str)) == 0;
              break;
            case 3: // in str
              Res=StrStrI(strTemp,str)!=NULL;
              break;
          }

          if(Res)
          {
            SelectPos=SetSelectPos(I,1);
            if(SelectPos != I)
            {
              SelectPos=SetSelectPos(SelectPos,1);
              return _i64(0);
            }
            ShowMenu(TRUE);
            return (__int64)(SelectPos+1);
          }
        }
      }
      return _i64(0);
    }

    case MCODE_F_MENU_GETHOTKEY:
    {
      if(iParam == _i64(-1))
        iParam=(__int64)SelectPos;

      if((int)iParam < ItemCount)  //???
        return (__int64)((DWORD)GetHighlights(GetItemPtr((int)iParam)));
      return _i64(0);
    }

  }

  return _i64(0);
}

int VMenu::ProcessKey(int Key)
{
  CriticalSectionLock Lock(CS);

  int I;

  if (Key==KEY_NONE || Key==KEY_IDLE)
    return(FALSE);


  if( Key == KEY_OP_PLAINTEXT)
  {
    const wchar_t *str = eStackAsString();
    if (!*str)
      return FALSE;
    Key=*str;
  }

  VMFlags.Set(VMENU_UPDATEREQUIRED);
  if (GetShowItemCount()==0)
    if (Key!=KEY_F1 && Key!=KEY_SHIFTF1 && Key!=KEY_F10 && Key!=KEY_ESC && Key!=KEY_ALTF9)
    {
      Modal::ExitCode=-1;
      return(FALSE);
    }

  if(!(((unsigned int)Key >= KEY_MACRO_BASE && (unsigned int)Key <= KEY_MACRO_ENDBASE) || ((unsigned int)Key >= KEY_OP_BASE && (unsigned int)Key <= KEY_OP_ENDBASE)))
  {
    DWORD S=Key&(KEY_CTRL|KEY_ALT|KEY_SHIFT|KEY_RCTRL|KEY_RALT);
    DWORD K=Key&(~(KEY_CTRL|KEY_ALT|KEY_SHIFT|KEY_RCTRL|KEY_RALT));

    if (K==KEY_MULTIPLY)
      Key=L'*'|S;
    else if (K==KEY_ADD)
      Key=L'+'|S;
    else if (K==KEY_SUBTRACT)
      Key=L'-'|S;
    else if (K==KEY_DIVIDE)
      Key=L'/'|S;
  }

  switch(Key)
  {
    case KEY_ALTF9:
      FrameManager->ProcessKey(KEY_ALTF9);
      break;

    case KEY_NUMENTER:
    case KEY_ENTER:
    {
      if(!VMenu::ParentDialog)
      {
        if(!(Item[SelectPos]->Flags&(LIF_DISABLE | LIF_HIDDEN | LIF_GRAYED)))
        {
          EndLoop=TRUE;
          Modal::ExitCode=SelectPos;
        }
      }
      break;
    }

    case KEY_ESC:
    case KEY_F10:
    {
      if(!VMenu::ParentDialog)
      {
        EndLoop=TRUE;
        Modal::ExitCode=-1;
      }
      break;
    }

    case KEY_HOME:         case KEY_NUMPAD7:
    case KEY_CTRLHOME:     case KEY_CTRLNUMPAD7:
    case KEY_CTRLPGUP:     case KEY_CTRLNUMPAD9:
    {
      SelectPos=SetSelectPos(0,1);
      ShowMenu(TRUE);
      break;
    }

    case KEY_END:          case KEY_NUMPAD1:
    case KEY_CTRLEND:      case KEY_CTRLNUMPAD1:
    case KEY_CTRLPGDN:     case KEY_CTRLNUMPAD3:
    {
      SelectPos=SetSelectPos(ItemCount-1,-1);
      ShowMenu(TRUE);
      break;
    }

    case KEY_PGUP:         case KEY_NUMPAD9:
    {
      /* $ 22.07.2001 KM
       ! ����������� ���������� �������� �� PgUp/PgDn
         � ������������� ������ VMENU_SHOWNOBOX (NO_BOX)
      */
      int y=((BoxType!=NO_BOX)?Y2-Y1-1:Y2-Y1);
      if (GetShowItemCount() != ItemCount)
      {
        int p=GetVisualPos(SelectPos)-y;
        int i=0;
        for (int v=0; i < ItemCount && v < p; i++)
        {
          if(!(Item[i]->Flags&LIF_HIDDEN))
            v++;
        }
        I=i;
      }
      else
      {
        I=SelectPos-y;
      }
      if(I < 0)
        I=0;
      SelectPos=SetSelectPos(I,1);
      ShowMenu(TRUE);
      break;
    }

    case KEY_PGDN:         case KEY_NUMPAD3:
    {
      int y=((BoxType!=NO_BOX)?Y2-Y1-1:Y2-Y1);
      if (GetShowItemCount() != ItemCount)
      {
        int i=SelectPos;
        for (int v=0; i < ItemCount && v < y; i++)
        {
          if(!(Item[i]->Flags&LIF_HIDDEN))
            v++;
        }
        while (i<ItemCount && (Item[i]->Flags&LIF_HIDDEN))
          i++;
        I=i;
      }
      else
      {
        I=SelectPos+y;
      }
      if(I >= ItemCount)
        I=ItemCount-1;
      SelectPos=SetSelectPos(I,-1);
      ShowMenu(TRUE);
      break;
    }

    case KEY_ALTHOME:           case KEY_NUMPAD7|KEY_ALT:
    case KEY_ALTEND:            case KEY_NUMPAD1|KEY_ALT:
    {
      if(Key == KEY_ALTHOME || Key == (KEY_NUMPAD7|KEY_ALT))
      {
        for(I=0; I < ItemCount; ++I)
          Item[I]->ShowPos=0;
      }
      else
      {
        int _len;
        for(I=0; I < ItemCount; ++I)
        {
          if(VMFlags.Check(VMENU_SHOWAMPERSAND))
            _len=StrLength(Item[I]->strName);
          else
            _len=HiStrlen(Item[I]->strName);

          if(_len >= GetMaxLineWidth())
            Item[I]->ShowPos = _len - GetMaxLineWidth();
        }
      }

      ShowMenu(TRUE);
      break;
    }

		case KEY_ALTLEFT:  case KEY_NUMPAD4|KEY_ALT: case KEY_MSWHEEL_LEFT:
		case KEY_ALTRIGHT: case KEY_NUMPAD6|KEY_ALT: case KEY_MSWHEEL_RIGHT:
    {
      BOOL NeedRedraw=FALSE;
      for(I=0; I < ItemCount; ++I)
				if(ShiftItemShowPos(I,(Key == KEY_ALTLEFT || Key == (KEY_NUMPAD4|KEY_ALT) || Key == KEY_MSWHEEL_LEFT)?-1:1))
          NeedRedraw=TRUE;

      if(NeedRedraw)
        ShowMenu(TRUE);
      break;
    }

    case KEY_ALTSHIFTLEFT:      case KEY_NUMPAD4|KEY_ALT|KEY_SHIFT:
    case KEY_ALTSHIFTRIGHT:     case KEY_NUMPAD6|KEY_ALT|KEY_SHIFT:
    {
      if(ShiftItemShowPos(SelectPos,(Key == KEY_ALTSHIFTLEFT || Key == (KEY_NUMPAD4|KEY_ALT|KEY_SHIFT))?-1:1))
        ShowMenu(TRUE);
      break;
    }

    case KEY_MSWHEEL_UP: // $ 27.04.2001 VVM - ��������� KEY_MSWHEEL_XXXX
    case KEY_LEFT:         case KEY_NUMPAD4:
    case KEY_UP:           case KEY_NUMPAD8:
    {
      SelectPos=SetSelectPos(SelectPos-1,-1);
      ShowMenu(TRUE);
      break;
    }

    case KEY_MSWHEEL_DOWN: // $ 27.04.2001 VVM + ��������� KEY_MSWHEEL_XXXX
    case KEY_RIGHT:        case KEY_NUMPAD6:
    case KEY_DOWN:         case KEY_NUMPAD2:
    {
      SelectPos=SetSelectPos(SelectPos+1,1);
      ShowMenu(TRUE);
      break;
    }

    case KEY_TAB:
    case KEY_SHIFTTAB:
    default:
    {
      int OldSelectPos=SelectPos;

      if(!CheckKeyHiOrAcc(Key,0,0))
      {
        if(Key == KEY_SHIFTF1 || Key == KEY_F1)
        {
          if(VMenu::ParentDialog)
            ;//VMenu::ParentDialog->ProcessKey(Key);
          else
            ShowHelp();
          break;
        }
        else
        {
          if(!CheckKeyHiOrAcc(Key,1,FALSE))
            CheckKeyHiOrAcc(Key,1,TRUE);
        }
      }

      if(VMenu::ParentDialog && OldSelectPos!=SelectPos && Dialog::SendDlgMessage((HANDLE)ParentDialog,DN_LISTHOTKEY,DialogItemID,SelectPos))
      {
        Item[SelectPos]->Flags&=~LIF_SELECTED;
        Item[OldSelectPos]->Flags|=LIF_SELECTED;
        SelectPos=OldSelectPos;
        ShowMenu(TRUE);
      }

      return(FALSE);
    }
  }
  return(TRUE);
}

int VMenu::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
  CriticalSectionLock Lock(CS);

  int MsPos,MsX,MsY;

  VMFlags.Set(VMENU_UPDATEREQUIRED);
  if (GetShowItemCount()==0)
  {
    if(MouseEvent->dwButtonState && MouseEvent->dwEventFlags==0)
      EndLoop=TRUE;
    Modal::ExitCode=-1;
    return(FALSE);
  }

  MsX=MouseEvent->dwMousePosition.X;
  MsY=MouseEvent->dwMousePosition.Y;

	if(MouseEvent->dwButtonState&FROM_LEFT_1ST_BUTTON_PRESSED&&(MsX==X1+2||MsX==X2-1-((VMFlags.Check(VMENU_COMBOBOX)||VMFlags.Check(VMENU_LISTBOX))?0:2)))
	{
		while(IsMouseButtonPressed())
			ProcessKey(MsX==X1+2?KEY_ALTLEFT:KEY_ALTRIGHT);
		return TRUE;
	}

  /* $ 06.07.2000 tran
     + mouse support for menu scrollbar
  */

  int SbY1=((BoxType!=NO_BOX)?Y1+1:Y1), SbY2=((BoxType!=NO_BOX)?Y2-1:Y2);

  /* $ 12.10.2001 VVM
    ! ���� �� � ��� ���������? */
  int bShowScrollBar = FALSE;
  if (VMFlags.Check(VMENU_LISTBOX|VMENU_ALWAYSSCROLLBAR) || Opt.ShowMenuScrollbar)
    bShowScrollBar = TRUE;

  if (bShowScrollBar && MsX==X2 && ((BoxType!=NO_BOX)?Y2-Y1-1:Y2-Y1+1)<ItemCount &&
      (MouseEvent->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) )
  {
    if (MsY==SbY1)
    {
      while (IsMouseButtonPressed())
      {
        /* $ 11.12.2000 tran
           ��������� ����� �� ������ ������� ����
        */
        if (SelectPos!=0)
            ProcessKey(KEY_UP);
        ShowMenu(TRUE);
      }
      return(TRUE);
    }
    if (MsY==SbY2)
    {
      while (IsMouseButtonPressed())
      {
        /* $ 11.12.2000 tran
           ��������� ����� �� ������ ������� ����
        */
        if (SelectPos!=ItemCount-1)
            ProcessKey(KEY_DOWN);
        ShowMenu(TRUE);
      }
      return(TRUE);
    }
    if (MsY>SbY1 && MsY<SbY2)
    {
      int SbHeight;
      int Delta=0;
      while (IsMouseButtonPressed())
      {
        SbHeight=Y2-Y1-2;
        MsPos=(ItemCount-1)*(MouseY-Y1)/(SbHeight);
        if(MsPos >= ItemCount)
        {
          MsPos=ItemCount-1;
          Delta=-1;
        }
        if(MsPos < 0)
        {
          MsPos=0;
          Delta=1;
        }
        if(!(Item[MsPos]->Flags&LIF_SEPARATOR) && !(Item[MsPos]->Flags&(LIF_DISABLE | LIF_HIDDEN)))
          SelectPos=SetSelectPos(MsPos,Delta); //??
        ShowMenu(TRUE);
      }
      return(TRUE);
    }
  }

  // dwButtonState & 3 - Left & Right button
  if (BoxType!=NO_BOX && (MouseEvent->dwButtonState & 3) && MsX>X1 && MsX<X2)
  {
    if (MsY==Y1)
    {
      while (MsY==Y1 && SelectPos>0 && IsMouseButtonPressed())
        ProcessKey(KEY_UP);
      return(TRUE);
    }
    if (MsY==Y2)
    {
      while (MsY==Y2 && SelectPos<ItemCount-1 && IsMouseButtonPressed())
        ProcessKey(KEY_DOWN);
      return(TRUE);
    }
  }

  if ((BoxType!=NO_BOX)?
      (MsX>X1 && MsX<X2 && MsY>Y1 && MsY<Y2):
      (MsX>=X1 && MsX<=X2 && MsY>=Y1 && MsY<=Y2))
  {
    MsPos=TopPos+((BoxType!=NO_BOX)?MsY-Y1-1:MsY-Y1);
    if (MsPos<ItemCount && !(Item[MsPos]->Flags&LIF_SEPARATOR) && !(Item[MsPos]->Flags&(LIF_DISABLE | LIF_HIDDEN)))
    {
      if (MouseX!=PrevMouseX || MouseY!=PrevMouseY || MouseEvent->dwEventFlags==0)
      {
/* TODO:

   ��� ��������� ��� ���������� ���������� ������ "�� � ����� ����" - ����� �������
   ��������� ������ (�������) ������ �� �����...

        if(!CheckFlags(VMENU_LISTBOX|VMENU_COMBOBOX) && MouseEvent->dwEventFlags==MOUSE_MOVED ||
            CheckFlags(VMENU_LISTBOX|VMENU_COMBOBOX) && MouseEvent->dwEventFlags!=MOUSE_MOVED)
*/
        if( (VMenu::VMFlags.Check(VMENU_MOUSEREACTION) && MouseEvent->dwEventFlags==MOUSE_MOVED)
         ||
            (!VMenu::VMFlags.Check(VMENU_MOUSEREACTION) && MouseEvent->dwEventFlags!=MOUSE_MOVED)
         ||
            (MouseEvent->dwButtonState & (FROM_LEFT_1ST_BUTTON_PRESSED|RIGHTMOST_BUTTON_PRESSED))
          )
        {
          Item[SelectPos]->Flags&=~LIF_SELECTED;
          Item[MsPos]->Flags|=LIF_SELECTED;
          SelectPos=MsPos;
        }
        ShowMenu(TRUE);
      }
      /* $ 13.10.2001 VVM
        + ��������� ������� ������� ����� � ������ � ���� ������ ����������� ��� ���������� */
      if (MouseEvent->dwEventFlags==0 &&
         (MouseEvent->dwButtonState & (FROM_LEFT_1ST_BUTTON_PRESSED|RIGHTMOST_BUTTON_PRESSED)))
        VMenu::VMFlags.Set(VMENU_MOUSEDOWN);
      if (MouseEvent->dwEventFlags==0 &&
         (MouseEvent->dwButtonState & (FROM_LEFT_1ST_BUTTON_PRESSED|RIGHTMOST_BUTTON_PRESSED))==0 &&
          VMenu::VMFlags.Check(VMENU_MOUSEDOWN))
      {
        VMenu::VMFlags.Clear(VMENU_MOUSEDOWN);
        ProcessKey(KEY_ENTER);
      }
    }
    return(TRUE);
  }
  else
    if (BoxType!=NO_BOX && (MouseEvent->dwButtonState & 3) && MouseEvent->dwEventFlags==0)
    {
      ProcessKey(KEY_ESC);
      return(TRUE);
    }

  return(FALSE);
}


void VMenu::DeleteItems()
{
  CriticalSectionLock Lock(CS);

  if(Item)
  {
    for(int I=0; I < ItemCount; ++I)
    {
      if(Item[I]->UserDataSize > (int)sizeof(Item[I]->UserData) && Item[I]->UserData)
        xf_free(Item[I]->UserData);

      delete Item[I];
    }
    xf_free(Item);
  }
  Item=NULL;
  ItemCount=0;
  ItemHiddenCount=0;
  SelectPos=-1;
  TopPos=0;
  MaxLength=(int)Max(strTitle.GetLength(),strBottomTitle.GetLength())+2;
  if(MaxLength > ScrX-8)
    MaxLength=ScrX-8;
  VMFlags.Set(VMENU_UPDATEREQUIRED);
}


/* $ 01.08.2000 SVS
   ������� �������� N ������� ����
*/
int VMenu::DeleteItem(int ID,int Count)
{
  CriticalSectionLock Lock(CS);

  int I;

  if(ID < 0 || ID >= ItemCount || Count <= 0)
    return ItemCount;
  if(ID+Count > ItemCount)
    Count=ItemCount-ID; //???
  if(Count <= 0)
    return ItemCount;

  int OldItemSelected=-1;
  for(I=0; I < ItemCount; ++I)
  {
    if(Item[I]->Flags & LIF_SELECTED)
      OldItemSelected=I;
    Item [I]->Flags&=~LIF_SELECTED;
  }

  if(OldItemSelected >= ID)
  {
    if(ID+Count >= ItemCount)
      OldItemSelected=ID-1;
  }

  if(OldItemSelected < 0)
    OldItemSelected=0;

  // ������� ������� ������, ���� ������ �� ������ �� ����
  for(I=0; I < Count; ++I)
  {
    MenuItemEx *PtrItem=Item[ID+I];
    if(PtrItem->UserDataSize > (int)sizeof(PtrItem->UserData) && PtrItem->UserData)
      xf_free(PtrItem->UserData);
  }

  // � ��� ������ �����������
  if(ItemCount > 1)
    memmove(Item+ID,Item+ID+Count,sizeof(*Item)*(ItemCount-(ID+Count))); //BUGBUG

  // ��������� ������� �������
  if(SelectPos >= ID && SelectPos < ID+Count)
  {
    SelectPos=ID;
    if(ID+Count == ItemCount)
      SelectPos--;
    /* $ 23.02.2002 DJ
       ����������� �� ������� ��������� �� ���������
    */
    while (SelectPos > 0 &&(Item [SelectPos]->Flags & (LIF_SEPARATOR | LIF_DISABLE | LIF_HIDDEN)))
      SelectPos--;
  }

  VMFlags.Clear(VMENU_SELECTPOSNONE);
  if(SelectPos < 0)
  {
    VMFlags.Set(VMENU_SELECTPOSNONE);
    SelectPos=0;
  }

  ItemCount-=Count;

  if(SelectPos < TopPos || SelectPos > TopPos+Y2-Y1 || TopPos >= ItemCount)
  {
    TopPos=0;
    VMFlags.Set(VMENU_UPDATEREQUIRED);
  }

  // ����� �� �������� �����?
  if ((ID >= TopPos && ID < TopPos+Y2-Y1) ||
      (ID+Count >= TopPos && ID+Count < TopPos+Y2-Y1)) //???
  {
    VMFlags.Set(VMENU_UPDATEREQUIRED);
  }

  SelectPos=SetSelectPos(OldItemSelected,1);

  if(SelectPos > -1)
	{
		Item[SelectPos]->Flags|=LIF_SELECTED;
		if (Item[SelectPos]->Flags & (LIF_SEPARATOR | LIF_DISABLE | LIF_HIDDEN))
			VMFlags.Set(VMENU_SELECTPOSNONE);
	}

  RecalcItemHiddenCount();
  return(ItemCount);
}

int VMenu::RecalcItemHiddenCount()
{
  ItemHiddenCount=0;
  for (int I=0;I<ItemCount;I++)
  {
    if (Item[I]->Flags&LIF_HIDDEN)
      ItemHiddenCount++;
  }
  return ItemHiddenCount;
}

int VMenu::AddItem(const MenuItemEx *NewItem,int PosAdd)
{
  CriticalSectionLock Lock(CS);

  if(!NewItem)
    return -1; //???

  MenuItemEx **NewPtr;
  int Length;

  if(PosAdd >= ItemCount)
    PosAdd=ItemCount;

  if (UpdateRequired())
    VMFlags.Set(VMENU_UPDATEREQUIRED);

  if ((ItemCount & 255)==0)
  {
    if ((NewPtr=(MenuItemEx **)xf_realloc(Item, sizeof(*Item)*(ItemCount+256+1)))==NULL)
      return -1;
    Item=NewPtr;
  }

  // ���� < 0 - ���������� ������ � ������� �������, �.� ������� ������
  if(PosAdd < 0)
    PosAdd=0;

  if(PosAdd < ItemCount)
    memmove(Item+PosAdd+1,Item+PosAdd,sizeof(*Item)*(ItemCount-PosAdd)); //??

  Item[PosAdd] = new MenuItemEx;

  Item[PosAdd]->Clear();

  Item[PosAdd]->Flags = NewItem->Flags;
  Item[PosAdd]->strName = NewItem->strName;
  Item[PosAdd]->AccelKey = NewItem->AccelKey;

  _SetUserData (Item[PosAdd], NewItem->UserData, NewItem->UserDataSize);

/*  Item[PosAdd]->UserDataSize = NewItem->UserDataSize;
  Item[PosAdd]->UserData = NewItem->UserData;*/
  Item[PosAdd]->AmpPos = NewItem->AmpPos;
  Item[PosAdd]->Len[0] = NewItem->Len[0];
  Item[PosAdd]->Len[1] = NewItem->Len[1];
  Item[PosAdd]->Idx2 = NewItem->Idx2;
  Item[PosAdd]->ShowPos = NewItem->ShowPos;

  Item[PosAdd]->ShowPos = 0;

  if(VMFlags.Check(VMENU_SHOWAMPERSAND))
    Length=(int)Item[PosAdd]->strName.GetLength();
  else
    Length=HiStrlen(Item[PosAdd]->strName);

  if (Length>MaxLength)
    MaxLength=Length;
  if(MaxLength > ScrX-8)
    MaxLength=ScrX-8;
  if (Item[PosAdd]->Flags&LIF_SELECTED)
    SelectPos=PosAdd;
  if(Item[PosAdd]->Flags&0x0000FFFF)
  {
    Item[PosAdd]->Flags|=LIF_CHECKED;
    if((Item[PosAdd]->Flags&0x0000FFFF) == 1)
      Item[PosAdd]->Flags&=0xFFFF0000;
  }
  Item[PosAdd]->AmpPos=-1;

  VMFlags.Clear(VMENU_SELECTPOSNONE);
  if(SelectPos < 0)
  {
    VMFlags.Set(VMENU_SELECTPOSNONE);
    //SelectPos=0;
  }

  LastAddedItem = PosAdd;

  ItemCount++;

  RecalcItemHiddenCount();
  return ItemCount-1;
}

int VMenu::AddItem(const wchar_t *NewStrItem)
{
  CriticalSectionLock Lock(CS);

	FarListItem FarListItem0={0};

  if(!NewStrItem || NewStrItem[0] == 0x1)
  {
    FarListItem0.Flags=LIF_SEPARATOR;
    FarListItem0.Text=NewStrItem+1;
  }
  else
  {
    FarListItem0.Text=NewStrItem;
  }
	FarList FarList0={1,&FarListItem0};
  return VMenu::AddItem(&FarList0)-1; //-1 ������ ��� AddItem(FarList) ���������� ���������� ���������
}

int VMenu::AddItem(const FarList *List)
{
  CriticalSectionLock Lock(CS);

  if(List && List->Items)
  {
    MenuItemEx MItem;
		for(int i=0;i<List->ItemsNumber;i++)
		{
			AddItem(FarList2MenuItem(&List->Items[i],&MItem));
		}
  }
  return ItemCount;
}

int VMenu::UpdateItem(const FarListUpdate *NewItem)
{
  CriticalSectionLock Lock(CS);

  if(NewItem && (DWORD)NewItem->Index < (DWORD)ItemCount)
  {
    MenuItemEx MItem;
    // ��������� ������... �� ����� �������� ;-)
    MenuItemEx *PItem=Item[NewItem->Index];
    if(PItem->UserDataSize > (int)sizeof(PItem->UserData) && PItem->UserData && (NewItem->Item.Flags&LIF_DELETEUSERDATA))
    {
      xf_free(PItem->UserData);
      PItem->UserData=NULL;
      PItem->UserDataSize=0;
    }

    FarList2MenuItem(&NewItem->Item,&MItem);
    PItem->Flags=MItem.Flags;

    PItem->strName = MItem.strName;

    /* $ 23.02.2002 DJ
       ���� ������� selected - �������� �� ���� ���������
    */
    if (PItem->Flags & LIF_SELECTED && !(PItem->Flags & (LIF_SEPARATOR | LIF_DISABLE | LIF_HIDDEN)))
      SelectPos = NewItem->Index;
    AdjustSelectPos();

    RecalcItemHiddenCount();
    return TRUE;
  }
  return FALSE;
}

int VMenu::InsertItem(const FarListInsert *NewItem)
{
  CriticalSectionLock Lock(CS);

  if(NewItem)
  {
    MenuItemEx MItem;
    if (AddItem(FarList2MenuItem(&NewItem->Item,&MItem),NewItem->Index) >= 0)
    {
		RecalcItemHiddenCount();
    	return ItemCount;
    }
  }
  return -1;
}

int VMenu::GetUserDataSize(int Position)
{
  CriticalSectionLock Lock(CS);

  if (ItemCount==0)
    return(0);

  int DataSize=Item[GetItemPosition(Position)]->UserDataSize;

  return(DataSize);
}

int VMenu::_SetUserData(MenuItemEx *PItem,
                       const void *Data,   // ������
                       int Size)     // ������, ���� =0 �� ��������������, ��� � Data-������
{
  if(PItem->UserDataSize > (int)sizeof(PItem->UserData) && PItem->UserData)
    xf_free(PItem->UserData);

  PItem->UserDataSize=0;
  PItem->UserData=NULL;

  if(Data)
  {
    int SizeReal=Size;

    // ���� Size=0, �� ���������������, ��� � Data ��������� ASCIIZ ������
    if(!Size)
      SizeReal=(int)((StrLength((const wchar_t*)Data)+1)*sizeof(wchar_t));

    // ���� ������ ������ Size=0 ��� Size ������ 4 ���� (sizeof(void*))
    if(!Size ||
        Size > (int)sizeof(PItem->UserData)) // ���� � 4 ����� �� �������, ��...
    {
      // ������ ������ 4 ����?
      if(SizeReal > (int)sizeof(PItem->UserData))
      {
        // ...������ �������� ������ ������.
        if((PItem->UserData=(char*)xf_malloc(SizeReal)) != NULL)
        {
          PItem->UserDataSize=SizeReal;
          memcpy(PItem->UserData,Data,SizeReal);
        }
      }
      else // ��� ������ ���������� � 4 �����!
      {
        PItem->UserDataSize=SizeReal;
        memcpy(PItem->Str4,Data,SizeReal);
      }
    }
    else // ��. ������ ���������� � 4 �����...
    {
      PItem->UserDataSize=0;         // ������� ����, ��� ������ ���� ���, ����
      PItem->UserData=(char*)Data;   // ��� ���������� � 4 �����
    }
  }
  return(PItem->UserDataSize);
}

void* VMenu::_GetUserData(MenuItemEx *PItem,void *Data,int Size)
{
  int DataSize=PItem->UserDataSize;
  char *PtrData=PItem->UserData; // PtrData ��������: ���� ��������� �� ���-�� ����
                                 // 4 �����!
  /* $ 12.06.2001 KM
     - ����������� �������� �������. ������, ��� ������ � ����
       ����� ���� � ������� MenuItem.Name
  */
  if (Size > 0 && Data != NULL)
  {
    if (PtrData) // ������ ����?
    {
      // ��������� ������ 4 ����?
      if(DataSize > (int)sizeof(PItem->UserData))
      {
        memmove(Data,PtrData,Min(Size,DataSize));
      }
      else if(DataSize > 0) // � ������ �� ������ ����? �.�. ���� � UserData
      {                     // ���� ������ �� 4 ���� (UserDataSize ��� ���� > 0)
        memmove(Data,PItem->Str4,Min(Size,DataSize));
      }
      // else � �����... � PtrData ��� ��������� �����!
    }
    else // ... ������ ���, ������ ����� ��� ������!
    {
			memcpy (Data,(const wchar_t *)PItem->strName,
              Min(Size,static_cast<int>((PItem->strName.GetLength()+1)*sizeof(wchar_t))));
    }
  }
  return(PtrData);
}

FarListItem *VMenu::MenuItem2FarList(const MenuItemEx *MItem, FarListItem *FItem)
{
  if(FItem && MItem)
  {
		memset(FItem,0,sizeof(FarListItem));
    FItem->Flags=MItem->Flags&(~MIF_USETEXTPTR); //??

    FItem->Text=MItem->strName;
    return FItem;
  }
  return NULL;
}

MenuItemEx *VMenu::FarList2MenuItem(const FarListItem *FItem,
                                         MenuItemEx *MItem)
{
  if(FItem && MItem)
  {
    MItem->Clear ();
    MItem->Flags=FItem->Flags;

    MItem->strName = FItem->Text;

    return MItem;
  }
  return NULL;
}

// �������� ������� ������� � ������� ������� �����
int VMenu::GetSelectPos(FarListPos *ListPos)
{
  CriticalSectionLock Lock(CS);

  ListPos->SelectPos=GetSelectPos();
  if(VMFlags.Check(VMENU_SELECTPOSNONE))
    ListPos->SelectPos=-1;
  ListPos->TopPos=TopPos;
  return ListPos->SelectPos;
}

void VMenu::SetMaxHeight(int NewMaxHeight)
{
  CriticalSectionLock Lock(CS);

  VMenu::MaxHeight=NewMaxHeight;
  if(MaxLength > ScrX-8) //
    MaxLength=ScrX-8;
}

// ���������� ������ � ������� ����
int VMenu::SetSelectPos(FarListPos *ListPos)
{
  CriticalSectionLock Lock(CS);

  int Ret=SetSelectPos(ListPos->SelectPos,1);
  if(Ret > -1)
  {
    TopPos=ListPos->TopPos;
    if(ListPos->TopPos == -1)
    {
      if(ItemCount < MaxHeight)
        TopPos=0;
      else
      {

        //TopPos=Ret-MaxHeight/2;               //?????????
        TopPos = (ListPos->SelectPos-ListPos->TopPos+1) > MaxHeight?ListPos->TopPos+1:ListPos->TopPos;
        if(TopPos+MaxHeight > ItemCount)
          TopPos=ItemCount-MaxHeight;

      }
    }

    if(TopPos < 0)
      TopPos = 0;
  }
  return Ret;
}

// ����������� ������ � ������ Disabled & Separator
int VMenu::SetSelectPos(int Pos,int Direct)
{
  CriticalSectionLock Lock(CS);

  if(!Item || !ItemCount)
    return -1;

  int Pass=0, I=0;

  do
  {
    /* $ 21.02.2002 DJ
       � ���� ��� WRAPMODE ������� OrigPos == Pos ������� �� ���������� =>
       ����� ������������ ������� ������ ������ ��� ������ �� �����
    */
    if (Pos<0)
    {
      if (VMFlags.Check(VMENU_WRAPMODE))
        Pos=ItemCount-1;
      else
      {
        Pos=0;
        Pass++;
      }
    }

    if (Pos>=ItemCount)
    {
      if (VMFlags.Check(VMENU_WRAPMODE))
        Pos=0;
      else
      {
        Pos=ItemCount-1;
        Pass++;
      }
    }

    if(!(Item[Pos]->Flags&LIF_SEPARATOR) && !(Item[Pos]->Flags&(LIF_DISABLE | LIF_HIDDEN)))
      break;

    Pos+=Direct;

    if(Pass)
      return SelectPos;

    if (I>=ItemCount) // ���� ������� - ������ �� ������� :-(
      Pass++;

    ++I;
  } while (1);

  /* $ 30.01.2003 KM
     - ������ ��� �����. ��� ���������� ���� SelectPos ��� ����� -1.
  */
  if (SelectPos!=-1)
    Item[SelectPos]->Flags&=~LIF_SELECTED;
  Item[Pos]->Flags|=LIF_SELECTED;
  SelectPos=Pos;

  if (SelectPos!=-1)
    VMFlags.Clear(VMENU_SELECTPOSNONE);
  /* $ 01.07.2001 KM
    ����� �����, ��� ������� ���������� ��� ����������� (������
    ������ �� "�������", ��� ������� ����������).
  */
  VMFlags.Set(VMENU_UPDATEREQUIRED);
  return Pos;
}

/* $ 21.02.2002 DJ
   ������������� ������� ������� (����� �� ���� ���� ���������� ���������,
   ��� ����� ���������� ������� �� ��� �����������)
*/

void VMenu::AdjustSelectPos()
{
  CriticalSectionLock Lock(CS);

  if (!Item || !ItemCount)
    return;

  /* $ 20.07.2004 KM
     ������� �������� �� -1, � ��������� ������ ������ ����
     �� Dialog API.
  */
//  if (SelectPos!=-1)
//  {
    int OldSelectPos = SelectPos;
    // ���� selection ����� � ������������ ����� - ������� ���
    if (SelectPos >= 0 && Item [SelectPos]->Flags & (LIF_SEPARATOR | LIF_DISABLE | LIF_HIDDEN))
      SelectPos = -1;

    for (int i=0; i<ItemCount; i++)
    {
      if (Item [i]->Flags & (LIF_SEPARATOR | LIF_DISABLE | LIF_HIDDEN))
        Item [i]->SetSelect (FALSE);
      else
      {
        if (SelectPos == -1)
        {
          Item [i]->SetSelect (TRUE);
          SelectPos = i;
        }
        else if (SelectPos >= 0 && SelectPos != i)
        {
          // ���� ��� ���� ���������� ������� - ������� ��� ����
          Item [i]->SetSelect (FALSE);
        }
      }
    }

    // ���� ������ �� ����� - ������� ��� ����
    if (SelectPos == -1)
    {
      SelectPos = OldSelectPos;
      if (SelectPos >= 0)
        Item [SelectPos]->SetSelect (TRUE);
    }
//  }

  if (SelectPos == -1)
    VMFlags.Set(VMENU_SELECTPOSNONE); //??
  else
    VMFlags.Clear(VMENU_SELECTPOSNONE);
}


void VMenu::SetTitle(const wchar_t *Title)
{
  CriticalSectionLock Lock(CS);

  int Length;
  VMFlags.Set(VMENU_UPDATEREQUIRED);

  if ( Title )
    strTitle = Title;
  else
    strTitle = L"";

  Length=(int)strTitle.GetLength()+2;

  if (Length > MaxLength)
    MaxLength=Length;
  if(MaxLength > ScrX-8)
    MaxLength=ScrX-8;

  if(VMFlags.Check(VMENU_CHANGECONSOLETITLE))
  {
    if( !strTitle.IsEmpty() )
    {
      if(!OldTitle)
        OldTitle=new ConsoleTitle;

      SetFarTitle(strTitle);
    }
    else
    {
      if(OldTitle)
      {
        delete OldTitle;
        OldTitle=NULL;
      }
    }
  }
}


string &VMenu::GetTitle(string &strDest,int,int)
{
  CriticalSectionLock Lock(CS);

  /* $ 23.02.2002 DJ
     ���� ��������� ������ - ��� �� ������, ��� ��� ������ �������!
  */
  strDest = strTitle;

  return strDest;
}


void VMenu::SetBottomTitle(const wchar_t *BottomTitle)
{
  CriticalSectionLock Lock(CS);

  int Length;
  VMFlags.Set(VMENU_UPDATEREQUIRED);

  if ( BottomTitle )
    strBottomTitle  = BottomTitle;
  else
    strBottomTitle = L"";
  Length=(int)strBottomTitle.GetLength()+2;
  if (Length > MaxLength)
    MaxLength=Length;
  if(MaxLength > ScrX-8)
    MaxLength=ScrX-8;
}


string &VMenu::GetBottomTitle(string &strDest)
{
  CriticalSectionLock Lock(CS);

  strDest = strBottomTitle;

  return strDest;
}


void VMenu::SetBoxType(int BoxType)
{
  CriticalSectionLock Lock(CS);

  VMenu::BoxType=BoxType;
}

int VMenu::GetItemPosition(int Position)
{
  CriticalSectionLock Lock(CS);

  int DataPos=(Position==-1) ? SelectPos : Position;
  if (DataPos>=ItemCount)
    DataPos=ItemCount-1;
  return DataPos;
}


int VMenu::GetSelection(int Position)
{
  CriticalSectionLock Lock(CS);

  if (ItemCount==0)
    return(0);

  int DataPos=GetItemPosition(Position);
  if (Item[DataPos]->Flags&LIF_SEPARATOR)
    return(0);
  int Checked=Item[DataPos]->Flags&0xFFFF;
  return((Item[DataPos]->Flags&LIF_CHECKED)?(Checked?Checked:1):0);
}


void VMenu::SetSelection(int Selection,int Position)
{
  CriticalSectionLock Lock(CS);

  if (ItemCount==0)
    return;
  Item[GetItemPosition(Position)]->SetCheck(Selection);
}

// ������� GetItemPtr - �������� ��������� �� ������ Item.
MenuItemEx *VMenu::GetItemPtr(int Position)
{
  CriticalSectionLock Lock(CS);

  if (ItemCount==0)
    return NULL;
  return Item[GetItemPosition(Position)];
}

wchar_t VMenu::GetHighlights(const MenuItemEx *_item)
{
  CriticalSectionLock Lock(CS);

  wchar_t Ch=0;
  if(_item)
  {
		const wchar_t *Name=((MenuItemEx *)_item)->strName;
    const wchar_t *ChPtr=wcschr(Name,L'&');

    if (ChPtr || _item->AmpPos > -1)
    {
      if (!ChPtr && _item->AmpPos > -1)
      {
        ChPtr=Name+_item->AmpPos;
        Ch=*ChPtr;
      }
      else
        Ch=ChPtr[1];

      if(VMFlags.Check(VMENU_SHOWAMPERSAND))
      {
        ChPtr=wcschr(ChPtr+1,L'&');
        if(ChPtr)
          Ch=ChPtr[1];
      }
    }
  }

  return Ch;
}

BOOL VMenu::CheckHighlights(WORD CheckSymbol)
{
  CriticalSectionLock Lock(CS);

  for (int I=0; I < ItemCount; I++)
  {
    if(Item[I]->Flags&LIF_HIDDEN) //???
        continue;
    wchar_t Ch=GetHighlights(Item[I]);

    if(Ch && Upper(CheckSymbol) == Upper(Ch))
      return TRUE;
  }
  return FALSE;
}

void VMenu::AssignHighlights(int Reverse)
{
  CriticalSectionLock Lock(CS);

	memset(Used,0,WCHAR_MAX);

  /* $ 02.12.2001 KM
     + ������� VMENU_SHOWAMPERSAND ������������ ��� ����������
       ������ ShowMenu ������� ���������� ������ �����, � ���������
       ������ ���� � ������� ������������� DI_LISTBOX ��� �����
       DIF_LISTNOAMPERSAND, �� ���������� ������������ � ������
       ������ ���� ��� �� ���������� ShowMenu.
  */
  if (VMFlags.Check(VMENU_SHOWAMPERSAND))
    VMOldFlags.Set(VMENU_SHOWAMPERSAND);
  if (VMOldFlags.Check(VMENU_SHOWAMPERSAND))
    VMFlags.Set(VMENU_SHOWAMPERSAND);

  int I, Delta = Reverse ? -1 : 1;
  for (I = Reverse ? ItemCount-1 : 0; I>=0 && I<ItemCount; I+=Delta)
  {
    wchar_t Ch = 0;
    int ShowPos = HiFindRealPos(Item[I]->strName, Item[I]->ShowPos, VMFlags.Check(VMENU_SHOWAMPERSAND));
    const wchar_t *Name = Item[I]->strName.CPtr() + ShowPos;
    Item[I]->AmpPos = -1;
    // TODO: �������� �� LIF_HIDDEN
    const wchar_t *ChPtr = wcschr(Name, L'&');
    if (ChPtr)
    {
      Ch = ChPtr[1];
      if (Ch && VMFlags.Check(VMENU_SHOWAMPERSAND))
      {
        ChPtr = wcschr(ChPtr+1, L'&');
        if (ChPtr)
          Ch=ChPtr[1];
      }
    }

    if (Ch && !Used[Upper(Ch)] && !Used[Lower(Ch)])
    {
      Used[Upper(Ch)] = true;
      Used[Lower(Ch)] = true;
      Item[I]->AmpPos = static_cast<int>(ChPtr-Name) + ShowPos;
    }
  }
//_SVS(SysLogDump("Used Pre",0,Used,sizeof(Used),NULL));

  // TODO:  ���� ���� ����� �������� - �������� ������� ��������� (���� �� ������)
  for (I = Reverse ? ItemCount-1 : 0; I>=0 && I<ItemCount; I+=Delta)
  {
    int ShowPos = HiFindRealPos(Item[I]->strName, Item[I]->ShowPos, VMFlags.Check(VMENU_SHOWAMPERSAND));
    const wchar_t *Name = Item[I]->strName.CPtr() + ShowPos;
    const wchar_t *ChPtr = wcschr(Name, L'&');
    if (ChPtr==NULL || VMFlags.Check(VMENU_SHOWAMPERSAND))
    {
      // TODO: �������� �� LIF_HIDDEN
      for (int J=0; Name[J]; J++)
      {
        wchar_t Ch = Name[J];
        if ((Ch == L'&' || IsAlpha(Ch) || (Ch >= L'0' && Ch <=L'9')) && !Used[Upper(Ch)] && !Used[Lower(Ch)])
        {
          Used[Upper(Ch)] = true;
          Used[Lower(Ch)] = true;
          Item[I]->AmpPos = J + ShowPos;
          break;
        }
      }
    }
  }
//_SVS(SysLogDump("Used Post",0,Used,sizeof(Used),NULL));
  VMFlags.Set(VMENU_AUTOHIGHLIGHT|(Reverse?VMENU_REVERSEHIGHLIGHT:0));
  VMFlags.Clear(VMENU_SHOWAMPERSAND);
}

void VMenu::SetColors(FarListColors *Colors)
{
  CriticalSectionLock Lock(CS);

  if(Colors)
    memmove(VMenu::Colors,Colors->Colors,sizeof(VMenu::Colors));
  else
  {
    static short StdColor[2][3][VMENU_COLOR_COUNT]=
    {
      // Not VMENU_WARNDIALOG
      {
        { // VMENU_LISTBOX
          COL_DIALOGLISTTEXT,                        // ��������
          COL_DIALOGLISTBOX,                         // �����
          COL_DIALOGLISTTITLE,                       // ��������� - ������� � ������
          COL_DIALOGLISTTEXT,                        // ����� ������
          COL_DIALOGLISTHIGHLIGHT,                   // HotKey
          COL_DIALOGLISTBOX,                         // separator
          COL_DIALOGLISTSELECTEDTEXT,                // ���������
          COL_DIALOGLISTSELECTEDHIGHLIGHT,           // ��������� - HotKey
          COL_DIALOGLISTSCROLLBAR,                   // ScrollBar
          COL_DIALOGLISTDISABLED,                    // Disabled
          COL_DIALOGLISTARROWS,                      // Arrow
          COL_DIALOGLISTARROWSSELECTED,              // ��������� - Arrow
          COL_DIALOGLISTARROWSDISABLED,              // Arrow Disabled
          COL_DIALOGLISTGRAY,                        // "�����"
          COL_DIALOGLISTSELECTEDGRAYTEXT,            // ��������� "�����"
        },
        { // VMENU_COMBOBOX
          COL_DIALOGCOMBOTEXT,                       // ��������
          COL_DIALOGCOMBOBOX,                        // �����
          COL_DIALOGCOMBOTITLE,                      // ��������� - ������� � ������
          COL_DIALOGCOMBOTEXT,                       // ����� ������
          COL_DIALOGCOMBOHIGHLIGHT,                  // HotKey
          COL_DIALOGCOMBOBOX,                        // separator
          COL_DIALOGCOMBOSELECTEDTEXT,               // ���������
          COL_DIALOGCOMBOSELECTEDHIGHLIGHT,          // ��������� - HotKey
          COL_DIALOGCOMBOSCROLLBAR,                  // ScrollBar
          COL_DIALOGCOMBODISABLED,                   // Disabled
          COL_DIALOGCOMBOARROWS,                     // Arrow
          COL_DIALOGCOMBOARROWSSELECTED,             // ��������� - Arrow
          COL_DIALOGCOMBOARROWSDISABLED,             // Arrow Disabled
          COL_DIALOGCOMBOGRAY,                       // "�����"
          COL_DIALOGCOMBOSELECTEDGRAYTEXT,           // ��������� "�����"
        },
        { // VMenu
          COL_MENUBOX,                               // ��������
          COL_MENUBOX,                               // �����
          COL_MENUTITLE,                             // ��������� - ������� � ������
          COL_MENUTEXT,                              // ����� ������
          COL_MENUHIGHLIGHT,                         // HotKey
          COL_MENUBOX,                               // separator
          COL_MENUSELECTEDTEXT,                      // ���������
          COL_MENUSELECTEDHIGHLIGHT,                 // ��������� - HotKey
          COL_MENUSCROLLBAR,                         // ScrollBar
          COL_MENUDISABLEDTEXT,                      // Disabled
          COL_MENUARROWS,                            // Arrow
          COL_MENUARROWSSELECTED,                    // ��������� - Arrow
          COL_MENUARROWSDISABLED,                    // Arrow Disabled
          COL_MENUGRAYTEXT,                          // "�����"
          COL_MENUSELECTEDGRAYTEXT,                  // ��������� "�����"
        }
      },

      // == VMENU_WARNDIALOG
      {
        { // VMENU_LISTBOX
          COL_WARNDIALOGLISTTEXT,                    // ��������
          COL_WARNDIALOGLISTBOX,                     // �����
          COL_WARNDIALOGLISTTITLE,                   // ��������� - ������� � ������
          COL_WARNDIALOGLISTTEXT,                    // ����� ������
          COL_WARNDIALOGLISTHIGHLIGHT,               // HotKey
          COL_WARNDIALOGLISTBOX,                     // separator
          COL_WARNDIALOGLISTSELECTEDTEXT,            // ���������
          COL_WARNDIALOGLISTSELECTEDHIGHLIGHT,       // ��������� - HotKey
          COL_WARNDIALOGLISTSCROLLBAR,               // ScrollBar
          COL_WARNDIALOGLISTDISABLED,                // Disabled
          COL_WARNDIALOGLISTARROWS,                  // Arrow
          COL_WARNDIALOGLISTARROWSSELECTED,          // ��������� - Arrow
          COL_WARNDIALOGLISTARROWSDISABLED,          // Arrow Disabled
          COL_WARNDIALOGLISTGRAY,                    // "�����"
          COL_WARNDIALOGLISTSELECTEDGRAYTEXT,        // ��������� "�����"
        },
        { // VMENU_COMBOBOX
          COL_WARNDIALOGCOMBOTEXT,                   // ��������
          COL_WARNDIALOGCOMBOBOX,                    // �����
          COL_WARNDIALOGCOMBOTITLE,                  // ��������� - ������� � ������
          COL_WARNDIALOGCOMBOTEXT,                   // ����� ������
          COL_WARNDIALOGCOMBOHIGHLIGHT,              // HotKey
          COL_WARNDIALOGCOMBOBOX,                    // separator
          COL_WARNDIALOGCOMBOSELECTEDTEXT,           // ���������
          COL_WARNDIALOGCOMBOSELECTEDHIGHLIGHT,      // ��������� - HotKey
          COL_WARNDIALOGCOMBOSCROLLBAR,              // ScrollBar
          COL_WARNDIALOGCOMBODISABLED,               // Disabled
          COL_WARNDIALOGCOMBOARROWS,                 // Arrow
          COL_WARNDIALOGCOMBOARROWSSELECTED,         // ��������� - Arrow
          COL_WARNDIALOGCOMBOARROWSDISABLED,         // Arrow Disabled
          COL_WARNDIALOGCOMBOGRAY,                   // "�����"
          COL_WARNDIALOGCOMBOSELECTEDGRAYTEXT,       // ��������� "�����"
        },
        { // VMenu
          COL_MENUBOX,                               // ��������
          COL_MENUBOX,                               // �����
          COL_MENUTITLE,                             // ��������� - ������� � ������
          COL_MENUTEXT,                              // ����� ������
          COL_MENUHIGHLIGHT,                         // HotKey
          COL_MENUBOX,                               // separator
          COL_MENUSELECTEDTEXT,                      // ���������
          COL_MENUSELECTEDHIGHLIGHT,                 // ��������� - HotKey
          COL_MENUSCROLLBAR,                         // ScrollBar
          COL_MENUDISABLEDTEXT,                      // Disabled
          COL_MENUARROWS,                            // Arrow
          COL_MENUARROWSSELECTED,                    // ��������� - Arrow
          COL_MENUARROWSDISABLED,                    // Arrow Disabled
          COL_MENUGRAYTEXT,                          // "�����"
          COL_MENUSELECTEDGRAYTEXT,                  // ��������� "�����"
        }
      }
    };
    int TypeMenu  = CheckFlags(VMENU_LISTBOX)?0:(CheckFlags(VMENU_COMBOBOX)?1:2);
    int StyleMenu = CheckFlags(VMENU_WARNDIALOG)?1:0;
    if(CheckFlags(VMENU_DISABLED))
    {
      VMenu::Colors[0]=FarColorToReal(StyleMenu?COL_WARNDIALOGDISABLED:COL_DIALOGDISABLED);
      for(int I=1; I < VMENU_COLOR_COUNT; ++I)
        VMenu::Colors[I]=VMenu::Colors[0];
    }
    else
    {
      for(int I=0; I < VMENU_COLOR_COUNT; ++I)
        VMenu::Colors[I]=FarColorToReal(StdColor[StyleMenu][TypeMenu][I]);
    }
  }
}

void VMenu::GetColors(FarListColors *Colors)
{
  CriticalSectionLock Lock(CS);

  memmove(Colors->Colors,VMenu::Colors,sizeof(VMenu::Colors));
}

void VMenu::SetOneColor (int Index, short Color)
{
  CriticalSectionLock Lock(CS);

	if ((DWORD)Index < countof(Colors))
    Colors [Index]=FarColorToReal(Color);
}


struct SortItemParam{
  int Direction;
  int Offset;
};

static int __cdecl  SortItem(const MenuItemEx **el1,
                           const MenuItemEx **el2,
                           const SortItemParam *Param)
{
  string strName1, strName2;

  strName1 = (*el1)->strName;
  strName2 = (*el2)->strName;
  RemoveChar (strName1,L'&',TRUE);
  RemoveChar (strName2,L'&',TRUE);
  int Res=StrCmpI((const wchar_t*)strName1+Param->Offset,(const wchar_t*)strName2+Param->Offset);
  return(Param->Direction==0?Res:(Res<0?1:(Res>0?-1:0)));
}

static int __cdecl  SortItemDataDWORD(const MenuItemEx **el1,
                           const MenuItemEx **el2,
                           const SortItemParam *Param)
{
  int Res;
  DWORD Dw1=(DWORD)(DWORD_PTR)((*el1)->UserData);
  DWORD Dw2=(DWORD)(DWORD_PTR)((*el2)->UserData);
  if(Dw1 == Dw2)
    Res=0;
  else if(Dw1 > Dw2)
    Res=1;
  else
    Res=-1;
  return(Param->Direction==0?Res:(Res<0?1:(Res>0?-1:0)));
}

// ���������� ��������� ������
// Offset - ������ ���������! �� ��������� =0
void VMenu::SortItems(int Direction,int Offset,BOOL SortForDataDWORD)
{
  CriticalSectionLock Lock(CS);

  typedef int (__cdecl *qsortex_fn)(const void*,const void*,void*);
	SortItemParam Param;
  Param.Direction=Direction;
  Param.Offset=Offset;

  int I;
  //_SVS(for(I=0; I < ItemCount; ++I)SysLog(L"%2d) 0x%08X - '%s'",I,Item[I].Flags,Item[I].Name));
  if(!SortForDataDWORD) // ������� ����������
    qsortex((char *)Item,
          ItemCount,
          sizeof(*Item),
          (qsortex_fn)SortItem,
          &Param);
  else
    qsortex((char *)Item,
          ItemCount,
          sizeof(*Item),
          (qsortex_fn)SortItemDataDWORD,
          &Param);
  //_SVS(for(I=0; I < ItemCount; ++I)SysLog(L"%2d) 0x%08X - '%s'",I,Item[I].Flags,Item[I].Name));

  // ������������� SelectPos
  for(I=0; I < ItemCount; ++I)
    if (Item[I]->Flags & LIF_SELECTED && !(Item[I]->Flags & (LIF_SEPARATOR | LIF_DISABLE | LIF_HIDDEN)))
    {
      SelectPos=I;
      break;
    }

  VMFlags.Set(VMENU_UPDATEREQUIRED);
}

// return Pos || -1
int VMenu::FindItem(const FarListFind *FItem)
{
  return FindItem(FItem->StartIndex,FItem->Pattern,FItem->Flags);
}

int VMenu::FindItem(int StartIndex,const wchar_t *Pattern,DWORD Flags)
{
  CriticalSectionLock Lock(CS);

  string strTmpBuf;
  if((DWORD)StartIndex < (DWORD)ItemCount)
  {
    int LenPattern=StrLength(Pattern);
    for(int I=StartIndex;I < ItemCount;I++)
    {
      strTmpBuf = Item[I]->strName;
      int LenNamePtr = (int)strTmpBuf.GetLength();

      RemoveChar (strTmpBuf, L'&');

      if(Flags&LIFIND_EXACTMATCH)
      {
        if(!StrCmpNI(strTmpBuf,Pattern,Max(LenPattern,LenNamePtr)))
          return I;
      }
      else
      {
        if(CmpName(Pattern,strTmpBuf,1))
          return I;
      }
    }
  }
  return -1;
}

BOOL VMenu::GetVMenuInfo(FarListInfo* Info)
{
  CriticalSectionLock Lock(CS);

  if(Info)
  {
    Info->Flags=VMFlags.Flags & (LINFO_SHOWNOBOX | LINFO_AUTOHIGHLIGHT
      | LINFO_REVERSEHIGHLIGHT | LINFO_WRAPMODE | LINFO_SHOWAMPERSAND);
    Info->ItemsNumber=ItemCount;
    Info->SelectPos=SelectPos;
    Info->TopPos=TopPos;
    Info->MaxHeight=MaxHeight;
    Info->MaxLength=MaxLength;
    memset(&Info->Reserved,0,sizeof(Info->Reserved));
    return TRUE;
  }
  return FALSE;
}

// ������������� � ����� ������.
int VMenu::SetUserData(LPCVOID Data,   // ������
                       int Size,     // ������, ���� =0 �� ��������������, ��� � Data-������
                       int Position) // ����� �����
{
  CriticalSectionLock Lock(CS);

  if (ItemCount==0 || Position < 0)
    return(0);

  int DataSize=VMenu::_SetUserData(Item[GetItemPosition(Position)],Data,Size);

  return DataSize;
}

// �������� ������
void* VMenu::GetUserData(void *Data,int Size,int Position)
{
  CriticalSectionLock Lock(CS);

  void *PtrData=NULL;
  if (ItemCount || Position < 0)
  {
    if((Position=GetItemPosition(Position)) >= 0)
      PtrData=VMenu::_GetUserData(Item[Position],Data,Size);
  }
  return(PtrData);
}

void VMenu::Process()
{
  Modal::Process();
}

void VMenu::ResizeConsole()
{
  CriticalSectionLock Lock(CS);

  /* $ 13.04.2002 KM
    - ������� �������� �� ������������� ������ ����������,
      �.�. ResizeConsole ���������� ������ ��� ���� �����������
      VMenu ��� AltF9.
  */
  if (SaveScr)
  {
    SaveScr->Discard();
    delete SaveScr;
    SaveScr=NULL;
  }
  if (this->CheckFlags(VMENU_NOTCHANGE))
  {
    return;
  }
  ObjWidth=ObjHeight=0;
  if (!this->CheckFlags(VMENU_NOTCENTER))
  {
    Y2=X2=Y1=X1=-1;
  }
  else
  {
    X1=5;
    if (!this->CheckFlags(VMENU_LEFTMOST) && ScrX>40)
    {
      X1=(ScrX+1)/2+5;
    }
    Y1=(ScrY+1-(this->ItemCount+5))/2;
    if (Y1<1) Y1=1;
    X2=Y2=0;
  }
}

int VMenu::GetTypeAndName(string &strType, string &strName)
{
  CriticalSectionLock Lock(CS);

  strType = MSG(MVMenuType);
  strName = strTitle;

  return(CheckFlags(VMENU_COMBOBOX)?MODALTYPE_COMBOBOX:MODALTYPE_VMENU);
}


// ������� ��������� ���� (�� ���������)
LONG_PTR WINAPI VMenu::DefMenuProc(HANDLE hVMenu,int Msg,int Param1,LONG_PTR Param2)
{
  return 0;
}

// ������� ������� ��������� ����
LONG_PTR WINAPI VMenu::SendMenuMessage(HANDLE hVMenu,int Msg,int Param1,LONG_PTR Param2)
{
  CriticalSectionLock Lock(((VMenu*)hVMenu)->CS);

  if(hVMenu)
    return ((VMenu*)hVMenu)->VMenuProc(hVMenu,Msg,Param1,Param2);
  return 0;
}

int VMenu::GetMaxLineWidth() const {
	int width = X2 - X1 + 1;
	if (BoxType != NO_BOX)
		width -= 2; // frame
	width -= 3; // check mark + horz. scroll
	if (!CheckFlags(VMENU_COMBOBOX | VMENU_LISTBOX))
		width -= 2; // sub menu arrow
	return width;
}
