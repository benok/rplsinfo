// rplsinfo.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#ifdef _MSC_VER
 #include "stdafx.h"
 #include <windows.h>
 #include <new.h>
 #include "conio.h"
#else
 #include <wchar.h>
 typedef char _TCHAR;
 typedef int HANDLE;
 #define INVALID_HANDLE_VALUE -1
 #define swprintf_s swprintf
 #define wcslen strlen
 #define _wtoi atoi
 #include <stdlib.h>
 #include <fcntl.h>
 #include <unistd.h>
#endif
#include <stdio.h>
#include <string.h>
#include <locale.h>

#include "rplsinfo.h"
#include "tsproginfo.h"
#include "rplsproginfo.h"

#include "tsprocess.h"
#include "convToUnicode.h"



// 定数など
#define APP_NAME_VER "rplsinfo version 1.5.2"

#ifdef _MSC_VER
#ifdef _WIN64
#define		NAMESTRING				"\n" ## APP_NAME_VER ## " (64bit)\n"
#else
#define		NAMESTRING				"\n" ## APP_NAME_VER ## " (32bit)\n"
#endif
#else
#define		NAMESTRING				"\n" ## APP_NAME_VER ## " (linux)\n"
#endif


// マクロ定義

#ifdef USE_UTF16
#define		printMsg(fmt, ...)	_tprintf(_T(fmt), __VA_ARGS__)
#define		printErrMsg(fmt, ...)	_tprintf(_T(fmt), __VA_ARGS__)
#else
#define		printMsg(...)	printf(__VA_ARGS__)
#define		printErrMsg(...)	fprintf(stderr, __VA_ARGS__)
#endif

// プロトタイプ宣言

void	initCopyParams(CopyParams*);
bool	parseCopyParams(const int32_t, _TCHAR *[], CopyParams *);
size_t	convForCsv(__WCHAR*, const size_t, const __WCHAR*, const size_t, const CopyParams*, const bool, const bool);
void	outputProgInfo(HANDLE, const ProgInfo*, const CopyParams*);



//

#ifdef _MSC_VER
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, _TCHAR** argv)
#endif
{
#ifdef linux
 #define _tsetlocale setlocale
 #define _T(x) x
#endif

#ifdef USE_UTF16
	_tsetlocale(LC_ALL, _T(""));
#else
	setlocale(LC_ALL, ".UTF-8"); // https://learn.microsoft.com/ja-jp/cpp/c-runtime-library/reference/setlocale-wsetlocale?view=msvc-170#utf-8-support
# ifdef _MSC_VER
	UINT codepage_before = GetConsoleOutputCP();
	SetConsoleOutputCP(CP_UTF8);
# endif // _MSC_VER
#endif

	// パラメータチェック

	if (argc == 1) {
		printMsg(NAMESTRING);
		exit(1);
	}

	CopyParams	param;
	initCopyParams(&param);

	if (!parseCopyParams(argc, argv, &param)) exit(1);																	// パラメータ異常なら終了


																														// 番組情報元ファイルから番組情報を読み込む

	ProgInfo	proginfo;
	memset(&proginfo, 0, sizeof(proginfo));

	if (!readFileProgInfo(argv[param.argSrc], &proginfo, &param)) exit(1);												// 失敗したら終了する


																														// 必要なら出力ファイルを開く
#ifdef USE_UTF16
	uint32_t	numWrite;
#endif
	HANDLE		hWriteFile = INVALID_HANDLE_VALUE;

	if (!param.bDisplay)
	{
#ifdef _MSC_VER
		hWriteFile = CreateFile(argv[param.argDest], GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
#else
		hWriteFile = open(argv[param.argDest], O_CREAT|O_TRUNC|O_RDWR, 0644);
#endif
		if (hWriteFile == INVALID_HANDLE_VALUE) {
			printErrMsg("保存先ファイル %s を開けませんでした.\n", argv[param.argDest]);
			exit(1);
		}

		int64_t		fsize;
		fsize = GetFileDataSize(hWriteFile);																			// ファイルサイズ取得

		if (param.bUseBOM && (fsize == 0)) {
#ifdef USE_UTF16
			const WCHAR	bom = 0xFEFF;
			WriteFileData(hWriteFile, (uint8_t*)&bom, sizeof(bom), &numWrite);											// ファイルが空ならBOM出力
#else
			// TODO: enable UTF8 BOM output here?
#endif
		}
		else {
			SeekFileData(hWriteFile, fsize);																			// 空でないならファイルの最後尾に移動
		}
	}


	// オプション指定に応じて番組内容を出力

	outputProgInfo(hWriteFile, &proginfo, &param);


	// 終了処理

	if (!param.bDisplay) {
#ifdef _MSC_VER
		CloseHandle(hWriteFile);
#else
		close(hWriteFile);
#endif
	}

#ifdef _MSC_VER
# ifndef USE_UTF16
	SetConsoleOutputCP(codepage_before);
# endif
#endif

	return 0;
}


void initCopyParams(CopyParams* param)
{
	param->argSrc = 0;
	param->argDest = 0;

	param->separator = S_NORMAL;
	param->bNoControl = true;
	param->bNoComma = true;
	param->bDQuot = false;
	param->bItemName = false;
	param->bJsonOutput = false;

	param->bDisplay = false;
	param->bCharSize = false;
	param->bIVS = false;
	param->bUseBOM = true;

	param->packet_limit = 0;
	param->tsfilepos = DEFAULTTSFILEPOS;

	memset(param->flags, 0, sizeof(param->flags));

	return;
}

#ifdef USE_UTF16
#  define ATOI _wtoi
#else
#  define ATOI atoi
#endif

bool parseCopyParams(const int32_t argn, _TCHAR *args[], CopyParams *param)
{
	int32_t		fnum = 0;
	bool		bArgSkip = false;

	for (int32_t i = 1; i < argn; i++)
	{
		if (args[i][0] == L'-')
		{
#ifdef USE_UTF16
			int32_t len = wcslen(args[i]);
#else
			int32_t len = (int32_t)strlen(args[i]);
#endif
			for (int32_t j = 1; j < len; j++)
			{
				switch (args[i][j])
				{
				case L'f':
					param->flags[fnum++] = F_FileName;
					break;
				case L'u':
					param->flags[fnum++] = F_FileNameFullPath;
					break;
				case L'k':
					param->flags[fnum++] = F_FileSize;
					break;
				case L'd':
					param->flags[fnum++] = F_RecDate;
					break;
				case L't':
					param->flags[fnum++] = F_RecTime;
					break;
				case L'p':
					param->flags[fnum++] = F_RecDuration;
					break;
				case L'z':
					param->flags[fnum++] = F_RecTimeZone;
					break;
				case L'a':
					param->flags[fnum++] = F_MakerID;
					break;
				case L'o':
					param->flags[fnum++] = F_ModelCode;
					break;
				case L's':
					param->flags[fnum++] = F_RecSrc;
					break;
				case L'c':
					param->flags[fnum++] = F_ChannelName;
					break;
				case L'n':
					param->flags[fnum++] = F_ChannelNum;
					break;
				case L'b':
					param->flags[fnum++] = F_ProgName;
					break;
				case L'i':
					param->flags[fnum++] = F_ProgDetail;
					break;
				case L'e':
					param->flags[fnum++] = F_ProgExtend;
					break;
				case L'g':
					param->flags[fnum++] = F_ProgGenre;
					break;
				case L'v':
					param->flags[fnum++] = F_ProgVideo;
					break;
				case L'm':
					param->flags[fnum++] = F_ProgAudio;
					break;
				case L'T':
					param->separator = S_TAB;
					param->bNoControl = true;
					param->bNoComma = false;
					param->bDQuot = false;
					param->bItemName = false;
					param->bJsonOutput = false;
					break;
				case L'S':
					param->separator = S_SPACE;
					param->bNoControl = true;
					param->bNoComma = false;
					param->bDQuot = false;
					param->bItemName = false;
					param->bJsonOutput = false;
					break;
				case L'C':
					param->separator = S_CSV;
					param->bNoControl = false;
					param->bNoComma = false;
					param->bDQuot = true;
					param->bItemName = false;
					param->bJsonOutput = false;
					break;
				case L'N':
					param->separator = S_NEWLINE;
					param->bNoControl = false;
					param->bNoComma = false;
					param->bDQuot = false;
					param->bItemName = false;
					param->bJsonOutput = false;
					break;
				case L'I':
					param->separator = S_ITEMNAME;
					param->bNoControl = false;
					param->bNoComma = false;
					param->bDQuot = false;
					param->bItemName = true;
					param->bJsonOutput = false;
					break;
				case L'J':
					param->separator = S_CSV;
					param->bNoControl = false;
					param->bNoComma = false;
					param->bDQuot = false;
					param->bItemName = false;
					param->bJsonOutput = true;
					break;
				case L'y':
					param->bCharSize = true;
					break;
				case L'j':
					param->bIVS = true;
					break;
				case L'q':
					param->bUseBOM = false;
					break;
				case L'F':
					if ((i == (argn - 1)) || (ATOI(args[i + 1]) < 0) || (ATOI(args[i + 1]) > 99)) {
						printErrMsg("-F オプションの引数が異常です.\n");
						return	false;
					}
					param->tsfilepos = ATOI(args[i + 1]);
					bArgSkip = true;
					break;
				case L'l':
					if ((i == (argn - 1)) || (ATOI(args[i + 1]) <= 0)) {
						printErrMsg("-l オプションの引数が異常です.\n");
						return false;
					}
					param->packet_limit = (int64_t)ATOI(args[i + 1]) * 5600;
					bArgSkip = true;
					break;
				default:
					printErrMsg("無効なスイッチが指定されました.\n");
					return	false;
				}

				if (fnum == MAXFLAGNUM) {
					printErrMsg("スイッチ指定が多すぎます.\n");
					return	false;
				}
			}

		}
		else {

			if (param->argSrc == 0) {
				param->argSrc = i;
			}
			else if (param->argDest == 0) {
				param->argDest = i;
			}
			else {
				printErrMsg("パラメータが多すぎます.\n");
				return false;
			}
		}

		if (bArgSkip) {
			i++;
			bArgSkip = false;
		}

	}

	if (param->argSrc == 0) {
		printErrMsg("パラメータが足りません.\n");
		return	false;
	}

	if (param->argDest == 0)	param->bDisplay = true;

	return	true;
}


size_t convForCsv(__WCHAR* dbuf, const size_t bufsize, const __WCHAR* sbuf, const size_t slen, const CopyParams* param, const bool is_first, const bool is_detail)
{
	size_t	dst = 0;

	if (param->bJsonOutput && is_first) dbuf[dst++] = 0x007b;		//  「{」						// JSON用出力なら先頭に"{"を出力

	if (param->bDQuot && (dst < bufsize)) dbuf[dst++] = 0x0022;		//  「"」						// CSV用出力なら項目の前後を「"」で囲む

	for (size_t src = 0; src < slen; src++)
	{
		const __WCHAR	s = sbuf[src];
		bool	bOutput = true;

		if (param->bNoControl && (s < L' '))	bOutput = false;										// 制御コードは出力しない
		if (param->bNoComma && (s == L','))		bOutput = false;										// コンマは出力しない
		if (param->bDisplay && (s == 0x000D))	bOutput = false;										// コンソール出力の場合は改行コードの0x000Dは省略する

		// JSONの詳細出力時改行を\nに変換
		if (param->bJsonOutput && is_detail && (s == 0x000D))	{ dbuf[dst++] = 0x005c; dbuf[dst++] = 0x006e; bOutput = false; }
		if (param->bJsonOutput && (s == 0x000A))	bOutput = false;

		if (param->bDQuot && (s == 0x0022) && (dst < bufsize)) dbuf[dst++] = 0x0022;					// CSV用出力なら「"」の前に「"」でエスケープ
		if (bOutput && (dst < bufsize)) dbuf[dst++] = s;										// 出力
	}

	if (param->bDQuot && (dst < bufsize)) dbuf[dst++] = 0x0022;		//  「"」

	if (dst < bufsize) dbuf[dst] = 0x0000;

	return dst;
}


void outputProgInfo(HANDLE hFile, const ProgInfo* proginfo, const CopyParams* param)
{
#ifdef USE_UTF16
	WCHAR		sstr[CONVBUFSIZE];
	WCHAR		dstr[CONVBUFSIZE];
#else
	char		sstr[CONVBUFSIZE];
	char16_t	dstr[CONVBUFSIZE];
#endif

	uint32_t	numWrite;
	int32_t		i = 0;

	while (param->flags[i] != 0) {

		size_t	slen = 0;

		// flagsに応じて出力する項目をsstrにコピーする

		switch (param->flags[i])
		{
		case F_FileName:
#ifdef USE_UTF16
			if (param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[ファイル名]\r\n");
			if (param->bJsonOutput) slen = swprintf_s(sstr, CONVBUFSIZE, L"\"filename\":\"");
			slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%s%s", proginfo->fname, proginfo->fext);
			if (param->bJsonOutput) slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"\"");
#else
			if (param->bItemName) slen = snprintf(sstr, CONVBUFSIZE, "[ファイル名]\r\n");
			if (param->bJsonOutput) slen = snprintf(sstr, CONVBUFSIZE, "\"filename\":\"");
			slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "%s%s", proginfo->fname, proginfo->fext);
			if (param->bJsonOutput) slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "\"");
#endif
			break;
		case F_FileNameFullPath:
#ifdef USE_UTF16
			if (param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[フルパスファイル名]\r\n");
			if (param->bJsonOutput) slen = swprintf_s(sstr, CONVBUFSIZE, L"\"fullpath\":\"");
			slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%s", proginfo->fullpath);
			if (param->bJsonOutput) slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"\"");
#else
			if (param->bItemName) slen = snprintf(sstr, CONVBUFSIZE, "[フルパスファイル名]\r\n");
			if (param->bJsonOutput) slen = snprintf(sstr, CONVBUFSIZE, "\"fullpath\":\"");
			slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "%s", proginfo->fullpath);
			if (param->bJsonOutput) slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "\"");
#endif
			break;
		case F_FileSize:
#ifdef USE_UTF16
			if (param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[ファイルサイズ]\r\n");
			if (param->bJsonOutput) slen = swprintf_s(sstr, CONVBUFSIZE, L"\"fsize\":");
			slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%lld", proginfo->fsize);
#else
			if (param->bItemName) slen = snprintf(sstr, CONVBUFSIZE, "[ファイルサイズ]\r\n");
			if (param->bJsonOutput) slen = snprintf(sstr, CONVBUFSIZE, "\"fsize\":");
			slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "%lld", proginfo->fsize);
#endif
			break;
		case F_RecDate:
#ifdef USE_UTF16
			if (param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[録画日付]\r\n");
			if (param->bJsonOutput) slen = swprintf_s(sstr, CONVBUFSIZE, L"\"date\":\"");
			slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%.4d/%.2d/%.2d", proginfo->recyear, proginfo->recmonth, proginfo->recday);
			if (param->bJsonOutput) slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"\"");
#else
			if (param->bItemName) slen = snprintf(sstr, CONVBUFSIZE, "[録画日付]\r\n");
			if (param->bJsonOutput) slen = snprintf(sstr, CONVBUFSIZE, "\"date\":\"");
			slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "%.4d/%.2d/%.2d", proginfo->recyear, proginfo->recmonth, proginfo->recday);
			if (param->bJsonOutput) slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "\"");
#endif
			break;
		case F_RecTime:
#ifdef USE_UTF16
			if (param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[録画時刻]\r\n");
			if (param->bJsonOutput) slen = swprintf_s(sstr, CONVBUFSIZE, L"\"time\":\"");
			slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%.2d:%.2d:%.2d", proginfo->rechour, proginfo->recmin, proginfo->recsec);
			if (param->bJsonOutput) slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"\"");
#else
			if (param->bItemName) slen = snprintf(sstr, CONVBUFSIZE, "[録画時刻]\r\n");
			if (param->bJsonOutput) slen = snprintf(sstr, CONVBUFSIZE, "\"time\":\"");
			slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "%.2d:%.2d:%.2d", proginfo->rechour, proginfo->recmin, proginfo->recsec);
			if (param->bJsonOutput) slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "\"");
#endif
			break;
		case F_RecDuration:
#ifdef USE_UTF16
			if (param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[録画期間]\r\n");
			if (param->bJsonOutput) slen = swprintf_s(sstr, CONVBUFSIZE, L"\"duration\":\"");
			slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%.2d:%.2d:%.2d", proginfo->durhour, proginfo->durmin, proginfo->dursec);
			if (param->bJsonOutput) slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"\"");
#else
			if (param->bItemName) slen = snprintf(sstr, CONVBUFSIZE, "[録画期間]\r\n");
			if (param->bJsonOutput) slen = snprintf(sstr, CONVBUFSIZE, "\"duration\":\"");
			slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "%.2d:%.2d:%.2d", proginfo->durhour, proginfo->durmin, proginfo->dursec);
			if (param->bJsonOutput) slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "\"");
#endif
			break;
		case F_RecTimeZone:
#ifdef USE_UTF16
			if (param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[タイムゾーン]\r\n");
			if (param->bJsonOutput) slen = swprintf_s(sstr, CONVBUFSIZE, L"\"tz_id\":");
			slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%d", proginfo->rectimezone);
#else
			if (param->bItemName) slen = snprintf(sstr, CONVBUFSIZE, "[タイムゾーン]\r\n");
			if (param->bJsonOutput) slen = snprintf(sstr, CONVBUFSIZE, "\"tz_id\":");
			slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "%d", proginfo->rectimezone);
#endif
			break;
		case F_MakerID:
#ifdef USE_UTF16
			if (param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[メーカーID]\r\n");
			if (param->bJsonOutput) slen = swprintf_s(sstr, CONVBUFSIZE, L"\"maker_id\":");
			if (proginfo->makerid != -1) {
				slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%d", proginfo->makerid);
			}
			else {
				if (param->bJsonOutput)
				slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"-1");
				else
				slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"n/a");
			}
#else
			if (param->bItemName) slen = snprintf(sstr, CONVBUFSIZE, "[メーカーID]\r\n");
			if (param->bJsonOutput) slen = snprintf(sstr, CONVBUFSIZE, "\"maker_id\":");
			if (proginfo->makerid != -1) {
				slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "%d", proginfo->makerid);
			}
			else {
				if (param->bJsonOutput)
				slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "-1");
				else
				slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "n/a");
			}
#endif
			break;
		case F_ModelCode:
#ifdef USE_UTF16
			if (param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[メーカー機種コード]\r\n");
			if (param->bJsonOutput) slen = swprintf_s(sstr, CONVBUFSIZE, L"\"model_id\":");
			if (proginfo->modelcode != -1) {
				slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%d", proginfo->modelcode);
			}
			else {
				if (param->bJsonOutput)
				slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"-1");
				else
				slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"n/a");
			}
#else
			if (param->bItemName) slen = snprintf(sstr, CONVBUFSIZE, "[メーカー機種コード]\r\n");
			if (param->bJsonOutput) slen = snprintf(sstr, CONVBUFSIZE, "\"model_id\":");
			if (proginfo->modelcode != -1) {
				slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "%d", proginfo->modelcode);
			}
			else {
				if (param->bJsonOutput)
				slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "-1");
				else
				slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "n/a");
			}
#endif
			break;
		case F_RecSrc:
#ifdef USE_UTF16
			if (param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[放送種別]\r\n");
			if (param->bJsonOutput) slen = swprintf_s(sstr, CONVBUFSIZE, L"\"recsrc\":\"");
			slen += getRecSrcStr(sstr + slen, CONVBUFSIZE - slen, proginfo->recsrc);
			if (param->bJsonOutput) slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"\"");
#else
			if (param->bItemName) slen = snprintf(sstr, CONVBUFSIZE, "[放送種別]\r\n");
			if (param->bJsonOutput) slen = snprintf(sstr, CONVBUFSIZE, "\"recsrc\":\"");
			slen += getRecSrcStr(sstr + slen, CONVBUFSIZE - slen, proginfo->recsrc);
			if (param->bJsonOutput) slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "\"");
#endif
			break;
		case F_ChannelNum:
#ifdef USE_UTF16
			if (param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[チャンネル番号]\r\n");
			if (param->bJsonOutput) slen = swprintf_s(sstr, CONVBUFSIZE, L"\"ch\":\"");
			slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%.3dch", proginfo->chnum);
			if (param->bJsonOutput) slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"\"");
#else
			if (param->bItemName) slen = snprintf(sstr, CONVBUFSIZE, "[チャンネル番号]\r\n");
			if (param->bJsonOutput) slen = snprintf(sstr, CONVBUFSIZE, "\"ch\":\"");
			slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "%.3dch", proginfo->chnum);
			if (param->bJsonOutput) slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "\"");
#endif
			break;
		case F_ChannelName:
#ifdef USE_UTF16
			if (param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[放送局名]\r\n");
			if (param->bJsonOutput) slen = swprintf_s(sstr, CONVBUFSIZE, L"\"station\":\"");
			slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%s", proginfo->chname);
			if (param->bJsonOutput) slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"\"");
#else
			if (param->bItemName) slen = snprintf(sstr, CONVBUFSIZE, "[放送局名]\r\n");
			if (param->bJsonOutput) slen = snprintf(sstr, CONVBUFSIZE, "\"station\":\"");
			slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "%s", u16tou8(proginfo->chname));
			if (param->bJsonOutput) slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "\"");
#endif
			break;
		case F_ProgName:
#ifdef USE_UTF16
			if (param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[番組名]\r\n");
			if (param->bJsonOutput) slen = swprintf_s(sstr, CONVBUFSIZE, L"\"title\":\"");
			slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%s", proginfo->pname);
			if (param->bJsonOutput) slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"\"");
#else
			if (param->bItemName) slen = snprintf(sstr, CONVBUFSIZE, "[番組名]\r\n");
			if (param->bJsonOutput) slen = snprintf(sstr, CONVBUFSIZE, "\"title\":\"");
			slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "%s", u16tou8(proginfo->pname));
			if (param->bJsonOutput) slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "\"");
#endif
			break;
		case F_ProgDetail:
#ifdef USE_UTF16
			if (param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[番組内容]\r\n");
			if (param->bJsonOutput) slen = swprintf_s(sstr, CONVBUFSIZE, L"\"detail\":\"");
			slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%s", proginfo->pdetail);
			if (param->bJsonOutput) slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"\"");
#else
			if (param->bItemName) slen = snprintf(sstr, CONVBUFSIZE, "[番組内容]\r\n");
			if (param->bJsonOutput) slen = snprintf(sstr, CONVBUFSIZE, "\"detail\":\"");
			slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "%s", u16tou8(proginfo->pdetail));
			if (param->bJsonOutput) slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "\"");
#endif
			break;
		case F_ProgExtend:
#ifdef USE_UTF16
			if (param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[内容詳細]\r\n");
			if (param->bJsonOutput) slen = swprintf_s(sstr, CONVBUFSIZE, L"\"extend\":\"");
			if (proginfo->pextendlen == -1) {
				slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"n/a");
			}
			else {
				slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%s", proginfo->pextend);
			}
			if (param->bJsonOutput) slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"\"");
#else
			if (param->bItemName) slen = snprintf(sstr, CONVBUFSIZE, "[内容詳細]\r\n");
			if (param->bJsonOutput) slen = snprintf(sstr, CONVBUFSIZE, "\"extend\":\"");
			if (proginfo->pextendlen == -1) {
				slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "n/a");
			}
			else {
				slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "%s", u16tou8(proginfo->pextend));
			}
			if (param->bJsonOutput) slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "\"");
#endif
			break;
		case F_ProgGenre:
#ifdef USE_UTF16
			if (param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[番組ジャンル]\r\n");
			if (param->bJsonOutput) slen = swprintf_s(sstr, CONVBUFSIZE, L"\"genre\":");
			slen += putGenreStr(sstr + slen, CONVBUFSIZE - slen, proginfo->genre, param->bJsonOutput);
#else
			if (param->bItemName) slen = snprintf(sstr, CONVBUFSIZE, "[番組ジャンル]\r\n");
			if (param->bJsonOutput) slen = snprintf(sstr, CONVBUFSIZE, "\"genre\":");
			slen += putGenreStr(sstr + slen, CONVBUFSIZE - slen, proginfo->genre, param->bJsonOutput);
#endif
			break;
		case F_ProgVideo:
#ifdef USE_UTF16
			if (param->bItemName) slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"[映像]\r\n");
			if (param->bJsonOutput) slen = swprintf_s(sstr, CONVBUFSIZE, L"\"videofmt\":\"");
			slen += putFormatStr(sstr + slen, CONVBUFSIZE - slen, proginfo->videoformat);
			if (param->bJsonOutput) slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"\"");
#else
			if (param->bItemName) slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "[映像]\r\n");
			if (param->bJsonOutput) slen = snprintf(sstr, CONVBUFSIZE, "\"videofmt\":\"");
			slen += putFormatStr(sstr + slen, CONVBUFSIZE - slen, proginfo->videoformat);
			if (param->bJsonOutput) slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "\"");
#endif
			break;
		case F_ProgAudio:
#ifdef USE_UTF16
			if (param->bItemName) slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"[音声]\r\n");
			if (param->bJsonOutput) slen = swprintf_s(sstr, CONVBUFSIZE, L"\"audiofmt\":\"");
			for (int32_t audioNum = 0; audioNum < 8; audioNum++)
			{
				if (proginfo->audioformat[audioNum] == -1) break;
				if (audioNum != 0) slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"\r\n");
				slen += putFormatStr(sstr + slen, CONVBUFSIZE - slen, proginfo->audioformat[audioNum]);
				slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L" ");
				slen += putSamplingrateStr(sstr + slen, CONVBUFSIZE - slen, proginfo->audiosamplingrate[audioNum]);
				if (proginfo->audiotextlen[audioNum] != 0) slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L" %s", proginfo->audiotext[audioNum]);
				slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L" (%hs)", proginfo->audiolang[audioNum]);
			}
			if (param->bJsonOutput) slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"\"");
#else
			if (param->bItemName) slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "[音声]\r\n");
			if (param->bJsonOutput) slen = snprintf(sstr, CONVBUFSIZE, "\"audiofmt\":\"");
			for (int32_t audioNum = 0; audioNum < 8; audioNum++)
			{
				if (proginfo->audioformat[audioNum] == -1) break;
				if (audioNum != 0) slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "\r\n");
				slen += putFormatStr(sstr + slen, CONVBUFSIZE - slen, proginfo->audioformat[audioNum]);
				slen += snprintf(sstr + slen, CONVBUFSIZE - slen, " ");
				slen += putSamplingrateStr(sstr + slen, CONVBUFSIZE - slen, proginfo->audiosamplingrate[audioNum]);
				if (proginfo->audiotextlen[audioNum] != 0) slen += snprintf(sstr + slen, CONVBUFSIZE - slen, " %s", u16tou8(proginfo->audiotext[audioNum]));
				slen += snprintf(sstr + slen, CONVBUFSIZE - slen, " (%hs)", proginfo->audiolang[audioNum]);
			}
			if (param->bJsonOutput) slen += snprintf(sstr + slen, CONVBUFSIZE - slen, "\"");
#endif
			break;
		default:
			slen += 0;
			break;
		}


		// 出力形式に応じてsstrを調整
		bool is_detail = ((param->flags[i] == F_ProgDetail) || (param->flags[i] == F_ProgExtend)); // 項目が詳細情報か否か(JSON出力用)
#ifdef USE_UTF16
		size_t	dlen = convForCsv(dstr, CONVBUFSIZE - 5, sstr, slen, param, i == 0, is_detail);
#else
		size_t  u16slen;
		__WCHAR*  u16s = u8tou16(sstr, &u16slen);
		size_t	dlen = convForCsv(dstr, CONVBUFSIZE - 5, u16s, u16slen, param, i == 0, is_detail);
#endif

		// セパレータに関する処理

		if (param->flags[i + 1] != 0)
		{
			switch (param->separator)
			{
			case S_NORMAL:
			case S_CSV:
#ifdef USE_UTF16
				dstr[dlen++] = L',';								// COMMA
#else
				dstr[dlen++] = ',';									// COMMA
#endif
				break;
			case S_TAB:
				dstr[dlen++] = 0x0009;								// TAB
				break;
			case S_SPACE:
#ifdef USE_UTF16
				dstr[dlen++] = L' ';								// SPACE
#else
				dstr[dlen++] = ' ';									// SPACE
#endif
				break;
			case S_NEWLINE:
			case S_ITEMNAME:
				if (!param->bDisplay) dstr[dlen++] = 0x000D;			// 改行2回
				dstr[dlen++] = 0x000A;
				if (!param->bDisplay) dstr[dlen++] = 0x000D;
				dstr[dlen++] = 0x000A;
				break;
			default:
				break;
			}
		}
		else
		{
			if (param->bJsonOutput) dstr[dlen++] = 0x007d;										// JSONなら"}"を出力
			if (!param->bDisplay) dstr[dlen++] = 0x000D;											// 全項目出力後の改行
			dstr[dlen++] = 0x000A;
			if ((param->separator == S_NEWLINE) || (param->separator == S_ITEMNAME)) {				// セパレータがS_NEWLINE, S_ITEMNAMEの場合は１行余分に改行する
				if (!param->bDisplay) dstr[dlen++] = 0x000D;
				dstr[dlen++] = 0x000A;
			}
		}


		// データ出力

		if (!param->bDisplay) {
#ifdef USE_UTF16
			WriteFileData(hFile, (uint8_t*)dstr, (uint32_t)dlen * sizeof(WCHAR), &numWrite);		// ディスク出力
#else
			dstr[dlen] = 0x0000;
			size_t len;
			char* u8 = u16tou8(dstr, &len);
			WriteFileData(hFile, (uint8_t*)u8, (uint32_t)len, &numWrite);							// ディスク出力
#endif
		}
		else {
			dstr[dlen] = 0x0000;
#ifdef USE_UTF16
			printMsg("%s", dstr);																	// コンソール出力
#else
			printMsg("%s", u16tou8(dstr));															// コンソール出力
#endif
		}

		i++;
	}

	return;
}

