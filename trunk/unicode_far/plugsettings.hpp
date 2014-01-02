#pragma once

/*
plugsettings.hpp

API ��� �������� ��������� ��������.
*/
/*
Copyright � 2011 Far Group
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

#include "configdb.hpp"

class AbstractSettings
{
	private:
		char* Add(const wchar_t* Data,size_t Size);
	protected:
		std::list<char_ptr> m_Data;
		char* Add(const string& String);
		char* Add(size_t Size);
	public:
		virtual ~AbstractSettings(){};
		virtual bool IsValid(void);
		virtual int Set(const FarSettingsItem& Item)=0;
		virtual int Get(FarSettingsItem& Item)=0;
		virtual int Enum(FarSettingsEnum& Enum)=0;
		virtual int Delete(const FarSettingsValue& Value)=0;
		virtual int SubKey(const FarSettingsValue& Value, bool bCreate)=0;
};

class FarSettingsNameItems: NonCopyable
{
public:
	FarSettingsNameItems() {}
	FarSettingsNameItems(FarSettingsNameItems&& rhs) { *this = std::move(rhs); }
	~FarSettingsNameItems()
	{
		std::for_each(CONST_RANGE(Items, i)
		{
			delete[] i.Name;
		});
	}

	MOVE_OPERATOR_BY_SWAP(FarSettingsNameItems);
	
	void swap(FarSettingsNameItems& rhs)
	{
		Items.swap(rhs.Items);
	}

	std::vector<FarSettingsName> Items;
};

STD_SWAP_SPEC(FarSettingsNameItems);

class PluginSettings: public AbstractSettings
{
	private:
		std::vector<FarSettingsNameItems> m_Enum;
		std::vector<unsigned __int64> m_Keys;
		HierarchicalConfigUniquePtr PluginsCfg;
		PluginSettings();
	public:
		PluginSettings(const GUID& Guid, bool Local);
		bool IsValid(void);
		int Set(const FarSettingsItem& Item);
		int Get(FarSettingsItem& Item);
		int Enum(FarSettingsEnum& Enum);
		int Delete(const FarSettingsValue& Value);
		int SubKey(const FarSettingsValue& Value, bool bCreate);
};

class FarSettingsHistoryItems: NonCopyable
{
public:
	FarSettingsHistoryItems() {}
	FarSettingsHistoryItems(FarSettingsHistoryItems&& rhs) { *this = std::move(rhs); }
	~FarSettingsHistoryItems()
	{
		std::for_each(CONST_RANGE(Items, i)
		{
			delete[] i.Name;
			delete[] i.Param;
			delete[] i.File;
		});
	}

	MOVE_OPERATOR_BY_SWAP(FarSettingsHistoryItems);

	void swap(FarSettingsHistoryItems& rhs)
	{
		Items.swap(rhs.Items);
	}

	std::vector<FarSettingsHistory> Items;
};

STD_SWAP_SPEC(FarSettingsHistoryItems);

class FarSettings: public AbstractSettings
{
	private:
		std::vector<FarSettingsHistoryItems> m_Enum;
		std::vector<string> m_Keys;
		typedef bool (*HistoryFilter)(int Type);
		int FillHistory(int Type,const string& HistoryName,FarSettingsEnum& Enum,HistoryFilter Filter);
	public:
		int Set(const FarSettingsItem& Item);
		int Get(FarSettingsItem& Item);
		int Enum(FarSettingsEnum& Enum);
		int Delete(const FarSettingsValue& Value);
		int SubKey(const FarSettingsValue& Value, bool bCreate);
};
