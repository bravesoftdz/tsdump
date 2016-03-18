#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <new.h>
#include <locale.h>
#include <tchar.h>
#include <inttypes.h>

#include "ts_parser.h"
#include "modules_def.h"

#include "rplsinfo.h"
#include "proginfo.h"
#include "convtounicode.h"

#include <conio.h>


// �萔�Ȃ�

#define		MAXFLAGNUM				256
#define		FILE_INVALID			0
#define		FILE_RPLS				1
#define		FILE_188TS				188
#define		FILE_192TS				192
#define		DEFAULTTSFILEPOS		50

#define		NAMESTRING				L"\nrplsinfo version 1.3 "

#ifdef _WIN64
	#define		NAMESTRING2				L"(64bit)\n"
#else
	#define		NAMESTRING2				L"(32bit)\n"
#endif


// �\���̐錾

typedef struct {
	int		argSrc;
	int		argDest;
	int		separator;
	int		flags[MAXFLAGNUM + 1];
	BOOL	bNoControl;
	BOOL	bNoComma;
	BOOL	bDQuot;
	BOOL	bItemName;
	BOOL	bDisplay;
	BOOL	bCharSize;
	int		tsfilepos;
} CopyParams;


// �v���g�^�C�v�錾

void	initCopyParams(CopyParams*);
BOOL	parseCopyParams(int, _TCHAR*[], CopyParams*);
int		rplsTsCheck(BYTE*);
BOOL	rplsMakerCheck(unsigned char*, int);
void	readRplsProgInfo(HANDLE, ProgInfo*, BOOL);
void	outputProgInfo(HANDLE, ProgInfo*, CopyParams*);
//int		putGenreStr(WCHAR*, int, int*, int*);
int		getRecSrcIndex(int);
int		putRecSrcStr(WCHAR*, int, int);
int		convforcsv(WCHAR*, int, WCHAR*, int, BOOL, BOOL, BOOL, BOOL);


// ���C��

int get_proginfo( ProgInfo *proginfo, BYTE *buf, int size )
{
	int i;
	BYTE *buf_t;
	int	sfiletype;

	proginfo->isok = FALSE;

	if (size < 188 * 4) {
		return 0;
	}

	/* buf��188�o�C�g�ɃA���C�������g����Ȃ�����(��:BonDriver_Friio)�ɑΉ� */
	for( i=0; i<188; i++ ) {
		sfiletype = rplsTsCheck(&buf[i]);
		if(sfiletype == FILE_188TS) {
			break;
		}
	}
	if(sfiletype != FILE_188TS) {
		//fprintf(stderr, "\n�����ts�X�g���[���ł͂���܂���\n");
		output_message(MSG_ERROR, L"�����ts�X�g���[���ł͂���܂���");
		return 0;
	}

	buf_t = &buf[i];

	// �ԑg���̓ǂݍ���
	memset(proginfo, 0, sizeof(ProgInfo));
	BOOL bResult = readTsProgInfo( buf_t, size-i, proginfo, sfiletype, 1 );
	if(!bResult) {
		//wprintf(L"�L���Ȕԑg�������o�ł��܂���ł���.\n");
		return 0;
	}

	proginfo->isok = TRUE;
	return 1;
}

int rplsTsCheck( BYTE *buf )
{
	if( (buf[188 * 0] == 0x47) && (buf[188 * 1] == 0x47) && (buf[188 * 2] == 0x47) && (buf[188 * 3] == 0x47) ){
		return	FILE_188TS;
	}
	return	FILE_INVALID;
}


BOOL rplsMakerCheck(unsigned char *buf, int idMaker)
{
	unsigned int	mpadr = (buf[ADR_MPDATA] << 24) + (buf[ADR_MPDATA + 1] << 16) + (buf[ADR_MPDATA + 2] << 8) + buf[ADR_MPDATA + 3];
	if(mpadr == 0) return FALSE;

	unsigned char	*mpdata = buf + mpadr;
	int		makerid = (mpdata[ADR_MPMAKERID] << 8) + mpdata[ADR_MPMAKERID + 1];

	if(makerid != idMaker) return FALSE;

	return TRUE;
}

int putGenreStr(WCHAR *buf, const int bufsize, const int *genretype, const int *genre)
{
	WCHAR	*str_genreL[] = {
		L"�j���[�X�^��",			L"�X�|�[�c",	L"���^���C�h�V���[",	L"�h���}",
		L"���y",					L"�o���G�e�B",	L"�f��",				L"�A�j���^���B",
		L"�h�L�������^���[�^���{",	L"����^����",	L"��^����",			L"����",
		L"�\��",					L"�\��",		L"�g��",				L"���̑�"
	};

	WCHAR	*str_genreM[] = {
		L"�莞�E����", L"�V�C", L"���W�E�h�L�������g", L"�����E����", L"�o�ρE�s��", L"�C�O�E����", L"���", L"���_�E��k",
		L"�񓹓���", L"���[�J���E�n��", L"���", L"-", L"-", L"-", L"-", L"���̑�",

		L"�X�|�[�c�j���[�X", L"�싅", L"�T�b�J�[", L"�S���t", L"���̑��̋��Z", L"���o�E�i���Z", L"�I�����s�b�N�E���ۑ��", L"�}���\���E����E���j",
		L"���[�^�[�X�|�[�c", L"�}�����E�E�B���^�[�X�|�[�c", L"���n�E���c���Z", L"-", L"-", L"-", L"-", L"���̑�",

		L"�|�\�E���C�h�V���[", L"�t�@�b�V����", L"��炵�E�Z�܂�", L"���N�E���", L"�V���b�s���O�E�ʔ�", L"�O�����E����", L"�C�x���g", L"�ԑg�Љ�E���m�点",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

		L"�����h���}", L"�C�O�h���}", L"���㌀", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

		L"�������b�N�E�|�b�v�X", L"�C�O���b�N�E�|�b�v�X", L"�N���V�b�N�E�I�y��", L"�W���Y�E�t���[�W����", L"�̗w�ȁE����", L"���C�u�E�R���T�[�g", L"�����L���O�E���N�G�X�g", L"�J���I�P�E�̂ǎ���",
		L"���w�E�M�y", L"���w�E�L�b�Y", L"�������y�E���[���h�~���[�W�b�N", L"-", L"-", L"-", L"-", L"���̑�",

		L"�N�C�Y", L"�Q�[��", L"�g�[�N�o���G�e�B", L"���΂��E�R���f�B", L"���y�o���G�e�B", L"���o���G�e�B", L"�����o���G�e�B", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

		L"�m��", L"�M��", L"�A�j��", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

		L"�����A�j��", L"�C�O�A�j��", L"���B", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

		L"�Љ�E����", L"���j�E�I�s", L"���R�E�����E��", L"�F���E�Ȋw�E��w", L"�J���`���[�E�`���|�\", L"���w�E���|", L"�X�|�[�c", L"�h�L�������^���[�S��",
		L"�C���^�r���[�E���_", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

		L"���㌀�E�V��", L"�~���[�W�J��", L"�_���X�E�o���G", L"����E���|", L"�̕���E�ÓT", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

		L"���E�ނ�E�A�E�g�h�A", L"���|�E�y�b�g�E��|", L"���y�E���p�E�H�|", L"�͌�E����", L"�����E�p�`���R", L"�ԁE�I�[�g�o�C", L"�R���s���[�^�E�s�u�Q�[��", L"��b�E��w",
		L"�c���E���w��", L"���w���E���Z��", L"��w���E��", L"���U����E���i", L"������", L"-", L"-", L"���̑�",

		L"�����", L"��Q��", L"�Љ��", L"�{�����e�B�A", L"��b", L"�����i�����j", L"�������", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",

		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",

		L"BS/�n��f�W�^�������p�ԑg�t�����", L"�L�ш�CS�f�W�^�������p�g��", L"�q���f�W�^�����������p�g��", L"�T�[�o�[�^�ԑg�t�����", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",

		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�"
	};

	int len;

	if(genretype[2] == 0x01) {
		len =  swprintf_s(buf, (size_t)bufsize, L"%s �k%s�l�@%s �k%s�l�@%s �k%s�l", str_genreL[genre[0] >> 4], str_genreM[genre[0]], str_genreL[genre[1] >> 4], str_genreM[genre[1]], str_genreL[genre[2] >> 4], str_genreM[genre[2]]);
	} else if(genretype[1] == 0x01) {
		len =  swprintf_s(buf, (size_t)bufsize, L"%s �k%s�l�@%s �k%s�l", str_genreL[genre[0] >> 4], str_genreM[genre[0]], str_genreL[genre[1] >> 4], str_genreM[genre[1]]);
	} else {
		len =  swprintf_s(buf, (size_t)bufsize, L"%s �k%s�l", str_genreL[genre[0] >> 4],  str_genreM[genre[0]]);
	}

	return len;
}


int getRecSrcIndex(int num)
{
	int		recsrc_table[] = {		// ������putRecSrcStr�ƑΉ�����K�v����
		0x5444,						// TD	�n��f�W�^��
		0x4244,						// BD	BS�f�W�^��
		0x4331,						// C1	CS�f�W�^��1
		0x4332,						// C2	CS�f�W�^��2
		0x694C,						// iL	i.LINK(TS)����
		0x4D56,						// MV	AVCHD
		0x534B,						// SK	�X�J�p�[(DLNA)
		0x4456,						// DV	DV����
		0x5441,						// TA	�n��A�i���O
		0x4E4C,						// NL	���C������
	};

	// �����ȃe�[�u���Ȃ̂ŏ����T����

	for(int i = 0; i < (sizeof(recsrc_table) / sizeof(int)); i++) {
		if(num == recsrc_table[i]) return i;
	}

	return	(sizeof(recsrc_table) / sizeof(int));			// �s����recsrc
}


int putRecSrcStr(WCHAR *buf, int bufsize, int index)
{
	WCHAR	*str_recsrc[] = {
		L"�n��f�W�^��",
		L"BS�f�W�^��",
		L"CS�f�W�^��1",
		L"CS�f�W�^��2",
		L"i.LINK(TS)",
		L"AVCHD",
		L"�X�J�p�[(DLNA)",
		L"DV����",
		L"�n��A�i���O",
		L"���C������",
		L"unknown",
	};

	int	len = 0;

	if( (index >= 0) && (index < (sizeof(str_recsrc) / sizeof(WCHAR*))) ) {
		len = swprintf_s(buf, (size_t)bufsize, L"%s", str_recsrc[index]);
	}

	return len;
}


int convforcsv(WCHAR *dbuf, int bufsize, WCHAR *sbuf, int slen, BOOL bNoControl, BOOL bNoComma, BOOL bDQuot, BOOL bDisplay)
{
	int dst = 0;

	if( bDQuot && (dst < bufsize) ) dbuf[dst++] = 0x0022;		//  �u"�v						// CSV�p�o�͂Ȃ獀�ڂ̑O����u"�v�ň͂�

	for(int	src = 0; src < slen; src++)
	{
		WCHAR	s = sbuf[src];
		BOOL	bOut = TRUE;

		if( bNoControl && (s < L' ') ) bOut = FALSE;											// ����R�[�h�͏o�͂��Ȃ�
		if( bNoComma && (s == L',') ) bOut = FALSE;												// �R���}�͏o�͂��Ȃ�
		if( bDisplay && (s == 0x000D) ) bOut = FALSE;											// �R���\�[���o�͂̏ꍇ�͉��s�R�[�h��0x000D�͏ȗ�����

		if( bDQuot && (s == 0x0022) && (dst < bufsize) ) dbuf[dst++] = 0x0022;					// CSV�p�o�͂Ȃ�u"�v�̑O�Ɂu"�v�ŃG�X�P�[�v
		if( bOut && (dst < bufsize) ) dbuf[dst++] = s;											// �o��
	}

	if( bDQuot && (dst < bufsize) ) dbuf[dst++] = 0x0022;		//  �u"�v

	if(dst < bufsize) dbuf[dst] = 0x0000;

	return dst;
}