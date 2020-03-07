// rplsproginfo.cpp
//

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>

#include "rplsinfo.h"
#include "tsprocess.h"
#include "rplsproginfo.h"
#include "tsproginfo.h"
#include "convToUnicode.h"


// �萔�Ȃ�

#define		RPLSFILESIZE			(16 * 1024)


// �}�N����`

#define		printMsg(fmt, ...)		_tprintf(_T(fmt), __VA_ARGS__)
#define		printErrMsg(fmt, ...)	_tprintf(_T(fmt), __VA_ARGS__)


//

bool readFileProgInfo(_TCHAR *fname, ProgInfo* proginfo, const CopyParams* param)
{
	HANDLE	hFile = CreateFile(fname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		printErrMsg("�ԑg��񌳃t�@�C�� %s ���J���̂Ɏ��s���܂���.\n", fname);
		return	false;
	}

	const int32_t	srcfiletype = rplsTsCheck(hFile);

	if ((srcfiletype != FILE_188TS) && (srcfiletype != FILE_192TS) && (srcfiletype != FILE_RPLS)) {										// �����ȃt�@�C���̏ꍇ
		printErrMsg("�ԑg��񌳃t�@�C�� %s �͗L����TS, rpls�t�@�C���ł͂���܂���.\n", fname);
		CloseHandle(hFile);
		return	false;
	}

	// �ԑg���̓ǂݍ���

	_wfullpath(proginfo->fullpath, fname, _MAX_PATH);																	// �t���p�X���擾
	_wsplitpath_s(proginfo->fullpath, NULL, 0, NULL, 0, proginfo->fname, _MAX_PATH, proginfo->fext, _MAX_PATH);			// �x�[�X�t�@�C�����Ɗg���q
	
	proginfo->fsize = GetFileDataSize(hFile);																			// �t�@�C���T�C�Y�擾

	bool	bResult;

	if (srcfiletype == FILE_RPLS) {
		bResult = readRplsProgInfo(hFile, proginfo, param);
	}
	else {
		bResult = readTsProgInfo(hFile, proginfo, srcfiletype, param);
	}

	if (!bResult) {
		printErrMsg("�ԑg��񌳃t�@�C�� %s ����L���Ȕԑg�������o�ł��܂���ł���.\n", fname);
		CloseHandle(hFile);
		return	false;
	}

	CloseHandle(hFile);

	return	true;
}

int32_t rplsTsCheck(HANDLE hReadFile)
{
	uint32_t	numRead;
	uint8_t		buf[1024] = { 0 };

	SeekFileData(hReadFile, 0);
	ReadFileData(hReadFile, buf, 1024, &numRead);

	if ((buf[0] == 'P') && (buf[1] == 'L') && (buf[2] == 'S') && (buf[3] == 'T')) {														// rpls file
		return	FILE_RPLS;
	}
	else if ((buf[188 * 0] == 0x47) && (buf[188 * 1] == 0x47) && (buf[188 * 2] == 0x47) && (buf[188 * 3] == 0x47)) {						// 188 byte packet ts
		return	FILE_188TS;
	}
	else if ((buf[192 * 0 + 4] == 0x47) && (buf[192 * 1 + 4] == 0x47) && (buf[192 * 2 + 4] == 0x47) && (buf[192 * 3 + 4] == 0x47)) {		// 192 byte packet ts
		return	FILE_192TS;
	}

	return	FILE_INVALID;																													// unknown file
}


bool rplsMakerCheck(const uint8_t *buf, const int32_t idMaker)
{
	const uint32_t	mpadr = (buf[ADR_MPDATA] << 24) + (buf[ADR_MPDATA + 1] << 16) + (buf[ADR_MPDATA + 2] << 8) + buf[ADR_MPDATA + 3];
	if (mpadr == 0) return false;

	const uint8_t	*mpdata = buf + mpadr;
	const int32_t	makerid = (mpdata[ADR_MPMAKERID] << 8) + mpdata[ADR_MPMAKERID + 1];

	if (makerid != idMaker) return false;

	return true;
}


bool readRplsProgInfo(HANDLE hFile, ProgInfo *proginfo, const CopyParams *param)
{

	// �t�@�C���ǂݍ���

	uint8_t		buf[RPLSFILESIZE];
	uint32_t	numRead;

	SeekFileData(hFile, 0);
	ReadFileData(hFile, buf, RPLSFILESIZE, &numRead);

	// rpls���̎擾

	const bool	bSonyRpls = rplsMakerCheck(buf, MAKERID_SONY);														// ���[�J�[�`�F�b�N�Dsony�y��panasonic
	const bool	bPanaRpls = rplsMakerCheck(buf, MAKERID_PANASONIC);

	const uint8_t	*appinfo = buf + ADR_APPINFO;
	const uint8_t	*mpdata  = buf + (buf[ADR_MPDATA] << 24) + (buf[ADR_MPDATA + 1] << 16) + (buf[ADR_MPDATA + 2] << 8) + buf[ADR_MPDATA + 3];

	// �^�����

	proginfo->recyear	= (appinfo[ADR_RECYEAR]  >> 4) * 1000 + (appinfo[ADR_RECYEAR]  & 0x0F) * 100 + (appinfo[ADR_RECYEAR + 1] >> 4) * 10 + (appinfo[ADR_RECYEAR + 1] & 0x0F);
	proginfo->recmonth	= (appinfo[ADR_RECMONTH] >> 4) * 10   + (appinfo[ADR_RECMONTH] & 0x0F);
	proginfo->recday	= (appinfo[ADR_RECDAY]   >> 4) * 10   + (appinfo[ADR_RECDAY]   & 0x0F);
	proginfo->rechour	= (appinfo[ADR_RECHOUR]  >> 4) * 10   + (appinfo[ADR_RECHOUR]  & 0x0F);
	proginfo->recmin	= (appinfo[ADR_RECMIN]   >> 4) * 10   + (appinfo[ADR_RECMIN]   & 0x0F);
	proginfo->recsec	= (appinfo[ADR_RECSEC]   >> 4) * 10   + (appinfo[ADR_RECSEC]   & 0x0F);

	// �^�����

	proginfo->durhour	= (appinfo[ADR_DURHOUR] >> 4) * 10 + (appinfo[ADR_DURHOUR] & 0x0F);
	proginfo->durmin	= (appinfo[ADR_DURMIN]  >> 4) * 10 + (appinfo[ADR_DURMIN]  & 0x0F);
	proginfo->dursec	= (appinfo[ADR_DURSEC]  >> 4) * 10 + (appinfo[ADR_DURSEC]  & 0x0F);

	// �^�C���]�[��

	proginfo->rectimezone = appinfo[ADR_TIMEZONE];

	// ���[�J�[ID, ���[�J�[�@��R�[�h

	proginfo->makerid	= appinfo[ADR_MAKERID]   * 256 + appinfo[ADR_MAKERID + 1];
	proginfo->modelcode	= appinfo[ADR_MODELCODE] * 256 + appinfo[ADR_MODELCODE + 1];

	// ������ʏ��ipanasonic���R�����j

	proginfo->recsrc	= bPanaRpls ? mpdata[ADR_RECSRC_PANA] * 256 + mpdata[ADR_RECSRC_PANA + 1] : -1;				// ������ʏ����擾�D�p�i�ȊO�Ȃ������ʏ�񖳂�(-1)

	// �`�����l���ԍ�, �`�����l�����i�����ǖ��j

	proginfo->chnum		= appinfo[ADR_CHANNELNUM] * 256 + appinfo[ADR_CHANNELNUM + 1];
	proginfo->chnamelen = (int32_t)conv_to_unicode((char16_t*)proginfo->chname, CONVBUFSIZE, appinfo + ADR_CHANNELNAME + 1, (size_t)appinfo[ADR_CHANNELNAME], param->bCharSize, param->bIVS);

	// �ԑg��

	proginfo->pnamelen = (int32_t)conv_to_unicode((char16_t*)proginfo->pname, CONVBUFSIZE, appinfo + ADR_PNAME + 1, (size_t)appinfo[ADR_PNAME], param->bCharSize, param->bIVS);

	// �ԑg���e

	const size_t	pdetaillen = appinfo[ADR_PDETAIL] * 256 + appinfo[ADR_PDETAIL + 1];
	proginfo->pdetaillen = (int32_t)conv_to_unicode((char16_t*)proginfo->pdetail, CONVBUFSIZE, appinfo + ADR_PDETAIL + 2, pdetaillen, param->bCharSize, param->bIVS);

	if (bSonyRpls)		// sony���R�[�_�[�̏ꍇ
	{
		// �ԑg���e�ڍ�

		const int32_t	pextendlen = mpdata[ADR_PEXTENDLEN] * 256 + mpdata[ADR_PEXTENDLEN + 1];
		proginfo->pextendlen = (int32_t)conv_to_unicode((char16_t*)proginfo->pextend, CONVBUFSIZE, appinfo + ADR_PDETAIL + 2 + pdetaillen, pextendlen, param->bCharSize, param->bIVS);

		// �ԑg�W���������

		for (int32_t i = 0; i < 3; i++) proginfo->genre[i] = (mpdata[ADR_GENRE + i * 4 + 0] == 0x01) ? mpdata[ADR_GENRE + i * 4 + 1] : -1;
	}

	if (bPanaRpls)		// panasonic���R�[�_�[�̏ꍇ
	{
		proginfo->pextendlen = -1;														// �u�ԑg���e�ڍׁv��L���Ȃ�

		// �ԑg�W���������

		for (int32_t i = 0; i < 3; i++) proginfo->genre[i] = -1;

		switch (mpdata[ADR_GENRE_PANA])
		{
		case 0xD5:
			proginfo->genre[2] = ((mpdata[ADR_GENRE_PANA + 1] & 0x0F) << 4) + (mpdata[ADR_GENRE_PANA + 1] >> 4);
		case 0xE5:
			proginfo->genre[1] = ((mpdata[ADR_GENRE_PANA + 2] & 0x0F) << 4) + (mpdata[ADR_GENRE_PANA + 2] >> 4);
		case 0xE9:
			proginfo->genre[0] = ((mpdata[ADR_GENRE_PANA + 3] & 0x0F) << 4) + (mpdata[ADR_GENRE_PANA + 3] >> 4);
			break;
		default:
			break;
		}
	}

	if (!bSonyRpls && !bPanaRpls)		// sony, pana�ȊO
	{
		for (int32_t i = 0; i < 3; i++) proginfo->genre[i] = -1;						// �ԑg�W��������񖳂�
		proginfo->pextendlen = -1;														// �u�ԑg���e�ڍׁv��L���Ȃ�
	}

	return true;
}


int compareForRecSrcStr(const void *item1, const void *item2)
{
	return wcscmp(*(WCHAR**)item1, *(WCHAR**)item2);
}


size_t getRecSrcStr(WCHAR *dst, const size_t maxbufsize, const int32_t src)
{
	static const WCHAR	*nameList[] =
	{
		L"TD",		L"�n��f�W�^��",
		L"BD",		L"BS�f�W�^��",
		L"C1",		L"CS�f�W�^��1",
		L"C2",		L"CS�f�W�^��2",
		L"iL",		L"i.LINK(TS)",
		L"MV",		L"AVCHD",
		L"SK",		L"�X�J�p�[(DLNA)",
		L"DV",		L"DV����",
		L"TA",		L"�n��A�i���O",
		L"NL",		L"���C������"
	};

	static bool		bTableInitialized = false;

	if (!bTableInitialized)
	{
		qsort(nameList, sizeof(nameList) / sizeof(WCHAR*) / 2, sizeof(WCHAR*) * 2, compareForRecSrcStr);
		bTableInitialized = true;
	}

	static const WCHAR	*errNameList[] =
	{
		L"unknown",
		L"n/a"
	};

	const WCHAR	*srcStr = errNameList[1];

	if (src != -1)
	{
		WCHAR  key[3];
		key[0] = (WCHAR)((src >> 8) & 0x00FF);
		key[1] = (WCHAR)(src & 0x00FF);
		key[2] = 0;
		WCHAR *pkey = key;

		void *result = bsearch(&pkey, nameList, sizeof(nameList) / sizeof(WCHAR*) / 2, sizeof(WCHAR*) * 2, compareForRecSrcStr);

		if (result != NULL) {
			srcStr = *((WCHAR**)result + 1);
		}
		else {
			srcStr = errNameList[0];
		}
	}

	size_t i = 0;

	while (i < maxbufsize)
	{
		dst[i] = srcStr[i];
		if (srcStr[i++] == 0) break;
	}

	return i - 1;
}