#include "core/tsdump_def.h"

#ifdef TSD_PLATFORM_MSVC
#include <Windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>

#include "utils/arib_proginfo.h"
#include "core/module_hooks.h"
#include "core/tsdump.h"
#include "utils/tsdstr.h"
#include "utils/path.h"

TSDCHAR param_base_dir[MAX_PATH_LEN] = {TSD_CHAR('\0')};

static void normalize_fname(TSDCHAR *fname, size_t fname_max)
{
	tsdstr_replace_set_t replace_sets[] = {
		{ TSD_TEXT("\\"), TSD_TEXT("��") },
		{ TSD_TEXT("/"), TSD_TEXT("�^") },
		{ TSD_TEXT("*"), TSD_TEXT("��") },
		{ TSD_TEXT("?"), TSD_TEXT("�H") },
		{ TSD_TEXT("\""), TSD_TEXT("�h") },
		{ TSD_TEXT("<"), TSD_TEXT("��") },
		{ TSD_TEXT(">"), TSD_TEXT("��") },
		{ TSD_TEXT("|"), TSD_TEXT("�b") },
		{ TSD_TEXT(":"), TSD_TEXT("�F") },
	};

	tsd_replace_sets(fname, fname_max, replace_sets, sizeof(replace_sets) / sizeof(tsdstr_replace_set_t), 0);
}

static void get_fname(TSDCHAR* fname, const proginfo_t *pi, const ch_info_t *ch_info, TSDCHAR *ext)
{
	int64_t tn;
	int i, isok = 0;
	const TSDCHAR *chname, *pname;
	time_mjd_t time_mjd;

	TSDCHAR pname_n[256], chname_n[256];
	TSDCHAR filename[MAX_PATH_LEN + 1];

	pname = TSD_TEXT("�ԑg���Ȃ�");

	if (PGINFO_READY(pi->status)) {
		tn = timenum_start(pi);
		chname = pi->service_name.str;
		pname = pi->event_name.str;
		isok = 1;
	} else {
		if ((pi->status&PGINFO_GET_SERVICE_INFO)) {
			chname = pi->service_name.str;
			isok = 1;
		} else {
			chname = ch_info->ch_str;
		}

		if (get_stream_timestamp_rough(pi, &time_mjd)) {
			tn = timenum_timemjd(time_mjd);
		} else {
			tn = timenumnow();
		}
	}

	tsd_strncpy(pname_n, pname, 256 - 1);
	tsd_strncpy(chname_n, chname, 256 - 1);

	normalize_fname(pname_n, 256-1);
	normalize_fname(chname_n, 256-1);

	/* tn�͔ԑg���̊J�n���� */
	if (!isok && ch_info->n_services > 1) {
		tsd_snprintf(filename, MAX_PATH_LEN - 1, TSD_TEXT("%"PRId64"_%s(sv=%d)_%s%s"), tn, chname_n, pi->service_id, pname_n, ext);
	} else {
		tsd_snprintf(filename, MAX_PATH_LEN - 1, TSD_TEXT("%"PRId64"_%s_%s%s"), tn, chname_n, pname_n, ext);
	}
	path_join(fname, param_base_dir, filename);
	if (!path_isexist(fname)) {
		goto END;
	}

	/* �t�@�C�������ɑ��݂�����tn�����ݎ����� */
	tn = timenumnow();
	if (!isok && ch_info->n_services > 1) {
		tsd_snprintf(filename, MAX_PATH_LEN - 1, TSD_TEXT("%"PRId64"_%s(sv=%d)_%s%s"), tn, chname_n, pi->service_id, pname_n, ext);
	} else {
		tsd_snprintf(filename, MAX_PATH_LEN - 1, TSD_TEXT("%"PRId64"_%s_%s%s"), tn, chname_n, pname_n, ext);
	}
	path_join(fname, param_base_dir, filename);
	if (!path_isexist(fname)) {
		goto END;
	}

	/* ����ł����݂�����suffix������ */
	for (i = 2;; i++) {
		if (!isok && ch_info->n_services > 1) {
			tsd_snprintf(filename, MAX_PATH_LEN - 1, TSD_TEXT("%"PRId64"_%s(sv=%d)_%s_%d%s"), tn, chname_n, pi->service_id, pname_n, i, ext);
		} else {
			tsd_snprintf(filename, MAX_PATH_LEN - 1, TSD_TEXT("%"PRId64"_%s_%s_%d%s"), tn, chname_n, pname_n, i, ext);
		}
		path_join(fname, param_base_dir, filename);
		if (!path_isexist(fname)) {
			goto END;
		}
	}

END:
	return;
}

static const TSDCHAR* hook_path_resolver(const proginfo_t *pi, const ch_info_t *ch_info)
{
	TSDCHAR *fname = (TSDCHAR*)malloc(sizeof(TSDCHAR)*MAX_PATH_LEN);
	get_fname(fname, pi, ch_info, TSD_TEXT(".ts"));
	return fname;
}

static const TSDCHAR* set_dir(const TSDCHAR *param)
{
	tsd_strncpy(param_base_dir, param, MAX_PATH_LEN);
	//PathAddBackslash(param_base_dir);
	return NULL;
}

static int hook_postconfig()
{
	if (param_base_dir[0] == L'\0') {
		output_message(MSG_ERROR, TSD_TEXT("�o�̓f�B���N�g�����w�肳��Ă��Ȃ����A�܂��͕s���ł�"));
		return 0;
	}
	return 1;
}

static cmd_def_t cmds[] = {
	{ TSD_TEXT("--dir"), TSD_TEXT("�o�͐�f�B���N�g�� *"), 1, set_dir },
	{ NULL },
};

static void register_hooks()
{
	register_hook_postconfig(hook_postconfig);
	register_hook_path_resolver(hook_path_resolver);
}

TSD_MODULE_DEF(
	mod_path_resolver,
	register_hooks,
	cmds,
	NULL
);