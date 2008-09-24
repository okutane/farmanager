#ifndef __SHELLCOPY_HPP__
#define __SHELLCOPY_HPP__
/*
copy.hpp

class ShellCopy - ����������� ������
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

#include "dizlist.hpp"
#include "udlist.hpp"

class Panel;

enum COPY_CODES {
  COPY_CANCEL,
  COPY_NEXT,
  COPY_FAILURE,
  COPY_FAILUREREAD,
  COPY_SUCCESS,
  COPY_SUCCESS_MOVE
};

enum COPY_FLAGS {
  FCOPY_COPYTONUL               = 0x00000001, // ������� ����������� � NUL
  FCOPY_CURRENTONLY             = 0x00000002, // ������ ������?
  FCOPY_ONLYNEWERFILES          = 0x00000004, // Copy only newer files
  FCOPY_OVERWRITENEXT           = 0x00000008, // Overwrite all
  FCOPY_LINK                    = 0x00000010, // �������� ������
  FCOPY_MOVE                    = 0x00000040, // �������/��������������
  FCOPY_DIZREAD                 = 0x00000080, //
  FCOPY_COPYSECURITY            = 0x00000100, // [x] Copy access rights
  FCOPY_NOSHOWMSGLINK           = 0x00000200, // �� ���������� ������ ��� ���������
  FCOPY_VOLMOUNT                = 0x00000400, // �������� ������������� ����
  FCOPY_STREAMSKIP              = 0x00000800, // ������
  FCOPY_STREAMALL               = 0x00001000, // ������
  FCOPY_SKIPSETATTRFLD          = 0x00002000, // ������ �� �������� ������� �������� ��� ��������� - ����� ������ Skip All
  FCOPY_COPYSYMLINKCONTENTS     = 0x00004000, // ���������� ���������� ������������ ������?
  FCOPY_COPYPARENTSECURITY      = 0x00008000, // ����������� ������������ �����, � ������ ���� �� �� �������� ����� �������
  FCOPY_LEAVESECURITY           = 0x00010000, // Move: [?] ������ �� ������ � ������� �������
  FCOPY_DECRYPTED_DESTINATION   = 0x00020000, // ��� ������������ ������ - ��������������...
  FCOPY_USESYSTEMCOPY           = 0x00040000, // ������������ ��������� ������� �����������
  FCOPY_COPYLASTTIME            = 0x10000000, // ��� ����������� � ��������� ��������� ��������������� ��� ����������.
  FCOPY_UPDATEPPANEL            = 0x80000000, // ���������� �������� ��������� ������
};

class ConsoleTitle;

class ShellCopy
{
  private:
    DWORD Flags;

    Panel *SrcPanel;
    int    SrcPanelMode;
    int    SrcDriveType;

    string strSrcDriveRoot;
    string strSrcFSName;
    DWORD  SrcFSFlags;

    Panel *DestPanel;
    int    DestPanelMode;
    int    DestDriveType;

    string strDestDriveRoot;
    string strDestFSName;
    DWORD  DestFSFlags;

    char   *sddata; // Security

    DizList DestDiz;

    string strDestDizPath;

    char *CopyBuffer;
    int CopyBufSize;
    int CopyBufferSize;
    clock_t StartTime;
    clock_t StopTime;

    string strCopiedName;
    string strRenamedName;

    int OvrMode;
    int ReadOnlyOvrMode;
    int ReadOnlyDelMode;
    int SkipMode;          // ...��� �������� ��� ����������� ���������� ������.
    int SkipEncMode;

    long TotalFiles;
    int SelectedFolderNameLength;

    UserDefinedList DestList;
    ConsoleTitle *CopyTitle;   // ���������

    // ��� ������������ ������������.
    // ��� AltF6 ����� ��, ��� ������ ���� � �������,
    // � ��������� ������� - RP_EXACTCOPY - ��� � ���������
    ReparsePointTypes RPT;

  private:
    COPY_CODES CopyFileTree(const wchar_t *Dest);

    COPY_CODES ShellCopyOneFile(const wchar_t *Src,
                                const FAR_FIND_DATA_EX &SrcData,
                                const wchar_t *Dest,
                                int KeepPathPos, int Rename);


    COPY_CODES CheckStreams(const wchar_t *Src,const wchar_t *DestPath);


    int  ShellCopyFile(const wchar_t *SrcName,const FAR_FIND_DATA_EX &SrcData,
                       const wchar_t *DestName, DWORD DestAttr,int Append);


    int  ShellSystemCopy(const wchar_t *SrcName,const wchar_t *DestName,const FAR_FIND_DATA_EX &SrcData);

    int  DeleteAfterMove(const wchar_t *Name,DWORD Attr);

    void SetDestDizPath(const wchar_t *DestPath);

    int  AskOverwrite(const FAR_FIND_DATA_EX &SrcData,const wchar_t *DestName,
                      DWORD DestAttr,int SameName,int Rename,int AskAppend,
                      int &Append,int &RetCode);


    int  GetSecurity(const wchar_t *FileName,SECURITY_ATTRIBUTES &sa);
    int  SetSecurity(const wchar_t *FileName,const SECURITY_ATTRIBUTES &sa);
    int  SetRecursiveSecurity(const wchar_t *FileName,const SECURITY_ATTRIBUTES &sa);

    int  IsSameDisk(const wchar_t *SrcPath,const wchar_t *DestPath);

    bool CalcTotalSize();

    string& GetParentFolder(const wchar_t *Src, string &strDest);

    int  CmpFullNames(const wchar_t *Src,const wchar_t *Dest);

    int  CmpFullPath(const wchar_t *Src,const wchar_t *Dest);

    BOOL LinkRules(DWORD *Flags7,DWORD* Flags5,int* Selected5,const wchar_t *SrcDir,const wchar_t *DstDir,struct CopyDlgParam *CDP);

    int  ShellSetAttr(const wchar_t *Dest,DWORD Attr);

    BOOL CheckNulOrCon(const wchar_t *Src);

    void CheckUpdatePanel(); // ���������� ���� FCOPY_UPDATEPPANEL

    void ShellCopyMsg(const wchar_t *Src,const wchar_t *Dest,int Flags);

  public:
    ShellCopy(Panel *SrcPanel,int Move,int Link,int CurrentOnly,int Ask,
              int &ToPlugin, const wchar_t *PluginDestPath);

    ~ShellCopy();

  public:
    static int  ShowBar(unsigned __int64 WrittenSize,unsigned __int64 TotalSize,bool TotalBar);
    static void ShowTitle(int FirstTime);
    static LONG_PTR WINAPI CopyDlgProc(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2);
    static int  MkSymLink(const wchar_t *SelName,const wchar_t *Dest,ReparsePointTypes LinkType,DWORD Flags);
    static void PR_ShellCopyMsg(void);
};


#endif  // __SHELLCOPY_HPP__
