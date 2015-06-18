#include <Windows.h>
#include <stdio.h>
#include <inttypes.h>

#include "modules_def.h"
#include "IBonDriver2.h"

typedef struct {
	HMODULE hdll;
	pCreateBonDriver_t *pCreateBonDriver;
	IBonDriver *pBon;
	IBonDriver2 *pBon2;
	DWORD n_rem;
} bondriver_stat_t;

//static WCHAR errmsg[1024];

static const WCHAR *bon_dll_name = NULL;
static int sp_num = -1;
static int ch_num = -1;

//static const WCHAR *reg_hook_msg;
static int reg_hook = 0;

LPWSTR lasterr_msg()
{
	LPWSTR msg;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)(&msg),
		0,
		NULL
	);
	return msg;
}

static int hook_postconfig()
{
	if (bon_dll_name == NULL) {
		return 1;
	}

	//if (reg_hook_msg != NULL) {
	if (!reg_hook) {
		//_snwprintf_s(errmsg, 1024 - 1, L"generator�t�b�N�̓o�^�Ɏ��s���܂���: %s", reg_hook_msg);
		//return errmsg;
		output_message(MSG_ERROR, L"generator�t�b�N�̓o�^�Ɏ��s���܂���");
		return 0;
	}

	if (ch_num < 0) {
		//return L"�`�����l�����w�肳��Ă��Ȃ����A�܂��͕s���ł�";
		output_message(MSG_ERROR, L"�`�����l�����w�肳��Ă��Ȃ����A�܂��͕s���ł�");
		return 0;
	}
	if (sp_num < 0) {
		//return L"�`���[�i�[��Ԃ��w�肳��Ă��Ȃ����A�܂��͕s���ł�";
		output_message(MSG_ERROR, L"�`���[�i�[��Ԃ��w�肳��Ă��Ȃ����A�܂��͕s���ł�");
		return 0;
	}

	//return NULL;
	return 1;
}

static void hook_stream_generator(void *param, unsigned char **buf, int *size)
{
	DWORD n_recv;

	bondriver_stat_t *pstat = (bondriver_stat_t*)param;
	/* ��擾���\�������ꍇ�����҂��Ă݂� */
	if (pstat->n_rem == 0) {
		pstat->pBon2->WaitTsStream(100);
	}
	/* ts���`���[�i�[����擾 */
	pstat->pBon2->GetTsStream(buf, &n_recv, &pstat->n_rem);
	*size = n_recv;
}

static int hook_stream_generator_open(void **param, ch_info_t *chinfo)
{
	bondriver_stat_t *pstat, stat;
	ch_info_t ci;

	stat.hdll = LoadLibrary(bon_dll_name);
	if (stat.hdll == NULL) {
		//print_err(L"LoadLibrary", GetLastError());
		//return L"BonDriver�����[�h�ł��܂���ł���";
		output_message(MSG_SYSERROR, L"BonDriver�����[�h�ł��܂���ł���(LoadLibrary)");
		return 0;
	}

	stat.pCreateBonDriver = (pCreateBonDriver_t*)GetProcAddress(stat.hdll, "CreateBonDriver");
	if (stat.pCreateBonDriver == NULL) {
		//print_err(L"GetProcAddress", GetLastError());
		FreeLibrary(stat.hdll);
		//return L"CreateBonDriver()�̃|�C���^���擾�ł��܂���ł���";
		output_message(MSG_SYSERROR, L"CreateBonDriver()�̃|�C���^���擾�ł��܂���ł���(GetProcAddress)");
		return 0;
	}

	stat.pBon = stat.pCreateBonDriver();
	if (stat.pBon == NULL) {
		FreeLibrary(stat.hdll);
		//return L"CreateBonDriver() returns NULL";
		output_message(MSG_ERROR, L"CreateBonDriver()�Ɏ��s���܂���");
		return 0;
	}

	stat.pBon2 = dynamic_cast<IBonDriver2 *>(stat.pBon);

	if (! stat.pBon2->OpenTuner()) {
		FreeLibrary(stat.hdll);
		//return L"OpenTuner() returns FALSE";
		output_message(MSG_ERROR, L"OpenTuner()�Ɏ��s���܂���");
		return 0;
	}

	ci.ch_str = stat.pBon2->EnumChannelName(sp_num, ch_num);
	ci.sp_str = stat.pBon2->EnumTuningSpace(sp_num);
	ci.tuner_name = stat.pBon2->GetTunerName();
	ci.ch_num = ch_num;
	ci.sp_num = sp_num;

	wprintf(L"BonTuner: %s\n", ci.tuner_name);
	wprintf(L"Space: %s\n", ci.sp_str);
	wprintf(L"Channel: %s\n", ci.ch_str);
	if (!stat.pBon2->SetChannel(sp_num, ch_num)) {
		stat.pBon2->CloseTuner();
		FreeLibrary(stat.hdll);
		//return L"SetChannel() returns FALSE";
		output_message(MSG_ERROR, L"SetChannel()�Ɏ��s���܂���");
		return 0;
	}

	stat.n_rem = 1;

	*chinfo = ci;
	pstat = (bondriver_stat_t*)malloc(sizeof(bondriver_stat_t));
	*pstat = stat;
	*param = pstat;
	//return NULL;
	return 1;
}

static double hook_stream_generator_siglevel(void *param)
{
	bondriver_stat_t *pstat = (bondriver_stat_t*)param;
	return pstat->pBon2->GetSignalLevel();
}

static void hook_stream_generator_close(void *param)
{
	bondriver_stat_t *pstat = (bondriver_stat_t*)param;
	pstat->pBon2->CloseTuner();
	FreeLibrary(pstat->hdll);
}

static hooks_stream_generator_t hooks_stream_generator = {
	hook_stream_generator_open,
	hook_stream_generator,
	hook_stream_generator_siglevel,
	hook_stream_generator_close
};

static void register_hooks()
{
	if (bon_dll_name) {
		reg_hook = register_hooks_stream_generator(&hooks_stream_generator);
	}
	register_hook_postconfig(hook_postconfig);
}

static const WCHAR *set_bon(const WCHAR* param)
{
	bon_dll_name = _wcsdup(param);
	return NULL;
}

static const WCHAR* set_sp(const WCHAR *param)
{
	sp_num = _wtoi(param);
	if (sp_num < 0) {
		return L"�X�y�[�X�ԍ����s���ł�";
	}
	return NULL;
}

static const WCHAR* set_ch(const WCHAR *param)
{
	ch_num = _wtoi(param);
	if (ch_num < 0) {
		return L"�`�����l���ԍ����s���ł�";
	}
	return NULL;
}

static cmd_def_t cmds[] = {
	{ L"--bon", L"BonDriver��DLL*", 1, set_bon },
	{ L"--sp", L"�`���[�i�[��Ԕԍ�*", 1, set_sp },
	{ L"--ch", L"�`�����l���ԍ�*", 1, set_ch },
	NULL,
};

MODULE_DEF module_def_t mod_bondriver = {
	TSDUMP_MODULE_V2,
	L"mod_bondriver",
	register_hooks,
	cmds
};
