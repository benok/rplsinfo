#pragma once

#include <stdint.h>

#ifdef __linux__
#include <wchar.h>
typedef char _TCHAR;
typedef char16_t WCHAR;
typedef int HANDLE;
#endif

#include "rplsinfo.h"


// 定数

#define		MEDIATYPE_TB		0x5442
#define		MEDIATYPE_BS		0x4253
#define		MEDIATYPE_CS		0x4353
#define		MEDIATYPE_UNKNOWN	0xFFFF

#define		PID_PAT				0x0000
#define		PID_SIT				0x001f
#define		PID_EIT				0x0012
#define		PID_NIT				0x0010
#define		PID_SDT				0x0011
#define		PID_BIT				0x0024
#define		PID_INVALID			0xffff

#define		PSIBUFSIZE			65536

// #define		MAXDESCSIZE			258
// #define		MAXEXTEVENTSIZE		4096

//




// プロトタイプ宣言

bool			readTsProgInfo(HANDLE, ProgInfo *, const int32_t, const CopyParams*);

int32_t			getSitEit(HANDLE, uint8_t*, const int32_t, const int32_t, const int64_t);
bool			getSdt(HANDLE, uint8_t *, const int32_t, const int32_t, const int32_t, const int64_t);
void			mjd_dec(const int32_t, int32_t*, int32_t*, int32_t*);
int32_t			mjd_enc(const int32_t, const int32_t, const int32_t);
int				comparefornidtable(const void*, const void*);
int32_t			getTbChannelNum(const int32_t, const int32_t, int32_t);

void			parseSit(const uint8_t*, ProgInfo*, const CopyParams*);
int32_t			parseEit(const uint8_t*, ProgInfo*, const CopyParams*);
void			parseSdt(const uint8_t*, ProgInfo*, const int32_t, const CopyParams*);

#ifndef USE_UTF16
#define WCHAR char
#ifndef swprintf_s
# define swprintf_s sprintf
#endif
#endif

size_t			putGenreStr(WCHAR *, const size_t, const int32_t *, bool);
size_t			putFormatStr(WCHAR *buf, const size_t bufsize, const int32_t format);
size_t			putSamplingrateStr(WCHAR *, const size_t, const int32_t);

#ifndef USE_UTF16
#undef WCHAR
#undef swprintf_s
#endif
