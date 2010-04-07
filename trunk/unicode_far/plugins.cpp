/*
plugins.cpp

������ � ��������� (������ �������, ���-��� ������ � flplugin.cpp)
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

#include "headers.hpp"
#pragma hdrstop

#include "plugins.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "flink.hpp"
#include "scantree.hpp"
#include "chgprior.hpp"
#include "constitle.hpp"
#include "cmdline.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "vmenu.hpp"
#include "dialog.hpp"
#include "rdrwdsk.hpp"
#include "savescr.hpp"
#include "ctrlobj.hpp"
#include "scrbuf.hpp"
#include "udlist.hpp"
#include "farexcpt.hpp"
#include "fileedit.hpp"
#include "RefreshFrameManager.hpp"
#include "registry.hpp"
#include "plugapi.hpp"
#include "TaskBar.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "processname.hpp"
#include "interf.hpp"

const wchar_t *FmtPluginsCache_PluginS=L"PluginsCache\\%s";
const wchar_t *FmtDiskMenuStringD=L"DiskMenuString%d";
const wchar_t *FmtDiskMenuNumberD=L"DiskMenuNumber%d";
const wchar_t *FmtPluginMenuStringD=L"PluginMenuString%d";
const wchar_t *FmtPluginConfigStringD=L"PluginConfigString%d";

static const wchar_t *PluginsFolderName=L"Plugins";

static const wchar_t *RKN_PluginsCache=L"PluginsCache";

static int _cdecl PluginsSort(const void *el1,const void *el2);

unsigned long CRC32(
    unsigned long crc,
    const char *buf,
    unsigned int len
)
{
	static unsigned long crc_table[256];

	if (!crc_table[1])
	{
		unsigned long c;
		int n, k;

		for (n = 0; n < 256; n++)
		{
			c = (unsigned long)n;

			for (k = 0; k < 8; k++) c = (c >> 1) ^(c & 1 ? 0xedb88320L : 0);

			crc_table[n] = c;
		}
	}

	crc = crc ^ 0xffffffffL;

	while (len-- > 0)
	{
		crc = crc_table[(crc ^(*buf++)) & 0xff] ^(crc >> 8);
	}

	return crc ^ 0xffffffffL;
}


#define CRC32_SETSTARTUPINFO    0xF537107A
#define CRC32_GETPLUGININFO     0xDB6424B4
#define CRC32_OPENPLUGIN        0x601AEDE8
#define CRC32_OPENFILEPLUGIN    0xAC9FF5CD
#define CRC32_EXITFAR           0x04419715
#define CRC32_SETFINDLIST       0x7A74A2E5
#define CRC32_CONFIGURE         0x4DC1BC1A
#define CRC32_GETMINFARVERSION  0x2BBAD952

#define CRC32_SETSTARTUPINFOW   0x972884E8
#define CRC32_GETPLUGININFOW    0xEBDA386B
#define CRC32_OPENPLUGINW       0x89BC5B7D
#define CRC32_OPENFILEPLUGINW   0xC2740A22
#define CRC32_EXITFARW          0x4AD48EA6
#define CRC32_SETFINDLISTW      0xF717498F
#define CRC32_CONFIGUREW        0xDA22131C
#define CRC32_GETMINFARVERSIONW 0xA243A1DB

DWORD ExportCRC32[] =
{
	CRC32_SETSTARTUPINFO,
	CRC32_GETPLUGININFO,
	CRC32_OPENPLUGIN,
	CRC32_OPENFILEPLUGIN,
	CRC32_EXITFAR,
	CRC32_SETFINDLIST,
	CRC32_CONFIGURE,
	CRC32_GETMINFARVERSION
};

DWORD ExportCRC32W[] =
{
	CRC32_SETSTARTUPINFOW,
	CRC32_GETPLUGININFOW,
	CRC32_OPENPLUGINW,
	CRC32_OPENFILEPLUGINW,
	CRC32_EXITFARW,
	CRC32_SETFINDLISTW,
	CRC32_CONFIGUREW,
	CRC32_GETMINFARVERSIONW
};

enum PluginType
{
	NOT_PLUGIN,
	UNICODE_PLUGIN,
	OEM_PLUGIN,
};

PluginType IsModulePlugin2(
    PBYTE hModule
)
{
	DWORD dwExportAddr;
	PIMAGE_DOS_HEADER pDOSHeader = (PIMAGE_DOS_HEADER)hModule;
	PIMAGE_NT_HEADERS pPEHeader;
	__try
	{

		if (pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE)
			return NOT_PLUGIN;

		pPEHeader = (PIMAGE_NT_HEADERS)&hModule[pDOSHeader->e_lfanew];

		if (pPEHeader->Signature != IMAGE_NT_SIGNATURE)
			return NOT_PLUGIN;

		if ((pPEHeader->FileHeader.Characteristics & IMAGE_FILE_DLL) == 0)
			return NOT_PLUGIN;

		if (pPEHeader->FileHeader.Machine!=
#ifdef _WIN64
#ifdef _M_IA64
		        IMAGE_FILE_MACHINE_IA64
#else
		        IMAGE_FILE_MACHINE_AMD64
#endif
#else
		        IMAGE_FILE_MACHINE_I386
#endif
		   )
			return NOT_PLUGIN;

		dwExportAddr = pPEHeader->OptionalHeader.DataDirectory[0].VirtualAddress;

		if (!dwExportAddr)
			return NOT_PLUGIN;

		PIMAGE_SECTION_HEADER pSection = (PIMAGE_SECTION_HEADER)IMAGE_FIRST_SECTION(pPEHeader);

		for (int i = 0; i < pPEHeader->FileHeader.NumberOfSections; i++)
		{
			if ((pSection[i].VirtualAddress == dwExportAddr) ||
			        ((pSection[i].VirtualAddress <= dwExportAddr) && ((pSection[i].Misc.VirtualSize+pSection[i].VirtualAddress) > dwExportAddr)))
			{
				int nDiff = pSection[i].VirtualAddress-pSection[i].PointerToRawData;
				PIMAGE_EXPORT_DIRECTORY pExportDir = (PIMAGE_EXPORT_DIRECTORY)&hModule[dwExportAddr-nDiff];
				DWORD* pNames = (DWORD *)&hModule[pExportDir->AddressOfNames-nDiff];
				bool bOemExports=false;

				for (DWORD n = 0; n < pExportDir->NumberOfNames; n++)
				{
					const char *lpExportName = (const char *)&hModule[pNames[n]-nDiff];
					DWORD dwCRC32 = CRC32(0, lpExportName, (unsigned int)strlen(lpExportName));

					// � ��� ��� �� ��� ����� ���, ��� ��� �����������, ���� 8-)
					for (size_t j = 0; j < countof(ExportCRC32W); j++)
						if (dwCRC32 == ExportCRC32W[j])
							return UNICODE_PLUGIN;

					if (!bOemExports)
						for (size_t j = 0; j < countof(ExportCRC32); j++)
							if (dwCRC32 == ExportCRC32[j])
								bOemExports=true;
				}

				if (bOemExports)
					return OEM_PLUGIN;
			}
		}

		return NOT_PLUGIN;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return NOT_PLUGIN;
	}
}

PluginType IsModulePlugin(const wchar_t *lpModuleName)
{
	PluginType bResult = NOT_PLUGIN;
	HANDLE hModuleFile = apiCreateFile(
	                         lpModuleName,
	                         GENERIC_READ,
	                         FILE_SHARE_READ,
	                         nullptr,
	                         OPEN_EXISTING,
	                         0
	                     );

	if (hModuleFile != INVALID_HANDLE_VALUE)
	{
		HANDLE hModuleMapping = CreateFileMapping(
		                            hModuleFile,
		                            nullptr,
		                            PAGE_READONLY,
		                            0,
		                            0,
		                            nullptr
		                        );

		if (hModuleMapping)
		{
			PBYTE pData = (PBYTE)MapViewOfFile(hModuleMapping, FILE_MAP_READ, 0, 0, 0);

			if (pData)
			{
				bResult = IsModulePlugin2(pData);
				UnmapViewOfFile(pData);
			}

			CloseHandle(hModuleMapping);
		}

		CloseHandle(hModuleFile);
	}

	return bResult;
}


PluginManager::PluginManager():
	PluginsData(nullptr),
	PluginsCount(0),
	OemPluginsCount(0),
	CurPluginItem(nullptr),
	CurEditor(nullptr),
	CurViewer(nullptr)
{
}

PluginManager::~PluginManager()
{
	CurPluginItem=nullptr;
	Plugin *pPlugin;

	for (int i = 0; i < PluginsCount; i++)
	{
		pPlugin = PluginsData[i];
		pPlugin->Unload(true);
		delete pPlugin;
	}

	xf_free(PluginsData);
}

bool PluginManager::AddPlugin(Plugin *pPlugin)
{
	Plugin **NewPluginsData=(Plugin**)xf_realloc(PluginsData,sizeof(*PluginsData)*(PluginsCount+1));

	if (!NewPluginsData)
		return false;

	PluginsData = NewPluginsData;
	PluginsData[PluginsCount]=pPlugin;
	PluginsCount++;
	if(pPlugin->IsOemPlugin())
	{
		OemPluginsCount++;
	}
	return true;
}

bool PluginManager::RemovePlugin(Plugin *pPlugin)
{
	for (int i = 0; i < PluginsCount; i++)
	{
		if (PluginsData[i] == pPlugin)
		{
			if(pPlugin->IsOemPlugin())
			{
				OemPluginsCount--;
			}
			delete pPlugin;
			memmove(&PluginsData[i], &PluginsData[i+1], (PluginsCount-i-1)*sizeof(Plugin*));
			PluginsCount--;
			return true;
		}
	}

	return false;
}


bool PluginManager::LoadPlugin(
    const wchar_t *lpwszModuleName,
    const FAR_FIND_DATA_EX &FindData
)
{
	Plugin *pPlugin = nullptr;

	switch (IsModulePlugin(lpwszModuleName))
	{
		case UNICODE_PLUGIN: pPlugin = new PluginW(this, lpwszModuleName); break;
		case OEM_PLUGIN: pPlugin = new PluginA(this, lpwszModuleName); break;
		default: return false;
	}

	if (!pPlugin)
		return false;

	if (!AddPlugin(pPlugin))
	{
		delete pPlugin;
		return false;
	}

	bool bResult = pPlugin->LoadFromCache(FindData);

	if (!bResult && !Opt.LoadPlug.PluginsCacheOnly)
	{
		bResult = pPlugin->Load();

		if (!bResult)
			RemovePlugin(pPlugin);
	}

	return bResult;
}

bool PluginManager::LoadPluginExternal(const wchar_t *lpwszModuleName)
{
	Plugin *pPlugin = GetPlugin(lpwszModuleName);

	if (pPlugin)
		return true;

	FAR_FIND_DATA_EX FindData;

	if (apiGetFindDataEx(lpwszModuleName, FindData))
	{
		if (LoadPlugin(lpwszModuleName, FindData))
		{
			far_qsort(PluginsData, PluginsCount, sizeof(*PluginsData), PluginsSort);
			return true;
		}
	}

	return false;
}

int PluginManager::UnloadPlugin(Plugin *pPlugin, DWORD dwException, bool bRemove)
{
	int nResult = FALSE;

	if (pPlugin && (dwException != EXCEPT_EXITFAR))   //�������, ���� ����� � EXITFAR, �� ������� � ��������, �� � ��� � Unload
	{
		//�����-�� ���������� ��������...
		CurPluginItem=nullptr;
		Frame *frame;

		if ((frame = FrameManager->GetBottomFrame()) != nullptr)
			frame->Unlock();

		if (Flags.Check(PSIF_DIALOG))   // BugZ#52 exception handling for floating point incorrect
		{
			Flags.Clear(PSIF_DIALOG);
			FrameManager->DeleteFrame();
			FrameManager->PluginCommit();
		}

		bool bPanelPlugin = pPlugin->IsPanelPlugin();

		if (dwException != (DWORD)-1)
			nResult = pPlugin->Unload(true);
		else
			nResult = pPlugin->Unload(false);

		if (bPanelPlugin /*&& bUpdatePanels*/)
		{
			CtrlObject->Cp()->ActivePanel->SetCurDir(L".",TRUE);
			Panel *ActivePanel=CtrlObject->Cp()->ActivePanel;
			ActivePanel->Update(UPDATE_KEEP_SELECTION);
			ActivePanel->Redraw();
			Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(ActivePanel);
			AnotherPanel->Update(UPDATE_KEEP_SELECTION|UPDATE_SECONDARY);
			AnotherPanel->Redraw();
		}

		if (bRemove)
			RemovePlugin(pPlugin);
	}

	return nResult;
}

int PluginManager::UnloadPluginExternal(const wchar_t *lpwszModuleName)
{
//BUGBUG ����� �������� �� ����������� ��������
	int nResult = FALSE;
	Plugin *pPlugin = GetPlugin(lpwszModuleName);

	if (pPlugin)
	{
		nResult = pPlugin->Unload(true);
		RemovePlugin(pPlugin);
	}

	return nResult;
}

Plugin *PluginManager::GetPlugin(const wchar_t *lpwszModuleName)
{
	Plugin *pPlugin;

	for (int i = 0; i < PluginsCount; i++)
	{
		pPlugin = PluginsData[i];

		if (!StrCmpI(lpwszModuleName, pPlugin->GetModuleName()))
			return pPlugin;
	}

	return nullptr;
}

Plugin *PluginManager::GetPlugin(int PluginNumber)
{
	if (PluginNumber < PluginsCount && PluginNumber >= 0)
		return PluginsData[PluginNumber];

	return nullptr;
}

void PluginManager::LoadPlugins()
{
	TaskBar TB;
	Flags.Clear(PSIF_PLUGINSLOADDED);

	if (Opt.LoadPlug.PluginsCacheOnly)  // $ 01.09.2000 tran  '/co' switch
	{
		LoadPluginsFromCache();
	}
	else if (Opt.LoadPlug.MainPluginDir || !Opt.LoadPlug.strCustomPluginsPath.IsEmpty() || (Opt.LoadPlug.PluginsPersonal && !Opt.LoadPlug.strPersonalPluginsPath.IsEmpty()))
	{
		ScanTree ScTree(FALSE,TRUE);
		UserDefinedList PluginPathList;  // �������� ������ ���������
		string strPluginsDir;
		string strFullName;
		FAR_FIND_DATA_EX FindData;
		PluginPathList.SetParameters(0,0,ULF_UNIQUE);

		// ������� ���������� ������
		if (Opt.LoadPlug.MainPluginDir) // ������ �������� � ������������?
		{
			strPluginsDir=g_strFarPath+PluginsFolderName;
			PluginPathList.AddItem(strPluginsDir);

			// ...� ������������ ����?
			if (Opt.LoadPlug.PluginsPersonal && !Opt.LoadPlug.strPersonalPluginsPath.IsEmpty() && !(Opt.Policies.DisabledOptions&FFPOL_PERSONALPATH))
				PluginPathList.AddItem(Opt.LoadPlug.strPersonalPluginsPath);
		}
		else if (!Opt.LoadPlug.strCustomPluginsPath.IsEmpty())  // ������ "��������" ����?
		{
			PluginPathList.AddItem(Opt.LoadPlug.strCustomPluginsPath);
		}

		const wchar_t *NamePtr;
		PluginPathList.Reset();

		// ������ ��������� �� ����� ����� ���������� ������
		while (nullptr!=(NamePtr=PluginPathList.GetNext()))
		{
			// ��������� �������� ����
			apiExpandEnvironmentStrings(NamePtr,strFullName);
			Unquote(strFullName); //??? ����� ��

			if (!IsAbsolutePath(strFullName))
			{
				strPluginsDir = g_strFarPath;
				strPluginsDir += strFullName;
				strFullName = strPluginsDir;
			}

			// ������� �������� �������� ������� �������� ����
			ConvertNameToFull(strFullName,strFullName);
			ConvertNameToLong(strFullName,strFullName);
			strPluginsDir = strFullName;

			if (strPluginsDir.IsEmpty())  // ���... � ����� �� ��� ������� ����� ����� ������������ ��������� ��������?
				continue;

			// ������ �� ����� ��������� ���� �� ������...
			ScTree.SetFindPath(strPluginsDir,L"*");

			// ...� ��������� �� ����
			while (ScTree.GetNextName(&FindData,strFullName))
			{
				if (CmpName(L"*.dll",FindData.strFileName,false) && (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0)
				{
					LoadPlugin(strFullName, FindData);
				}
			} // end while
		}
	}

	Flags.Set(PSIF_PLUGINSLOADDED);
	far_qsort(PluginsData, PluginsCount, sizeof(*PluginsData), PluginsSort);
}

/* $ 01.09.2000 tran
   Load cache only plugins  - '/co' switch */
void PluginManager::LoadPluginsFromCache()
{
	/*
		[HKEY_CURRENT_USER\Software\Far2\PluginsCache\C:/PROGRAM FILES/FAR/Plugins/ABOOK/AddrBook.dll]
	*/
	size_t ShiftLen = wcslen(RKN_PluginsCache)+1;
	string strModuleName;

	for (int i=0; EnumRegKey(RKN_PluginsCache, i, strModuleName); i++)
	{
		strModuleName.LShift(ShiftLen);

		ReplaceSlashToBSlash(strModuleName);

		FAR_FIND_DATA_EX FindData;

		if (apiGetFindDataEx(strModuleName, FindData))
			LoadPlugin(strModuleName, FindData);
	}
}

int _cdecl PluginsSort(const void *el1,const void *el2)
{
	Plugin *Plugin1=*((Plugin**)el1);
	Plugin *Plugin2=*((Plugin**)el2);
	return (StrCmpI(PointToName(Plugin1->GetModuleName()),PointToName(Plugin2->GetModuleName())));
}

/* OLD
HANDLE PluginManager::OpenFilePlugin(const wchar_t *Name, const unsigned char *Data, int DataSize, int OpMode)
{
	ChangePriority ChPriority(THREAD_PRIORITY_NORMAL);

	ConsoleTitle ct(Opt.ShowCheckingFile?MSG(MCheckingFileInPlugin):nullptr);

	Plugin *pPlugin = nullptr;

	string strFullName;

	if (Name)
	{
		ConvertNameToFull(Name,strFullName);
		Name = strFullName;
	}

	for (int i = 0; i < PluginsCount; i++)
	{
		pPlugin = PluginsData[i];

		if ( !pPlugin->HasOpenFilePlugin() || (pPlugin->HasAnalyse() && pPlugin->HasOpenPlugin()) )
			continue;

		if ( Opt.ShowCheckingFile )
			ct.Set(L"%s - [%s]...",MSG(MCheckingFileInPlugin),PointToName(pPlugin->GetModuleName()));

		HANDLE hPlugin;

		if ( pPlugin->HasOpenFilePlugin() )
			hPlugin = pPlugin->OpenFilePlugin (Name, Data, DataSize, OpMode);
		else
		{
			AnalyseData AData;

			AData.lpwszFileName = Name;
			AData.pBuffer = Data;
			AData.dwBufferSize = DataSize;
			AData.OpMode = OpMode;

			if ( !pPlugin->Analyse(&AData) )
				continue;

			hPlugin = pPlugin->OpenPlugin(OPEN_ANALYSE, 0);
		}

		if (hPlugin == (HANDLE)-2)
			return hPlugin;

		if (hPlugin != INVALID_HANDLE_VALUE)
		{
			PluginHandle *handle = new PluginHandle;
			handle->hPlugin = hPlugin;
			handle->pPlugin = pPlugin;

			return (HANDLE)handle;
		}
	}

	return INVALID_HANDLE_VALUE;
}

*/


HANDLE PluginManager::OpenFilePlugin(
    const wchar_t *Name,
    const unsigned char *Data,
    int DataSize,
    int OpMode
)
{
	ChangePriority ChPriority(THREAD_PRIORITY_NORMAL);
	ConsoleTitle ct(Opt.ShowCheckingFile?MSG(MCheckingFileInPlugin):nullptr);
	HANDLE hResult = INVALID_HANDLE_VALUE;
	PluginHandle *pResult = nullptr;
	TPointerArray<PluginHandle> items;
	string strFullName;

	if (Name)
	{
		ConvertNameToFull(Name,strFullName);
		Name = strFullName;
	}

	Plugin *pPlugin = nullptr;
	bool bFirstFound = false;

	for (int i = 0; i < PluginsCount; i++)
	{
		pPlugin = PluginsData[i];

		if (!pPlugin->HasOpenFilePlugin() && !(pPlugin->HasAnalyse() && pPlugin->HasOpenPlugin()))
			continue;

		if (Opt.ShowCheckingFile)
			ct.Set(L"%s - [%s]...",MSG(MCheckingFileInPlugin),PointToName(pPlugin->GetModuleName()));

		HANDLE hPlugin;

		if (pPlugin->HasOpenFilePlugin())
		{
			hPlugin = pPlugin->OpenFilePlugin(Name, Data, DataSize, OpMode);

			if (hPlugin == (HANDLE)-2)   //����� �� �����, ������ ����� ����� ���������� ��� ��� (Autorun/PictureView)!!!
			{
				hResult = (HANDLE)-2;
				break;
			}

			if (hPlugin != INVALID_HANDLE_VALUE)
			{
				PluginHandle *handle=items.addItem();
				handle->hPlugin = hPlugin;
				handle->pPlugin = pPlugin;
				bFirstFound = true;
			}
		}
		else
		{
			AnalyseData AData;
			AData.lpwszFileName = Name;
			AData.pBuffer = Data;
			AData.dwBufferSize = DataSize;
			AData.OpMode = OpMode;

			if (pPlugin->Analyse(&AData))
			{
				PluginHandle *handle=items.addItem();
				handle->pPlugin = pPlugin;
				handle->hPlugin = INVALID_HANDLE_VALUE;
				bFirstFound = true;
			}
		}

		if (bFirstFound && !Opt.PluginConfirm.OpenFilePlugin)
			break;
	}

	if (items.getCount() && (hResult != (HANDLE)-2))
	{
		if ((items.getCount() > 1) || (Opt.PluginConfirm.OpenFilePlugin && (Opt.PluginConfirm.StandardAssociation || Opt.PluginConfirm.EvenIfOnlyOnePlugin)))
		{
			VMenu menu(MSG(MMenuPluginConfirmation), nullptr, 0, ScrY-4);
			menu.SetPosition(-1, -1, 0, 0);
			menu.SetHelp(L"ChoosePluginMenu");
			menu.SetFlags(VMENU_SHOWAMPERSAND|VMENU_WRAPMODE);
			MenuItemEx mitem;

			for (UINT i = 0; i < items.getCount(); i++)
			{
				PluginHandle *handle = items.getItem(i);
				mitem.Clear();
				mitem.strName = PointToName(handle->pPlugin->GetModuleName());
				menu.SetUserData(handle, sizeof(handle), menu.AddItem(&mitem));
			}

			if (Opt.PluginConfirm.StandardAssociation)
			{
				mitem.Clear();
				mitem.Flags |= MIF_SEPARATOR;
				menu.AddItem(&mitem);
				mitem.Clear();
				mitem.strName = MSG(MMenuPluginStdAssociation);
				menu.AddItem(&mitem);
			}

			menu.Show();

			while (!menu.Done())
			{
				menu.ReadInput();
				menu.ProcessInput();
			}

			if (menu.GetExitCode() == -1)
				hResult = (HANDLE)-2;
			else
				pResult = (PluginHandle*)menu.GetUserData(nullptr, 0);
		}
		else
		{
			pResult = items.getItem(0);
		}

		if (pResult && pResult->hPlugin == INVALID_HANDLE_VALUE)
		{
			HANDLE h = pResult->pPlugin->OpenPlugin(OPEN_ANALYSE, 0);

			if (h != INVALID_HANDLE_VALUE)
				pResult->hPlugin = h;
			else
				pResult = nullptr;
		}
	}

	for (UINT i = 0; i < items.getCount(); i++)
	{
		PluginHandle *handle = items.getItem(i);

		if (handle != pResult)
		{
			if (handle->hPlugin != INVALID_HANDLE_VALUE)
				handle->pPlugin->ClosePlugin(handle->hPlugin);
		}
	}

	if (pResult)
	{
		PluginHandle* pDup=new PluginHandle;
		pDup->hPlugin=pResult->hPlugin;
		pDup->pPlugin=pResult->pPlugin;
		hResult=reinterpret_cast<HANDLE>(pDup);
	}

	return hResult;
}

HANDLE PluginManager::OpenFindListPlugin(const PluginPanelItem *PanelItem, int ItemsNumber)
{
	ChangePriority ChPriority(THREAD_PRIORITY_NORMAL);
	PluginHandle *pResult = nullptr;
	TPointerArray<PluginHandle> items;
	Plugin *pPlugin=nullptr;
	bool bFirstFound=false;

	for (int i = 0; i < PluginsCount; i++)
	{
		pPlugin = PluginsData[i];

		if (!pPlugin->HasSetFindList())
			continue;

		HANDLE hPlugin = pPlugin->OpenPlugin(OPEN_FINDLIST, 0);

		if (hPlugin != INVALID_HANDLE_VALUE)
		{
			PluginHandle *handle=items.addItem();
			handle->hPlugin = hPlugin;
			handle->pPlugin = pPlugin;
			bFirstFound = true;
		}

		if (bFirstFound && !Opt.PluginConfirm.SetFindList)
			break;
	}

	if (items.getCount())
	{
		if (items.getCount()>1)
		{
			VMenu menu(MSG(MMenuPluginConfirmation), nullptr, 0, ScrY-4);
			menu.SetPosition(-1, -1, 0, 0);
			menu.SetHelp(L"ChoosePluginMenu");
			menu.SetFlags(VMENU_SHOWAMPERSAND|VMENU_WRAPMODE);
			MenuItemEx mitem;

			for (UINT i=0; i<items.getCount(); i++)
			{
				PluginHandle *handle = items.getItem(i);
				mitem.Clear();
				mitem.strName=PointToName(handle->pPlugin->GetModuleName());
				menu.AddItem(&mitem);
			}

			menu.Show();

			while (!menu.Done())
			{
				menu.ReadInput();
				menu.ProcessInput();
			}

			int ExitCode=menu.GetExitCode();

			if (ExitCode>=0)
			{
				pResult=items.getItem(ExitCode);
			}
		}
		else
		{
			pResult=items.getItem(0);
		}
	}

	if (pResult)
	{
		if (!pResult->pPlugin->SetFindList(pResult->hPlugin, PanelItem, ItemsNumber))
		{
			pResult=nullptr;
		}
	}

	for (UINT i=0; i<items.getCount(); i++)
	{
		PluginHandle *handle=items.getItem(i);

		if (handle!=pResult)
		{
			if (handle->hPlugin!=INVALID_HANDLE_VALUE)
				handle->pPlugin->ClosePlugin(handle->hPlugin);
		}
	}

	if (pResult)
	{
		PluginHandle* pDup=new PluginHandle;
		pDup->hPlugin=pResult->hPlugin;
		pDup->pPlugin=pResult->pPlugin;
		pResult=pDup;
	}

	return pResult?reinterpret_cast<HANDLE>(pResult):INVALID_HANDLE_VALUE;
}


void PluginManager::ClosePlugin(HANDLE hPlugin)
{
	ChangePriority ChPriority(THREAD_PRIORITY_NORMAL);
	PluginHandle *ph = (PluginHandle*)hPlugin;
	ph->pPlugin->ClosePlugin(ph->hPlugin);
	delete ph;
}


int PluginManager::ProcessEditorInput(INPUT_RECORD *Rec)
{
	for (int i = 0; i < PluginsCount; i++)
	{
		Plugin *pPlugin = PluginsData[i];

		if (pPlugin->HasProcessEditorInput() && pPlugin->ProcessEditorInput(Rec))
			return TRUE;
	}

	return FALSE;
}


int PluginManager::ProcessEditorEvent(int Event,void *Param)
{
	int nResult = 0;

	if (CtrlObject->Plugins.CurEditor)
	{
		Plugin *pPlugin = nullptr;

		for (int i = 0; i < PluginsCount; i++)
		{
			pPlugin = PluginsData[i];

			if (pPlugin->HasProcessEditorEvent())
				nResult = pPlugin->ProcessEditorEvent(Event, Param);
		}
	}

	return nResult;
}


int PluginManager::ProcessViewerEvent(int Event, void *Param)
{
	int nResult = 0;

	for (int i = 0; i < PluginsCount; i++)
	{
		Plugin *pPlugin = PluginsData[i];

		if (pPlugin->HasProcessViewerEvent())
			nResult = pPlugin->ProcessViewerEvent(Event, Param);
	}

	return nResult;
}

int PluginManager::ProcessDialogEvent(int Event, void *Param)
{
	for (int i=0; i<PluginsCount; i++)
	{
		Plugin *pPlugin = PluginsData[i];

		if (pPlugin->HasProcessDialogEvent() && pPlugin->ProcessDialogEvent(Event,Param))
			return TRUE;
	}

	return FALSE;
}

int PluginManager::GetFindData(
    HANDLE hPlugin,
    PluginPanelItem **pPanelData,
    int *pItemsNumber,
    int OpMode
)
{
	ChangePriority ChPriority(THREAD_PRIORITY_NORMAL);
	PluginHandle *ph = (PluginHandle *)hPlugin;
	*pItemsNumber = 0;
	return ph->pPlugin->GetFindData(ph->hPlugin, pPanelData, pItemsNumber, OpMode);
}


void PluginManager::FreeFindData(
    HANDLE hPlugin,
    PluginPanelItem *PanelItem,
    int ItemsNumber
)
{
	PluginHandle *ph = (PluginHandle *)hPlugin;
	ph->pPlugin->FreeFindData(ph->hPlugin, PanelItem, ItemsNumber);
}


int PluginManager::GetVirtualFindData(
    HANDLE hPlugin,
    PluginPanelItem **pPanelData,
    int *pItemsNumber,
    const wchar_t *Path
)
{
	ChangePriority ChPriority(THREAD_PRIORITY_NORMAL);
	PluginHandle *ph = (PluginHandle*)hPlugin;
	*pItemsNumber=0;
	return ph->pPlugin->GetVirtualFindData(ph->hPlugin, pPanelData, pItemsNumber, Path);
}


void PluginManager::FreeVirtualFindData(
    HANDLE hPlugin,
    PluginPanelItem *PanelItem,
    int ItemsNumber
)
{
	PluginHandle *ph = (PluginHandle*)hPlugin;
	return ph->pPlugin->FreeVirtualFindData(ph->hPlugin, PanelItem, ItemsNumber);
}


int PluginManager::SetDirectory(
    HANDLE hPlugin,
    const wchar_t *Dir,
    int OpMode
)
{
	ChangePriority ChPriority(THREAD_PRIORITY_NORMAL);
	PluginHandle *ph = (PluginHandle*)hPlugin;
	return ph->pPlugin->SetDirectory(ph->hPlugin, Dir, OpMode);
}


int PluginManager::GetFile(
    HANDLE hPlugin,
    PluginPanelItem *PanelItem,
    const wchar_t *DestPath,
    string &strResultName,
    int OpMode
)
{
	ChangePriority ChPriority(THREAD_PRIORITY_NORMAL);
	PluginHandle *ph = (PluginHandle*)hPlugin;
	SaveScreen *SaveScr=nullptr;
	int Found=FALSE;
	KeepUserScreen=FALSE;

	if ((OpMode & OPM_FIND)==0)
		SaveScr = new SaveScreen; //???

	UndoGlobalSaveScrPtr UndSaveScr(SaveScr);
	int GetCode = ph->pPlugin->GetFiles(ph->hPlugin, PanelItem, 1, 0, &DestPath, OpMode);
	string strFindPath;
	strFindPath = DestPath;
	AddEndSlash(strFindPath);
	strFindPath += L"*";
	FAR_FIND_DATA_EX fdata;
	FindFile Find(strFindPath);
	bool Done = true;
	while(Find.Get(fdata))
	{
		if(!(fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			Done = false;
			break;
		}
	}

	if (!Done)
	{
		strResultName = DestPath;
		AddEndSlash(strResultName);
		strResultName += fdata.strFileName;

		if (GetCode!=1)
		{
			apiSetFileAttributes(strResultName,FILE_ATTRIBUTE_NORMAL);
			apiDeleteFile(strResultName); //BUGBUG
		}
		else
			Found=TRUE;
	}

	ReadUserBackgound(SaveScr);
	delete SaveScr;
	return Found;
}


int PluginManager::DeleteFiles(
    HANDLE hPlugin,
    PluginPanelItem *PanelItem,
    int ItemsNumber,
    int OpMode
)
{
	ChangePriority ChPriority(THREAD_PRIORITY_NORMAL);
	PluginHandle *ph = (PluginHandle*)hPlugin;
	SaveScreen SaveScr;
	KeepUserScreen=FALSE;
	int Code = ph->pPlugin->DeleteFiles(ph->hPlugin, PanelItem, ItemsNumber, OpMode);

	if (Code)
		ReadUserBackgound(&SaveScr); //???

	return Code;
}


int PluginManager::MakeDirectory(
    HANDLE hPlugin,
    const wchar_t **Name,
    int OpMode
)
{
	ChangePriority ChPriority(THREAD_PRIORITY_NORMAL);
	PluginHandle *ph = (PluginHandle*)hPlugin;
	SaveScreen SaveScr;
	KeepUserScreen=FALSE;
	int Code = ph->pPlugin->MakeDirectory(ph->hPlugin, Name, OpMode);

	if (Code != -1)   //???BUGBUG
		ReadUserBackgound(&SaveScr);

	return Code;
}


int PluginManager::ProcessHostFile(
    HANDLE hPlugin,
    PluginPanelItem *PanelItem,
    int ItemsNumber,
    int OpMode
)
{
	PluginHandle *ph = (PluginHandle*)hPlugin;
	ChangePriority ChPriority(THREAD_PRIORITY_NORMAL);
	SaveScreen SaveScr;
	KeepUserScreen=FALSE;
	int Code = ph->pPlugin->ProcessHostFile(ph->hPlugin, PanelItem, ItemsNumber, OpMode);

	if (Code)   //BUGBUG
		ReadUserBackgound(&SaveScr);

	return Code;
}


int PluginManager::GetFiles(
    HANDLE hPlugin,
    PluginPanelItem *PanelItem,
    int ItemsNumber,
    int Move,
    const wchar_t **DestPath,
    int OpMode
)
{
	ChangePriority ChPriority(THREAD_PRIORITY_NORMAL);
	PluginHandle *ph=(PluginHandle*)hPlugin;
	return ph->pPlugin->GetFiles(ph->hPlugin, PanelItem, ItemsNumber, Move, DestPath, OpMode);
}


int PluginManager::PutFiles(
    HANDLE hPlugin,
    PluginPanelItem *PanelItem,
    int ItemsNumber,
    int Move,
    int OpMode
)
{
	PluginHandle *ph = (PluginHandle*)hPlugin;
	ChangePriority ChPriority(THREAD_PRIORITY_NORMAL);
	SaveScreen SaveScr;
	KeepUserScreen=FALSE;
	int Code = ph->pPlugin->PutFiles(ph->hPlugin, PanelItem, ItemsNumber, Move, OpMode);

	if (Code)   //BUGBUG
		ReadUserBackgound(&SaveScr);

	return Code;
}

void PluginManager::GetOpenPluginInfo(
    HANDLE hPlugin,
    OpenPluginInfo *Info
)
{
	if (!Info)
		return;

	memset(Info, 0, sizeof(*Info));
	PluginHandle *ph = (PluginHandle*)hPlugin;
	ph->pPlugin->GetOpenPluginInfo(ph->hPlugin, Info);

	if (Info->CurDir == nullptr)  //���...
		Info->CurDir = L"";

	if ((Info->Flags & OPIF_REALNAMES) && (CtrlObject->Cp()->ActivePanel->GetPluginHandle() == hPlugin) && *Info->CurDir && !IsNetworkServerPath(Info->CurDir))
		apiSetCurrentDirectory(Info->CurDir, false);
}


int PluginManager::ProcessKey(
    HANDLE hPlugin,
    int Key,
    unsigned int ControlState
)
{
	PluginHandle *ph = (PluginHandle*)hPlugin;
	return ph->pPlugin->ProcessKey(ph->hPlugin, Key, ControlState);
}


int PluginManager::ProcessEvent(
    HANDLE hPlugin,
    int Event,
    void *Param
)
{
	PluginHandle *ph = (PluginHandle*)hPlugin;
	return ph->pPlugin->ProcessEvent(ph->hPlugin, Event, Param);
}


int PluginManager::Compare(
    HANDLE hPlugin,
    const PluginPanelItem *Item1,
    const PluginPanelItem *Item2,
    unsigned int Mode
)
{
	PluginHandle *ph = (PluginHandle*)hPlugin;
	return ph->pPlugin->Compare(ph->hPlugin, Item1, Item2, Mode);
}

void PluginManager::ConfigureCurrent(Plugin *pPlugin, int INum)
{
	if (pPlugin->Configure(INum))
	{
		int PMode[2];
		PMode[0]=CtrlObject->Cp()->LeftPanel->GetMode();
		PMode[1]=CtrlObject->Cp()->RightPanel->GetMode();

		for (size_t I=0; I < countof(PMode); ++I)
		{
			if (PMode[I] == PLUGIN_PANEL)
			{
				Panel *pPanel=(I==0?CtrlObject->Cp()->LeftPanel:CtrlObject->Cp()->RightPanel);
				pPanel->Update(UPDATE_KEEP_SELECTION);
				pPanel->SetViewMode(pPanel->GetViewMode());
				pPanel->Redraw();
			}
		}
		pPlugin->SaveToCache();
	}
}

struct PluginMenuItemData
{
	Plugin *pPlugin;
	int nItem;
};

/* $ 29.05.2001 IS
   ! ��� ��������� "���������� ������� �������" ��������� ���� � ��
     ������� ������ ��� ������� �� ESC
*/
void PluginManager::Configure(int StartPos)
{
	// ������� 4 - ��������� ������� �������
	if (Opt.Policies.DisabledOptions&FFPOL_MAINMENUPLUGINS)
		return;

	{
		VMenu PluginList(MSG(MPluginConfigTitle),nullptr,0,ScrY-4);
		PluginList.SetFlags(VMENU_WRAPMODE);
		PluginList.SetHelp(L"PluginsConfig");

		for (;;)
		{
			BOOL NeedUpdateItems=TRUE;
			int MenuItemNumber=0;
			string strFirstHotKey;
			int HotKeysPresent=EnumRegKey(L"PluginHotkeys",0,strFirstHotKey);

			if (NeedUpdateItems)
			{
				PluginList.ClearDone();
				PluginList.DeleteItems();
				PluginList.SetPosition(-1,-1,0,0);
				MenuItemNumber=0;
				LoadIfCacheAbsent();
				string strHotKey, strRegKey, strValue, strName;
				PluginInfo Info={0};

				for (int I=0; I<PluginsCount; I++)
				{
					Plugin *pPlugin = PluginsData[I];
					bool bCached = pPlugin->CheckWorkFlags(PIWF_CACHED)?true:false;

					if (bCached)
					{
						strRegKey.Format(FmtPluginsCache_PluginS, pPlugin->GetCacheName());
					}
					else
					{
						if (!pPlugin->GetPluginInfo(&Info))
							continue;
					}

					for (int J=0; ; J++)
					{
						if (bCached)
						{
							strValue.Format(FmtPluginConfigStringD, J);

							if (!GetRegKey(strRegKey, strValue, strName, L""))
								break;
						}
						else
						{
							if (J >= Info.PluginConfigStringsNumber)
								break;

							strName = Info.PluginConfigStrings[J];
						}

						GetPluginHotKey(pPlugin,J,L"ConfHotkey",strHotKey);
						MenuItemEx ListItem;
						ListItem.Clear();

						if (pPlugin->IsOemPlugin())
							ListItem.Flags=LIF_CHECKED|L'A';

						if (!HotKeysPresent)
							ListItem.strName = strName;
						else if (!strHotKey.IsEmpty())
							ListItem.strName.Format(L"&%c%s  %s",strHotKey.At(0),(strHotKey.At(0)==L'&'?L"&":L""), (const wchar_t*)strName);
						else
							ListItem.strName.Format(L"   %s", (const wchar_t*)strName);

						//ListItem.SetSelect(MenuItemNumber++ == StartPos);
						MenuItemNumber++;
						PluginMenuItemData item;
						item.pPlugin = pPlugin;
						item.nItem = J;
						PluginList.SetUserData(&item, sizeof(PluginMenuItemData),PluginList.AddItem(&ListItem));
					}
				}

				PluginList.AssignHighlights(FALSE);
				PluginList.SetBottomTitle(MSG(MPluginHotKeyBottom));
				PluginList.ClearDone();
				PluginList.SortItems(0,HotKeysPresent?3:0);
				PluginList.SetSelectPos(StartPos,1);
				NeedUpdateItems=FALSE;
			}

			string strPluginModuleName;
			PluginList.Show();

			while (!PluginList.Done())
			{
				DWORD Key=PluginList.ReadInput();
				int SelPos=PluginList.GetSelectPos();
				PluginMenuItemData *item = (PluginMenuItemData*)PluginList.GetUserData(nullptr,0,SelPos);
				string strRegKey;

				switch (Key)
				{
					case KEY_SHIFTF1:
						strPluginModuleName = item->pPlugin->GetModuleName();

						if (!FarShowHelp(strPluginModuleName,L"Config",FHELP_SELFHELP|FHELP_NOSHOWERROR) &&
						        !FarShowHelp(strPluginModuleName,L"Configure",FHELP_SELFHELP|FHELP_NOSHOWERROR))
						{
							FarShowHelp(strPluginModuleName,nullptr,FHELP_SELFHELP|FHELP_NOSHOWERROR);
						}

						break;
					case KEY_F4:

						if (PluginList.GetItemCount() > 0 && SelPos<MenuItemNumber)
						{
							string strName00;
							int nOffset = HotKeysPresent?3:0;
							strName00 = (const wchar_t*)PluginList.GetItemPtr()->strName+nOffset;
							RemoveExternalSpaces(strName00);
							GetHotKeyRegKey(item->pPlugin, item->nItem,strRegKey);

							if (SetHotKeyDialog(strName00,strRegKey,L"ConfHotkey"))
							{
								PluginList.Hide();
								NeedUpdateItems=TRUE;
								StartPos=SelPos;
								PluginList.SetExitCode(SelPos);
								PluginList.Show();
								break;
							}
						}

						break;
					default:
						PluginList.ProcessInput();
						break;
				}
			}

			if (!NeedUpdateItems)
			{
				StartPos=PluginList.Modal::GetExitCode();
				PluginList.Hide();

				if (StartPos<0)
					break;

				PluginMenuItemData *item = (PluginMenuItemData*)PluginList.GetUserData(nullptr,0,StartPos);
				ConfigureCurrent(item->pPlugin, item->nItem);
			}
		}
	}
}

int PluginManager::CommandsMenu(int ModalType,int StartPos,const wchar_t *HistoryName)
{
	int MenuItemNumber=0;
	int PrevMacroMode=CtrlObject->Macro.GetMode();
	CtrlObject->Macro.SetMode(MACRO_MENU);
	int Editor = ModalType==MODALTYPE_EDITOR,
	             Viewer = ModalType==MODALTYPE_VIEWER,
	                      Dialog = ModalType==MODALTYPE_DIALOG;
	string strRegKey;
	PluginMenuItemData item;
	{
		VMenu PluginList(MSG(MPluginCommandsMenuTitle),nullptr,0,ScrY-4);
		PluginList.SetFlags(VMENU_WRAPMODE);
		PluginList.SetHelp(L"PluginCommands");
		BOOL NeedUpdateItems=TRUE;
		BOOL Done=FALSE;

		while (!Done)
		{
			string strFirstHotKey;
			int HotKeysPresent=EnumRegKey(L"PluginHotkeys",0,strFirstHotKey);

			if (NeedUpdateItems)
			{
				PluginList.ClearDone();
				PluginList.DeleteItems();
				PluginList.SetPosition(-1,-1,0,0);
				LoadIfCacheAbsent();
				string strHotKey, strRegKey, strValue, strName;
				PluginInfo Info={0};

				for (int I=0; I<PluginsCount; I++)
				{
					Plugin *pPlugin = PluginsData[I];
					bool bCached = pPlugin->CheckWorkFlags(PIWF_CACHED)?true:false;
					int IFlags;

					if (bCached)
					{
						strRegKey.Format(FmtPluginsCache_PluginS, pPlugin->GetCacheName());
						IFlags=GetRegKey(strRegKey,L"Flags",0);
					}
					else
					{
						if (!pPlugin->GetPluginInfo(&Info))
							continue;

						IFlags = Info.Flags;
					}

					if ((Editor && (IFlags & PF_EDITOR)==0) ||
					        (Viewer && (IFlags & PF_VIEWER)==0) ||
					        (Dialog && (IFlags & PF_DIALOG)==0) ||
					        (!Editor && !Viewer && !Dialog && (IFlags & PF_DISABLEPANELS)))
						continue;

					for (int J=0; ; J++)
					{
						if (bCached)
						{
							strValue.Format(FmtPluginMenuStringD, J);

							if (!GetRegKey(strRegKey, strValue, strName, L""))
								break;
						}
						else
						{
							if (J >= Info.PluginMenuStringsNumber)
								break;

							strName = Info.PluginMenuStrings[J];
						}

						GetPluginHotKey(pPlugin,J,L"Hotkey",strHotKey);
						MenuItemEx ListItem;
						ListItem.Clear();

						if (pPlugin->IsOemPlugin())
							ListItem.Flags=LIF_CHECKED|L'A';

						if (!HotKeysPresent)
							ListItem.strName = strName;
						else if (!strHotKey.IsEmpty())
							ListItem.strName.Format(L"&%c%s  %s",strHotKey.At(0),(strHotKey.At(0)==L'&'?L"&":L""), (const wchar_t*)strName);
						else
							ListItem.strName.Format(L"   %s", (const wchar_t*)strName);

						//ListItem.SetSelect(MenuItemNumber++ == StartPos);
						MenuItemNumber++;
						PluginMenuItemData item;
						item.pPlugin = pPlugin;
						item.nItem = J;
						PluginList.SetUserData(&item, sizeof(PluginMenuItemData),PluginList.AddItem(&ListItem));
					}
				}

				PluginList.AssignHighlights(FALSE);
				PluginList.SetBottomTitle(MSG(MPluginHotKeyBottom));
				PluginList.SortItems(0,HotKeysPresent?3:0);
				PluginList.SetSelectPos(StartPos,1);
				NeedUpdateItems=FALSE;
			}

			PluginList.Show();

			while (!PluginList.Done())
			{
				DWORD Key=PluginList.ReadInput();
				int SelPos=PluginList.GetSelectPos();
				PluginMenuItemData *item = (PluginMenuItemData*)PluginList.GetUserData(nullptr,0,SelPos);

				switch (Key)
				{
					case KEY_SHIFTF1:
						// �������� ������ �����, ������� �������� � CommandsMenu()
						FarShowHelp(item->pPlugin->GetModuleName(),HistoryName,FHELP_SELFHELP|FHELP_NOSHOWERROR|FHELP_USECONTENTS);
						break;
					case KEY_ALTF11:
						WriteEvent(FLOG_PLUGINSINFO);
						break;
					case KEY_F4:

						if (PluginList.GetItemCount() > 0 && SelPos<MenuItemNumber)
						{
							string strName00;
							int nOffset = HotKeysPresent?3:0;
							strName00 = (const wchar_t*)PluginList.GetItemPtr()->strName+nOffset;
							RemoveExternalSpaces(strName00);
							GetHotKeyRegKey(item->pPlugin, item->nItem, strRegKey);

							if (SetHotKeyDialog(strName00,strRegKey,L"Hotkey"))
							{
								PluginList.Hide();
								NeedUpdateItems=TRUE;
								StartPos=SelPos;
								PluginList.SetExitCode(SelPos);
								PluginList.Show();
							}
						}

						break;
					case KEY_ALTSHIFTF9:
					{
						PluginList.Hide();
						NeedUpdateItems=TRUE;
						StartPos=SelPos;
						PluginList.SetExitCode(SelPos);
						Configure();
						PluginList.Show();
						break;
					}
					case KEY_SHIFTF9:
					{
						if (PluginList.GetItemCount() > 0 && SelPos<MenuItemNumber)
						{
							NeedUpdateItems=TRUE;
							StartPos=SelPos;

							if (item->pPlugin->HasConfigure())
								ConfigureCurrent(item->pPlugin, item->nItem);

							PluginList.SetExitCode(SelPos);
							PluginList.Show();
						}

						break;
					}
					default:
						PluginList.ProcessInput();
						break;
				}
			}

			if (!NeedUpdateItems && PluginList.Done())
				break;
		}

		int ExitCode=PluginList.Modal::GetExitCode();
		PluginList.Hide();

		if (ExitCode<0)
		{
			CtrlObject->Macro.SetMode(PrevMacroMode);
			return(FALSE);
		}

		ScrBuf.Flush();
		item = *(PluginMenuItemData*)PluginList.GetUserData(nullptr,0,ExitCode);
	}

	Panel *ActivePanel=CtrlObject->Cp()->ActivePanel;
	int OpenCode=OPEN_PLUGINSMENU;
	INT_PTR Item=item.nItem;
	OpenDlgPluginData pd;

	if (Editor)
	{
		OpenCode=OPEN_EDITOR;
	}
	else if (Viewer)
	{
		OpenCode=OPEN_VIEWER;
	}
	else if (Dialog)
	{
		OpenCode=OPEN_DIALOG;
		pd.hDlg=(HANDLE)FrameManager->GetCurrentFrame();
		pd.ItemNumber=item.nItem;
		Item=(INT_PTR)&pd;
	}

	HANDLE hPlugin=OpenPlugin(item.pPlugin,OpenCode,Item);

	if (hPlugin!=INVALID_HANDLE_VALUE && !Editor && !Viewer && !Dialog)
	{
		if (ActivePanel->ProcessPluginEvent(FE_CLOSE,nullptr))
		{
			ClosePlugin(hPlugin);
			return(FALSE);
		}

		Panel *NewPanel=CtrlObject->Cp()->ChangePanel(ActivePanel,FILE_PANEL,TRUE,TRUE);
		NewPanel->SetPluginMode(hPlugin,L"",true);
		NewPanel->Update(0);
		NewPanel->Show();
	}

	if (Editor && CurEditor)
	{
		CurEditor->SetPluginTitle(nullptr);
	}

	CtrlObject->Macro.SetMode(PrevMacroMode);
	return(TRUE);
}

void PluginManager::GetHotKeyRegKey(Plugin *pPlugin,int ItemNumber,string &strRegKey)
{
	/*
	FarPath
	C:\Program Files\Far\

	ModuleName                                             PluginName
	---------------------------------------------------------------------------------------
	C:\Program Files\Far\Plugins\MultiArc\MULTIARC.DLL  -> Plugins\MultiArc\MULTIARC.DLL
	C:\MultiArc\MULTIARC.DLL                            -> C:\MultiArc\MULTIARC.DLL
	---------------------------------------------------------------------------------------
	*/
	string strPluginName(pPlugin->GetCacheName());
	size_t FarPathLength=g_strFarPath.GetLength();
	strRegKey.Clear();;

	if (FarPathLength < pPlugin->GetModuleName().GetLength() && !StrCmpNI(pPlugin->GetModuleName(), g_strFarPath, (int)FarPathLength))
		strPluginName.LShift(FarPathLength);

	if (ItemNumber>0)
	{
		strRegKey.Format(L"PluginHotkeys\\%s%%%d", strPluginName.CPtr(), ItemNumber);
	}
	else
	{
		strRegKey = L"PluginHotkeys\\";
		strRegKey += strPluginName;
	}
}

void PluginManager::GetPluginHotKey(Plugin *pPlugin, int ItemNumber, const wchar_t *HotKeyType, string &strHotKey)
{
	string strRegKey;
	strHotKey.Clear();
	GetHotKeyRegKey(pPlugin, ItemNumber, strRegKey);
	GetRegKey(strRegKey, HotKeyType, strHotKey, L"");
}

bool PluginManager::SetHotKeyDialog(
    const wchar_t *DlgPluginTitle,  // ��� �������
    const wchar_t *RegKey,          // ����, ������ ����� ��������
    const wchar_t *RegValueName     // �������� ��������� �� �������
)
{
	/*
	�================ Assign plugin hot key =================�
	� Enter hot key (letter or digit)                        �
	� _                                                      �
	L========================================================-
	*/
	static DialogDataEx PluginDlgData[]=
	{
		/* 00 */DI_DOUBLEBOX,3,1,60,4,0,0,0,0,(const wchar_t *)MPluginHotKeyTitle,
		/* 01 */DI_TEXT,5,2,0,2,0,0,0,0,(const wchar_t *)MPluginHotKey,
		/* 02 */DI_FIXEDIT,5,3,5,3,1,0,0,1,L"",
		/* 03 */DI_TEXT,8,3,58,3,0,0,0,0,L"",
	};
	PluginDlgData[3].Data=DlgPluginTitle;
	MakeDialogItemsEx(PluginDlgData,PluginDlg);
	GetRegKey(RegKey,RegValueName,PluginDlg[2].strData,L"");
	int ExitCode;
	{
		Dialog Dlg(PluginDlg,countof(PluginDlg));
		Dlg.SetPosition(-1,-1,64,6);
		Dlg.Process();
		ExitCode=Dlg.GetExitCode();
	}

	if (ExitCode==2)
	{
		PluginDlg[2].strData.SetLength(1);
		RemoveLeadingSpaces(PluginDlg[2].strData);

		if (PluginDlg[2].strData.IsEmpty())
			DeleteRegValue(RegKey,RegValueName);
		else
			SetRegKey(RegKey,RegValueName,PluginDlg[2].strData);

		return true;
	}

	return false;
}

bool PluginManager::GetDiskMenuItem(
     Plugin *pPlugin,
     int PluginItem,
     bool &ItemPresent,
     int &PluginTextNumber,
     string &strPluginText
)
{
	LoadIfCacheAbsent();

	if (pPlugin->CheckWorkFlags(PIWF_CACHED))
	{
		string strRegKey, strValue;
		strRegKey.Format(FmtPluginsCache_PluginS, pPlugin->GetCacheName());
		strValue.Format(FmtDiskMenuStringD,PluginItem);
		GetRegKey(strRegKey,strValue,strPluginText,L"");
		strValue.Format(FmtDiskMenuNumberD,PluginItem);
		GetRegKey(strRegKey,strValue,PluginTextNumber,0);
		ItemPresent=!strPluginText.IsEmpty();

		return true;
	}

	PluginInfo Info;

	if (!pPlugin->GetPluginInfo(&Info) || Info.DiskMenuStringsNumber <= PluginItem)
	{
		ItemPresent=false;
	}
	else
	{
		if (Info.DiskMenuNumbers)
			PluginTextNumber=Info.DiskMenuNumbers[PluginItem];
		else
			PluginTextNumber=0;

		strPluginText = Info.DiskMenuStrings[PluginItem];
		ItemPresent=true;
	}

	return true;
}

int PluginManager::UseFarCommand(HANDLE hPlugin,int CommandType)
{
	OpenPluginInfo Info;
	GetOpenPluginInfo(hPlugin,&Info);

	if ((Info.Flags & OPIF_REALNAMES)==0)
		return(FALSE);

	PluginHandle *ph = (PluginHandle*)hPlugin;

	switch (CommandType)
	{
		case PLUGIN_FARGETFILE:
		case PLUGIN_FARGETFILES:
			return(!ph->pPlugin->HasGetFiles() || (Info.Flags & OPIF_EXTERNALGET));
		case PLUGIN_FARPUTFILES:
			return(!ph->pPlugin->HasPutFiles() || (Info.Flags & OPIF_EXTERNALPUT));
		case PLUGIN_FARDELETEFILES:
			return(!ph->pPlugin->HasDeleteFiles() || (Info.Flags & OPIF_EXTERNALDELETE));
		case PLUGIN_FARMAKEDIRECTORY:
			return(!ph->pPlugin->HasMakeDirectory() || (Info.Flags & OPIF_EXTERNALMKDIR));
	}

	return(TRUE);
}


void PluginManager::ReloadLanguage()
{
	Plugin *PData;

	for (int I=0; I<PluginsCount; I++)
	{
		PData = PluginsData[I];
		PData->CloseLang();
	}

	DiscardCache();
}


void PluginManager::DiscardCache()
{
	for (int I=0; I<PluginsCount; I++)
	{
		Plugin *pPlugin = PluginsData[I];
		pPlugin->Load();
	}

	DeleteKeyTree(RKN_PluginsCache);
}


void PluginManager::LoadIfCacheAbsent()
{
	if (!CheckRegKey(RKN_PluginsCache))
	{
		for (int I=0; I<PluginsCount; I++)
		{
			Plugin *pPlugin = PluginsData[I];
			pPlugin->Load();
		}
	}
}

//template parameters must have external linkage
struct PluginData
{
	Plugin *pPlugin;
	DWORD PluginFlags;
};

int PluginManager::ProcessCommandLine(const wchar_t *CommandParam,Panel *Target)
{
	size_t PrefixLength=0;
	string strCommand=CommandParam;
	UnquoteExternal(strCommand);
	RemoveLeadingSpaces(strCommand);

	for (;;)
	{
		wchar_t Ch=strCommand.At(PrefixLength);

		if (Ch==0 || IsSpace(Ch) || Ch==L'/' || PrefixLength>64)
			return(FALSE);

		if (Ch==L':' && PrefixLength>0)
			break;

		PrefixLength++;
	}

	LoadIfCacheAbsent();
	string strPrefix(strCommand,PrefixLength);
	string strPluginPrefix;
	bool bFirstFound = false;
	TPointerArray<PluginData> items;

	for (int I=0; I<PluginsCount; I++)
	{
		int PluginFlags=0;

		if (PluginsData[I]->CheckWorkFlags(PIWF_CACHED))
		{
			string strRegKey;
			strRegKey.Format(FmtPluginsCache_PluginS,PluginsData[I]->GetCacheName());
			GetRegKey(strRegKey,L"CommandPrefix",strPluginPrefix, L"");
			PluginFlags=GetRegKey(strRegKey,L"Flags",0);
		}
		else
		{
			PluginInfo Info;

			if (PluginsData[I]->GetPluginInfo(&Info))
			{
				strPluginPrefix = Info.CommandPrefix;
				PluginFlags = Info.Flags;
			}
			else
				continue;
		}

		if (strPluginPrefix.IsEmpty())
			continue;

		const wchar_t *PrStart = strPluginPrefix;
		PrefixLength=strPrefix.GetLength();

		for (;;)
		{
			const wchar_t *PrEnd = wcschr(PrStart, L':');
			size_t Len=PrEnd==nullptr ? StrLength(PrStart):(PrEnd-PrStart);

			if (Len<PrefixLength)Len=PrefixLength;

			if (StrCmpNI(strPrefix, PrStart, (int)Len)==0)
			{
				if (PluginsData[I]->Load() && PluginsData[I]->HasOpenPlugin())
				{
					bFirstFound = true;
					PluginData *pD=items.addItem();
					pD->pPlugin=PluginsData[I];
					pD->PluginFlags=PluginFlags;
					break;
				}
			}

			if (PrEnd == nullptr)
				break;

			PrStart = ++PrEnd;
		}

		if (bFirstFound && !Opt.PluginConfirm.Prefix)
			break;
	}

	if (!items.getCount())
		return(FALSE);

	Panel *ActivePanel=CtrlObject->Cp()->ActivePanel;
	Panel *CurPanel=(Target)?Target:ActivePanel;

	if (CurPanel->ProcessPluginEvent(FE_CLOSE,nullptr))
		return(FALSE);

	PluginData* PData=nullptr;

	if (items.getCount()>1)
	{
		VMenu menu(MSG(MMenuPluginConfirmation), nullptr, 0, ScrY-4);
		menu.SetPosition(-1, -1, 0, 0);
		menu.SetHelp(L"ChoosePluginMenu");
		menu.SetFlags(VMENU_SHOWAMPERSAND|VMENU_WRAPMODE);
		MenuItemEx mitem;

		for (UINT i=0; i<items.getCount(); i++)
		{
			mitem.Clear();
			mitem.strName=PointToName(items.getItem(i)->pPlugin->GetModuleName());
			menu.AddItem(&mitem);
		}

		menu.Show();

		while (!menu.Done())
		{
			menu.ReadInput();
			menu.ProcessInput();
		}

		int ExitCode=menu.GetExitCode();

		if (ExitCode>=0)
		{
			PData=items.getItem(ExitCode);
		}
	}
	else
	{
		PData=items.getItem(0);
	}

	if (PData)
	{
		CtrlObject->CmdLine->SetString(L"");
		string strPluginCommand=strCommand.CPtr()+(PData->PluginFlags & PF_FULLCMDLINE ? 0:PrefixLength+1);
		RemoveTrailingSpaces(strPluginCommand);
		HANDLE hPlugin=OpenPlugin(PData->pPlugin,OPEN_COMMANDLINE,(INT_PTR)(const wchar_t*)strPluginCommand); //BUGBUG

		if (hPlugin!=INVALID_HANDLE_VALUE)
		{
			Panel *NewPanel=CtrlObject->Cp()->ChangePanel(CurPanel,FILE_PANEL,TRUE,TRUE);
			NewPanel->SetPluginMode(hPlugin,L"",!Target || Target == ActivePanel);
			NewPanel->Update(0);
			NewPanel->Show();
		}
	}

	return(TRUE);
}


void PluginManager::ReadUserBackgound(SaveScreen *SaveScr)
{
	FilePanels *FPanel=CtrlObject->Cp();
	FPanel->LeftPanel->ProcessingPluginCommand++;
	FPanel->RightPanel->ProcessingPluginCommand++;

	if (KeepUserScreen)
	{
		if (SaveScr)
			SaveScr->Discard();

		RedrawDesktop Redraw;
	}

	FPanel->LeftPanel->ProcessingPluginCommand--;
	FPanel->RightPanel->ProcessingPluginCommand--;
}


/* $ 27.09.2000 SVS
  ������� CallPlugin - ����� ������ �� ID � ���������
  � ���������� ���������!
*/
int PluginManager::CallPlugin(DWORD SysID,int OpenFrom, void *Data)
{
	Plugin *pPlugin = FindPlugin(SysID);

	if (pPlugin)
	{
		if (pPlugin->HasOpenPlugin() && !ProcessException)
		{
			HANDLE hNewPlugin=OpenPlugin(pPlugin,OpenFrom,(INT_PTR)Data);

			if (hNewPlugin!=INVALID_HANDLE_VALUE &&
			        (OpenFrom == OPEN_PLUGINSMENU || OpenFrom == OPEN_FILEPANEL))
			{
				int CurFocus=CtrlObject->Cp()->ActivePanel->GetFocus();
				Panel *NewPanel=CtrlObject->Cp()->ChangePanel(CtrlObject->Cp()->ActivePanel,FILE_PANEL,TRUE,TRUE);
				NewPanel->SetPluginMode(hNewPlugin,L"",CurFocus || !CtrlObject->Cp()->GetAnotherPanel(NewPanel)->IsVisible());

				if (Data && *(const wchar_t *)Data)
					SetDirectory(hNewPlugin,(const wchar_t *)Data,0);

				/* $ 04.04.2001 SVS
					��� ���������������! ������� ��������� �������� ������ � CallPlugin()
					���� ���-�� �� ��� - �����������������!!!
				*/
//        NewPanel->Update(0);
//        NewPanel->Show();
			}

			return TRUE;
		}
	}

	return FALSE;
}

Plugin *PluginManager::FindPlugin(DWORD SysID)
{
	if (SysID != 0 && SysID != 0xFFFFFFFFUl) // �� ����������� 0 � -1
	{
		Plugin *PData;

		for (int I=0; I<PluginsCount; I++)
		{
			PData = PluginsData[I];

			if (PData->GetSysID() == SysID)
				return PData;
		}
	}

	return nullptr;
}

HANDLE PluginManager::OpenPlugin(Plugin *pPlugin,int OpenFrom,INT_PTR Item)
{
	HANDLE hPlugin = pPlugin->OpenPlugin(OpenFrom, Item);

	if (hPlugin != INVALID_HANDLE_VALUE)
	{
		PluginHandle *handle = new PluginHandle;
		handle->hPlugin = hPlugin;
		handle->pPlugin = pPlugin;
		return (HANDLE)handle;
	}

	return hPlugin;
}
