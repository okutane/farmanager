/*
window.cpp

Parent class ��� ����������� ��������
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

#include "window.hpp"
#include "keybar.hpp"
#include "manager.hpp"
#include "syslog.hpp"

static int windowID=0;

window::window():
	m_ID(windowID++),
	m_CanLoseFocus(FALSE),
	m_ExitCode(-1),
	m_KeyBarVisible(0),
	m_TitleBarVisible(0),
	m_windowKeyBar(nullptr),
	m_Deleting(false),
	m_BlockCounter(0),
	m_MacroMode(MACROAREA_OTHER)
{
	_OT(SysLog(L"[%p] window::window()", this));
}

window::~window()
{
	_OT(SysLog(L"[%p] window::~window()", this));
}

void window::UpdateKeyBar()
{
	if (m_windowKeyBar && m_KeyBarVisible)
		m_windowKeyBar->RedrawIfChanged();
}

int window::IsTopWindow() const
{
	return Global->WindowManager->GetCurrentWindow().get() == this;
}

void window::OnChangeFocus(int focus)
{
	if (focus)
	{
		Show();
	}
	else
	{
		Hide();
	}
}

bool window::CanFastHide() const
{
	return true;
}

bool window::HasSaveScreen() const
{
	if (this->SaveScr||this->ShadowSaveScr)
	{
		return true;
	}

	return false;
}

void window::SetDeleting(void)
{
	m_Deleting=true;
}

bool window::IsDeleting(void) const
{
	return m_Deleting;
}

void window::SetBlock(void)
{
	++m_BlockCounter;
}

void window::RemoveBlock(void)
{
	assert(m_BlockCounter>0);
	--m_BlockCounter;
}

bool window::IsBlocked(void) const
{
	return m_BlockCounter>0;
}

void window::SetMacroMode(FARMACROAREA Area)
{
	m_MacroMode=Area;
}
