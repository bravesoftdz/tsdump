#pragma comment(lib, "shlwapi.lib")

#include <windows.h>
#include <stdio.h>

#include <shlwapi.h>

#include "modules_def.h"

typedef struct{
	int used;
	HANDLE write_pipe;
	int write_busy;
	HANDLE child_process;
} pipestat_t;

#define MAX_PIPECMDS 32

static int n_pipecmds = 0;
static WCHAR *pipecmds[MAX_PIPECMDS];
static int pcwindow_min = 0;

static void create_pipe(pipestat_t *ps, const WCHAR *cmdname, const WCHAR *fname)
{
	HANDLE hRead, hWrite, hReadTemp;
	STARTUPINFO si = {};
	PROCESS_INFORMATION pi = {};

	WCHAR cmdarg[2048];

	ps->used = 0;

	if (!CreatePipe(&hReadTemp, &hWrite, NULL, 0)) {
		fprintf(stderr, "[ERROR] �p�C�v�̍쐬�Ɏ��s\n");
		return;
	}
	if (!DuplicateHandle(
		GetCurrentProcess(),
		hReadTemp,
		GetCurrentProcess(),
		&hRead,
		0,
		TRUE,
		DUPLICATE_SAME_ACCESS)) {
		fprintf(stderr, "[ERROR] DuplicateHandle()�Ɏ��s\n");
		CloseHandle(hWrite);
		CloseHandle(hReadTemp);
		return;
	}

	CloseHandle(hReadTemp);

	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdInput = hRead;
	si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

	if (pcwindow_min) {
		si.dwFlags |= STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_MINIMIZE;
	}

	swprintf(cmdarg, 2048-1, L"\"%s\" \"%s\"", cmdname, fname);

	wprintf(L"[PIPE] exec: %s\n", cmdarg);

	if (!CreateProcess(cmdname, cmdarg, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
		fprintf(stderr, "[ERROR] �q�v���Z�X�̐����Ɏ��s\n");
		CloseHandle(hWrite);
		CloseHandle(hRead);
		return;
	}

	ps->child_process = pi.hProcess;
	ps->write_pipe = hWrite;
	ps->used = 1;
	ps->write_busy = 0;

	CloseHandle(pi.hThread);
	CloseHandle(hRead);

	return;
}

static void *hook_pgoutput_create(const WCHAR *fname, const ProgInfo *, const ch_info_t*)
{
	int i;
	pipestat_t *ps = (pipestat_t*)malloc( sizeof(pipestat_t) * n_pipecmds );
	for (i = 0; i < n_pipecmds; i++) {
		create_pipe(&ps[i], pipecmds[i], fname);
	}
	return ps;
}

static void hook_pgoutput(void *pstat, const unsigned char *buf, const size_t size)
{
	DWORD written_total, written;
	BOOL ret;

	pipestat_t *ps = (pipestat_t*)pstat;
	int i;
	for (i = 0; i < n_pipecmds; i++) {
		if (!ps[i].used) {
			continue;
		}
		written_total = 0;
		while (written_total < (DWORD)size) {
			ret = WriteFile(ps[i].write_pipe, &buf[written_total], size - written_total, &written, NULL);
			if (!ret) {
				fprintf(stderr, "[ERROR] Broken pipe(close pipe)\n");
				CloseHandle(ps[i].write_pipe);
				CloseHandle(ps[i].child_process);
				ps[i].used = 0;
				break;
			}
			written_total += written;
		}
	}
}

static void hook_pgoutput_close(void *pstat, const ProgInfo *)
{
	pipestat_t *ps = (pipestat_t*)pstat;
	int i;
	for (i = 0; i < n_pipecmds; i++) {
		if (ps[i].used) {
			CloseHandle(ps[i].write_pipe);
		}
	}
	for (i = 0; i < n_pipecmds; i++) {
		if (ps[i].used) {
			WaitForSingleObject(ps[i].child_process, 500);
			CloseHandle(ps[i].child_process);
			ps[i].used = 0;
		}
	}
	free(ps);
}

static const WCHAR* set_pipe(const WCHAR *param)
{
	if (n_pipecmds >= MAX_PIPECMDS) {
		return L"�w�肷��p�C�v���s�R�}���h�̐����������܂�";
	}
	pipecmds[n_pipecmds] = _wcsdup(param);
	n_pipecmds++;
	return NULL;
}

static const WCHAR *set_min(const WCHAR*)
{
	pcwindow_min = 1;
	return NULL;
}

static void register_hooks()
{
	register_hook_pgoutput_create(hook_pgoutput_create);
	register_hook_pgoutput(hook_pgoutput);
	register_hook_pgoutput_close(hook_pgoutput_close);
}

static cmd_def_t cmds[] = {
	{ L"-pipe", L"�p�C�v���s�R�}���h (�����w���)", 1, set_pipe },
	{ L"-pwmin", L"�p�C�v���s�R�}���h�̃E�B���h�E���ŏ�������", 0, set_min },
	NULL,
};

MODULE_DEF module_def_t mod_pipeoutput_win = {
	TSDUMP_MODULE_V1,
	L"mod_pipeoutput_win",
	register_hooks,
	cmds
};