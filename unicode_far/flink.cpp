﻿/*
flink.cpp

Куча разных функций по обработке Link`ов - Hard&Sym
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

#include "flink.hpp"
#include "imports.hpp"
#include "config.hpp"
#include "pathmix.hpp"
#include "drivemix.hpp"
#include "privilege.hpp"
#include "message.hpp"
#include "language.hpp"
#include "dirmix.hpp"
#include "treelist.hpp"
#include "elevation.hpp"

bool CreateVolumeMountPoint(const string& TargetVolume, const string& Object)
{
	string VolumeName;
	return os::GetVolumeNameForVolumeMountPoint(TargetVolume, VolumeName) && SetVolumeMountPoint(Object.data(), VolumeName.data());
}

static bool FillREPARSE_DATA_BUFFER(REPARSE_DATA_BUFFER* rdb, const string& PrintName, const string& SubstituteName)
{

	switch (rdb->ReparseTag)
	{
	// IO_REPARSE_TAG_MOUNT_POINT and IO_REPARSE_TAG_SYMLINK buffers are filled differently:
	// different order of print and substitute names and additional zero bytes in IO_REPARSE_TAG_MOUNT_POINT.
	// No particular reason to do that, but Windows for some reason does and we just mimic its approach to make
	// reparse points created by Far indistinguishable from created by, say, mklink.

	case IO_REPARSE_TAG_MOUNT_POINT:
		{
			const size_t SubstituteNameOffset = 0;
			const size_t SubstituteNameLength = SubstituteName.size() * sizeof(wchar_t);
			const size_t PrintNameOffset = SubstituteNameLength + 1 * sizeof(wchar_t);
			const size_t PrintNameLength = PrintName.size() * sizeof(wchar_t);
			const size_t ReparseDataLength = offsetof(REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer) - REPARSE_DATA_BUFFER_HEADER_SIZE + PrintNameOffset + PrintNameLength + 1 * sizeof(wchar_t);

			if (ReparseDataLength + REPARSE_DATA_BUFFER_HEADER_SIZE > MAXIMUM_REPARSE_DATA_BUFFER_SIZE)
				return false;

			rdb->ReparseDataLength = static_cast<USHORT>(ReparseDataLength);
			rdb->Reserved = 0;

			auto& Buffer = rdb->MountPointReparseBuffer;
			Buffer.SubstituteNameOffset = static_cast<USHORT>(SubstituteNameOffset);
			Buffer.SubstituteNameLength = static_cast<USHORT>(SubstituteNameLength);
			Buffer.PrintNameOffset = static_cast<USHORT>(PrintNameOffset);
			Buffer.PrintNameLength = static_cast<USHORT>(PrintNameLength);
			*std::copy(ALL_CONST_RANGE(SubstituteName), Buffer.PathBuffer + SubstituteNameOffset / sizeof(wchar_t)) = 0;
			*std::copy(ALL_CONST_RANGE(PrintName), Buffer.PathBuffer + PrintNameOffset / sizeof(wchar_t)) = 0;
			return true;
		}

	case IO_REPARSE_TAG_SYMLINK:
		{
			const size_t PrintNameOffset = 0;
			const size_t PrintNameLength = PrintName.size() * sizeof(wchar_t);
			const size_t SubstituteNameOffset = PrintNameLength;
			const size_t SubstituteNameLength = SubstituteName.size() * sizeof(wchar_t);
			const size_t ReparseDataLength = offsetof(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer) - REPARSE_DATA_BUFFER_HEADER_SIZE + SubstituteNameOffset + SubstituteNameLength;

			if (ReparseDataLength + REPARSE_DATA_BUFFER_HEADER_SIZE > MAXIMUM_REPARSE_DATA_BUFFER_SIZE)
				return false;

			rdb->ReparseDataLength = static_cast<USHORT>(ReparseDataLength);
			rdb->Reserved = 0;

			auto& Buffer = rdb->SymbolicLinkReparseBuffer;
			Buffer.SubstituteNameOffset = static_cast<USHORT>(SubstituteNameOffset);
			Buffer.SubstituteNameLength = static_cast<USHORT>(SubstituteNameLength);
			Buffer.PrintNameOffset = static_cast<USHORT>(PrintNameOffset);
			Buffer.PrintNameLength = static_cast<USHORT>(PrintNameLength);
			std::copy(ALL_CONST_RANGE(SubstituteName), Buffer.PathBuffer + SubstituteNameOffset / sizeof(wchar_t));
			std::copy(ALL_CONST_RANGE(PrintName), Buffer.PathBuffer + PrintNameOffset / sizeof(wchar_t));
			return true;
		}

	default:
		return false;
	}
}

static DWORD GetDesiredAccessForReparsePointChange()
{
	static const auto DesiredAccess = IsWindowsXPOrGreater()? FILE_WRITE_ATTRIBUTES : GENERIC_WRITE;
	return DesiredAccess;
}

static bool SetREPARSE_DATA_BUFFER(const string& Object, REPARSE_DATA_BUFFER* rdb)
{
	if (!IsReparseTagValid(rdb->ReparseTag))
		return false;

	SCOPED_ACTION(os::security::privilege)(SE_CREATE_SYMBOLIC_LINK_NAME);

	const auto Attributes = os::GetFileAttributes(Object);
	if(Attributes&FILE_ATTRIBUTE_READONLY)
	{
		os::SetFileAttributes(Object, Attributes&~FILE_ATTRIBUTE_READONLY);
	}

	SCOPE_EXIT
	{
		if (Attributes&FILE_ATTRIBUTE_READONLY)
			os::SetFileAttributes(Object, Attributes);
	};

	const auto& SetBuffer = [&](bool ForceElevation)
	{
		os::fs::file fObject;
		DWORD dwBytesReturned;
		return fObject.Open(Object, GetDesiredAccessForReparsePointChange(), 0, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr, ForceElevation) &&
			fObject.IoControl(FSCTL_SET_REPARSE_POINT, rdb, rdb->ReparseDataLength + REPARSE_DATA_BUFFER_HEADER_SIZE, nullptr, 0, &dwBytesReturned);
	};

	if (SetBuffer(false))
		return true;

	if (ElevationRequired(ELEVATION_MODIFY_REQUEST))
	{
		// Open() succeeded, but IoControl() failed. We can't handle this automatically :(
		return SetBuffer(true);
	}

	return false;
}

bool CreateReparsePoint(const string& Target, const string& Object,ReparsePointTypes Type)
{
	bool Result=false;

	{
		switch (Type)
		{
			case RP_HARDLINK:
				break;
			case RP_EXACTCOPY:
				Result=DuplicateReparsePoint(Target,Object);
				break;
			case RP_SYMLINK:
			case RP_SYMLINKFILE:
			case RP_SYMLINKDIR:
				{
					os::fs::file_status ObjectStatus(Object);
					if(Type == RP_SYMLINK)
					{
						Type = os::fs::is_directory(Target)? RP_SYMLINKDIR : RP_SYMLINKFILE;
					}
					if (Imports().CreateSymbolicLinkW && !os::fs::exists(ObjectStatus))
					{
						Result=os::CreateSymbolicLink(Object,Target,Type==RP_SYMLINKDIR?SYMBOLIC_LINK_FLAG_DIRECTORY:0);
					}
					else
					{
						bool ObjectCreated=false;
						if (Type==RP_SYMLINKDIR)
						{
							ObjectCreated = os::fs::is_directory(ObjectStatus) || os::CreateDirectory(Object,nullptr) != FALSE;
						}
						else
						{
							ObjectCreated = os::fs::is_file(ObjectStatus) || os::fs::file().Open(Object, 0, 0, nullptr, CREATE_NEW);
						}

						if (ObjectCreated)
						{
							block_ptr<REPARSE_DATA_BUFFER> rdb(MAXIMUM_REPARSE_DATA_BUFFER_SIZE);

							rdb->ReparseTag=IO_REPARSE_TAG_SYMLINK;
							const auto& strPrintName = Target;
							auto strSubstituteName = Target;

							if (IsAbsolutePath(Target))
							{
								strSubstituteName = KernelPath(NTPath(strSubstituteName));
								rdb->SymbolicLinkReparseBuffer.Flags=0;
							}
							else
							{
								rdb->SymbolicLinkReparseBuffer.Flags=SYMLINK_FLAG_RELATIVE;
							}

							if (FillREPARSE_DATA_BUFFER(rdb.get(), strPrintName, strSubstituteName))
							{
								Result=SetREPARSE_DATA_BUFFER(Object,rdb.get());
							}
							else
							{
								SetLastError(ERROR_INSUFFICIENT_BUFFER);
							}
						}
					}
				}
				break;
			case RP_JUNCTION:
			case RP_VOLMOUNT:
			{
				const auto strPrintName = ConvertNameToFull(Target);
				auto strSubstituteName = KernelPath(NTPath(strPrintName));
				block_ptr<REPARSE_DATA_BUFFER> rdb(MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
				rdb->ReparseTag=IO_REPARSE_TAG_MOUNT_POINT;

				if (FillREPARSE_DATA_BUFFER(rdb.get(), strPrintName, strSubstituteName))
				{
					Result=SetREPARSE_DATA_BUFFER(Object,rdb.get());
				}
				else
				{
					SetLastError(ERROR_INSUFFICIENT_BUFFER);
				}
			}
			break;
		}
	}

	return Result;
}

static bool GetREPARSE_DATA_BUFFER(const string& Object, REPARSE_DATA_BUFFER* rdb)
{
	const auto FileAttr = os::GetFileAttributes(Object);
	if (FileAttr == INVALID_FILE_ATTRIBUTES || !(FileAttr & FILE_ATTRIBUTE_REPARSE_POINT))
		return false;

	os::fs::file fObject;
	if (!fObject.Open(Object, 0, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT))
		return false;

	DWORD dwBytesReturned;
	return fObject.IoControl(FSCTL_GET_REPARSE_POINT, nullptr, 0, rdb, MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &dwBytesReturned) && IsReparseTagValid(rdb->ReparseTag);
}

bool DeleteReparsePoint(const string& Object)
{
	block_ptr<REPARSE_DATA_BUFFER> rdb(MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
	if (!GetREPARSE_DATA_BUFFER(Object, rdb.get()))
		return false;

	os::fs::file fObject;
	if (!fObject.Open(Object, GetDesiredAccessForReparsePointChange(), 0, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT))
		return false;

	DWORD dwBytes;
	REPARSE_GUID_DATA_BUFFER rgdb{rdb->ReparseTag};
	return fObject.IoControl(FSCTL_DELETE_REPARSE_POINT,&rgdb,REPARSE_GUID_DATA_BUFFER_HEADER_SIZE,nullptr,0,&dwBytes);
}

bool GetReparsePointInfo(const string& Object, string &strDestBuff,LPDWORD ReparseTag)
{
	block_ptr<REPARSE_DATA_BUFFER> rdb(MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
	if (!GetREPARSE_DATA_BUFFER(Object, rdb.get()))
		return false;

	if (ReparseTag)
		*ReparseTag=rdb->ReparseTag;

	const auto& Extract = [&](const auto& Buffer)
	{
		const wchar_t *PathBuffer = nullptr;
		auto NameLength = Buffer.PrintNameLength / sizeof(wchar_t);

		if (NameLength)
		{
			PathBuffer = &Buffer.PathBuffer[Buffer.PrintNameOffset / sizeof(wchar_t)];
		}
		else
		{
			NameLength = Buffer.SubstituteNameLength / sizeof(wchar_t);
			PathBuffer = &Buffer.PathBuffer[Buffer.SubstituteNameOffset / sizeof(wchar_t)];
		}

		if (!NameLength)
			return false;

		strDestBuff.assign(PathBuffer, NameLength);
		return true;
	};

	switch (rdb->ReparseTag)
	{
	case IO_REPARSE_TAG_SYMLINK:
		return Extract(rdb->SymbolicLinkReparseBuffer);

	case IO_REPARSE_TAG_MOUNT_POINT:
		return Extract(rdb->MountPointReparseBuffer);

	default:
		return false;
	}
}

int GetNumberOfLinks(const string& Name, bool negative_if_error)
{
	int NumberOfLinks = (negative_if_error ? -1 : +1);
	os::fs::file file;
	if(file.Open(Name, 0, FILE_SHARE_READ|FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT))
	{
		BY_HANDLE_FILE_INFORMATION bhfi;
		if (file.GetInformation(bhfi))
		{
			NumberOfLinks=bhfi.nNumberOfLinks;
		}
	}
	return NumberOfLinks;
}

int MkHardLink(const string& ExistingName,const string& NewName, bool Silent)
{
	const auto Result = os::CreateHardLink(NewName, ExistingName, nullptr);

	if (!Result && !Silent)
	{
		Global->CatchError();
		Message(MSG_WARNING|MSG_ERRORTYPE,1,MSG(MError),MSG(MCopyCannotCreateLink),NewName.data(),MSG(MOk));
	}
	return Result;
}

bool EnumStreams(const string& FileName,UINT64 &StreamsSize,DWORD &StreamsCount)
{
	bool Result=false;
	
	UINT64 Size = 0;
	DWORD Count = 0;

	for (const auto& i: os::fs::enum_stream(FileName))
	{
		++Count;
		Size += i.StreamSize.QuadPart;
	}

	if (Count)
	{
		StreamsCount = Count;
		StreamsSize = Size;
		Result = true;
	}

	return Result;
}

bool DelSubstDrive(const string& DeviceName)
{
	string strTargetPath;
	return os::QueryDosDevice(DeviceName, strTargetPath) &&
		DefineDosDevice(DDD_RAW_TARGET_PATH | DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, DeviceName.data(), strTargetPath.data()) != FALSE;
}

bool GetSubstName(int DriveType,const string& DeviceName, string &strTargetPath)
{
	bool Ret=false;
	/*
	+ Обработка в зависимости от Global->Opt->SubstNameRule
	битовая маска:
	0 - если установлен, то опрашивать сменные диски
	1 - если установлен, то опрашивать все остальные
	*/
	bool DriveRemovable = (DriveType==DRIVE_REMOVABLE || DriveType==DRIVE_CDROM);

	if (DriveType==DRIVE_NOT_INIT || (((Global->Opt->SubstNameRule & 1) || !DriveRemovable) && ((Global->Opt->SubstNameRule & 2) || DriveRemovable)))
	{
		const auto Type = ParsePath(DeviceName);
		if (Type == PATH_DRIVELETTER)
		{
			string Name;
			if (os::QueryDosDevice(DeviceName, Name))
			{
				if (Name.compare(0, 8, L"\\??\\UNC\\") == 0)
				{
					strTargetPath = L"\\\\" + Name.substr(8);
					Ret = true;
				}
				else if (Name.compare(0, 4, L"\\??\\") == 0)
				{
					strTargetPath=Name.substr(4);
					Ret=true;
				}
			}
		}
	}

	return Ret;
}

bool GetVHDInfo(const string& DeviceName, string &strVolumePath, VIRTUAL_STORAGE_TYPE* StorageType)
{
	bool Result=false;
	os::fs::file Device;
	if(Device.Open(DeviceName, FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, nullptr, OPEN_EXISTING))
	{
		ULONG Size = 4096;
		block_ptr<STORAGE_DEPENDENCY_INFO> StorageDependencyInfo(Size);

		StorageDependencyInfo->Version = STORAGE_DEPENDENCY_INFO_VERSION_2;
		DWORD Used = 0;
		Result = Device.GetStorageDependencyInformation(GET_STORAGE_DEPENDENCY_FLAG_HOST_VOLUMES, Size, StorageDependencyInfo.get(), &Used);
		if(!Result && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			StorageDependencyInfo.reset(Used);
			if(StorageDependencyInfo)
			{
				StorageDependencyInfo->Version = STORAGE_DEPENDENCY_INFO_VERSION_2;
				Result = Device.GetStorageDependencyInformation(GET_STORAGE_DEPENDENCY_FLAG_HOST_VOLUMES, Used, StorageDependencyInfo.get(), &Used);
			}
		}
		if(Result)
		{
			if(StorageDependencyInfo->NumberEntries)
			{
				if(StorageType)
					*StorageType = StorageDependencyInfo->Version2Entries[0].VirtualStorageType;
				strVolumePath = StorageDependencyInfo->Version2Entries[0].HostVolumeName;
				strVolumePath += StorageDependencyInfo->Version2Entries[0].DependentVolumeRelativePath;
				// trick: ConvertNameToReal also converts \\?\{GUID} to drive letter, if possible.
				strVolumePath = ConvertNameToReal(strVolumePath);
			}
		}
	}
	return Result;
}

string GetPathRoot(const string& Path)
{
	return ExtractPathRoot(ConvertNameToReal(Path));
}

bool ModifyReparsePoint(const string& Object,const string& NewData)
{
	block_ptr<REPARSE_DATA_BUFFER> rdb(MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
	if (!GetREPARSE_DATA_BUFFER(Object, rdb.get()))
		return false;

	switch (rdb->ReparseTag)
	{
	case IO_REPARSE_TAG_MOUNT_POINT:
		{
			const auto strPrintName = ConvertNameToFull(NewData);
			const auto strSubstituteName = KernelPath(NTPath(strPrintName));
			if (!FillREPARSE_DATA_BUFFER(rdb.get(), strPrintName, strSubstituteName))
			{
				SetLastError(ERROR_INSUFFICIENT_BUFFER);
				return false;
			}
		}
		break;

	case IO_REPARSE_TAG_SYMLINK:
		{
			const auto& strPrintName = NewData;
			auto strSubstituteName = NewData;

			if (IsAbsolutePath(NewData))
			{
				strSubstituteName = KernelPath(NTPath(strSubstituteName));
				rdb->SymbolicLinkReparseBuffer.Flags=0;
			}
			else
			{
				rdb->SymbolicLinkReparseBuffer.Flags=SYMLINK_FLAG_RELATIVE;
			}

			if (!FillREPARSE_DATA_BUFFER(rdb.get(), strPrintName, strSubstituteName))
			{
				SetLastError(ERROR_INSUFFICIENT_BUFFER);
				return false;
			}
		}
		break;

	default:
		return false;
	}

	return SetREPARSE_DATA_BUFFER(Object,rdb.get());
}

bool DuplicateReparsePoint(const string& Src,const string& Dst)
{
	block_ptr<REPARSE_DATA_BUFFER> rdb(MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
	return GetREPARSE_DATA_BUFFER(Src, rdb.get()) && SetREPARSE_DATA_BUFFER(Dst, rdb.get());
}

void NormalizeSymlinkName(string &strLinkName)
{
	if (strLinkName.compare(0, 4, L"\\??\\"))
		return;

	strLinkName[1] = L'\\';
	if (ParsePath(strLinkName) != PATH_DRIVELETTERUNC)
		return;

	strLinkName.erase(0, 4);
}

// Кусок для создания SymLink для каталогов.
int MkSymLink(const string& Target, const string& LinkName, ReparsePointTypes LinkType, bool Silent, bool HoldTarget)
{
	if (!Target.empty() && !LinkName.empty())
	{
		string strFullTarget, strFullLink, strSelOnlyName;
		// выделим имя
		strSelOnlyName = Target;
		DeleteEndSlash(strSelOnlyName);
		const auto SlashPos = FindLastSlash(strSelOnlyName);
		const auto SelName = SlashPos != string::npos? strSelOnlyName.substr(SlashPos + 1) : strSelOnlyName;

		bool symlink = LinkType==RP_SYMLINK || LinkType==RP_SYMLINKFILE || LinkType==RP_SYMLINKDIR;

		if (Target[1] == L':' && (!Target[2] || (IsSlash(Target[2]) && !Target[3]))) // C: или C:/
		{
//      if(Flags&FCOPY_VOLMOUNT)
			{
				strFullTarget = Target;
				AddEndSlash(strFullTarget);
			}
			/*
			  Вот здесь - ну очень умное поведение!
			  Т.е. если в качестве SelName передали "C:", то в этом куске происходит
			  коррекция типа линка - с symlink`а на volmount
			*/
			LinkType=RP_VOLMOUNT;
		}
		else
			strFullTarget = ConvertNameToFull(Target);

		strFullLink = ConvertNameToFull(LinkName);

		if (IsSlash(strFullLink.back()))
		{
			if (LinkType!=RP_VOLMOUNT)
				strFullLink += SelName;
			else
			{
				strFullLink.append(L"Disk_").append(Target, 0, 1);
			}
		}

		if (LinkType==RP_VOLMOUNT)
		{
			AddEndSlash(strFullTarget);
			AddEndSlash(strFullLink);
		}

			if (symlink)
			{
				// в этом случае создается путь, но не сам каталог
				string strPath=strFullLink;

				if (CutToSlash(strPath))
				{
					if (!os::fs::exists(strPath))
						CreatePath(strPath);
				}
			}
			else
			{
				bool CreateDir=true;

				if (LinkType==RP_EXACTCOPY)
				{
					// в этом случае создается или каталог, или пустой файл
					if (os::fs::is_file(strFullTarget))
						CreateDir=false;
				}

				if (CreateDir)
				{
					if (os::CreateDirectory(strFullLink,nullptr))
						TreeList::AddTreeName(strFullLink);
					else
						CreatePath(strFullLink);
				}
				else
				{
					string strPath=strFullLink;

					if (CutToSlash(strPath))
					{
						if (!os::fs::exists(strPath))
							CreatePath(strPath);
						os::fs::file().Open(strFullLink, 0, 0, nullptr, CREATE_NEW, os::GetFileAttributes(strFullTarget));
					}
				}

				if (!os::fs::exists(strFullLink))
				{
					if (!Silent)
					{
						Global->CatchError();
						Message(MSG_WARNING|MSG_ERRORTYPE,1,MSG(MError),
						        MSG(MCopyCannotCreateLink),strFullLink.data(),MSG(MOk));
					}

					return 0;
				}
			}

		if (LinkType!=RP_VOLMOUNT)
		{
			if (CreateReparsePoint(HoldTarget && symlink ? Target : strFullTarget, strFullLink, LinkType))
			{
				return 1;
			}
			else
			{
				if (!Silent)
				{
					Global->CatchError();
					Message(MSG_WARNING|MSG_ERRORTYPE,1,MSG(MError),
					        MSG(MCopyCannotCreateLink),strFullLink.data(),MSG(MOk));
				}

				return 0;
			}
		}
		else
		{
			if (CreateVolumeMountPoint(strFullTarget,strFullLink))
			{
				return 1;
			}
			else
			{
				if (!Silent)
				{
					Global->CatchError();
					Message(MSG_WARNING|MSG_ERRORTYPE,1,
						MSG(MError),
						string_format(MCopyMountVolFailed, Target).data(),
						string_format(MCopyMountVolFailed2, strFullLink).data(),
						MSG(MOk));
				}

				return 0;
			}
		}
	}

	return 2;
}
