/*
Global->Console->cpp

Console functions
*/
/*
Copyright � 2010 Far Group
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

#include "console.hpp"
#include "imports.hpp"
#include "config.hpp"
#include "palette.hpp"
#include "colors.hpp"
#include "colormix.hpp"
#include "interf.hpp"
#include "setcolor.hpp"

class basicconsole:public console {
public:

basicconsole():
	// �����/������ �������� �� 32 K, ����� ��������.
	MAXSIZE(0x8000)
{}

virtual ~basicconsole() {}

virtual bool Allocate() const
{
	return AllocConsole()!=FALSE;
}

virtual bool Free() const
{
	return FreeConsole()!=FALSE;
}

virtual HANDLE GetInputHandle() const
{
	return GetStdHandle(STD_INPUT_HANDLE);
}

virtual HANDLE GetOutputHandle() const
{
	return GetStdHandle(STD_OUTPUT_HANDLE);
}

virtual HANDLE GetErrorHandle() const
{
	return GetStdHandle(STD_ERROR_HANDLE);
}

virtual HWND GetWindow() const
{
	return GetConsoleWindow();
}

virtual bool GetSize(COORD& Size) const
{
	bool Result=false;
	CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;
	if(GetConsoleScreenBufferInfo(GetOutputHandle(), &ConsoleScreenBufferInfo))
	{
		if(Global->Opt->WindowMode)
		{
			Size.X=ConsoleScreenBufferInfo.srWindow.Right-ConsoleScreenBufferInfo.srWindow.Left+1;
			Size.Y=ConsoleScreenBufferInfo.srWindow.Bottom-ConsoleScreenBufferInfo.srWindow.Top+1;
		}
		else
		{
			Size=ConsoleScreenBufferInfo.dwSize;
		}
		Result=true;
	}
	return Result;
}

virtual bool SetSize(COORD Size) const
{
	bool Result=false;
	if(Global->Opt->WindowMode)
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(GetOutputHandle(), &csbi);
		csbi.srWindow.Left=0;
		csbi.srWindow.Right=Size.X-1;
		csbi.srWindow.Bottom=csbi.dwSize.Y-1;
		csbi.srWindow.Top=csbi.srWindow.Bottom-(Size.Y-1);
		COORD WindowCoord={static_cast<SHORT>(csbi.srWindow.Right-csbi.srWindow.Left+1), static_cast<SHORT>(csbi.srWindow.Bottom-csbi.srWindow.Top+1)};
		if(WindowCoord.X>csbi.dwSize.X || WindowCoord.Y>csbi.dwSize.Y)
		{
			WindowCoord.X=std::max(WindowCoord.X,csbi.dwSize.X);
			WindowCoord.Y=std::max(WindowCoord.Y,csbi.dwSize.Y);
			SetConsoleScreenBufferSize(GetOutputHandle(), WindowCoord);
		}
		if(SetWindowRect(csbi.srWindow))
		{
			csbi.dwSize.X = Size.X;
			Result=SetConsoleScreenBufferSize(GetOutputHandle(), csbi.dwSize)!=FALSE;
		}
	}
	else
	{
		Result=SetConsoleScreenBufferSize(GetOutputHandle(), Size)!=FALSE;
	}
	return Result;
}

virtual bool GetWindowRect(SMALL_RECT& ConsoleWindow) const
{
	bool Result=false;
	CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;
	if(GetConsoleScreenBufferInfo(GetOutputHandle(), &ConsoleScreenBufferInfo))
	{
		ConsoleWindow=ConsoleScreenBufferInfo.srWindow;
		Result=true;
	}
	return Result;
}

virtual bool SetWindowRect(const SMALL_RECT& ConsoleWindow) const
{
	return SetConsoleWindowInfo(GetOutputHandle(), true, &ConsoleWindow)!=FALSE;
}

virtual bool GetWorkingRect(SMALL_RECT& WorkingRect) const
{
	bool Result=false;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if(GetConsoleScreenBufferInfo(GetOutputHandle(), &csbi))
	{
		WorkingRect.Bottom=csbi.dwSize.Y-1;
		WorkingRect.Left=0;
		WorkingRect.Right=WorkingRect.Left+ScrX;
		WorkingRect.Top=WorkingRect.Bottom-ScrY;
		Result=true;
	}
	return Result;
}

virtual bool GetTitle(string &strTitle) const
{
	DWORD dwSize = 0;
	DWORD dwBufferSize = MAX_PATH;
	wchar_t *lpwszTitle = nullptr;

	do
	{
		dwBufferSize <<= 1;
		lpwszTitle = (wchar_t*)xf_realloc_nomove(lpwszTitle, dwBufferSize*sizeof(wchar_t));
		dwSize = GetConsoleTitle(lpwszTitle, dwBufferSize);
	}
	while (!dwSize && GetLastError() == ERROR_SUCCESS);

	if (dwSize)
		strTitle = lpwszTitle;

	xf_free(lpwszTitle);
	return dwSize!=0;
}

virtual bool SetTitle(LPCWSTR Title) const
{
	return SetConsoleTitle(Title)!=FALSE;
}

virtual bool GetKeyboardLayoutName(string &strName) const
{
	bool Result=false;
	strName.Clear();
	wchar_t *p = strName.GetBuffer(KL_NAMELENGTH+1);
	if (p && Global->ifn->GetConsoleKeyboardLayoutNameW(p))
	{
		Result=true;
	}
	strName.ReleaseBuffer();
	return Result;
}

virtual uintptr_t GetInputCodepage() const
{
	return GetConsoleCP();
}

virtual bool SetInputCodepage(uintptr_t Codepage) const
{
	return SetConsoleCP(Codepage)!=FALSE;
}

virtual uintptr_t GetOutputCodepage() const
{
	return GetConsoleOutputCP();
}

virtual bool SetOutputCodepage(uintptr_t Codepage) const
{
	return SetConsoleOutputCP(Codepage)!=FALSE;
}

virtual bool SetControlHandler(PHANDLER_ROUTINE HandlerRoutine, bool Add) const
{
	return SetConsoleCtrlHandler(HandlerRoutine, Add)!=FALSE;
}

virtual bool GetMode(HANDLE ConsoleHandle, DWORD& Mode) const
{
	return GetConsoleMode(ConsoleHandle, &Mode)!=FALSE;
}

virtual bool SetMode(HANDLE ConsoleHandle, DWORD Mode) const
{
	return SetConsoleMode(ConsoleHandle, Mode)!=FALSE;
}

virtual bool PeekInput(INPUT_RECORD* Buffer, size_t Length, size_t& NumberOfEventsRead) const
{
	DWORD dwNumberOfEventsRead = 0;
	bool Result=PeekConsoleInput(GetInputHandle(), Buffer, static_cast<DWORD>(Length), &dwNumberOfEventsRead)!=FALSE;
	NumberOfEventsRead = dwNumberOfEventsRead;
	if(Global->Opt->WindowMode && Buffer->EventType==MOUSE_EVENT)
	{
		Buffer->Event.MouseEvent.dwMousePosition.Y=std::max(0, Buffer->Event.MouseEvent.dwMousePosition.Y-GetDelta());
		COORD Size={};
		GetSize(Size);
		Buffer->Event.MouseEvent.dwMousePosition.X=std::min(Buffer->Event.MouseEvent.dwMousePosition.X, static_cast<SHORT>(Size.X-1));
	}
	return Result;
}

virtual bool ReadInput(INPUT_RECORD* Buffer, size_t Length, size_t& NumberOfEventsRead) const
{
	DWORD dwNumberOfEventsRead = 0;
	bool Result=ReadConsoleInput(GetInputHandle(), Buffer, static_cast<DWORD>(Length), &dwNumberOfEventsRead)!=FALSE;
	NumberOfEventsRead = dwNumberOfEventsRead;
	if(Global->Opt->WindowMode && Buffer->EventType==MOUSE_EVENT)
	{
		Buffer->Event.MouseEvent.dwMousePosition.Y=std::max(0, Buffer->Event.MouseEvent.dwMousePosition.Y-GetDelta());
		COORD Size={};
		GetSize(Size);
		Buffer->Event.MouseEvent.dwMousePosition.X=std::min(Buffer->Event.MouseEvent.dwMousePosition.X, static_cast<SHORT>(Size.X-1));
	}
	return Result;
}

virtual bool WriteInput(INPUT_RECORD* Buffer, size_t Length, size_t& NumberOfEventsWritten) const
{
	if(Global->Opt->WindowMode && Buffer->EventType==MOUSE_EVENT)
	{
		Buffer->Event.MouseEvent.dwMousePosition.Y+=GetDelta();
	}
	DWORD dwNumberOfEventsWritten = 0;
	bool Result = WriteConsoleInput(GetInputHandle(), Buffer, static_cast<DWORD>(Length), &dwNumberOfEventsWritten)!=FALSE;
	NumberOfEventsWritten = dwNumberOfEventsWritten;
	return Result;
}

virtual bool ReadOutput(FAR_CHAR_INFO* Buffer, COORD BufferSize, COORD BufferCoord, SMALL_RECT& ReadRegion) const
{
	bool Result=false;
	int Delta=Global->Opt->WindowMode?GetDelta():0;
	ReadRegion.Top+=Delta;
	ReadRegion.Bottom+=Delta;

	// skip unused region
	int Offset = BufferCoord.Y*BufferSize.X;
	array_ptr<CHAR_INFO> ConsoleBuffer(BufferSize.X*BufferSize.Y-Offset, true);
	BufferSize.Y-=BufferCoord.Y;
	BufferCoord.Y=0;

	if(BufferSize.X*BufferSize.Y*sizeof(CHAR_INFO)>MAXSIZE)
	{
		SHORT SavedY = BufferSize.Y;
		BufferSize.Y=static_cast<SHORT>(std::max(static_cast<int>(MAXSIZE/(BufferSize.X*sizeof(CHAR_INFO))),1));
		int Height=ReadRegion.Bottom-ReadRegion.Top+1;
		int Start=ReadRegion.Top;
		SMALL_RECT SavedReadRegion=ReadRegion;
		for(int i=0;i<Height;i+=BufferSize.Y)
		{
			ReadRegion=SavedReadRegion;
			ReadRegion.Top=Start+i;
			PCHAR_INFO BufPtr=ConsoleBuffer.get()+i*BufferSize.X;
			Result=ReadConsoleOutput(GetOutputHandle(), BufPtr, BufferSize, BufferCoord, &ReadRegion)!=FALSE;
		}
		BufferSize.Y = SavedY;
	}
	else
	{
		Result=ReadConsoleOutput(GetOutputHandle(), ConsoleBuffer.get(), BufferSize, BufferCoord, &ReadRegion)!=FALSE;
	}

	for(int i = 0; i < BufferSize.X*BufferSize.Y; ++i)
	{
		Buffer[i+Offset].Char = ConsoleBuffer[i].Char.UnicodeChar;
		Colors::ConsoleColorToFarColor(ConsoleBuffer[i].Attributes, Buffer[i+Offset].Attributes);
	}

	if(Global->Opt->WindowMode)
	{
		ReadRegion.Top-=Delta;
		ReadRegion.Bottom-=Delta;
	}

	return Result;
}

virtual bool WriteOutput(const FAR_CHAR_INFO* Buffer, COORD BufferSize, COORD BufferCoord, SMALL_RECT& WriteRegion) const
{
	bool Result=false;
	int Delta=Global->Opt->WindowMode?GetDelta():0;
	WriteRegion.Top+=Delta;
	WriteRegion.Bottom+=Delta;

	// skip unused region
	int Offset = BufferCoord.Y*BufferSize.X;
	array_ptr<CHAR_INFO> ConsoleBuffer(BufferSize.X*BufferSize.Y-Offset);
	for(int i = Offset; i < BufferSize.X*BufferSize.Y; ++i)
	{
		ConsoleBuffer[i-Offset].Char.UnicodeChar = Buffer[i].Char;
		ConsoleBuffer[i-Offset].Attributes = Colors::FarColorToConsoleColor(Buffer[i].Attributes);
	}

	BufferSize.Y-=BufferCoord.Y;
	BufferCoord.Y=0;

	if(BufferSize.X*BufferSize.Y*sizeof(CHAR_INFO)>MAXSIZE)
	{
		SHORT SavedY = BufferSize.Y;
		BufferSize.Y=static_cast<SHORT>(std::max(static_cast<int>(MAXSIZE/(BufferSize.X*sizeof(CHAR_INFO))),1));
		int Height=WriteRegion.Bottom-WriteRegion.Top+1;
		int Start=WriteRegion.Top;
		SMALL_RECT SavedWriteRegion=WriteRegion;
		for(int i=0;i<Height;i+=BufferSize.Y)
		{
			WriteRegion=SavedWriteRegion;
			WriteRegion.Top=Start+i;
			const CHAR_INFO* BufPtr=ConsoleBuffer.get()+i*BufferSize.X;
			Result=WriteConsoleOutput(GetOutputHandle(), BufPtr, BufferSize, BufferCoord, &WriteRegion)!=FALSE;
		}
		BufferSize.Y = SavedY;
	}
	else
	{
		Result=WriteConsoleOutput(GetOutputHandle(), ConsoleBuffer.get(), BufferSize, BufferCoord, &WriteRegion)!=FALSE;
	}

	if(Global->Opt->WindowMode)
	{
		WriteRegion.Top-=Delta;
		WriteRegion.Bottom-=Delta;
	}

	return Result;
}

virtual bool Write(LPCWSTR Buffer) const
{
	return Write(Buffer, StrLength(Buffer));
}

virtual bool Write(const string& Buffer) const
{
	return Write(Buffer, Buffer.GetLength());
}

virtual bool Write(LPCWSTR Buffer, size_t NumberOfCharsToWrite) const
{
	bool Result = false;
	DWORD NumberOfCharsWritten;
	HANDLE OutputHandle = GetOutputHandle();
	DWORD Mode;
	if(GetMode(OutputHandle, Mode))
	{
		Result =  WriteConsole(OutputHandle, Buffer, static_cast<DWORD>(NumberOfCharsToWrite), &NumberOfCharsWritten, nullptr)!=FALSE;
	}
	else
	{
		Result = WriteFile(OutputHandle, Buffer, static_cast<DWORD>(NumberOfCharsToWrite*sizeof(wchar_t)), &NumberOfCharsWritten, nullptr)!=FALSE;
	}
	return Result;
}

virtual bool Commit() const
{
	// reserved
	return true;
}

virtual bool GetTextAttributes(FarColor& Attributes) const
{
	bool Result=false;
	CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;
	if(GetConsoleScreenBufferInfo(GetOutputHandle(), &ConsoleScreenBufferInfo))
	{
		Colors::ConsoleColorToFarColor(ConsoleScreenBufferInfo.wAttributes, Attributes);
		Result=true;
	}
	return Result;
}

virtual bool SetTextAttributes(const FarColor& Attributes) const
{
	return SetConsoleTextAttribute(GetOutputHandle(), Colors::FarColorToConsoleColor(Attributes))!=FALSE;
}

virtual bool GetCursorInfo(CONSOLE_CURSOR_INFO& ConsoleCursorInfo) const
{
	return GetConsoleCursorInfo(GetOutputHandle(), &ConsoleCursorInfo)!=FALSE;
}

virtual bool SetCursorInfo(const CONSOLE_CURSOR_INFO& ConsoleCursorInfo) const
{
	return SetConsoleCursorInfo(GetOutputHandle(), &ConsoleCursorInfo)!=FALSE;
}

virtual bool GetCursorPosition(COORD& Position) const
{
	bool Result=false;
	CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;
	if(GetConsoleScreenBufferInfo(GetOutputHandle(), &ConsoleScreenBufferInfo))
	{
		Position=ConsoleScreenBufferInfo.dwCursorPosition;
		if(Global->Opt->WindowMode)
		{
			Position.Y-=GetDelta();
		}
		Result=true;
	}
	return Result;
}

virtual bool SetCursorPosition(COORD Position) const
{

	if(Global->Opt->WindowMode)
	{
		ResetPosition();
		COORD Size={};
		GetSize(Size);
		Position.X=std::min(Position.X,static_cast<SHORT>(Size.X-1));
		Position.Y=std::max(static_cast<SHORT>(0),Position.Y);
		Position.Y+=GetDelta();
	}
	return SetConsoleCursorPosition(GetOutputHandle(), Position)!=FALSE;
}

virtual bool FlushInputBuffer() const
{
	return FlushConsoleInputBuffer(GetInputHandle())!=FALSE;
}

virtual bool GetNumberOfInputEvents(size_t& NumberOfEvents) const
{
	DWORD dwNumberOfEvents = 0;
	bool Result = GetNumberOfConsoleInputEvents(GetInputHandle(), &dwNumberOfEvents)!=FALSE;
	NumberOfEvents = dwNumberOfEvents;
	return Result;
}

virtual bool GetAlias(LPCWSTR Source, LPWSTR TargetBuffer, size_t TargetBufferLength, LPCWSTR ExeName) const
{
	return GetConsoleAlias(const_cast<LPWSTR>(Source), TargetBuffer, static_cast<DWORD>(TargetBufferLength), const_cast<LPWSTR>(ExeName))!=0;
}

virtual bool GetDisplayMode(DWORD& Mode) const
{
	return GetConsoleDisplayMode(&Mode)!=FALSE;
}

virtual COORD GetLargestWindowSize() const
{
	COORD Result = GetLargestConsoleWindowSize(GetOutputHandle());
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetOutputHandle(), &csbi);
	if(csbi.dwSize.Y > Result.Y)
	{
		CONSOLE_FONT_INFO FontInfo;
		if (GetCurrentConsoleFont(GetOutputHandle(), FALSE, &FontInfo))
		{
			// in XP FontInfo.dwFontSize contains something else than size in pixels.
			FontInfo.dwFontSize = GetConsoleFontSize(GetOutputHandle(), FontInfo.nFont);
			Result.X -= Round(static_cast<SHORT>(GetSystemMetrics(SM_CXVSCROLL)), FontInfo.dwFontSize.X);
		}
	}
	return Result;
}

virtual bool SetActiveScreenBuffer(HANDLE ConsoleOutput) const
{
	return SetConsoleActiveScreenBuffer(ConsoleOutput)!=FALSE;
}

virtual bool ClearExtraRegions(const FarColor& Color) const
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetOutputHandle(), &csbi);
	DWORD TopSize = csbi.dwSize.X*csbi.srWindow.Top;
	DWORD CharsWritten;
	COORD TopCoord = {};
	FillConsoleOutputCharacter(GetOutputHandle(), L' ', TopSize, TopCoord, &CharsWritten);
	WORD ConColor = Colors::FarColorToConsoleColor(Color);
	FillConsoleOutputAttribute(GetOutputHandle(), ConColor, TopSize, TopCoord, &CharsWritten );

	DWORD RightSize = csbi.dwSize.X-csbi.srWindow.Right;
	COORD RightCoord={csbi.srWindow.Right,GetDelta()};
	for(; RightCoord.Y<csbi.dwSize.Y; RightCoord.Y++)
	{
		FillConsoleOutputCharacter(GetOutputHandle(), L' ', RightSize, RightCoord, &CharsWritten);
		FillConsoleOutputAttribute(GetOutputHandle(), ConColor, RightSize, RightCoord, &CharsWritten);
	}
	return true;
}

virtual bool ScrollWindow(int Lines,int Columns) const
{
	bool process=false;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetOutputHandle(), &csbi);

	if((Lines<0 && csbi.srWindow.Top) || (Lines>0 && csbi.srWindow.Bottom!=csbi.dwSize.Y-1))
	{
		csbi.srWindow.Top+=Lines;
		csbi.srWindow.Bottom+=Lines;

		if(csbi.srWindow.Top<0)
		{
			csbi.srWindow.Bottom-=csbi.srWindow.Top;
			csbi.srWindow.Top=0;
		}

		if(csbi.srWindow.Bottom>=csbi.dwSize.Y)
		{
			csbi.srWindow.Top-=(csbi.srWindow.Bottom-(csbi.dwSize.Y-1));
			csbi.srWindow.Bottom=csbi.dwSize.Y-1;
		}
		process=true;
	}

	if((Columns<0 && csbi.srWindow.Left) || (Columns>0 && csbi.srWindow.Right!=csbi.dwSize.X-1))
	{
		csbi.srWindow.Left+=Columns;
		csbi.srWindow.Right+=Columns;

		if(csbi.srWindow.Left<0)
		{
			csbi.srWindow.Right-=csbi.srWindow.Left;
			csbi.srWindow.Left=0;
		}

		if(csbi.srWindow.Right>=csbi.dwSize.X)
		{
			csbi.srWindow.Left-=(csbi.srWindow.Right-(csbi.dwSize.X-1));
			csbi.srWindow.Right=csbi.dwSize.X-1;
		}
		process=true;
	}

	if (process)
	{
		SetWindowRect(csbi.srWindow);
		return true;
	}

	return false;
}

virtual bool ScrollWindowToBegin() const
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetOutputHandle(), &csbi);

	if(csbi.srWindow.Top > 0)
	{
		csbi.srWindow.Bottom-=csbi.srWindow.Top;
		csbi.srWindow.Top=0;
		SetWindowRect(csbi.srWindow);
		return true;
	}

	return false;
}

virtual bool ScrollWindowToEnd() const
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetOutputHandle(), &csbi);

	if(csbi.srWindow.Bottom < csbi.dwSize.Y - 1)
	{
		csbi.srWindow.Top += csbi.dwSize.Y - 1 - csbi.srWindow.Bottom;
		csbi.srWindow.Bottom = csbi.dwSize.Y - 1;
		SetWindowRect(csbi.srWindow);
		return true;
	}

	return false;
}

virtual bool ScrollScreenBuffer(int Lines) const
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetOutputHandle(), &csbi);
	SMALL_RECT ScrollRectangle={0, 0, static_cast<SHORT>(csbi.dwSize.X-1), static_cast<SHORT>(csbi.dwSize.Y-1)};
	COORD DestinationOrigin={0,static_cast<SHORT>(-Lines)};
	CHAR_INFO Fill={L' ', Colors::FarColorToConsoleColor(ColorIndexToColor(COL_COMMANDLINEUSERSCREEN))};
	return ScrollConsoleScreenBuffer(GetOutputHandle(), &ScrollRectangle, nullptr, DestinationOrigin, &Fill)!=FALSE;
}

virtual bool IsFullscreenSupported() const
{
#ifdef _WIN64
	return false;
#else
	bool Result = true;
	CONSOLE_SCREEN_BUFFER_INFOEX csbiex = {sizeof(csbiex)};
	if(Global->ifn->GetConsoleScreenBufferInfoEx(GetOutputHandle(), &csbiex))
	{
		Result = csbiex.bFullscreenSupported != FALSE;
	}
	return Result;
#endif
}

virtual bool ResetPosition() const
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetOutputHandle(), &csbi);
	if(csbi.srWindow.Left || csbi.srWindow.Bottom!=csbi.dwSize.Y-1)
	{
		csbi.srWindow.Right-=csbi.srWindow.Left;
		csbi.srWindow.Left=0;
		csbi.srWindow.Top+=csbi.dwSize.Y-1-csbi.srWindow.Bottom;
		csbi.srWindow.Bottom=csbi.dwSize.Y-1;
		SetWindowRect(csbi.srWindow);
	}
	return true;
}

virtual bool GetColorDialog(FarColor& Color, bool Centered, bool AddTransparent) const
{
	return GetColorDialogInternal(Color, Centered, AddTransparent);
}

virtual short GetDelta() const
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetOutputHandle(), &csbi);
	return csbi.dwSize.Y-(csbi.srWindow.Bottom-csbi.srWindow.Top+1);
}

private:
	const unsigned int MAXSIZE;
};

class extendedconsole:public basicconsole
{
public:
	extendedconsole():
		Module(LoadLibrary(L"extendedconsole.dll")),
		ImportsPresent(false)
	{
		ClearStruct(Imports);
		if(Module)
		{
			#define InitImport(Name) InitImport(Imports.p##Name, #Name)

			InitImport(ReadOutput);
			InitImport(WriteOutput);
			InitImport(Commit);
			InitImport(GetTextAttributes);
			InitImport(SetTextAttributes);
			InitImport(ClearExtraRegions);
			InitImport(GetColorDialog);

			#undef InitImport

			if(!ImportsPresent)
			{
				FreeLibrary(Module);
				Module = nullptr;
			}
		}
	}

	virtual ~extendedconsole()
	{
		if(Module)
		{
			FreeLibrary(Module);
		}
	}

	virtual bool ReadOutput(FAR_CHAR_INFO* Buffer, COORD BufferSize, COORD BufferCoord, SMALL_RECT& ReadRegion) const
	{
		bool Result = false;
		if(Imports.pReadOutput)
		{
			Result = Imports.pReadOutput(Buffer, BufferSize, BufferCoord, &ReadRegion) != FALSE;
		}
		else
		{
			Result = basicconsole::ReadOutput(Buffer, BufferSize, BufferCoord, ReadRegion);
		}
		return Result;
	}

	virtual bool WriteOutput(const FAR_CHAR_INFO* Buffer, COORD BufferSize, COORD BufferCoord, SMALL_RECT& WriteRegion) const
	{
		bool Result = false;
		if(Imports.pWriteOutput)
		{
			Result = Imports.pWriteOutput(Buffer, BufferSize, BufferCoord, &WriteRegion) != FALSE;
		}
		else
		{
			Result = basicconsole::WriteOutput(Buffer, BufferSize, BufferCoord, WriteRegion);
		}
		return Result;
	}

	virtual bool Commit() const
	{
		bool Result = false;
		if(Imports.pCommit)
		{
			Result = Imports.pCommit() != FALSE;
		}
		else
		{
			Result = basicconsole::Commit();
		}
		return Result;
	}

	virtual bool GetTextAttributes(FarColor& Attributes) const
	{
		bool Result = false;
		if(Imports.pGetTextAttributes)
		{
			Result = Imports.pGetTextAttributes(&Attributes) != FALSE;
		}
		else
		{
			Result = basicconsole::GetTextAttributes(Attributes);
		}
		return Result;
	}

	virtual bool SetTextAttributes(const FarColor& Attributes) const
	{
		bool Result = false;
		if(Imports.pSetTextAttributes)
		{
			Result = Imports.pSetTextAttributes(&Attributes) != FALSE;
		}
		else
		{
			Result = basicconsole::SetTextAttributes(Attributes);
		}
		return Result;
	}

	virtual bool ClearExtraRegions(const FarColor& Color) const
	{
		bool Result = false;
		if(Imports.pClearExtraRegions)
		{
			Result = Imports.pClearExtraRegions(&Color) != FALSE;
		}
		else
		{
			Result = basicconsole::ClearExtraRegions(Color);
		}
		return Result;
	}

	virtual bool GetColorDialog(FarColor& Color, bool Centered, bool AddTransparent) const
	{
		bool Result = false;
		if(Imports.pGetColorDialog)
		{
			Result = Imports.pGetColorDialog(&Color, Centered, AddTransparent) != FALSE;
		}
		else
		{
			Result = basicconsole::GetColorDialog(Color, Centered, AddTransparent);
		}
		return Result;
	}

private:
	typedef BOOL (WINAPI *READOUTPUT)(FAR_CHAR_INFO* Buffer, COORD BufferSize, COORD BufferCoord, SMALL_RECT* ReadRegion);
	typedef BOOL (WINAPI *WRITEOUTPUT)(const FAR_CHAR_INFO* Buffer, COORD BufferSize, COORD BufferCoord, SMALL_RECT* WriteRegion);
	typedef BOOL (WINAPI *COMMIT)();
	typedef BOOL (WINAPI *GETTEXTATTRIBUTES)(FarColor* Attributes);
	typedef BOOL (WINAPI *SETTEXTATTRIBUTES)(const FarColor* Attributes);
	typedef BOOL (WINAPI *CLEAREXTRAREGIONS)(const FarColor* Color);
	typedef BOOL (WINAPI *GETCOLORDIALOG)(FarColor* Color, BOOL Centered, BOOL AddTransparent);

	struct ModuleImports
	{
		READOUTPUT pReadOutput;
		WRITEOUTPUT pWriteOutput;
		COMMIT pCommit;
		GETTEXTATTRIBUTES pGetTextAttributes;
		SETTEXTATTRIBUTES pSetTextAttributes;
		CLEAREXTRAREGIONS pClearExtraRegions;
		GETCOLORDIALOG pGetColorDialog;
	}
	Imports;

	template<typename T>
	inline void InitImport(T& Address, const char * ProcName)
	{
		Address = reinterpret_cast<T>(GetProcAddress(Module, ProcName));
		if (!ImportsPresent)
		{
			ImportsPresent = Address != nullptr;
		}
	}

	HMODULE Module;
	bool ImportsPresent;
};

console* console::CreateInstance(bool extended)
{
	return extended? new extendedconsole() : new basicconsole();
}
