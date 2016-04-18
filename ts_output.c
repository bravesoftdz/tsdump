#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <inttypes.h>

#include "module_def.h"
#include "ts_proginfo.h"
#include "module_hooks.h"
#include "ts_parser.h"
#include "tsdump.h"
#include "ts_output.h"
#include "load_modules.h"
#include "strfuncs.h"

//#include "timecalc.h"

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
	WCHAR et[4096];

	if (!PGINFO_READY(pi->status)) {
		if (pi->status & PGINFO_GET_SERVICE_INFO) {
			output_message(MSG_DISP, L"<<< ------------ �ԑg��� ------------\n"
				L"[�ԑg���Ȃ�]\n%s\n"
				L"---------------------------------- >>>", pi->service_name.str);
			return;
		} else {
			output_message(MSG_DISP, L"<<< ------------ �ԑg��� ------------\n"
				L"[�ԑg���Ȃ�]\n"
				L"---------------------------------- >>>");
			return;
		}
	}

	get_extended_text(et, sizeof(et) / sizeof(WCHAR), pi);

	if ( pi_endtime_unknown(pi) ) {
		output_message(MSG_DISP, L"<<< ------------ �ԑg��� ------------\n"
			L"[%d/%02d/%02d %02d:%02d:%02d�` +����]\n%s:%s\n�u%s�v\n"
			L"---------------------------------- >>>",
			pi->start.year, pi->start.mon, pi->start.day,
			pi->start.hour, pi->start.min, pi->start.sec,
			pi->service_name.str,
			pi->event_name.str,
			et
		);
	} else {
		time_add_offset(&endtime, &pi->start, &pi->dur);
		output_message(MSG_DISP, L"<<< ------------ �ԑg��� ------------\n"
			L"[%d/%02d/%02d %02d:%02d:%02d�`%02d:%02d:%02d]\n%s:%s\n�u%s�v\n"
			L"---------------------------------- >>>",
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
		tos->pgos[i].fn = (WCHAR*)malloc(MAX_PATH_LEN*sizeof(WCHAR));
	}

	tos->buf = (BYTE*)malloc(BUFSIZE);
	tos->pos_filled = 0;
	tos->pos_filled_old = 0;
	tos->pos_write = 0;
	tos->write_busy = 0;
	tos->dropped_bytes = 0;

	init_proginfo(&tos->last_proginfo);
	tos->last_checkpi_time = gettime();
	tos->proginfo_retry_count = 0;
	tos->pcr_retry_count = 0;
	tos->last_bufminimize_time = gettime();

	tos->n_th = OVERLAP_SEC * 1000 / CHECK_INTERVAL + 1;
	if (tos->n_th < 2) {
		tos->n_th = 2; /* tos->th[1]�ւ̃A�N�Z�X����̂ōŒ�ł�2 */
	}
	tos->th = (transfer_history_t*)malloc(tos->n_th * sizeof(transfer_history_t));
	memset(tos->th, 0, tos->n_th * sizeof(transfer_history_t));

	return;
}

void close_tos(ts_output_stat_t *tos)
{
	int i;

	/* close all output */
	for (i = 0; i < tos->n_pgos; i++) {
		if (0 < tos->pgos[i].closetime) {
			/* �}�[�W���^��t�F�[�Y�Ɉڂ��Ă�����ۑ�����Ă���final_pi���g�� */
			do_pgoutput_close(tos->pgos[i].modulestats, &tos->pgos[i].final_pi);
		} else {
			/* �����łȂ���΍ŐV��proginfo�� */
			do_pgoutput_close(tos->pgos[i].modulestats, tos->proginfo);
		}
	}

	for (i = 0; i < MAX_PGOVERLAP; i++) {
		free((WCHAR*)tos->pgos[i].fn);
	}
	free(tos->pgos);

	free(tos->th);
	free(tos->buf);
	//free(tos);
}

void ts_copy_backward(ts_output_stat_t *tos, int64_t nowtime)
{
	int backward_size, start_pos;
	int i;

	backward_size = 0;
	for (i = 0; i < tos->n_th; i++) {
		if (tos->th[i].time < nowtime - OVERLAP_SEC * 1000) {
			break;
		}
		backward_size += tos->th[i].bytes;
	}
	//backward_size = ((backward_size - 1) / 188 + 1) * 188; /* 188byte units */

	start_pos = tos->pos_filled - backward_size;
	if (start_pos < 0) {
		start_pos = 0;
	}
	if ( tos->n_pgos > 0 && start_pos > tos->pos_write ) {
		tos->pgos[tos->n_pgos].delay_remain = start_pos - tos->pos_write;
	} else if ( tos->pos_write > start_pos ) {
		do_pgoutput(tos->pgos[tos->n_pgos].modulestats, &(tos->buf[start_pos]), tos->pos_write - start_pos);
		tos->write_busy = 1;
	}
}

int ts_wait_pgoutput(ts_output_stat_t *tos)
{
	int i;
	int err = 0;
	if (tos->write_busy) {
		for (i = 0; i < tos->n_pgos; i++) {
			err |= do_pgoutput_wait(tos->pgos[i].modulestats);
		}
		tos->write_busy = 0;
	}
	return err;
}

void ts_check_pgoutput(ts_output_stat_t *tos)
{
	int i;
	if (tos->write_busy) {
		tos->write_busy = 0;
		for (i = 0; i < tos->n_pgos; i++) {
			tos->write_busy |= do_pgoutput_check(tos->pgos[i].modulestats);
		}
	}
}

void ts_close_oldest_pg(ts_output_stat_t *tos)
{
	int i;
	pgoutput_stat_t pgos;

	/* ��ԌÂ�pgos����� */
	do_pgoutput_close(tos->pgos[0].modulestats, &tos->pgos[0].final_pi);

	/* pgos[0]�̒��g���c���Ă����Ȃ��Ƃ����Ȃ��̂́Apgos[0].fn_base�̃|�C���^��ۑ����邽�� */
	pgos = tos->pgos[0];
	for (i = 0; i < tos->n_pgos - 1; i++) {
		tos->pgos[i] = tos->pgos[i + 1];
	}
	tos->pgos[tos->n_pgos - 1] = pgos;
	tos->n_pgos--;
}

void ts_output(ts_output_stat_t *tos, int64_t nowtime, int force_write)
{
	int i, write_size, diff;

	if (tos->write_busy) {
		/* �ŐV�̏������݊�����Ԃ��`�F�b�N */
		//tc_start("check");
		ts_check_pgoutput(tos);
		//tc_end();
	}

	if (!tos->write_busy) {
		/* �o�b�t�@�؂�l�� */
		if ( nowtime - tos->last_bufminimize_time >= OVERLAP_SEC*1000/4 ) {
			/* (OVERLAP_SEC/4)�b�ȏ�o���Ă�����o�b�t�@�؂�l�߂����s */
			ts_minimize_buf(tos);
			tos->last_bufminimize_time = nowtime;
		} else if ( nowtime < tos->last_bufminimize_time ) {
			/* �����̊����߂�ɑΉ� */
			tos->last_bufminimize_time = nowtime;
		}

		/* �t�@�C������ */
		if (tos->n_pgos >= 1 && 0 < tos->pgos[0].closetime && tos->pgos[0].closetime < nowtime) {
			if ( ! tos->pgos[0].close_flag ) {
				tos->pgos[0].close_flag = 1;
				tos->pgos[0].close_remain = tos->pos_filled - tos->pos_write;
				tos->pgos[0].delay_remain = 0;
			} else if ( tos->pgos[0].close_remain <= 0 ) {
				ts_close_oldest_pg(tos);
			}
		}

		/* �t�@�C�������o�� */
		write_size = tos->pos_filled - tos->pos_write;
		if ( write_size < 1024*1024 && !force_write ) { /* �����o���������x���܂��Ă��Ȃ��ꍇ�̓p�X */
			//ts_update_transfer_history(tos, nowtime, 0);
			return;
		} else if (write_size > MAX_WRITE_SIZE) {
			write_size = MAX_WRITE_SIZE;
		}

		/* �V���ȏ������݂��J�n */
		//tc_start("write");
		for (i = 0; i < tos->n_pgos; i++) {
			/* �����o���̊J�n���x���������Ă���ꍇ */
			if (tos->pgos[i].delay_remain > 0) {
				if (tos->pgos[i].delay_remain < write_size) {
					diff = write_size - tos->pgos[i].delay_remain;
					do_pgoutput(tos->pgos[i].modulestats, &(tos->buf[tos->pos_write+write_size-diff]), diff);
					tos->pgos[i].delay_remain = 0; /* ������0�ɂ��Ȃ��̂̓o�O�������H */
				} else {
					tos->pgos[i].delay_remain -= write_size;
				}
			/* �[���̏����o�����c���Ă���ꍇ */
			} else if ( tos->pgos[i].close_flag && tos->pgos[i].close_remain > 0 ) {
				if (tos->pgos[i].close_remain > write_size) {
					do_pgoutput(tos->pgos[i].modulestats, &(tos->buf[tos->pos_write]), write_size);
					tos->pgos[i].close_remain -= write_size;
				} else {
					do_pgoutput(tos->pgos[i].modulestats, &(tos->buf[tos->pos_write]), tos->pgos[i].close_remain);
					tos->pgos[i].close_remain = 0;
				}
			/* �ʏ�̏����o�� */
			} else {
				do_pgoutput(tos->pgos[i].modulestats, &(tos->buf[tos->pos_write]), write_size);
			}
		}
		//tc_end();
		tos->write_busy = 1;
		tos->pos_write += write_size;
	}
}

void ts_minimize_buf(ts_output_stat_t *tos)
{
	int i;
	int backward_size, clear_size, move_size;

	if (tos->write_busy) {
		//printf("[DEBUG] �������݂��������Ă��Ȃ��̂�ts_minimize_buf()���p�X\n");
		return;
	}

	for (backward_size = i = 0; i < tos->n_th; i++) {
		backward_size += tos->th[i].bytes;
	}
	clear_size = tos->pos_filled - backward_size;
	if (clear_size > tos->pos_write) {
		clear_size = tos->pos_write;
	}

	int min_clear_size = (int)((double)MIN_CLEAR_RATIO * BUFSIZE);
	if ( clear_size < min_clear_size ) {
		return;
	}

	move_size = tos->pos_filled - clear_size;
	memmove(tos->buf, &(tos->buf[clear_size]), move_size);
	tos->pos_filled -= clear_size;
	tos->pos_write -= clear_size;
	//printf("[DEBUG] ts_minimize_buf()�����s: clear_size=%d\n", clear_size);
}

void ts_require_buf(ts_output_stat_t *tos, int require_size)
{
	output_message(MSG_WARNING, L"�o�b�t�@������܂���B�o�b�t�@�̋󂫗e�ʂ�������̂�ҋ@���܂��B");
	ts_wait_pgoutput(tos);
	while (BUFSIZE - tos->pos_filled + tos->pos_write < require_size) {
		ts_output(tos, gettime(), 1);
		ts_wait_pgoutput(tos);
	}
	//printf("����\n");
	output_message(MSG_WARNING, L"�ҋ@����");

	int move_size = tos->pos_write;

	memmove(tos->buf, &(tos->buf[tos->pos_filled - move_size]), move_size);
	tos->pos_filled -= move_size;
	tos->pos_write = 0;
}

void ts_copybuf(ts_output_stat_t *tos, BYTE *buf, int n_buf)
{
	//fprintf(logfp, " filled=%d n_buf=%d\n", tos->filled, n_buf);

	if (tos->pos_filled + n_buf > BUFSIZE) {
		if ( ! param_nowait ) {
			ts_wait_pgoutput(tos);
		}
		ts_minimize_buf(tos);
	}
	if (tos->pos_filled + n_buf > BUFSIZE) {
		if (param_nowait) {
			tos->dropped_bytes += n_buf;
			return; /* �f�[�^���̂Ă� */
		} else {
			ts_require_buf(tos, n_buf);
		}
	}
	memcpy(&(tos->buf[tos->pos_filled]), buf, n_buf);
	tos->pos_filled += n_buf;
}

void ts_check_extended_text(ts_output_stat_t *tos)
{
	pgoutput_stat_t *current_pgos;
	WCHAR et[4096];

	if (tos->n_pgos <= 0) {
		return;
	}

	current_pgos = &tos->pgos[tos->n_pgos - 1];
	if (!(current_pgos->initial_pi_status & PGINFO_GET_EXTEND_TEXT) &&
			(tos->proginfo->status & PGINFO_GET_EXTEND_TEXT)) {
		/* �g���`���C�x���g��񂪓r�����痈���ꍇ */
		get_extended_text(et, sizeof(et) / sizeof(WCHAR), tos->proginfo);
		output_message(MSG_DISP, L"<<< ------------ �ǉ��ԑg��� ------------\n"
			L"�u%s�v\n"
			L"---------------------------------- >>>",
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
	int i, actually_start = 0;
	time_mjd_t curr_time;
	time_offset_t offset;
	pgoutput_stat_t *pgos;
	proginfo_t *final_pi, tmp_pi;

	/* print */
	printpi(tos->proginfo);

	/* split! */
	if (tos->n_pgos >= MAX_PGOVERLAP) {
		output_message(MSG_WARNING, L"�ԑg�̐؂�ւ���Z���ԂɘA�����Č��o�������߃X�L�b�v���܂�");
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
			output_message(MSG_NOTIFY, L"�I������������̂܂ܔԑg���I�������̂Ō��݂̃^�C���X�^���v��ԑg�I�������ɂ��܂�");
		} else {
			final_pi = &tos->last_proginfo;
		}

		/* end�t�b�N���Ăяo�� */
		for (i = 0; i < tos->n_pgos; i++) {
			do_pgoutput_end(tos->pgos[i].modulestats, final_pi);
		}

		/* pgos�̒ǉ� */
		pgos = &(tos->pgos[tos->n_pgos]);
		if (tos->n_pgos >= 1) {
			actually_start = 1; /* �O�̔ԑg������Ƃ������Ƃ͖{���̃X�^�[�g */
			pgos[-1].closetime = nowtime + OVERLAP_SEC * 1000;
			pgos[-1].final_pi = *final_pi;
		}

		pgos->initial_pi_status = tos->proginfo->status;
		pgos->fn = do_path_resolver(tos->proginfo, ch_info); /* ������ch_info�ɃA�N�Z�X */
		pgos->modulestats = do_pgoutput_create(pgos->fn, tos->proginfo, ch_info, actually_start); /* ������ch_info�ɃA�N�Z�X */
		pgos->closetime = -1;
		pgos->close_flag = 0;
		pgos->close_remain = 0;
		pgos->delay_remain = 0;
		ts_copy_backward(tos, nowtime);

		tos->n_pgos++;
	}
}

void ts_check_pi(ts_output_stat_t *tos, int64_t nowtime, ch_info_t *ch_info)
{
	int changed = 0, time_changed = 0;
	time_mjd_t endtime, last_endtime, time1, time2;
	WCHAR msg1[64], msg2[64];

	check_stream_timeinfo(tos);

	if ( !(tos->proginfo->status & PGINFO_READY_UPDATED) && 
			( (PGINFO_READY(tos->last_proginfo.status) && tos->n_pgos > 0) || tos->n_pgos == 0 ) ) {
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
		} else if (tos->n_pgos == 0) {
			/* �܂��o�͂��n�܂��Ă��Ȃ������狭���J�n */
			changed = 1;
		}
	}

	if (changed) {
		/* �ԑg���؂�ւ���� */
		ts_prog_changed(tos, nowtime, ch_info);
	} else if( PGINFO_READY(tos->proginfo->status) && PGINFO_READY(tos->last_proginfo.status) ) {
		
		if( proginfo_cmp(tos->proginfo, &tos->last_proginfo) ) {
			if (tos->n_pgos >= 1) {
				/* �Ăяo���͍̂ŐV�̔ԑg�ɑ΂��Ă̂� */
				do_pgoutput_changed(tos->pgos[tos->n_pgos-1].modulestats, &tos->last_proginfo, tos->proginfo);
			}

			/* �ԑg�̎��Ԃ��r���ŕύX���ꂽ�ꍇ */
			time_add_offset(&endtime, &tos->proginfo->start, &tos->proginfo->dur);
			time_add_offset(&last_endtime, &tos->last_proginfo.start, &tos->last_proginfo.dur);

			if ( get_time_offset(NULL, &tos->proginfo->start, &tos->last_proginfo.start) != 0 ) {
				swprintf(msg1, 128, L"�ԑg�J�n���Ԃ̕ύX(�T�[�r�X%d)", tos->proginfo->service_id);

				time_changed = 1;
				output_message( MSG_NOTIFY, L"%s: %02d:%02d:%02d �� %02d:%02d:%02d", msg1,
					tos->proginfo->start.hour, tos->proginfo->start.min, tos->proginfo->start.sec,
					tos->last_proginfo.start.hour, tos->last_proginfo.start.min, tos->last_proginfo.start.sec );
			}
			if ( get_time_offset(NULL, &endtime, &last_endtime) != 0 ) {
				swprintf(msg1, 128, L"�ԑg�I�����Ԃ̕ύX(�T�[�r�X%d)", tos->proginfo->service_id);

				if ( pi_endtime_unknown(&tos->last_proginfo) ) {
					tsd_strcpy(msg2, TSD_TEXT("���� ��"));
				} else {
					wsprintf( msg2, L"%02d:%02d:%02d ��", last_endtime.hour, last_endtime.min, last_endtime.sec );
				}

				time_changed = 1;
				if ( pi_endtime_unknown(tos->proginfo) ) {
					output_message(MSG_NOTIFY, L"%s: %s ����", msg1, msg2);
				} else {
					output_message( MSG_NOTIFY, L"%s: %s %02d:%02d:%02d", msg1, msg2,
						endtime.hour, endtime.min, endtime.sec );
				}
			}

			if (!time_changed) {
				output_message(MSG_NOTIFY, L"�ԑg���̕ύX");
				printpi(tos->proginfo);
			}
		}

	}

	tos->last_proginfo = *tos->proginfo;
	tos->last_checkpi_time = nowtime;
}
