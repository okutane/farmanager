#include <all_far.h>
#pragma hdrstop

#include "ftp_Int.h"

FTP *FTP::Backups[ FTP_MAXBACKUPS ] = { 0 };
int  FTP::BackupCount = 0;

FTP::FTP()
{
  Host.Init();

  hConnect          = NULL;
  ResetCache        = TRUE;
  ShowHosts         = TRUE;
  SwitchingToFTP    = FALSE;
  OverrideMsgCode   = ocNone;
  LastMsgCode       = ocNone;

  UrlsList          = NULL;
  QuequeSize        = 0;

  CallLevel         = 0;

  FP_GetRegKey( "LastHostsPath",HostsPath,NULL,sizeof(HostsPath) );

  RereadRequired    = FALSE;
  CurrentState      = fcsNormal;

  KeepAlivePeriod   = Opt.KeepAlive ? FP_PeriodCreate(Opt.KeepAlive*1000) : NULL;
  LongBeep          = NULL;

  PanelInfo  pi;
  FP_Info->Control(INVALID_HANDLE_VALUE,FCTL_GETPANELINFO,&pi);
  StartViewMode = pi.ViewMode;
}

FTP::~FTP()
{
  if (hConnect) {
    delete hConnect;
    hConnect = NULL;
  }

  FP_PeriodDestroy(KeepAlivePeriod);
  LongBeepEnd( TRUE );

  FP_Info->Control( this,FCTL_SETVIEWMODE,&StartViewMode );
  DeleteFromBackup();

  ClearQueue();
}

void FTP::Call( void )
  {
    LastUsedPlugin = this;
    if ( CallLevel == 0 )
      LongBeepCreate();

    CallLevel++;
}
void FTP::End( int rc )
  {
    if ( rc != -156 ) {
      Log(( "rc=%d",rc ));
    }
    ShowMemInfo();

    if (!CallLevel) return;

    CallLevel--;

    if ( !CallLevel ) {
      LongBeepEnd();
      if (KeepAlivePeriod)
        FP_PeriodReset(KeepAlivePeriod);
    }
}
CONSTSTR FTP::CloseQuery( void )
  {
     if ( UrlsList != NULL )
       return FMSG("Process queque is not empty");

 return NULL;
}

int FTP::GetFindData( PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode )
  {  PROC(("FTP::GetFindData",NULL))
     TIME_TYPE        b,e;
     char            *Data[3];

    *pPanelItem   = NULL;
    *pItemsNumber = 0;

//Hosts
  if (ShowHosts) {
    EnumHost        Enum(HostsPath);
    FP_SizeItemList il( FALSE );
    PluginPanelItem tmp;
    FTPHost         h;

    if ( !IS_SILENT(OpMode) ) {
      MemSet( &tmp, 0, sizeof(tmp) );
      StrCpy( tmp.FindData.cFileName,"..");
      tmp.FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
      if ( !IS_SILENT(OpMode) ) {
        tmp.Description               = "..";
        tmp.CustomColumnNumber        = 3;
        tmp.CustomColumnData          = Data;
        tmp.CustomColumnData[0]       = "..";
        tmp.CustomColumnData[1]       = "..";
        tmp.CustomColumnData[2]       = "..";
       }
      if ( !il.Add(&tmp) )
        return FALSE;
    }

    while( 1 ) {
      if ( !Enum.GetNextHost(&h) )
        break;
      if ( !h.Read(NULL) )
        continue;

      MemSet( &tmp, 0, sizeof(tmp) );
      /* Panel item MUST have name the save as file saved to disk
         in case you want to copy between panels work.
      */
      h.MkINIFile( tmp.FindData.cFileName,NULL,"" );
      tmp.FindData.ftLastWriteTime  = h.LastWrite;
      tmp.FindData.dwFileAttributes = h.Folder ? FILE_ATTRIBUTE_DIRECTORY : 0;
      tmp.Flags                     = PPIF_USERDATA;
      tmp.PackSizeHigh              = FTP_HOSTID;
      tmp.UserData                  = (DWORD)&h;
      if ( !IS_SILENT(OpMode) ) {
        tmp.Description               = h.HostDescr;
        tmp.CustomColumnNumber        = 3;
        tmp.CustomColumnData          = Data;
        tmp.CustomColumnData[0]       = h.Host;  //C0
        tmp.CustomColumnData[1]       = h.Home;  //C1
        tmp.CustomColumnData[2]       = h.User;  //C2
      }

      if ( !il.Add(&tmp) )
        return FALSE;

      Log(( "Item[%d]=[%s] attr=%08X",
            il.Count()-1, FTP_FILENAME( il.Item(il.Count()-1) ),
            il.Item(il.Count()-1)->FindData.dwFileAttributes ));
    }
    *pPanelItem   = il.Items();
    *pItemsNumber = il.Count();
    return TRUE;
  }

//FTP
  FP_Screen _scr;

  if ( !hConnect ) {
    goto AskConnect;
  }

Restart:

  FTPFileInfo FileInfo;

  if ( !FtpFindFirstFile( hConnect, "*", &FileInfo, &ResetCache ) ) {
    if ( GetLastError() == ERROR_NO_MORE_FILES ) {
      *pItemsNumber = 0;
      return TRUE;
    }

    if ( SwitchingToFTP && GetLastError() == ERROR_CANCELLED ) {
      ;
    } else {
      if ( CurrentState == fcsExpandList ) {
        FreeFindData(*pPanelItem,*pItemsNumber);
        *pPanelItem   = NULL;
        *pItemsNumber = 0;
        return FALSE;
      }

//Query reconnect
      do{
        if ( !hConnect )
          break;

        if ( GetLastError() == ERROR_CANCELLED )
          break;

        if ( !hConnect->ConnectMessageTimeout( MConnectionLost,Host.HostName,-MRestore ) ) {
          Log(( "WaitMessage cancelled" ));
          break;
        }

        if ( FtpCmdLineAlive(hConnect) &&
             FtpKeepAlive(hConnect) )
          goto Restart;

        if ( SelectFile.Length() && CurrentState != fcsExpandList )
          SaveUsedDirNFile();

AskConnect:
        if ( Connect() )
          goto Restart;
         else
          break;

      }while(1);
    }

    if ( !ShowHosts )
      BackToHosts();

    FreeFindData( *pPanelItem, *pItemsNumber );

    return GetFindData(pPanelItem,pItemsNumber,OpMode);
  }

  GET_TIME(b);
  do{
    if ( Opt.ShowIdle ) {
      char str[ 200 ];
      GET_TIME( e );
      if ( CMP_TIME(e,b) > 0.5 ) {
        SNprintf( str,sizeof(str),"%s%d", FP_GetMsg(MReaded), *pItemsNumber );
        SetLastError( ERROR_SUCCESS );
        IdleMessage( str,Opt.ProcessColor );
        b = e;
        if ( CheckForEsc(FALSE) ) {
          SetLastError( ERROR_CANCELLED );
          return FALSE;
        }
      }
    }

    PluginPanelItem *NewPanelItem=*pPanelItem;
    if ( (*pItemsNumber % 1024) == 0 ) {
      if ( !NewPanelItem )
        NewPanelItem = (PluginPanelItem *)_Alloc( (1024+1)*sizeof(PluginPanelItem) );
       else
        NewPanelItem = (PluginPanelItem *)_Realloc( NewPanelItem,(*pItemsNumber+1024+1)*sizeof(PluginPanelItem) );

      if ( NewPanelItem == NULL ) {
/*-*/Log(("GetFindData(file)::!reallocate plugin panels items %d -> %d",*pItemsNumber,*pItemsNumber+1024+1));
        return FALSE;
      }
      *pPanelItem=NewPanelItem;
    }

    PluginPanelItem *CurItem = &NewPanelItem[*pItemsNumber];
    MemSet( CurItem, 0, sizeof(PluginPanelItem) );

    CurItem->FindData = FileInfo.FindData;
    if ( !IS_SILENT(OpMode) ) {
      CurItem->CustomColumnNumber             = FTP_COL_MAX;
      CurItem->Owner                          = FileInfo.FTPOwner[0] ? StrDup(FileInfo.FTPOwner) : NULL;
      CurItem->CustomColumnData               = (LPSTR*)_Alloc( sizeof(LPSTR*)*FTP_COL_MAX );
      CurItem->CustomColumnData[FTP_COL_MODE] = StrDup(FileInfo.UnixMode);
      CurItem->CustomColumnData[FTP_COL_LINK] = StrDup(FileInfo.Link);
      hConnect->ToOEM(CurItem->CustomColumnData[FTP_COL_LINK]);
    }

    (*pItemsNumber)++;
  }while( FtpFindNextFile(hConnect,&FileInfo) );

  return TRUE;
}

void FTP::FreeFindData(PluginPanelItem *PanelItem,int ItemsNumber)
{
   FP_SizeItemList::Free(PanelItem,ItemsNumber);
}

void FTP::SetBackupMode( void )
  {
    PanelInfo  pi;
    FP_Info->Control( INVALID_HANDLE_VALUE, FCTL_GETPANELINFO, &pi );
    ActiveColumnMode = pi.ViewMode;
}

void FTP::SetActiveMode( void )
  {
     NeedToSetActiveMode = TRUE;
     CurrentState        = fcsNormal;
}

BOOL FTP::isBackup( void )
  {
    for( int n = 0; n < FTP::BackupCount; n++ )
      if ( FTP::Backups[n] == this )
        return TRUE;
 return FALSE;
}

void FTP::DeleteFromBackup( void )
  {
    for( int n = 0; n < FTP::BackupCount; n++ )
      if ( FTP::Backups[n] == this ) {
        memmove( &FTP::Backups[n], &FTP::Backups[n+1], (FTP::BackupCount-n)*sizeof(FTP*) );
        FTP::BackupCount--;
      }
}

void FTP::AddToBackup( void )
  {
     if ( !Opt.UseBackups )
       return;

     if ( !isBackup() )
       FTP::Backups[ FTP::BackupCount++ ] = this;
}
