#pragma once

/*
format.hpp

�������������� �����
*/
/*
Copyright (c) 2009 Far Group
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
namespace fmt
{
	class Width
	{
		size_t Value;
	public:
		Width(size_t Value=0){this->Value=Value;}
		size_t GetValue()const{return Value;}
	};

	class Precision
	{
		size_t Value;
	public:
		Precision(size_t Value=static_cast<size_t>(-1)){this->Value=Value;}
		size_t GetValue()const{return Value;}
	};

	class FillChar
	{
		WCHAR Value;
	public:
		FillChar(WCHAR Value=L' '){this->Value=Value;}
		WCHAR GetValue()const{return Value;}
	};

	class LeftAlign{};

	class RightAlign{};

	enum AlignType
	{
		A_LEFT,
		A_RIGHT,
	};

};

class BaseFormat
{
	size_t _Width;
	size_t _Precision;
	WCHAR _FillChar;
	fmt::AlignType _Align;

	void Reset();
	void Put(LPCWSTR Data,size_t Length);

protected:
	virtual void Commit(const string& Data)=0;

public:
	BaseFormat(){Reset();}
	~BaseFormat(){}

	// attributes
	void SetPrecision(size_t Precision=static_cast<size_t>(-1)){_Precision=Precision;}
	void SetWidth(size_t Width=0){_Width=Width;}
	void SetAlign(fmt::AlignType Align=fmt::A_RIGHT){_Align=Align;}
	void SetFillChar(WCHAR Char=L' '){_FillChar=Char;}

	// data
	BaseFormat& operator<<(WCHAR Value);
	BaseFormat& operator<<(INT64 Value);
	BaseFormat& operator<<(LPCWSTR Data);
	BaseFormat& operator<<(string& String);

	// manipulators
	BaseFormat& operator<<(fmt::Width& Manipulator);
	BaseFormat& operator<<(fmt::Precision& Manipulator);
	BaseFormat& operator<<(fmt::LeftAlign& Manipulator);
	BaseFormat& operator<<(fmt::RightAlign& Manipulator);
	BaseFormat& operator<<(fmt::FillChar& Manipulator);
};

class FormatString:public BaseFormat
{
	string Value;
	void Commit(const string& Data){Value+=Data;}
public:
	const string& strValue()const{return Value;}
	void Clear(){Value.SetLength(0);}
};

class FormatScreen:public BaseFormat
{
	void Commit(const string& Data);
};
