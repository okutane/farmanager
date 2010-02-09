#pragma once

/*
syntax.hpp

���������� ������� ��� MacroDrive II

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

//---------------------------------------------------------------
// If this code works, it was written by Alexander Nazarenko.
// If not, I don't know who wrote it.
//---------------------------------------------------------------

struct TMacroKeywords
{
	int Type;              // ���: 0=Area, 1=Flags, 2=Condition
	const wchar_t *Name;   // ������������
	DWORD Value;           // ��������
	DWORD Reserved;
};

// � plugin.hpp ��� FARMACROPARSEERRORCODE
enum errParseCode
{
	err_Success,
	err_Unrecognized_keyword,
	err_Unrecognized_function,
	err_Func_Param,
	err_Not_expected_ELSE,
	err_Not_expected_END,
	err_Unexpected_EOS,
	err_Expected,
	err_Bad_Hex_Control_Char,
	err_Bad_Control_Char,
	err_Var_Expected,
	err_Expr_Expected,
	err_ZeroLengthMacro,
};

extern int MKeywordsSize;
extern struct TMacroKeywords MKeywords[];
extern int MKeywordsFlagsSize;
extern struct TMacroKeywords MKeywordsFlags[];

int __parseMacroString(DWORD *&CurMacroBuffer, int &CurMacroBufferSize, const wchar_t *BufPtr);
BOOL __getMacroParseError(DWORD* ErrCode, COORD* ErrPos, string *ErrSrc);
BOOL __getMacroParseError(string* Err1, string* Err2, string* Err3, string* Err4);
int  __getMacroErrorCode(int *nErr=NULL);
