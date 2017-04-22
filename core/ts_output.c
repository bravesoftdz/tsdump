#include "core/tsdump_def.h"

#ifdef TSD_PLATFORM_MSVC
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <inttypes.h>
#include <assert.h>

#include "utils/arib_proginfo.h"
#include "core/module_hooks.h"
#include "utils/ts_parser.h"
#include "core/tsdump.h"
#include "utils/advanced_buffer.h"
#include "core/load_modules.h"
#include "core/ts_output.h"
#include "utils/tsdstr.h"

//#include "timecalc.h"

static void module_buffer_output(ab_buffer_t *gb, void *param, const uint8_t *buf, int size)
{
	UNREF_ARG(gb);
	output_status_per_module_t *status = (output_status_per_module_t*)param;
	if (status->module->hooks.hook_pgoutput) {
		status->module->hooks.hook_pgoutput(status->module_status, buf, size);
	}
}

static void module_buffer_notify_skip(ab_buffer_t *gb, void *param, int skip_bytes)
{
	UNREF_ARG(gb);
	UNREF_ARG(param);
	UNREF_ARG(skip_bytes);
}

static void module_buffer_close(ab_buffer_t *gb, void *param, const uint8_t *buf, int remain_size)
{
	UNREF_ARG(gb);
	UNREF_ARG(buf);
	UNREF_ARG(remain_size);
	output_status_per_module_t *status = (output_status_per_module_t*)param;

	assert(status->parent->refcount > 0);

	if (status->module->hooks.hook_pgoutput_close) {
		status->module->hooks.hook_pgoutput_close(status->module_status, &status->parent->final_pi);
	}
	if (status->module->hooks.hook_pgoutput_postclose) {
		status->module->hooks.hook_pgoutput_postclose(status->module_status);
	}
	status->parent->refcount--;
	if (status->parent->refcount <= 0) {
		status->parent->parent->n_pgos--;
	}
}

static int module_buffer_pre_output(ab_buffer_t *gb, void *param, int *acceptable_bytes)
{
	UNREF_ARG(gb);
	int busy = 0;
	output_status_per_module_t *status = (output_status_per_module_t*)param;
	if (status->module->hooks.hook_pgoutput_check) {
		busy = status->module->hooks.hook_pgoutput_check(status->module_status);
		*acceptable_bytes = 1024 * 1024;
	}
	return busy;
}

static const ab_downstream_handler_t module_buffer_handlers = {
	module_buffer_output,
	module_buffer_notify_skip,
	module_buffer_close,
	NULL,
	module_buffer_pre_output
};

static output_status_per_module_t *do_pgoutput_create(ab_buffer_t *buf, ab_history_t *history, const TSDCHAR *fname, const proginfo_t *pi, ch_info_t *ch_info, const int actually_start)
{
	int i, stream_id;
	output_status_per_module_t *output_status = (output_status_per_module_t*)malloc(sizeof(output_status_per_module_t)*n_modules);

	for ( i = 0; i < n_modules; i++ ) {
		if (modules[i].hooks.hook_pgoutput_create) {
			output_status[i].module_status = modules[i].hooks.hook_pgoutput_create(fname, pi, ch_info, actually_start);
		} else {
			output_status[i].module_status = NULL;
		}
		output_status[i].module = &modules[i];
		stream_id = ab_connect_downstream_history_backward(
			buf, &module_buffer_handlers, 188, modules[i].hooks.output_block_size, &output_status[i], history
		);
		assert(stream_id >= 0);
		output_status[i].downstream_id = stream_id;
	}
	return output_status;
}

int do_pgoutput_changed(output_status_per_module_t *status, const proginfo_t *old_pi, const proginfo_t *new_pi)
{
	int i;
	int err = 0;
	for (i = 0; i < n_modules; i++) {
		if (modules[i].hooks.hook_pgoutput_changed) {
			modules[i].hooks.hook_pgoutput_changed(status[i].module_status, old_pi, new_pi);
		}
	}
	return err;
}

void do_pgoutput_end(output_status_per_module_t *status, const proginfo_t *pi)
{
	int i;
	for (i = 0; i < n_modules; i++) {
		if (modules[i].hooks.hook_pgoutput_end) {
			modules[i].hooks.hook_pgoutput_end(status[i].module_status, pi);
		}
	}
}

void do_pgoutput_close(pgoutput_stat_t *pgos)
{
	int i;
	for (i = 0; i < n_modules; i++) {
		pgos->per_module_status[i].parent = pgos;
		ab_disconnect_downstream(&pgos->parent->buf, 
			pgos->per_module_status[i].downstream_id, 0);
	}
}

static inline int pi_endtime_unknown(const proginfo_t *pi)
{
	if(pi->status & PGINFO_UNKNOWN_STARTTIME || pi->status & PGINFO_UNKNOWN_DURATION) {
		return 1;
	}
	return 0;
}

void printpi(const proginfo_t *pi)
{
	time_mjd_t endtime;
	TSDCHAR et[4096];

	if (!PGINFO_READY(pi->status)) {
		if (pi->status & PGINFO_GET_SERVICE_INFO) {
			output_message(MSG_DISP, TSD_TEXT("<<< ------------ �ԑg��� ------------\n")
				TSD_TEXT("[�ԑg���Ȃ�]\n%s\n")
				TSD_TEXT("---------------------------------- >>>"), pi->service_name.str);
			return;
		} else {
			output_message(MSG_DISP, TSD_TEXT("<<< ------------ �ԑg��� ------------\n")
				TSD_TEXT("[�ԑg���Ȃ�]\n")
				TSD_TEXT("---------------------------------- >>>"));
			return;
		}
	}

	get_extended_text(et, sizeof(et) / sizeof(TSDCHAR), pi);

	if ( pi_endtime_unknown(pi) ) {
		output_message(MSG_DISP, TSD_TEXT("<<< ------------ �ԑg��� ------------\n")
			TSD_TEXT("[%d/%02d/%02d %02d:%02d:%02d�` +����]\n%s:%s\n�u%s�v\n")
			TSD_TEXT("---------------------------------- >>>"),
			pi->start.year, pi->start.mon, pi->start.day,
			pi->start.hour, pi->start.min, pi->start.sec,
			pi->service_name.str,
			pi->event_name.str,
			et
		);
	} else {
		time_add_offset(&endtime, &pi->start, &pi->dur);
		output_message(MSG_DISP, TSD_TEXT("<<< ------------ �ԑg��� ------------\n")
			TSD_TEXT("[%d/%02d/%02d %02d:%02d:%02d�`%02d:%02d:%02d]\n%s:%s\n�u%s�v\n")
			TSD_TEXT("---------------------------------- >>>"),
			pi->start.year, pi->start.mon, pi->start.day,
			pi->start.hour, pi->start.min, pi->start.sec,
			endtime.hour, endtime.min, endtime.sec,
			pi->service_name.str,
			pi->event_name.str,
			et
		);
	}
}

int create_tos_per_service(ts_output_stat_t **ptos, ts_service_list_t *service_list, ch_info_t *ch_info)
{
	int i, j, k;
	int n_tos;
	ts_output_stat_t *tos;

	if (ch_info->mode_all_services) {
		n_tos = service_list->n_services;
		tos = (ts_output_stat_t*)malloc(n_tos*sizeof(ts_output_stat_t));
		for (i = 0; i < n_tos; i++) {
			init_tos(&tos[i]);
			tos[i].tps_index = i;
			tos[i].proginfo = &service_list->proginfos[i];
		}
	} else {
		n_tos = 0;
		for (i = 0; i < ch_info->n_services; i++) {
			for (j = 0; j < service_list->n_services; j++) {
				if (ch_info->services[i] == service_list->proginfos[j].service_id) {
					n_tos++;
					break;
				}
			}
		}
		tos = (ts_output_stat_t*)malloc(n_tos*sizeof(ts_output_stat_t));
		k = 0;
		for (i = 0; i < ch_info->n_services; i++) {
			for (j = 0; j < service_list->n_services; j++) {
				if (ch_info->services[i] == service_list->proginfos[j].service_id) {
					init_tos(&tos[k]);
					tos[k].tps_index = j;
					tos[k].proginfo = &service_list->proginfos[j];
					k++;
				}
			}
		}
	}

	ch_info->services = (unsigned int*)malloc(n_tos*sizeof(unsigned int));
	for (i = 0; i < n_tos; i++) {
		ch_info->services[i] = tos[i].proginfo->service_id;
	}
	ch_info->n_services = n_tos;

	*ptos = tos;
	return n_tos;
}

void init_tos(ts_output_stat_t *tos)
{
	int i;

	tos->n_pgos = 0;
	tos->pgos = (pgoutput_stat_t*)malloc(MAX_PGOVERLAP * sizeof(pgoutput_stat_t));
	for (i = 0; i < MAX_PGOVERLAP; i++) {
		tos->pgos[i].fn = (TSDCHAR*)malloc(MAX_PATH_LEN*sizeof(TSDCHAR));
		tos->pgos[i].refcount = 0;
	}

	ab_init_buf(&tos->buf, BUFSIZE);
	ab_set_history(&tos->buf, &tos->buf_history, CHECK_INTERVAL, OVERLAP_SEC * 1000);
	tos->dropped_bytes = 0;

	init_proginfo(&tos->last_proginfo);
	tos->last_checkpi_time = gettime();
	tos->proginfo_retry_count = 0;
	tos->pcr_retry_count = 0;
	tos->last_bufminimize_time = gettime();
	tos->curr_pgos = NULL;

	return;
}

void close_tos(ts_output_stat_t *tos)
{
	int i, j, busy, remain_size;

	/* close all output */
	for (i = j = 0; i < tos->n_pgos; j++) {
		assert(j < MAX_PGOVERLAP);
		if (tos->pgos[j].refcount <= 0) {
			continue;
		}
		i++;

		if (0 < tos->pgos[j].closetime) {
			/* �}�[�W���^��t�F�[�Y�Ɉڂ��Ă�����ۑ�����Ă���final_pi���g�� */
			do_pgoutput_close(&tos->pgos[j]);
		} else {
			/* �����łȂ���΍ŐV��proginfo�� */
			do_pgoutput_close(&tos->pgos[j]);
		}
	}

	/* �����o��������ҋ@(10�b�܂�) */
	for (i = 0; i < 10; i++) {
		busy = 0;
		for (j = ab_first_downstream(&tos->buf);
				j >= 0;
				j = ab_next_downstream(&tos->buf, j)) {
			busy |= ab_get_downstream_status(&tos->buf, j, NULL, &remain_size);
			if (remain_size > 0) {
				busy = 1;
			}
		}
		if (!busy) {
			break;
		}

		ab_output_buf(&tos->buf);
#ifdef TSD_PLATFORM_MSVC
		Sleep(100);
#else
		usleep(100 * 1000);
#endif
	}

	ab_close_buf(&tos->buf);

	for (i = 0; i < MAX_PGOVERLAP; i++) {
		free((TSDCHAR*)tos->pgos[i].fn);
	}
	free(tos->pgos);

	//free(tos->th);
	//free(tos->buf);
	//free(tos);
}

void ts_output(ts_output_stat_t *tos, int64_t nowtime)
{
	int i, j;

	/* �t�@�C���o�͏I���^�C�~���O���`�F�b�N */
	for (i = j = 0; i < tos->n_pgos; j++) {
		assert(j < MAX_PGOVERLAP);
		if (tos->pgos[j].refcount <= 0) {
			continue;
		}
		i++;

		/* �o�͏I�����`�F�b�N */
		if (0 < tos->pgos[j].closetime && tos->pgos[j].closetime < nowtime) {
			if (!tos->pgos[j].close_flag) {
				tos->pgos[j].close_flag = 1;
				/* close�t���O��ݒ� */
				do_pgoutput_close(&tos->pgos[j]);
			}
		}
	}

	/* �o�b�t�@�؂�l�� */
	if ( nowtime - tos->last_bufminimize_time >= OVERLAP_SEC*1000/4 ) {
		/* (OVERLAP_SEC/4)�b�ȏ�o���Ă�����o�b�t�@�؂�l�߂����s���� */
		//ts_minimize_buf(tos);
		ab_clear_buf(&tos->buf, 0);
		tos->last_bufminimize_time = nowtime;
	} else if ( nowtime < tos->last_bufminimize_time ) {
		/* �����̊����߂�ɑΉ� */
		tos->last_bufminimize_time = nowtime;
	}

	/* �t�@�C�������o�� */
	ab_output_buf(&tos->buf);
}

void ts_check_extended_text(ts_output_stat_t *tos)
{
	//pgoutput_stat_t *current_pgos;
	TSDCHAR et[4096];

	if (!tos->curr_pgos) {
		return;
	}

	//current_pgos = &tos->pgos[tos->n_pgos - 1];
	if (!(tos->curr_pgos->initial_pi_status & PGINFO_GET_EXTEND_TEXT) &&
			(tos->proginfo->status & PGINFO_GET_EXTEND_TEXT)) {
		/* �g���`���C�x���g��񂪓r�����痈���ꍇ */
		get_extended_text(et, sizeof(et) / sizeof(TSDCHAR), tos->proginfo);
		output_message(MSG_DISP, TSD_TEXT("<<< ------------ �ǉ��ԑg��� ------------\n")
			TSD_TEXT("�u%s�v\n")
			TSD_TEXT("---------------------------------- >>>"),
			et
		);
	}
}

void check_stream_timeinfo(ts_output_stat_t *tos)
{
	uint64_t diff_prc;

	/* check_pi�̃C���^�[�o�����Ƃ�PCR,TOT�֘A�̃`�F�b�N�A�N���A���s�� */
	if ((tos->proginfo->status&PGINFO_TIMEINFO) == PGINFO_TIMEINFO) {

		/* 1�b���x�ȏ�PCR�̍X�V���Ȃ���Ζ����Ƃ��ăN���A���� */
		if (tos->proginfo->status & PGINFO_PCR_UPDATED) {
			tos->pcr_retry_count = 0;
		} else {
			if (tos->pcr_retry_count > (1000 / CHECK_INTERVAL)) {
				tos->proginfo->status &= ~PGINFO_VALID_PCR;
			}
			tos->pcr_retry_count++;
			return;
		}

		/* 120�b�ȏ�Â�TOT�͖����Ƃ��ăN���A����(�ʏ��30�b�ȉ��̊Ԋu�ő��o) */
		diff_prc = tos->proginfo->PCR_base - tos->proginfo->TOT_PCR;
		if (tos->proginfo->PCR_wraparounded) {
			diff_prc += PCR_BASE_MAX;
		}

		if (diff_prc > 120 * PCR_BASE_HZ) {
			tos->proginfo->status &= ~PGINFO_GET_TOT;
			tos->proginfo->status &= ~PGINFO_VALID_TOT_PCR;
		}
	} else {
		tos->pcr_retry_count = 0;
	}

	tos->proginfo->status &= ~PGINFO_PCR_UPDATED;
}

void ts_prog_changed(ts_output_stat_t *tos, int64_t nowtime, ch_info_t *ch_info)
{
	int i, j, actually_start = 0;
	time_mjd_t curr_time;
	time_offset_t offset;
	pgoutput_stat_t *pgos;
	proginfo_t *final_pi, tmp_pi;

	/* print */
	printpi(tos->proginfo);

	/* split! */
	if (tos->n_pgos >= MAX_PGOVERLAP) {
		output_message(MSG_WARNING, TSD_TEXT("�ԑg�̐؂�ւ���Z���ԂɘA�����Č��o�������߃X�L�b�v���܂�"));
	} else {
		/* �ԑg�I�����Ԃ��^�C���X�^���v���瓾�邩�ǂ��� */
		if ( !(tos->last_proginfo.status & PGINFO_UNKNOWN_STARTTIME) &&		/* �J�n���Ԃ����m���� */
				(tos->last_proginfo.status & PGINFO_UNKNOWN_DURATION) &&	/* �������������m���� */
				get_stream_timestamp(&tos->last_proginfo, &curr_time) ) {	/* �^�C���X�^���v�͐��� */
			get_time_offset(&offset, &curr_time, &tos->last_proginfo.start);
			tmp_pi = tos->last_proginfo;
			tmp_pi.dur = offset;
			tmp_pi.status &= ~PGINFO_UNKNOWN_DURATION;
			final_pi = &tmp_pi;
			output_message(MSG_NOTIFY, TSD_TEXT("�I������������̂܂ܔԑg���I�������̂Ō��݂̃^�C���X�^���v��ԑg�I�������ɂ��܂�"));
		} else {
			final_pi = &tos->last_proginfo;
		}

		/* end�t�b�N���Ăяo�� */
		for (i = j = 0; i < tos->n_pgos; j++) {
			assert(j < MAX_PGOVERLAP);
			if (tos->pgos[j].refcount > 0) {
				do_pgoutput_end(tos->pgos[j].per_module_status, final_pi);
				i++;
			}
		}

		/* �ǉ����ׂ�pgos�̃X���b�g��T�� */
		pgos = NULL;
		for (i = 0; i <= tos->n_pgos; i++) {
			if (tos->pgos[i].refcount == 0) {
				pgos = &tos->pgos[i];
				break;
			}
		}
		assert(pgos);

		/* ���łɘ^�撆�̔ԑg�����邩�ǂ��� */
		if (tos->curr_pgos) {
			actually_start = 1;
			tos->curr_pgos->closetime = nowtime + OVERLAP_SEC * 1000;
			tos->curr_pgos->final_pi = *final_pi;
		}

		pgos->initial_pi_status = tos->proginfo->status;
		pgos->fn = do_path_resolver(tos->proginfo, ch_info); /* ������ch_info�ɃA�N�Z�X */
		pgos->per_module_status = do_pgoutput_create(&tos->buf, &tos->buf_history, pgos->fn, tos->proginfo, ch_info, actually_start); /* ������ch_info�ɃA�N�Z�X */
		pgos->closetime = -1;
		pgos->close_flag = 0;
		pgos->close_remain = 0;
		pgos->refcount = n_modules;
		pgos->parent = tos;

		tos->curr_pgos = pgos;
		tos->n_pgos++;
	}
}

void ts_check_pi(ts_output_stat_t *tos, int64_t nowtime, ch_info_t *ch_info)
{
	int changed = 0, time_changed = 0;
	time_mjd_t endtime, last_endtime, time1, time2;
	TSDCHAR msg1[64], msg2[64];

	check_stream_timeinfo(tos);

	if ( !(tos->proginfo->status & PGINFO_READY_UPDATED) && 
			( (PGINFO_READY(tos->last_proginfo.status) && tos->curr_pgos) || !tos->curr_pgos) ) {
		/* �ŐV�̔ԑg��񂪎擾�ł��Ă��Ȃ��Ă�15�b�͔����ۗ����� */
		if (tos->proginfo_retry_count < 15 * 1000 / CHECK_INTERVAL) {
			tos->proginfo_retry_count++;
			return;
		}
		clear_proginfo_all(tos->proginfo);
	}

	tos->proginfo->status &= ~PGINFO_READY_UPDATED;
	tos->proginfo_retry_count = 0;

	if ( PGINFO_READY(tos->proginfo->status) ) {
		if ( PGINFO_READY(tos->last_proginfo.status) ) {
			/* �ԑg��񂠂聨����̏ꍇ�C�x���gID���r */
			if (tos->last_proginfo.event_id != (int)tos->proginfo->event_id) {
				changed = 1;
			}
		} else {
			/* �ԑg���Ȃ�������̕ω� */
			changed = 1;
		}
	} else {
		if (PGINFO_READY(tos->last_proginfo.status)) {
			/* �ԑg��񂠂聨�Ȃ��̕ω� */
			changed = 1;
		/* �ԑg��񂪂Ȃ��Ă�1���Ԃ����ɔԑg��؂�ւ��� */
		} else if( get_stream_timestamp_rough(tos->proginfo, &time1) &&
				get_stream_timestamp_rough(&tos->last_proginfo, &time2) ) {
			/* �X�g���[���̃^�C���X�^���v������Ɏ擾�ł��Ă���΂�����r */
			if (time1.hour > time2.hour) {
				changed = 1;
			}
		} else if( timenum64(nowtime) / 100 > timenum64(tos->last_checkpi_time) / 100 ) {
			/* �����łȂ����PC�̌��ݎ������r */
			changed = 1;
		} else if (!tos->curr_pgos) {
			/* �܂��o�͂��n�܂��Ă��Ȃ������狭���J�n */
			changed = 1;
		}
	}

	if (changed) {
		/* �ԑg���؂�ւ���� */
		ts_prog_changed(tos, nowtime, ch_info);
	} else if( PGINFO_READY(tos->proginfo->status) && PGINFO_READY(tos->last_proginfo.status) ) {
		
		if( proginfo_cmp(tos->proginfo, &tos->last_proginfo) ) {
			if (tos->curr_pgos) {
				/* �Ăяo���͍̂ŐV�̔ԑg�ɑ΂��Ă̂� */
				do_pgoutput_changed(tos->curr_pgos->per_module_status, &tos->last_proginfo, tos->proginfo);
			}

			/* �ԑg�̎��Ԃ��r���ŕύX���ꂽ�ꍇ */
			time_add_offset(&endtime, &tos->proginfo->start, &tos->proginfo->dur);
			time_add_offset(&last_endtime, &tos->last_proginfo.start, &tos->last_proginfo.dur);

			if ( get_time_offset(NULL, &tos->proginfo->start, &tos->last_proginfo.start) != 0 ) {
				tsd_snprintf(msg1, 64, TSD_TEXT("�ԑg�J�n���Ԃ̕ύX(�T�[�r�X%d)"), tos->proginfo->service_id);

				time_changed = 1;
				output_message( MSG_NOTIFY, TSD_TEXT("%s: %02d:%02d:%02d �� %02d:%02d:%02d"), msg1,
					tos->proginfo->start.hour, tos->proginfo->start.min, tos->proginfo->start.sec,
					tos->last_proginfo.start.hour, tos->last_proginfo.start.min, tos->last_proginfo.start.sec );
			}
			if ( get_time_offset(NULL, &endtime, &last_endtime) != 0 ) {
				tsd_snprintf(msg1, 64, TSD_TEXT("�ԑg�I�����Ԃ̕ύX(�T�[�r�X%d)"), tos->proginfo->service_id);

				if ( pi_endtime_unknown(&tos->last_proginfo) ) {
					tsd_strcpy(msg2, TSD_TEXT("���� ��"));
				} else {
					tsd_snprintf( msg2, 64, TSD_TEXT("%02d:%02d:%02d ��"), last_endtime.hour, last_endtime.min, last_endtime.sec );
				}

				time_changed = 1;
				if ( pi_endtime_unknown(tos->proginfo) ) {
					output_message(MSG_NOTIFY, TSD_TEXT("%s: %s ����"), msg1, msg2);
				} else {
					output_message( MSG_NOTIFY, TSD_TEXT("%s: %s %02d:%02d:%02d"), msg1, msg2,
						endtime.hour, endtime.min, endtime.sec );
				}
			}

			if (!time_changed) {
				output_message(MSG_NOTIFY, TSD_TEXT("�ԑg���̕ύX"));
				printpi(tos->proginfo);
			}
		}

	}

	tos->last_proginfo = *tos->proginfo;
	tos->last_checkpi_time = nowtime;
}
