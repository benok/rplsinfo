#pragma once

#ifdef _MSC_VER
 #include <windows.h>
#endif
#include <stdint.h>

#ifdef USE_UTF16
  typedef WCHAR __WCHAR;
#else
  typedef char16_t __WCHAR;
  typedef char CHAR;
#endif

#ifdef __linux__
 typedef char _TCHAR;
 #include <linux/limits.h>
 #define _MAX_PATH PATH_MAX
#endif

// 定数など

#define		F_FileName				1
#define		F_FileNameFullPath		2
#define		F_FileSize				3
#define		F_RecDate				4
#define		F_RecTime				5
#define		F_RecDuration			6
#define		F_RecTimeZone			7
#define		F_MakerID				8
#define		F_ModelCode				9
#define		F_RecSrc				10
#define		F_ChannelName			11
#define		F_ChannelNum			12
#define		F_ProgName				13
#define		F_ProgDetail			14
#define		F_ProgExtend			15
#define		F_ProgGenre				16
#define		F_ProgVideo				17
#define		F_ProgAudio				18

#define		S_NORMAL				0
#define		S_TAB					1
#define		S_SPACE					2
#define		S_CSV					3
#define		S_NEWLINE				4
#define		S_ITEMNAME				5

#define		DEFAULTTSFILEPOS		50
#define		MAXFLAGNUM				256

#define		CONVBUFSIZE				65536


// 構造体宣言

typedef struct {
	int32_t		argSrc;
	int32_t		argDest;
	int32_t		separator;
	int32_t		flags[MAXFLAGNUM + 1];
	bool		bNoControl;
	bool		bNoComma;
	bool		bDQuot;
	bool		bItemName;
	bool		bJsonOutput;
	bool		bDisplay;
	bool		bCharSize;
	bool		bIVS;
	bool		bUseBOM;
	int32_t		tsfilepos;
	int64_t		packet_limit;
} CopyParams;

typedef struct {
	int32_t		recyear;
	int32_t		recmonth;
	int32_t		recday;
	int32_t		rechour;
	int32_t		recmin;
	int32_t		recsec;
	int32_t		durhour;
	int32_t		durmin;
	int32_t		dursec;
	int32_t		rectimezone;
	int32_t		makerid;
	int32_t		modelcode;
	int32_t		recsrc;
	int32_t		chnum;
	__WCHAR		chname[CONVBUFSIZE];
	int32_t		chnamelen;
	__WCHAR		pname[CONVBUFSIZE];
	int32_t		pnamelen;
	__WCHAR		pdetail[CONVBUFSIZE];
	int32_t		pdetaillen;
	__WCHAR		pextend[CONVBUFSIZE];
	int32_t		pextendlen;
	int32_t		genre[3];
	int32_t		videoformat;
	int32_t		audioformat[8];
	int32_t		audiosamplingrate[8];
	CHAR		audiolang[8][8];
	__WCHAR		audiotext[8][32];
	int32_t		audiotextlen[8];
	_TCHAR		fullpath[_MAX_PATH];
	_TCHAR		fname[_MAX_PATH];
	_TCHAR		fext[_MAX_PATH];
	int64_t		fsize;
} ProgInfo;
