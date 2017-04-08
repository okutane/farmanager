﻿#ifndef LANGUAGE_HPP_36726BFA_4EBB_4CFF_A8F0_42434C4F4865
#define LANGUAGE_HPP_36726BFA_4EBB_4CFF_A8F0_42434C4F4865
#pragma once

/*
language.hpp

Работа с LNG-файлами
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

enum class lng;

class i_language_data
{
public:
	virtual std::unique_ptr<i_language_data> create() = 0;
	virtual ~i_language_data() = default;

	virtual void reserve(size_t Size) = 0;
	virtual void add(string&& Str) = 0;
	virtual void set_at(size_t Index, string&& Str) = 0;
	virtual const string& at(size_t Index) const = 0;
	virtual size_t size() const = 0;

	bool validate(size_t MsgId) const;

	string m_FileName;
};

class language
{
public:
	NONCOPYABLE(language);

	virtual ~language() = default;

protected:
	language(std::unique_ptr<i_language_data>& Data): m_Data(Data) {}

	// Throws on failure, strong exception safety guarantee
	void load(const string& Path, int CountNeed = -1);

private:
	std::unique_ptr<i_language_data>& m_Data;
};

class plugin_language final: public language
{
public:
	plugin_language(const string& Path);
	const wchar_t* GetMsg(intptr_t Id) const;

private:
	std::unique_ptr<i_language_data> m_Data;
};

class far_language final: private language, public singleton<far_language>
{
	IMPLEMENTS_SINGLETON(far_language);

public:
	using language::load;
	bool is_loaded() const;
	const wchar_t* GetMsg(lng Id) const;

private:
	far_language();

	std::unique_ptr<i_language_data> m_Data;
};


bool OpenLangFile(os::fs::file& LangFile, const string& Path, const string& Mask, const string& Language, string &strFileName, uintptr_t &nCodePage, bool StrongLang = false, string *pstrLangName = nullptr);
bool GetLangParam(const os::fs::file& LangFile, const string& ParamName,string *strParam1, string *strParam2, UINT nCodePage);
bool GetOptionsParam(const os::fs::file& LangFile, const wchar_t *KeyName,string &strValue, UINT nCodePage);
bool SelectInterfaceLanguage();
bool SelectHelpLanguage();

#endif // LANGUAGE_HPP_36726BFA_4EBB_4CFF_A8F0_42434C4F4865
