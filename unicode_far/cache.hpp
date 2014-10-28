#pragma once

/*
cache.hpp

����������� ������ � ����/������ �� �����
*/
/*
Copyright � 2009 Far Group
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

class CachedRead: NonCopyable
{
public:
	CachedRead(api::File& file, DWORD buffer_size=0);
	~CachedRead();
	void AdjustAlignment(); // file have to be opened already
	bool Read(LPVOID Data, DWORD DataSize, LPDWORD BytesRead);
	bool FillBuffer();
	bool Unread(DWORD BytesUnread);
	void Clear();

private:
	api::File& file;
	static const DWORD DefaultBufferSize = 0x10000;
	DWORD ReadSize;
	DWORD BytesLeft;
	INT64 LastPtr;
	int Alignment;
	std::vector<BYTE> Buffer; // = 2*k*Alignment (k >= 2)
};


class CachedWrite: NonCopyable
{
public:
	CachedWrite(api::File& file);
	~CachedWrite();
	bool Write(LPCVOID Data, size_t DataSize);
	bool Flush();

private:
	api::File& file;
	std::vector<BYTE> Buffer;
	size_t FreeSize;
	bool Flushed;
};