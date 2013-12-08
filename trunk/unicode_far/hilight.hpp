#pragma once

/*
hilight.hpp

Files highlighting
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

class VMenu2;
class FileFilterParams;
class FileList;
struct FileListItem;

class HighlightFiles
{
public:
	enum highlight_color
	{
		NORMAL_COLOR,
		SELECTED_COLOR,
		UNDERCURSOR_COLOR,
		SELECTEDUNDERCURSOR_COLOR,

		HIGHLIGHT_COUNT
	};

	struct highlight_item
	{
		struct
		{
			FarColor FileColor;
			FarColor MarkColor;
		}
		Color[HIGHLIGHT_COUNT];

		struct
		{
			wchar_t Char;
			bool Transparent;
		}
		Mark;
	};

	HighlightFiles();

	void UpdateCurrentTime();
	void GetHiColor(FileListItem* To, bool UseAttrHighlighting = false);
	int GetGroup(const FileListItem *fli);
	void HiEdit(int MenuPos);
	void UpdateHighlighting(bool RefreshMasks = false);
	void SaveHiData();

private:
	void InitHighlightFiles(class HierarchicalConfig* cfg);
	void ClearData();
	int  MenuPosToRealPos(int MenuPos, int **Count, bool Insert=false);
	void FillMenu(VMenu2 *HiMenu,int MenuPos);
	void ProcessGroups();

	std::vector<std::unique_ptr<FileFilterParams>> HiData;
	int FirstCount, UpperCount, LowerCount, LastCount;
	unsigned __int64 CurrentTime;
	bool Changed;
};
