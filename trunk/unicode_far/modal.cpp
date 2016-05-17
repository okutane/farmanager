﻿/*
modal.cpp

привет автодетектор кодировки!
Parent class для модальных объектов
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

#include "modal.hpp"
#include "help.hpp"

void SimpleModal::Process()
{
	Global->WindowManager->ExecuteWindow(shared_from_this());
	Global->WindowManager->ExecuteModal(shared_from_this());
}

bool SimpleModal::Done() const
{
	return m_EndLoop;
}


void SimpleModal::ClearDone()
{
	m_EndLoop=false;
}

void SimpleModal::SetDone(void)
{
	m_EndLoop=true;
}

void SimpleModal::SetExitCode(int Code)
{
	m_ExitCode=Code;
	SetDone();
}

void SimpleModal::Close(int Code)
{
	SetExitCode(Code);
	Hide();
	Global->WindowManager->DeleteWindow(shared_from_this());
}

void SimpleModal::SetHelp(const wchar_t *Topic)
{
	m_HelpTopic = Topic;
}


void SimpleModal::ShowHelp() const
{
	if (!m_HelpTopic.empty())
		Help::create(m_HelpTopic);
}
