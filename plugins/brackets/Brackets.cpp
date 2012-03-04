﻿#include <CRT/crt.hpp>
#include <plugin.hpp>
#include <PluginSettings.hpp>
#include <DlgBuilder.hpp>
#include "Brackets.hpp"
#include "BrackLng.hpp"
#include "version.hpp"
#include <initguid.h>
#include "guid.hpp"

#if defined(__GNUC__)
#ifdef __cplusplus
extern "C" {
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

static struct Options Opt;
static struct PluginStartupInfo Info;

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
	PluginSettings settings(MainGuid, ::Info.SettingsControl);
	Opt.IgnoreQuotes=settings.Get(0,L"IgnoreQuotes",0);
	Opt.IgnoreAfter=settings.Get(0,L"IgnoreAfter",0);
	Opt.BracketPrior=settings.Get(0,L"BracketPrior",1);
	Opt.JumpToPair=settings.Get(0,L"JumpToPair",1);
	Opt.Beep=settings.Get(0,L"Beep",0);
	settings.Get(0,L"QuotesType",Opt.QuotesType,ARRAYSIZE(Opt.QuotesType),L"''\"\"`'``„”");
	settings.Get(0,L"Brackets1",Opt.Brackets1,ARRAYSIZE(Opt.Brackets1),L"<>{}[]()\"\"''%%«»");
	settings.Get(0,L"Brackets2",Opt.Brackets2,ARRAYSIZE(Opt.Brackets2),L"/**/<\?\?><%%>");
}

void WINAPI GetPluginInfoW(struct PluginInfo *Info)
{
	Info->StructSize=sizeof(*Info);
	Info->Flags=PF_EDITOR|PF_DISABLEPANELS;
	static const wchar_t *PluginMenuStrings[1];
	PluginMenuStrings[0]=GetMsg(MTitle);
	Info->PluginMenu.Guids=&MenuGuid;
	Info->PluginMenu.Strings=PluginMenuStrings;
	Info->PluginMenu.Count=ARRAYSIZE(PluginMenuStrings);
	Info->PluginConfig.Guids=&MenuGuid;
	Info->PluginConfig.Strings=PluginMenuStrings;
	Info->PluginConfig.Count=ARRAYSIZE(PluginMenuStrings);
}

int Config()
{
	PluginDialogBuilder Builder(Info, MainGuid, DialogGuid, MTitle, nullptr);
	Builder.StartSingleBox(MRules, true);
	Builder.AddCheckbox(MIgnoreQuotation, &Opt.IgnoreQuotes);
	Builder.AddCheckbox(MIgnoreAfter, &Opt.IgnoreAfter);
	Builder.AddCheckbox(MPriority, &Opt.BracketPrior);
	Builder.AddCheckbox(MJumpToPair, &Opt.JumpToPair);
	Builder.AddCheckbox(MBeep, &Opt.Beep);
	Builder.EndSingleBox();
	Builder.StartSingleBox(MDescriptions, true);
	Builder.AddText(MTypeQuotes);
	Builder.AddFixEditField(Opt.QuotesType, ARRAYSIZE(Opt.QuotesType), ARRAYSIZE(Opt.QuotesType)-1);
	Builder.AddText(MDescript1);
	Builder.AddFixEditField(Opt.Brackets1, ARRAYSIZE(Opt.Brackets1), ARRAYSIZE(Opt.Brackets1)-1);
	Builder.AddText(MDescript2);
	Builder.AddFixEditField(Opt.Brackets2, ARRAYSIZE(Opt.Brackets2), ARRAYSIZE(Opt.Brackets2)-1);
	Builder.EndSingleBox();
	Builder.AddOKCancel(MSave, MCancel);

	if(Builder.ShowDialog())
	{
		PluginSettings settings(MainGuid, Info.SettingsControl);
		settings.Set(0,L"IgnoreQuotes",Opt.IgnoreQuotes);
		settings.Set(0,L"IgnoreAfter",Opt.IgnoreAfter);
		settings.Set(0,L"BracketPrior",Opt.BracketPrior);
		settings.Set(0,L"JumpToPair",Opt.JumpToPair);
		settings.Set(0,L"Beep",Opt.Beep);
		settings.Set(0,L"QuotesType",Opt.QuotesType);
		settings.Set(0,L"Brackets1",Opt.Brackets1);
		settings.Set(0,L"Brackets2",Opt.Brackets2);
		return TRUE;
	}

	return FALSE;
}

int WINAPI ConfigureW(const ConfigureInfo* Info)
{
	return Config();
}

int ShowMenu(int Type)
{
	struct FarMenuItem shMenu[4]= {};
	static const wchar_t *HelpTopic[2]= {L"Find",L"Direct"};
	shMenu[0].Text = GetMsg((Type?MBForward:MBrackMath));
	shMenu[1].Text = GetMsg((Type?MBBackward:MBrackSelect));
	shMenu[2].Flags = MIF_SEPARATOR;
	shMenu[3].Text = GetMsg(MConfigure);
	int Ret;

	while(1)
	{
		Ret=Info.Menu(&MainGuid, nullptr,-1,-1,0,FMENU_WRAPMODE,GetMsg(MTitle),NULL,HelpTopic[Type&1],NULL,NULL,shMenu,ARRAYSIZE(shMenu));

		if(Ret == 3)
			Config();
		else
			break;
	}

	return Ret;
}

HANDLE WINAPI OpenW(const struct OpenInfo *OInfo)
{
	struct EditorGetString egs;
	struct EditorSetPosition esp,espo;
	struct EditorSelect es;

	wchar_t Bracket,Bracket1,Bracket_1;
	wchar_t Ch,Ch1,Ch_1;
	wchar_t B21=0,B22=0,B23=0,B24=0;

	int nQuotes=0;
	int isSelect=-1;
	int CurPos,i=3,j,k;
	int Direction=0,DirectQuotes=-1;
	int types=BrZERO;
	BOOL found=FALSE;
	int MatchCount=1;

	int idxBrackets1=0;
	int lenBrackets1=0;
	int idxBrackets2=0;
	int lenBrackets2=0;

	EditorInfo ei;
	Info.EditorControl(-1,ECTL_GETINFO,0,&ei);

	espo.CurTabPos=ei.CurTabPos;
	espo.TopScreenLine=ei.TopScreenLine;
	espo.LeftPos=ei.LeftPos;
	espo.Overtype=ei.Overtype;
	espo.CurLine=ei.CurLine;
	espo.CurPos=CurPos=ei.CurPos;
	egs.StringNumber=-1;
	Info.EditorControl(-1,ECTL_GETSTRING,0,&egs);

	if (OInfo->OpenFrom==OPEN_FROMMACRO)
	{
		OpenMacroInfo* mi=(OpenMacroInfo*)OInfo->Data;
		if (mi->Count)
		{
			if (FMVT_INTEGER==mi->Values[0].Type||FMVT_UNKNOWN==mi->Values[0].Type)
			{
				switch (mi->Values[0].Integer)
				{
					case 0: // search fwd
						isSelect=0;
						DirectQuotes=1;
						break;
					case 1: // search back
						isSelect=0;
						DirectQuotes=0;
						break;
					case 2: // select fwd
						DirectQuotes=1;
						isSelect=1;
						break;
					case 3: // select back
						DirectQuotes=0;
						isSelect=1;
						break;
					case 4:
						Config();
					default:
						return INVALID_HANDLE_VALUE;
				}
			}
			else // other var type ==> $Recycle.Bin
				return INVALID_HANDLE_VALUE;
		}
	}

	if(isSelect == -1)
		if((isSelect=ShowMenu(0)) == -1)
			return INVALID_HANDLE_VALUE;

	if(CurPos > egs.StringLength)
		return INVALID_HANDLE_VALUE;

	egs.StringNumber=espo.CurLine;
	isSelect=isSelect == 1;
	Bracket_1=(CurPos-1 >= 0?egs.StringText[CurPos-1]:L'\0');
	Bracket1=(CurPos+1 < egs.StringLength?egs.StringText[CurPos+1]:L'\0');

	if(!Opt.QuotesType[0])
	{
		Opt.IgnoreQuotes=1;
	}
	else
	{
		// размер Opt.QuotesType должен быть кратный двум (иначе усекаем)
		i=lstrlen(Opt.QuotesType);

		if((i&1) == 1)
		{
			if(--i > 0)
				Opt.QuotesType[i]=0;
			else
				Opt.IgnoreQuotes=1;
		}

		nQuotes=i;
	}

	Opt.BracketPrior&=1;
	Opt.IgnoreAfter&=1;

	if(Opt.IgnoreQuotes == 0)
	{
		for(i=0; i < nQuotes; i+=2)
			if(Bracket_1 == Opt.QuotesType[i] && Bracket1 == Opt.QuotesType[i+1])
				return INVALID_HANDLE_VALUE;
	}

	Bracket=(CurPos == egs.StringLength)?L'\0':egs.StringText[CurPos];

	// размер Opt.Brackets1 должен быть кратный двум (иначе усекаем)
	if(((lenBrackets1=lstrlen(Opt.Brackets1)) & 1) != 0)
	{
		lenBrackets1-=(lenBrackets1&1);
		Opt.Brackets1[lenBrackets1]=0;
	}

	lenBrackets1>>=1;

	// размер Opt.Brackets1 должен быть кратный четырем (иначе усекаем)
	if(((lenBrackets2=lstrlen(Opt.Brackets2)) & 3) != 0)
	{
		lenBrackets2-=(lenBrackets2&3);
		Opt.Brackets2[lenBrackets2]=0;
	}

	lenBrackets2>>=2;
	// анализ того, что под курсором
	i=3;
	short BracketPrior=Opt.BracketPrior;

	while(--i)
	{
		if(BracketPrior == 1)
		{
			BracketPrior--;

			if(types == BrZERO && lenBrackets2)
			{
				for(idxBrackets2=0; lenBrackets2 > 0; --lenBrackets2,idxBrackets2+=4)
				{
					B21=Opt.Brackets2[idxBrackets2+0];
					B22=Opt.Brackets2[idxBrackets2+1];
					B23=Opt.Brackets2[idxBrackets2+2];
					B24=Opt.Brackets2[idxBrackets2+3];

					if(Bracket == B21 || Bracket == B22 || Bracket == B23 || Bracket == B24)
					{
						if((Bracket==B21 && Bracket1==B22) || (Bracket==B22 && Bracket_1==B21))
						{
							types=BrTwo;
							Direction=1;
						}
						else if((Bracket_1==B23 && Bracket==B24) || (Bracket==B23 && Bracket1==B24))
						{
							types=BrTwo;
							Direction=-1;
						}

						break;
					}
				}
			}
		}
		else
		{
			BracketPrior++;

			if(types == BrZERO && lenBrackets1)
			{
				int LB=lenBrackets1;

				for(idxBrackets1=0; lenBrackets1 > 0; --lenBrackets1,idxBrackets1+=2)
				{
					B21=Opt.Brackets1[idxBrackets1+0];
					B22=Opt.Brackets1[idxBrackets1+1];

					if(Bracket==B21)
					{
						types=BrOne;
						Direction=1;
						break;
					}
					else if(Bracket==B22)
					{
						types=BrOne;
						Direction=-1;
						break;
					}
				}

				if(types == BrZERO && !Opt.IgnoreAfter)
				{
					for(idxBrackets1=0; LB > 0; --LB,idxBrackets1+=2)
					{
						B21=Opt.Brackets1[idxBrackets1+0];
						B22=Opt.Brackets1[idxBrackets1+1];

						if(Bracket_1==B21)
						{
							types=BrRight;
							Direction=1;
							break;
						}
						else if(Bracket_1==B22)
						{
							types=BrRight;
							Direction=-1;
							break;
						}
					}
				}
			}
		}
	}

	if(Opt.IgnoreAfter && types == BrRight)
		return INVALID_HANDLE_VALUE;

	if(types == BrZERO)
		return INVALID_HANDLE_VALUE;

	if(B21 == B22)
	{
		if (DirectQuotes == -1)
			if((DirectQuotes=ShowMenu(1)) == -1)
				return INVALID_HANDLE_VALUE;

		Direction=DirectQuotes == 0?1:-1;
		types=BrOneMath;
	}

	esp.CurPos=esp.CurTabPos=esp.TopScreenLine=esp.LeftPos=esp.Overtype=-1;
	esp.CurLine=egs.StringNumber;
	egs.StringNumber=-1;

	// поиск пары
	while(!found)
	{
		CurPos+=Direction;
		bool cond_gt=CurPos >= egs.StringLength?true:false;

		if(cond_gt || CurPos < 0)
		{
			if(cond_gt)
				esp.CurLine++;
			else
				esp.CurLine--;

			if(esp.CurLine >= ei.TotalLines || esp.CurLine < 0)
				break;

			Info.EditorControl(-1,ECTL_SETPOSITION,0,&esp);
			Info.EditorControl(-1,ECTL_GETSTRING,0,&egs);

			if(cond_gt)
				CurPos=0;
			else
				CurPos=egs.StringLength-1;
		}

		if(CurPos > egs.StringLength || CurPos < 0)
			continue;

		Ch_1=(CurPos-1 >= 0?egs.StringText[CurPos-1]:L'\0');
		Ch=((CurPos == egs.StringLength)?L'\0':egs.StringText[CurPos]);
		Ch1=(CurPos+1 < egs.StringLength?egs.StringText[CurPos+1]:L'\0');

		// BUGBUGBUG!!!
		if(Opt.IgnoreQuotes == 1)
		{
			for(k=j=0; j < nQuotes; j+=2)
			{
				if(Ch_1 == Opt.QuotesType[j] && Ch1 == Opt.QuotesType[j+1])
				{
					k++;
					break;
				}
			}

			if(k)
				continue;
		}

		switch(types)
		{
				/***************************************************************/
			case BrOneMath:
			{
				if(Ch == Bracket)
				{
					if((Bracket_1==L'\\' && Ch_1==L'\\') || (Bracket_1!=L'\\' && Ch_1!=L'\\'))
						found=TRUE;
				}

				break;
			}
			/***************************************************************/
			case BrRight:
			{
				if(Ch == Bracket_1)
				{
					MatchCount++;
				}
				else if((Ch==B21 && Bracket_1==B22) || (Ch==B22 && Bracket_1==B21))
				{
					--MatchCount;

					if((Direction ==  1 && MatchCount == 0) || (Direction == -1 && MatchCount == 1))
						found=TRUE;
				}

				break;
			}
			/***************************************************************/
			case BrOne:
			{
				if(Ch == Bracket)
				{
					MatchCount++;
				}
				else if((Ch==B21 && Bracket==B22) || (Ch==B22 && Bracket==B21))
				{
					if(--MatchCount==0)
						found=TRUE;
				}

				break;
			}
			/***************************************************************/
			case BrTwo:
			{
				if((Direction == 1 &&
				        ((Bracket==B21 && Ch==B21 && Ch1  == B22) ||
				         (Bracket==B22 && Ch==B22 && Ch_1 == B21))
				   ) ||
				        (Direction == -1 &&
				         ((Bracket==B23 && Ch==B23 && Ch1  == B24) ||
				          (Bracket==B24 && Ch==B24 && Ch_1 == B23))
				        )
				  )
				{
					MatchCount++;
				}
				else if(
				    (Bracket==B21 && Ch==B23 && Bracket1 ==B22 && Ch1 ==B24) ||
				    (Bracket==B22 && Ch==B24 && Bracket_1==B21 && Ch_1==B23) ||
				    (Bracket==B23 && Ch==B21 && Bracket1 ==B24 && Ch1 ==B22) ||
				    (Bracket==B24 && Ch==B22 && Bracket_1==B23 && Ch_1==B21)
				)
				{
					if(--MatchCount==0)
						found=TRUE;
				}

				break;
			}
		}
	}

	if(found)
	{
		egs.StringNumber=esp.CurLine;

		if(types == BrTwo)
		{
			if(Bracket == B21 || Bracket == B24)
				CurPos+=Direction;
			else if(Bracket == B22 || Bracket == B23)
				CurPos-=Direction;
		}

		esp.CurPos=CurPos;
		esp.CurTabPos=esp.LeftPos=esp.Overtype=-1;

		if(egs.StringNumber<ei.TopScreenLine || egs.StringNumber>=ei.TopScreenLine+ei.WindowSizeY)
		{
			esp.TopScreenLine=esp.CurLine-ei.WindowSizeY/2;

			if(esp.TopScreenLine < 0)
				esp.TopScreenLine=0;
		}
		else
		{
			esp.TopScreenLine=-1;
		}

		if(!isSelect || (isSelect && Opt.JumpToPair))
			Info.EditorControl(-1,ECTL_SETPOSITION,0,&esp);

		if(Opt.Beep)
			MessageBeep(0);

		if(isSelect)
		{
			es.BlockType=BTYPE_STREAM;
			es.BlockStartLine=Min(esp.CurLine,espo.CurLine);
			es.BlockStartPos=(Direction > 0?espo.CurPos:esp.CurPos);
			es.BlockHeight=Max(esp.CurLine,espo.CurLine)-Min(esp.CurLine,espo.CurLine)+1;

			if(Direction > 0)
				es.BlockWidth=esp.CurPos-espo.CurPos+1;
			else
				es.BlockWidth=espo.CurPos-esp.CurPos+1;

			if(types == BrRight)
			{
				if(Direction > 0)
				{
					es.BlockStartPos--;
					es.BlockWidth++;
				}
				else if(Direction < 0)
				{
					es.BlockWidth--;
				}
			}

			Info.EditorControl(-1,ECTL_SELECT,0,&es);
		}
	}
	else
	{
		Info.EditorControl(-1,ECTL_SETPOSITION,0,&espo);
	}

	return INVALID_HANDLE_VALUE;
}
