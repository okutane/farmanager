#ifndef __FARWINAPI_HPP__
#define __FARWINAPI_HPP__

/*
farwinapi.hpp

������� ������ ��������� WinAPI �������
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

#include "plugin.hpp"
#include "UnicodeString.hpp"

// �������� �� "��������������" ������������� ��... ��������, �������� ����� � ������� �������!
BOOL apiCheckErrorCanContinue(
		DWORD Err
		);

DWORD apiGetEnvironmentVariable (
		const wchar_t *lpwszName,
		string &strBuffer
		);

DWORD apiGetCurrentDirectory (
		string &strCurDir
		);

DWORD apiGetTempPath (
		string &strBuffer
		);

DWORD apiGetModuleFileName (
		HMODULE hModule,
		string &strFileName
		);

DWORD apiExpandEnvironmentStrings (
		const wchar_t *src,
		string &strDest
		);

DWORD apiGetConsoleTitle (
		string &strConsoleTitle
		);

DWORD apiWNetGetConnection (
		const wchar_t *lpwszLocalName,
		string &strRemoteName
		);

BOOL apiGetVolumeInformation (
		const wchar_t *lpwszRootPathName,
		string *pVolumeName,
		LPDWORD lpVolumeSerialNumber,
		LPDWORD lpMaximumComponentLength,
		LPDWORD lpFileSystemFlags,
		string *pFileSystemName
		);

HANDLE apiFindFirstFile (
		const wchar_t *lpwszFileName,
		FAR_FIND_DATA_EX *pFindFileData,
		bool ScanSymLink=true);

BOOL apiFindNextFile (
		HANDLE hFindFile,
		FAR_FIND_DATA_EX *pFindFileData
		);

BOOL apiFindClose(
		HANDLE hFindFile
		);

void apiFindDataToDataEx (
		const FAR_FIND_DATA *pSrc,
		FAR_FIND_DATA_EX *pDest);

void apiFindDataExToData (
		const FAR_FIND_DATA_EX *pSrc,
		FAR_FIND_DATA *pDest
		);

void apiFreeFindData (
		FAR_FIND_DATA *pData
		);

BOOL apiGetFindDataEx (
		const wchar_t *lpwszFileName,
		FAR_FIND_DATA_EX *pFindData,
		bool ScanSymLink=true);

BOOL apiGetFileSize (
		HANDLE hFile,
		unsigned __int64 *pSize);

//junk

BOOL apiDeleteFile (
		const wchar_t *lpwszFileName
		);

BOOL apiRemoveDirectory (
		const wchar_t *DirName
		);

HANDLE apiCreateFile (
		const wchar_t *lpwszFileName,     // pointer to name of the file
		DWORD dwDesiredAccess,  // access (read-write) mode
		DWORD dwShareMode,      // share mode
		LPSECURITY_ATTRIBUTES lpSecurityAttributes, // pointer to security attributes
		DWORD dwCreationDistribution, // how to create
		DWORD dwFlagsAndAttributes,   // file attributes
		HANDLE hTemplateFile=NULL          // handle to file with attributes to copy
		);

BOOL apiCopyFileEx (
		const wchar_t *lpExistingFileName,
		const wchar_t *lpNewFileName,
		LPPROGRESS_ROUTINE lpProgressRoutine,
		LPVOID lpData,
		LPBOOL pbCancel,
		DWORD dwCopyFlags
		);

BOOL apiMoveFile (
		const wchar_t *lpwszExistingFileName, // address of name of the existing file
		const wchar_t *lpwszNewFileName   // address of new name for the file
		);

BOOL apiMoveFileEx (
		const wchar_t *lpwszExistingFileName, // address of name of the existing file
		const wchar_t *lpwszNewFileName,   // address of new name for the file
		DWORD dwFlags   // flag to determine how to move file
		);

int apiRegEnumKeyEx (
		HKEY hKey,
		DWORD dwIndex,
		string &strName,
		PFILETIME lpftLastWriteTime=NULL
		);

BOOL apiMoveFileThroughTemp (
		const wchar_t *Src,
		const wchar_t *Dest
		);

BOOL apiIsDiskInDrive(
		const wchar_t *Root
		);

int apiGetFileTypeByName(
		const wchar_t *Name
		);

BOOL apiGetDiskSize(
		const wchar_t *Root,
		unsigned __int64 *TotalSize,
		unsigned __int64 *TotalFree,
		unsigned __int64 *UserFree
		);

BOOL apiGetConsoleKeyboardLayoutName (
		string &strDest
		);

HANDLE apiFindFirstFileName(
		LPCWSTR lpFileName,
		DWORD dwFlags,
		string& strLinkName
		);

BOOL apiFindNextFileName(
		HANDLE hFindStream,
		string& strLinkName
		);

#endif // __FARWINAPI_HPP__
