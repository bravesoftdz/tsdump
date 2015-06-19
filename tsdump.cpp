#define _CRT_SECURE_NO_WARNINGS

#pragma comment(lib, "shlwapi.lib")

#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <locale.h>
#include <signal.h>
#include <time.h>
#include <shlwapi.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <inttypes.h>

#include "modules_def.h"
#include "tsdump.h"
#include "ts_parser.h"
#include "ts_output.h"
#include "load_modules.h"

//#define HAVE_TIMECALC_DECLARATION
//#include "timecalc.h"

int BUFSIZE = BUFSIZE_DEFAULT * 1024 * 1024;
int OVERLAP_SEC = OVERLAP_SEC_DEFAULT;
int CHECK_INTERVAL = CHECK_INTERVAL_DEFAULT;
int MAX_PGOVERLAP = MAX_PGOVERLAP_DEFAULT;

int termflag = 0;

int param_sp_num = -1;
int param_ch_num = -1;
WCHAR param_base_dir[MAX_PATH_LEN];
int param_all_services;
int param_services[MAX_SERVICES];
int param_n_services = 0;
int param_nowait = 0;

void signal_handler(int)
{
	termflag = 1;
	output_message(MSG_NOTIFY, L"\n�I���V�O�i�����L���b�`");
	//printf("\n�I���V�O�i�����L���b�`\n");
}

FILE *logfp;

/*void open_log(FILE **fp)
{
	TCHAR fn[MAX_PATH_LEN];
	_stprintf_s(fn, MAX_PATH_LEN - 1, _T("%s%s.log"), param_base_dir, bon_ch_name);
	*fp = _tfopen(fn, _T("a+"));
}*/

void _output_message(const char *fname, message_type_t msgtype, const WCHAR *fmt, ...)
{
	va_list list;
	va_start(list, fmt);
	DWORD lasterr, *plasterr = NULL;
	WCHAR modname[128], msg[2048], *wcp;
	const char *cp;
	int len;

	vswprintf(msg, 2048 - 1, fmt, list);

	if (msgtype == MSG_SYSERROR) {
		lasterr = GetLastError();
		plasterr = &lasterr;
	}

	if (strcmp(fname, "mod_") == 0) {
		/* �g���q���������t�@�C����=���W���[�������R�s�[ */
		for (wcp = modname, cp = fname; *cp != '\0' && *cp != '.' && wcp < &modname[128]; cp += len, wcp++) {
			len = mbtowc(wcp, cp, MB_CUR_MAX);
		}
		do_message(modname, msgtype, plasterr, msg);
	} else {
		do_message(NULL, msgtype, plasterr, msg);
	}

	va_end(list);
}

void print_stat(ts_output_stat_t *tos, int n_tos, const WCHAR *stat)
{
	int n, i, j, backward_size, console_width, width, multiline;
	char line[256], hor[256];
	char *p = line;
	static int cnt = 0;
	HANDLE hc;
	CONSOLE_SCREEN_BUFFER_INFO ci;
	COORD new_pos;
	double rate;

	if(!tos) {
		return;
	}

	multiline = 0;
	hc = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hc != INVALID_HANDLE_VALUE) {
		if ( GetConsoleScreenBufferInfo(hc, &ci) != 0 ) {
			if (ci.dwCursorPosition.X != 0 || ci.dwCursorPosition.Y != 0) { /* WINE���Ƃ�����擾�ł��Ȃ�(0���Z�b�g�����) */
				console_width = ci.dwSize.X;
				new_pos.X = 0;
				new_pos.Y = ci.dwCursorPosition.Y;
				multiline = 1;
			}
		}
	}

	if (!multiline) {
		rate = 100.0 * tos->pos_filled / BUFSIZE;
		wprintf(L"%s buf:%.1f%% \r", stat, rate);
		return;
	}

	if (console_width >= 256) {
		console_width = 255;
	}

	width = ( console_width - 6 - (n_tos-1) ) / n_tos;

	for (i = 0; i < n_tos; i++) {
		for (backward_size = j = 0; j < tos[i].n_th; j++) {
			backward_size += tos[i].th[j].bytes;
		}

		for (n = 0; n < width; n++) {
			int pos = int( (double)BUFSIZE / width * (n+0.5) );
			if (pos < tos[i].pos_write) {
				*p = '-';
			} else if (pos < tos[i].pos_filled) {
				*p = '!';
			} else {
				*p = '_';
			}
			if (pos > tos[i].pos_filled - backward_size) {
				if (*p == '-') {
					*p = '+';
				} else if(*p == '/') {
					*p = '|';
				}
			}
			p++;
		}
		if (i != n_tos - 1) {
			*(p++) = ' ';
		}
	}
	*p = '\0';

	memset(hor, '-', console_width - 1);
	hor[console_width - 1] = '\0';

	wprintf(L"%S\n%s\n", hor, stat);
	wprintf(L"buf: %S", line);
	SetConsoleCursorPosition(hc, new_pos);
}

void main_loop(void *generator_stat, void *decoder_stat, int encrypted, ch_info_t *ch_info)
{
	BYTE *recvbuf, *decbuf;

	int n_recv, n_dec;

	int64_t total = 0;
	int64_t subtotal = 0;

	int64_t nowtime, lasttime;

	ts_output_stat_t *tos = NULL;
	int n_tos = 0;
	ts_parse_stat_t tps = {};

	WCHAR title[256];
	decoder_stats_t stats;

	lasttime = nowtime = gettime();

	double tdiff, Mbps=0.0;

	int i;
	int single_mode = 0;

	int pos;

	//open_log(&logfp);

	if ( !param_all_services && param_n_services == 0 ) {
		single_mode = 1;
	}

	//int gettscount = 0;

	do_open_stream();

	while ( !termflag ) {
		nowtime = gettime();

		do_stream_generator(generator_stat, &recvbuf, &n_recv);
		do_encrypted_stream(recvbuf, n_recv);

		do_stream_decoder(decoder_stat, &decbuf, &n_dec, recvbuf, n_recv);
		do_stream(decbuf, n_dec, encrypted);

		//tc_start("bufcopy");
		if ( single_mode ) { /* �P�ꏑ���o�����[�h */
			/* tos�𐶐� */
			if ( ! tos ) {
				n_tos = param_n_services = 1;
				tos = (ts_output_stat_t*)malloc(1 * sizeof(ts_output_stat_t));
				init_tos(tos);
			}

			/* �p�P�b�g���o�b�t�@�ɃR�s�[ */
			ts_copybuf(tos, decbuf, n_dec);
			ts_update_transfer_history(tos, nowtime, n_dec);

		} else {  /* �T�[�r�X���Ə����o�����[�h */
			/* pos_filled���R�s�[ */
			for (i = 0; i < n_tos; i++) {
				tos[i].pos_filled_old = tos[i].pos_filled;
			}

			/* �p�P�b�g������ */
			for (pos = 0; pos < (int)n_dec; pos += 188) {
				BYTE *packet = &decbuf[pos];

				/* PAT, PMT���擾 */
				parse_ts_packet(&tps, packet);

				/* tos�𐶐� */
				if ( ! tos && tps.payload_PAT.stat == PAYLOAD_STAT_FINISH ) {
					n_tos = param_n_services = create_tos_per_service(&tos, &tps);
				}

				/* �T�[�r�X���ƂɃp�P�b�g���o�b�t�@�ɃR�s�[ */
				for (i = 0; i < n_tos; i++) {
					copy_current_service_packet(&tos[i], &tps, packet);
				}
			}

			/* �R�s�[�����o�C�g���̗�����ۑ� */
			for (i = 0; i < n_tos; i++) {
				ts_update_transfer_history( &tos[i], nowtime, tos[i].pos_filled - tos[i].pos_filled_old );
			}
		}
		//tc_end();

		//tc_start("output");
		/* �o�b�t�@���o�� */
		/* 100��Ɉ��A�܂��͋�擾�������Ƃ������o�����s�� */
		/*if(gettscount > 100 || n_recv == 0) {
			for (i = 0; i < n_tos; i++) {
				ts_output(&tos[i], nowtime);
			}
			gettscount = 0;
		}*/
		for (i = 0; i < n_tos; i++) {
			//if ( tos[i].pos_filled - tos[i].pos_write > 1024*1024 ) {
				ts_output(&tos[i], nowtime, 0);
			//}
		}
		//tc_end();

		//fprintf(logfp, "%I64d n_recv=%d, n_dec=%d, filled=%d\n", nowtime, n_recv, n_dec, filled);

		//tc_start("proginfo");
		/* ����I�ɔԑg�����`�F�b�N */
		if ( nowtime / CHECK_INTERVAL != lasttime / CHECK_INTERVAL ) {
			tdiff = (double)(nowtime - lasttime) / 1000;
			Mbps = (double)subtotal * 8 / 1024 / 1024 / tdiff;



			do_stream_decoder_stats(decoder_stat, &stats);

			double siglevel = do_stream_generator_siglevel(generator_stat);

			_stprintf_s(title, 256, _T("%s:%s:%s|%.1fdb %.1fMbps D:%I64d S:%I64d %.1fGB"),
				ch_info->tuner_name, ch_info->sp_str, ch_info->ch_str, siglevel, Mbps,
				stats.n_dropped, stats.n_scrambled,
				(double)total / 1024 / 1024 / 1024 );
			SetConsoleTitle(title);

			lasttime = nowtime;
			subtotal = 0;

			/* �ԑg���̃`�F�b�N */
			for (i = 0; i < n_tos; i++) {
				if (tos[i].th[1].bytes > 0) { /* �O��interval�ŉ�����M�ł��ĂȂ����͔ԑg���̃`�F�b�N���p�X���� */
					ts_check_pi(&tos[i], nowtime, ch_info);
				}
			}

			//tc_start("printbuf");
			print_stat(tos, n_tos, title);
			//tc_end();

			/* ��ꂽ�o�C�g����\�� */
			for (i = 0; i < n_tos; i++) {
				if (tos[i].dropped_bytes > 0) {
					if ( tos[i].service_id != -1 ) {
						output_message(MSG_ERROR, L"[WARN] �o�b�t�@�t���̂��߃f�[�^�����܂���(�T�[�r�X%d, %d�o�C�g)", tos[i].service_id, tos[i].dropped_bytes);
					} else {
						output_message(MSG_ERROR, L"[WARN] �o�b�t�@�t���̂��߃f�[�^�����܂���(%d�o�C�g)", tos[i].dropped_bytes);
					}
					tos[i].dropped_bytes = 0;
				}
			}
		}
		//tc_end();

		//tc_start("minimize");
		/* minimize */
		for (i = 0; i < n_tos; i++) {
			ts_minimize_buf(&tos[i]);
		}
		//tc_end();

		subtotal += n_dec;
		total += n_dec;
	}

	int err;
	/* �I������ */
	//printf("�܂������o���Ă��Ȃ��o�b�t�@�������o�Ă��܂�\n");
	output_message(MSG_NOTIFY, L"�܂������o���Ă��Ȃ��o�b�t�@�������o�Ă��܂�");
	for (i = 0; i < n_tos; i++) {
		/* �܂������o���Ă��Ȃ��o�b�t�@�������o�� */
		err = ts_wait_pgoutput(&tos[i]);
		while (tos[i].pos_filled - tos[i].pos_write > 0 && !err) {
			ts_output(&tos[i], gettime(), 1);
			err = ts_wait_pgoutput(&tos[i]);
			print_stat(&tos[i], n_tos-i, L"");
		}
		close_tos(&tos[i]);
	}
	free(tos);

	do_close_stream();

	//tc_report_id(123);
}

void load_ini()
{
	int bufsize = GetPrivateProfileInt(_T("TSDUMP"), _T("BUFSIZE"), BUFSIZE_DEFAULT, _T(".\\tsdump.ini"));
	int overlap_sec = GetPrivateProfileInt(_T("TSDUMP"), _T("OVERLAP_SEC"), OVERLAP_SEC_DEFAULT, _T(".\\tsdump.ini"));
	int check_interval = GetPrivateProfileInt(_T("TSDUMP"), _T("CHECK_INTERVAL"), CHECK_INTERVAL_DEFAULT, _T(".\\tsdump.ini"));
	int max_pgoverlap = GetPrivateProfileInt(_T("TSDUMP"), _T("MAX_PGOVERLAP"), MAX_PGOVERLAP_DEFAULT, _T(".\\tsdump.ini"));

	BUFSIZE = bufsize * 1024 * 1024;
	OVERLAP_SEC = overlap_sec;
	CHECK_INTERVAL = check_interval;
	MAX_PGOVERLAP = max_pgoverlap;

	output_message(MSG_NONE, L"BUFSIZE: %dMiB\nOVERLAP_SEC: %ds\nCHECK_INTERVAL: %dms\nMAX_PGOVERLAP: %d\n",
		bufsize, OVERLAP_SEC, CHECK_INTERVAL, MAX_PGOVERLAP);
}

int wmain(int argc, WCHAR* argv[])
{
	int ret=0;
	ch_info_t ch_info;
	void *generator_stat = NULL;
	void *decoder_stat = NULL;
	int encrypted;
	//const WCHAR *err_msg;

	_tsetlocale(LC_ALL, _T("Japanese_Japan.932"));

	output_message(MSG_NONE, L"tsdump ver%S (%S)\n", VERSION_STR, DATE_STR);

	/* ini�t�@�C�������[�h */
	load_ini();

	/* ���W���[�������[�h */
	if (load_modules() < 0) {
		//fwprintf(stderr, L"[ERROR] ���W���[���̃��[�h���ɃG���[���������܂���!");
		output_message(MSG_ERROR, L"���W���[���̃��[�h���ɃG���[���������܂���!");
		ret = 1;
		goto END;
	}

	/* ���W���[���������� */
	if ( !init_modules(argc, argv) ) {
		//fwprintf(stderr, L"[ERROR] ���W���[���̏��������ɃG���[���������܂���!");
		output_message(MSG_ERROR, L"���W���[���̏��������ɃG���[���������܂���!");
		print_cmd_usage();
		ret = 1;
		goto END;
	}

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	if ( ! do_stream_generator_open(&generator_stat, &ch_info) ) {
		//fwprintf(stderr, L"�X�g���[���W�F�l���[�^���J���܂���ł���: %s\n", err_msg);
		output_message(MSG_ERROR, L"�X�g���[���W�F�l���[�^���J���܂���ł���");
		ret = 1;
		goto END;
	}

	if ( ! do_stream_decoder_open(&decoder_stat, &encrypted) ) {
		//fwprintf(stderr, L"�X�g���[���f�R�[�_���J���܂���ł���: %s\n", err_msg);
		output_message(MSG_ERROR, L"�X�g���[���f�R�[�_���J���܂���ł���");
		ret = 1;
		goto END1;
	}

	Sleep(500);

	/* �����̖{�� */
	main_loop(generator_stat, decoder_stat, encrypted, &ch_info);

	//printf("����I��\n");
	output_message(MSG_NONE, L"����I��");

	do_stream_decoder_close(decoder_stat);

END1:
	do_stream_generator_close(generator_stat);
	//fclose(fp);

END:
	do_close_module();

	free_modules();

	if( ret ) {
		//wprintf(L"�����L�[�������Ă�������");
		output_message(MSG_NOTIFY, L"\n�G���^�[�L�[�������ƏI�����܂�");
		getchar();
	}
	return ret;
}

static const WCHAR* set_dir(const WCHAR *param)
{
	wcsncpy(param_base_dir, param, MAX_PATH_LEN);
	PathAddBackslash(param_base_dir);
	return NULL;
}

static const WCHAR *set_nowait(const WCHAR *)
{
	param_nowait = 1;
	return NULL;
}

static const WCHAR* set_sv(const WCHAR *param)
{
	int sv;
	if (param_all_services) {
		return NULL;
	}
	if ( wcscmp(param, L"all") == 0 ) {
		param_all_services = 1;
	} else {
		if (param_n_services < MAX_SERVICES) {
			sv = _wtoi(param);
			if (sv <= 0 || sv > 65535) {
				return L"�T�[�r�X�ԍ����s���ł�";
			}
			param_services[param_n_services] = sv;
			param_n_services++;
		} else {
			return L"�w�肷��T�[�r�X�̐����������܂�\n";
		}
	}
	return NULL;
}

static int hook_postconfig()
{
	if ( ! param_base_dir ) {
		//return L"�o�̓f�B���N�g�����w�肳��Ă��Ȃ����A�܂��͕s���ł�";
		output_message(MSG_ERROR, L"�o�̓f�B���N�g�����w�肳��Ă��Ȃ����A�܂��͕s���ł�");
		return 0;
	}
	//return NULL;
	return 1;
}

void ghook_message(const WCHAR *modname, message_type_t msgtype, DWORD *err, const WCHAR *msg)
{
	const WCHAR *msgtype_str = L"";
	FILE *fp = stdout;
	WCHAR msgbuf[256];

	if (msgtype == MSG_WARNING) {
		msgtype_str = L"[WARNING] ";
		fp = stderr;
	} else if (msgtype == MSG_ERROR || msgtype == MSG_SYSERROR) {
		msgtype_str = L"[ERROR] ";
		fp = stderr;
	}

	if (msgtype == MSG_SYSERROR) {
		FormatMessage(
			/*FORMAT_MESSAGE_ALLOCATE_BUFFER |*/
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS |
			FORMAT_MESSAGE_MAX_WIDTH_MASK,
			NULL,
			*err,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			msgbuf,
			256,
			NULL
		);

		if (msgbuf[wcslen(msgbuf)-1] == L' ') {
			msgbuf[wcslen(msgbuf)-1] = L'\0';
		}

		if (modname) {
			fwprintf(fp, L"%s%s: %s <0x%x:%s>\n", msgtype_str, modname, msg, *err, msgbuf);
		} else {
			fwprintf(fp, L"%s%s <0x%x:%s>\n", msgtype_str, msg, *err, msgbuf);
		}
		//LocalFree(msgbuf);
	} else {
		if (modname) {
			fwprintf(fp, L"%s%s: %s\n", msgtype_str, modname, msg);
		} else {
			fwprintf(fp, L"%s%s\n", msgtype_str, msg);
		}
	}
}

static void register_hooks()
{
	register_hook_postconfig(hook_postconfig);
	//register_hook_message(hook_message);
}

static cmd_def_t cmds[] = {
	{ L"--dir", L"�o�͐�f�B���N�g�� *", 1, set_dir },
	{ L"--sv", L"�T�[�r�X�ԍ�(�����w��\)", 1, set_sv },
	{ L"--nowait", L"�o�b�t�@�t�����ɂ��ӂꂽ�f�[�^�͎̂Ă�", 0, set_nowait },
	NULL,
};

module_def_t mod_core = {
	TSDUMP_MODULE_V1,
	L"mod_core",
	register_hooks,
	cmds
};
