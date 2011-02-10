#include "Align.hpp"
#include "AlignReg.hpp"
#include "AlignLng.hpp"
#include "version.hpp"
#include <initguid.h>
#include "guid.hpp"

#if defined(__GNUC__)

#ifdef __cplusplus
extern "C"{
#endif
  BOOL WINAPI DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved);
#ifdef __cplusplus
};
#endif

BOOL WINAPI DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
{
  (void) lpReserved;
  (void) dwReason;
  (void) hDll;
  return TRUE;
}
#endif

#define GetCheck(i) (int)Info.SendDlgMessage(hDlg,DM_GETCHECK,i,0)
#define GetDataPtr(i) ((const wchar_t *)Info.SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,i,0))

static struct PluginStartupInfo Info;
static struct FarStandardFunctions FSF;
wchar_t *PluginRootKey;

static void ReformatBlock(int RightMargin,int SmartMode,int Justify);
static void JustifyBlock(int RightMargin);
static int JustifyString(int RightMargin,struct EditorSetString &ess);

const wchar_t *GetMsg(int MsgId)
{
  return Info.GetMsg(&MainGuid,MsgId);
}

void WINAPI GetGlobalInfoW(struct GlobalInfo *Info)
{
  Info->StructSize=sizeof(GlobalInfo);
  Info->MinFarVersion=FARMANAGERVERSION;
  Info->Version=PLUGIN_VERSION;
  Info->Guid=MainGuid;
  Info->Title=PLUGIN_NAME;
  Info->Description=PLUGIN_DESC;
  Info->Author=PLUGIN_AUTHOR;
}


void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info)
{
  ::Info=*Info;
  ::FSF=*Info->FSF;
  ::Info.FSF=&::FSF;
  PluginRootKey = (wchar_t *)malloc(lstrlen(Info->RootKey)*sizeof(wchar_t) + sizeof(L"\\Align"));
  lstrcpy(PluginRootKey,Info->RootKey);
  lstrcat(PluginRootKey,L"\\Align");
}


void WINAPI ExitFARW()
{
  free(PluginRootKey);
}


void WINAPI GetPluginInfoW(struct PluginInfo *Info)
{
  Info->StructSize=sizeof(*Info);
  Info->Flags=PF_EDITOR|PF_DISABLEPANELS;
  static const wchar_t *PluginMenuStrings[1];
  PluginMenuStrings[0]=GetMsg(MAlign);
  Info->PluginMenu.Guids=&MenuGuid;
  Info->PluginMenu.Strings=PluginMenuStrings;
  Info->PluginMenu.Count=ARRAYSIZE(PluginMenuStrings);
}


HANDLE WINAPI OpenPluginW(int OpenFrom,const GUID* Guid,INT_PTR Data)
{
  struct FarDialogItem DialogItems[]={
    {DI_DOUBLEBOX, 3,1,72,8, {0}, nullptr, nullptr, 0, 0, GetMsg(MAlign),0},
    {DI_FIXEDIT, 5,2,7,3, {0}, nullptr, nullptr, DIF_FOCUS, 0, L"",0},
    {DI_TEXT, 9,2,0,0, {0}, nullptr, nullptr, 0, 0, GetMsg(MRightMargin),0},
    {DI_CHECKBOX, 5,3,0,0, {0}, nullptr, nullptr, 0, 0, GetMsg(MReformat),0},
    {DI_CHECKBOX, 5,4,0,0, {0}, nullptr, nullptr, 0, 0, GetMsg(MSmartMode),0},
    {DI_CHECKBOX, 5,5,0,0, {0}, nullptr, nullptr, 0, 0, GetMsg(MJustify),0},
    {DI_TEXT, 5,6,0,0, {0}, nullptr, nullptr, DIF_BOXCOLOR|DIF_SEPARATOR, 0, L"",0},
    {DI_BUTTON,0,7,0,0, {0}, nullptr, nullptr, DIF_CENTERGROUP|DIF_DEFAULTBUTTON, 0, GetMsg(MOk),0},
    {DI_BUTTON,0,7,0,0, {0}, nullptr, nullptr, DIF_CENTERGROUP, 0, GetMsg(MCancel),0}
  };

  int RightMargin=GetRegKey(L"",L"RightMargin",75);
  int Reformat=GetRegKey(L"",L"Reformat",TRUE);
  int SmartMode=GetRegKey(L"",L"SmartMode",FALSE);
  int Justify=GetRegKey(L"",L"Justify",FALSE);

  wchar_t marstr[32];
  DialogItems[1].PtrData = marstr;
  FSF.sprintf(marstr,L"%d",RightMargin);

  DialogItems[3].Selected=Reformat;
  DialogItems[4].Selected=SmartMode;
  DialogItems[5].Selected=Justify;

  HANDLE hDlg=Info.DialogInit(&MainGuid,&DialogGuid,-1,-1,76,10,NULL,DialogItems,ARRAYSIZE(DialogItems),0,0,NULL,0);

  if (hDlg == INVALID_HANDLE_VALUE)
    return INVALID_HANDLE_VALUE;

  if (Info.DialogRun(hDlg) == 7)
  {
    RightMargin=FSF.atoi(GetDataPtr(1));
    Reformat=GetCheck(3);
    SmartMode=GetCheck(4);
    Justify=GetCheck(5);
    SetRegKey(L"",L"Reformat",Reformat);
    SetRegKey(L"",L"RightMargin",RightMargin);
    SetRegKey(L"",L"SmartMode",SmartMode);
    SetRegKey(L"",L"Justify",Justify);
    Info.EditorControl(0,ECTL_TURNOFFMARKINGBLOCK,0,0);
    if (Reformat)
      ReformatBlock(RightMargin,SmartMode,Justify);
    else if (Justify)
      JustifyBlock(RightMargin);
  }

  Info.DialogFree(hDlg);

  return INVALID_HANDLE_VALUE;
}


void ReformatBlock(int RightMargin,int SmartMode,int Justify)
{
  EditorInfo ei;
  Info.EditorControl(0,ECTL_GETINFO,0,(INT_PTR)&ei);

  if (ei.BlockType!=BTYPE_STREAM || RightMargin<1)
    return;

  struct EditorSetPosition esp;
  memset(&esp,-1,sizeof(esp));
  esp.CurLine=ei.BlockStartLine;
  esp.CurPos=0;
  Info.EditorControl(0,ECTL_SETPOSITION,0,(INT_PTR)&esp);

  wchar_t *TotalString=NULL;
  int TotalLength=0,IndentSize=0x7fffffff;

  while (1)
  {
    struct EditorGetString egs;
    egs.StringNumber=-1;
    if (!Info.EditorControl(0,ECTL_GETSTRING,0,(INT_PTR)&egs))
      break;
    if (egs.SelStart==-1 || egs.SelStart==egs.SelEnd)
      break;
    int ExpandNum=-1;
    Info.EditorControl(0,ECTL_EXPANDTABS,0,(INT_PTR)&ExpandNum);
    Info.EditorControl(0,ECTL_GETSTRING,0,(INT_PTR)&egs);

    int SpaceLength=0;
    while (SpaceLength<egs.StringLength && egs.StringText[SpaceLength]==L' ')
      SpaceLength++;

    while (egs.StringLength>0 && *egs.StringText==L' ')
    {
      egs.StringText++;
      egs.StringLength--;
    }

    if (egs.StringLength>0)
    {
      if (SpaceLength<IndentSize)
        IndentSize=SpaceLength;

      TotalString=(wchar_t *)realloc(TotalString,(TotalLength+egs.StringLength+2)*sizeof(wchar_t));
      if (TotalLength!=0 && TotalString[TotalLength-1]!=L' ')
        TotalString[TotalLength++]=L' ';

      wmemcpy(TotalString+TotalLength,egs.StringText,egs.StringLength);
      TotalLength+=egs.StringLength;
    }
    if (!Info.EditorControl(0,ECTL_DELETESTRING,0,0))
    {
      free(TotalString);
      return;
    }
  }
  if(TotalString)
    TotalString[TotalLength++]=L' ';

  if (IndentSize>=RightMargin)
    IndentSize=RightMargin-1;

  const int MaxIndent=1024;
  if (IndentSize>=MaxIndent)
    IndentSize=MaxIndent-1;

  wchar_t IndentBuf[MaxIndent];
  if (IndentSize>0)
  {
    wmemset(IndentBuf,L' ',IndentSize);
    IndentBuf[IndentSize]=0;
  }

  Info.EditorControl(0,ECTL_SETPOSITION,0,(INT_PTR)&esp);
  Info.EditorControl(0,ECTL_INSERTSTRING,0,0);
  Info.EditorControl(0,ECTL_SETPOSITION,0,(INT_PTR)&esp);

  int LastSplitPos=0,PrevSpacePos;
  while (LastSplitPos<TotalLength && TotalString[LastSplitPos]==L' ')
    LastSplitPos++;
  PrevSpacePos=LastSplitPos;

  for (int I=LastSplitPos;I<TotalLength;I++)
  {
    int Length=I-LastSplitPos;
    int LastLength=PrevSpacePos-LastSplitPos;
    if (TotalString[I]==L' ' && Length>RightMargin-IndentSize && LastLength<=RightMargin-IndentSize)
    {
      if (LastLength<=0)
      {
        PrevSpacePos=I;
        LastLength=Length;
      }

      if (SmartMode)
      {
        int Space1=-1,Space2=-1;
        for (int J=PrevSpacePos-1;J>LastSplitPos+20;J--)
        {
          if (TotalString[J]==L' ')
          {
            if (Space2==-1)
              Space2=J;
            else
            {
              if (Space1==-1)
                Space1=J;
              else
                break;
            }
          }
        }
        if (Space2!=-1 && PrevSpacePos-Space2<4)
          if (Space1==-1 || Space2-Space1>4 || PrevSpacePos-Space2==2)
          {
            PrevSpacePos=Space2;
            while (PrevSpacePos>LastSplitPos+1 && TotalString[PrevSpacePos-1]==L' ')
              PrevSpacePos--;
            LastLength=PrevSpacePos-LastSplitPos;
          }
      }

      I=PrevSpacePos;

      struct EditorSetString ess;
      ess.StringNumber=-1;
      ess.StringText=TotalString+LastSplitPos;
      ess.StringEOL=NULL;
      ess.StringLength=LastLength;
      while (ess.StringLength>0 && ess.StringText[ess.StringLength-1]==L' ')
        ess.StringLength--;
      if (!Justify || ess.StringLength>=RightMargin-IndentSize || !JustifyString(RightMargin-IndentSize,ess))
        Info.EditorControl(0,ECTL_SETSTRING,0,(INT_PTR)&ess);
      else
        LastLength=RightMargin;

      while (I<TotalLength && TotalString[I]==L' ')
        PrevSpacePos=I++;

      struct EditorSetPosition esp;
      memset(&esp,-1,sizeof(esp));
      esp.CurLine=-1;

      if (IndentSize>0)
      {
        esp.CurPos=0;
        Info.EditorControl(0,ECTL_SETPOSITION,0,(INT_PTR)&esp);
        Info.EditorControl(0,ECTL_INSERTTEXT,0,(INT_PTR)IndentBuf);
      }

      esp.CurPos=LastLength+IndentSize;
      Info.EditorControl(0,ECTL_SETPOSITION,0,(INT_PTR)&esp);
      Info.EditorControl(0,ECTL_INSERTSTRING,0,0);

      LastSplitPos=I;
    }
    if (TotalString[I]==L' ')
      PrevSpacePos=I;
  }
  struct EditorSetString ess;
  ess.StringNumber=-1;
  ess.StringText=TotalString+LastSplitPos;
  ess.StringEOL=NULL;
  ess.StringLength=TotalLength-LastSplitPos;
  while (ess.StringLength>0 && ess.StringText[ess.StringLength-1]==L' ')
    ess.StringLength--;
  Info.EditorControl(0,ECTL_SETSTRING,0,(INT_PTR)&ess);

  if (IndentSize>0)
  {
    struct EditorSetPosition esp;
    memset(&esp,-1,sizeof(esp));
    esp.CurLine=-1;
    esp.CurPos=0;
    Info.EditorControl(0,ECTL_SETPOSITION,0,(INT_PTR)&esp);
    Info.EditorControl(0,ECTL_INSERTTEXT,0,(INT_PTR)IndentBuf);
  }

  free(TotalString);

  memset(&esp,-1,sizeof(esp));
  esp.CurLine=ei.CurLine;
  esp.CurPos=ei.CurPos;
  Info.EditorControl(0,ECTL_SETPOSITION,0,(INT_PTR)&esp);
}


void JustifyBlock(int RightMargin)
{
  EditorInfo ei;
  Info.EditorControl(0,ECTL_GETINFO,0,(INT_PTR)&ei);

  if (ei.BlockType!=BTYPE_STREAM)
    return;

  struct EditorGetString egs;
  egs.StringNumber=ei.BlockStartLine;

  while (1)
  {
    if (!Info.EditorControl(0,ECTL_GETSTRING,0,(INT_PTR)&egs))
      break;
    if (egs.SelStart==-1 || egs.SelStart==egs.SelEnd)
      break;
    int ExpNum=egs.StringNumber;
    if (!Info.EditorControl(0,ECTL_EXPANDTABS,0,(INT_PTR)&ExpNum))
      break;
    Info.EditorControl(0,ECTL_GETSTRING,0,(INT_PTR)&egs);

    struct EditorSetString ess;
    ess.StringNumber=egs.StringNumber;

    ess.StringText=(wchar_t*)egs.StringText;
    ess.StringEOL=(wchar_t*)egs.StringEOL;
    ess.StringLength=egs.StringLength;

    if (ess.StringLength<RightMargin)
      JustifyString(RightMargin,ess);

    egs.StringNumber++;
  }
}


int JustifyString(int RightMargin,struct EditorSetString &ess)
{
  int WordCount=0;
  int I;
  for (I=0;I<ess.StringLength-1;I++)
    if (ess.StringText[I]!=L' ' && ess.StringText[I+1]==L' ')
      WordCount++;
  if (ess.StringLength>0 && ess.StringText[ess.StringLength-1]==L' ')
    WordCount--;
  if (WordCount<=0)
    return(FALSE);
  while (ess.StringLength>0 && ess.StringText[ess.StringLength-1]==L' ')
    ess.StringLength--;
  int TotalAddSize=RightMargin-ess.StringLength;
  int AddSize=TotalAddSize/WordCount;
  int Reminder=TotalAddSize%WordCount;

  wchar_t *NewString=(wchar_t *)malloc(RightMargin*sizeof(wchar_t));
  wmemset(NewString,L' ',RightMargin);
  wmemcpy(NewString,ess.StringText,ess.StringLength);

  for (I=0;I<RightMargin-1;I++)
  {
    if (NewString[I]!=L' ' && NewString[I+1]==L' ')
    {
      int MoveSize=AddSize;
      if (Reminder)
      {
        MoveSize++;
        Reminder--;
      }
      if (MoveSize==0)
        break;
      wmemmove(NewString+I+1+MoveSize,NewString+I+1,RightMargin-(I+1+MoveSize));
      while (MoveSize--)
        NewString[I+1+MoveSize]=L' ';
    }
  }

  ess.StringText=NewString;
  ess.StringLength=RightMargin;
  Info.EditorControl(0,ECTL_SETSTRING,0,(INT_PTR)&ess);
  free(NewString);
  return(TRUE);
}
