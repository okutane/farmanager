#pragma once

/*
ctrlobj.hpp

���������� ���������� ���������, ������� ��������� ���������� � ����
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

#include "windowsfwd.hpp"
#include "macro.hpp"

class CommandLine;
class History;
class KeyBar;
class MenuBar;
class HighlightFiles;
class FilePositionCache;
class Shortcuts;
class WindowHandler;
class PluginManager;
class Manager;

class ControlObject: NonCopyable
{
public:
	ControlObject();
	~ControlObject();

	void Init(int DirCount);
	FilePanels *Cp();
	void CreateDummyFilePanels();
	static void ShowCopyright(DWORD Flags=0);

	KeyMacro Macro;
	desktop_ptr Desktop;
	std::unique_ptr<HighlightFiles> HiFiles;
	std::unique_ptr<PluginManager> Plugins;

	std::unique_ptr<History> CmdHistory;
	std::unique_ptr<History> FolderHistory;
	std::unique_ptr<History> ViewHistory;
	std::unique_ptr<CommandLine> CmdLine;
	std::unique_ptr<MenuBar> TopMenuBar;

private:
	filepanels_ptr FPanels;
};