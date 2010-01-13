#pragma once

/*
dlgedit.hpp

��������� ������ �������������� ��� ������� (��� ��������� ������ Edit)
��������������
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

/*
  ���� ����� ���������� �� edit.hpp � editor.hpp ��� ����,
  �������� ����� �.. ��� ��� ��������� ������ � ��������
  ��� ���� ������ ������, ��������� ��� �������� ��������
*/

#include "scrobj.hpp"
#include "bitflags.hpp"
#include "edit.hpp"
#include "editor.hpp"

enum DLGEDITTYPE
{
	DLGEDIT_MULTILINE,
	DLGEDIT_SINGLELINE,
};

class Dialog;

class DlgEdit: public ScreenObject
{
	private: // ��������� ������
		Dialog* m_Dialog;
		unsigned m_Index;
		DLGEDITTYPE Type;
		History* iHistory;

		EditControl   *lineEdit;
#if defined(PROJECT_DI_MEMOEDIT)
		Editor *multiEdit;
#endif

	public:  // ��������� ������
		BitFlags& Flags();

	private: // ��������� ������
		virtual void DisplayObject();
		static void EditChange(void* aParam);
		void DoEditChange();

	public:
		DlgEdit(Dialog* pOwner,unsigned Index,DLGEDITTYPE Type);
		virtual ~DlgEdit();

	public: // ��������� ������
		virtual int  ProcessKey(int Key);
		virtual int  ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent);

		virtual void Show();
		virtual void SetPosition(int X1,int Y1,int X2,int Y2);
		virtual void GetPosition(int& X1,int& Y1,int& X2,int& Y2);

		virtual void Hide();
		virtual void Hide0();
		virtual void ShowConsoleTitle();
		virtual void SetScreenPosition();
		virtual void ResizeConsole();
		virtual __int64  VMProcess(int OpCode,void *vParam=NULL,__int64 iParam=0);

		void  SetDialogParent(DWORD Sets);
		void  SetDropDownBox(int NewDropDownBox);
		void  SetPasswordMode(int Mode);

		int   GetMaxLength();
		void  SetMaxLength(int Length);
		int   GetLength();
		int   GetStrSize(int Row=-1);

		void  SetInputMask(const wchar_t *InputMask);
		const wchar_t* GetInputMask();

		void  SetOvertypeMode(int Mode);
		int   GetOvertypeMode();

		void  SetEditBeyondEnd(int Mode);

		void  SetClearFlag(int Flag);
		int   GetClearFlag();

		void  SetString(const wchar_t *Str);
		void  InsertString(const wchar_t *Str);
		void  SetHiString(const wchar_t *Str);
		void  GetString(wchar_t *Str, int MaxSize,int Row=-1); // Row==-1 - current line
		void  GetString(string &strStr,int Row=-1);            // Row==-1 - current line
		const wchar_t* GetStringAddr();

		void  SetCurPos(int NewCol, int NewRow=-1); // Row==-1 - current line
		int   GetCurPos();
		int   GetCurRow();

		int   GetTabCurPos();
		void  SetTabCurPos(int NewPos);

		void  SetPersistentBlocks(int Mode);
		int   GetPersistentBlocks();
		void  SetDelRemovesBlocks(int NewMode);
		int   GetDelRemovesBlocks();

		void  SetObjectColor(int Color,int SelColor=0xf,int ColorUnChanged=COL_DIALOGEDITUNCHANGED);
		long  GetObjectColor();
		int   GetObjectColorUnChanged();

		void  FastShow();
		int   GetLeftPos();
		void  SetLeftPos(int NewPos,int Row=-1); // Row==-1 - current line

		void  DeleteBlock();

		void  Select(int Start,int End);           // TODO: �� ������ ��� multiline!
		void  GetSelection(int &Start,int &End);   // TODO: �� ������ ��� multiline!

		void Xlat(BOOL All=FALSE);

		void SetCursorType(int Visible,int Size);
		void GetCursorType(int &Visible,int &Size);

		int  GetReadOnly();
		void SetReadOnly(int NewReadOnly);

		void SetCallbackState(bool Enable){lineEdit->SetCallbackState(Enable);}
		void SetECFlags(DWORD Flags){lineEdit->ECFlags.Set(Flags);}
		void ClearECFlags(DWORD Flags){lineEdit->ECFlags.Clear(Flags);}
		void AutoComplete(bool Manual,bool DelBlock){return lineEdit->AutoComplete(Manual,DelBlock);}
};
