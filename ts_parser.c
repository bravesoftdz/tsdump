#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <sys/timeb.h>

typedef wchar_t			WCHAR;
typedef long			BOOL;
typedef unsigned long	DWORD;

#include "ts_parser.h"
#include "modules_def.h"
#include "aribstr.h"
#include "tsdump.h"

const WCHAR *genre_main[] = {
	L"�j���[�X�^��",			L"�X�|�[�c",	L"���^���C�h�V���[",	L"�h���}",
	L"���y",					L"�o���G�e�B",	L"�f��",				L"�A�j���^���B",
	L"�h�L�������^���[�^���{",	L"����^����",	L"��^����",			L"����",
	L"�\��",					L"�\��",		L"�g��",				L"���̑�"
};

const WCHAR *genre_detail[] = {
	/* 0x0 */
	L"�莞�E����", L"�V�C", L"���W�E�h�L�������g", L"�����E����", L"�o�ρE�s��", L"�C�O�E����", L"���", L"���_�E��k",
	L"�񓹓���", L"���[�J���E�n��", L"���", L"-", L"-", L"-", L"-", L"���̑�",

	/* 0x1 */
	L"�X�|�[�c�j���[�X", L"�싅", L"�T�b�J�[", L"�S���t", L"���̑��̋��Z", L"���o�E�i���Z", L"�I�����s�b�N�E���ۑ��", L"�}���\���E����E���j",
	L"���[�^�[�X�|�[�c", L"�}�����E�E�B���^�[�X�|�[�c", L"���n�E���c���Z", L"-", L"-", L"-", L"-", L"���̑�",

	/* 0x2 */
	L"�|�\�E���C�h�V���[", L"�t�@�b�V����", L"��炵�E�Z�܂�", L"���N�E���", L"�V���b�s���O�E�ʔ�", L"�O�����E����", L"�C�x���g", L"�ԑg�Љ�E���m�点",
	L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

	/* 0x3 */
	L"�����h���}", L"�C�O�h���}", L"���㌀", L"-", L"-", L"-", L"-", L"-",
	L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

	/* 0x4 */
	L"�������b�N�E�|�b�v�X", L"�C�O���b�N�E�|�b�v�X", L"�N���V�b�N�E�I�y��", L"�W���Y�E�t���[�W����", L"�̗w�ȁE����", L"���C�u�E�R���T�[�g", L"�����L���O�E���N�G�X�g", L"�J���I�P�E�̂ǎ���",
	L"���w�E�M�y", L"���w�E�L�b�Y", L"�������y�E���[���h�~���[�W�b�N", L"-", L"-", L"-", L"-", L"���̑�",

	/* 0x5 */
	L"�N�C�Y", L"�Q�[��", L"�g�[�N�o���G�e�B", L"���΂��E�R���f�B", L"���y�o���G�e�B", L"���o���G�e�B", L"�����o���G�e�B", L"-",
	L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

	/* 0x6 */
	L"�m��", L"�M��", L"�A�j��", L"-", L"-", L"-", L"-", L"-",
	L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

	/* 0x7 */
	L"�����A�j��", L"�C�O�A�j��", L"���B", L"-", L"-", L"-", L"-", L"-",
	L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

	/* 0x8 */
	L"�Љ�E����", L"���j�E�I�s", L"���R�E�����E��", L"�F���E�Ȋw�E��w", L"�J���`���[�E�`���|�\", L"���w�E���|", L"�X�|�[�c", L"�h�L�������^���[�S��",
	L"�C���^�r���[�E���_", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

	/* 0x9 */
	L"���㌀�E�V��", L"�~���[�W�J��", L"�_���X�E�o���G", L"����E���|", L"�̕���E�ÓT", L"-", L"-", L"-",
	L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

	/* 0xA */
	L"���E�ނ�E�A�E�g�h�A", L"���|�E�y�b�g�E��|", L"���y�E���p�E�H�|", L"�͌�E����", L"�����E�p�`���R", L"�ԁE�I�[�g�o�C", L"�R���s���[�^�E�s�u�Q�[��", L"��b�E��w",
	L"�c���E���w��", L"���w���E���Z��", L"��w���E��", L"���U����E���i", L"������", L"-", L"-", L"���̑�",

	/* 0xB */
	L"�����", L"��Q��", L"�Љ��", L"�{�����e�B�A", L"��b", L"�����i�����j", L"�������", L"-",
	L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

	/* 0xC */
	L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",
	L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",

	/* 0xD */
	L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",
	L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",

	/* 0xE */
	L"BS/�n��f�W�^�������p�ԑg�t�����", L"�L�ш�CS�f�W�^�������p�g��", L"�q���f�W�^�����������p�g��", L"�T�[�o�[�^�ԑg�t�����", L"IP�����p�ԑg�t�����", L"-", L"-", L"-",
	L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",

	/* 0xF */
	L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",
	L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�"
};

const WCHAR *genre_user[] = {
	L"���~�̉\������",
	L"�����̉\������",
	L"���f�̉\������",
	L"����V���[�Y�̕ʘb�������̉\������",
	L"�Ґ�����g",
	L"�J��グ�̉\������",
	L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",

	L"���f�j���[�X����", L"���Y�C�x���g�Ɋ֘A����Վ��T�[�r�X����"
	L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-"
};

void get_genre_str(const WCHAR **genre1, const WCHAR **genre2, Cd_t_item item)
{
	if (item.content_nibble_level_1 != 0xe) {
		*genre1 = genre_main[item.content_nibble_level_1];
		*genre2 = genre_detail[item.content_nibble_level_1 * 0x10 + item.content_nibble_level_2];
	} else {
		*genre1 = genre_detail[item.content_nibble_level_1 * 0x10 + item.content_nibble_level_2];
		if (item.user_nibble_1 <= 0x01) {
			*genre2 = genre_user[item.user_nibble_1 * 0x10 + item.user_nibble_2];
		} else {
			*genre2 = L"-";
		}
	}
}

int get_extended_text(WCHAR *dst, size_t n, const proginfo_t *pi)
{
	int i;
	WCHAR *p = dst, *end = &dst[n - 1];

	*p = L'\0';
	if (!(pi->status & PGINFO_GET_EXTEND_TEXT)) {
		return 0;
	}

	for (i = 0; i < pi->n_items && p < end; i++) {
		wcscpy_s(p, end - p, pi->items[i].desc);
		while (*p != L'\0') { p++; }
		wcscpy_s(p, end - p, L"\n");
		while (*p != L'\0') { p++; }
		wcscpy_s(p, end - p, pi->items[i].item);
		while (*p != L'\0') { p++; }
		wcscpy_s(p, end - p, L"\n");
		while (*p != L'\0') { p++; }
	}
	return 1;
}

int proginfo_cmp(const proginfo_t *pi1, const proginfo_t *pi2)
{
	WCHAR et1[4096], et2[4096];
	int i;

	if (pi1->status != pi2->status) {
		return 1;
	}

	if (pi1->dur_hour != pi2->dur_hour ||
		pi1->dur_min != pi2->dur_min ||
		pi1->dur_sec != pi2->dur_sec ||
		pi1->start_hour != pi2->start_hour ||
		pi1->start_min != pi2->start_min ||
		pi1->start_sec != pi2->start_sec ||
		pi1->start_year != pi2->start_year ||
		pi1->start_month != pi2->start_month ||
		pi1->start_day != pi2->start_day) {
		return 1;
	}

	if ( pi1->status & PGINFO_GET_SHORT_TEXT ) {
		if (wcscmp(pi1->event_text.str, pi2->event_text.str) != 0) {
			return 1;
		}
	}

	if ( pi1->status & PGINFO_GET_GENRE ) {
		if( pi1->genre_info.n_items != pi2->genre_info.n_items ) {
			return 1;
		}
		for (i = 0; i < pi1->genre_info.n_items; i++) {
			if ( memcmp(&pi1->genre_info.items[i], 
						&pi2->genre_info.items[i],
						sizeof(pi1->genre_info.items[i]) ) != 0 ) {
				return 1;
			}
		}
	}

	if ( get_extended_text(et1, sizeof(et1) / sizeof(WCHAR), pi1) !=
		get_extended_text(et2, sizeof(et2) / sizeof(WCHAR), pi2) ) {
		return 1;
	}
	if (wcscmp(et1, et2) != 0) {
		return 1;
	}

	return 0;
}

static inline void parse_PSI(const uint8_t *packet, PSI_parse_t *ps)
{
	int pos, remain, pointer_field;

	/* FINISHED��Ԃ�������Ԃɖ߂� */
	if (ps->stat == PAYLOAD_STAT_FINISHED) {
		if (ps->next_recv_payload > 0) {
			/* �O���M�����c��̃y�C���[�h */
			ps->stat = PAYLOAD_STAT_PROC;
			memcpy(ps->payload, ps->next_payload, ps->next_recv_payload);
			ps->n_payload = ps->n_next_payload;
			ps->recv_payload = ps->next_recv_payload;
			ps->n_next_payload = 0;
			ps->next_recv_payload = 0;
		} else {
			/* �O��̎c�肪�����̂ŏ�����Ԃɂ��� */
			ps->stat = PAYLOAD_STAT_INIT;
		}
	}

	/* �Ώ�PID���ǂ����`�F�b�N */
	if (ps->pid != ts_get_pid(packet)) {
		return;
	}

	/* �p�P�b�g�̏��� */
	if (ps->stat == PAYLOAD_STAT_INIT) {
		if (!ts_get_payload_unit_start_indicator(packet)) {
			//printf("pass!\n");
			return;
		}
		ps->stat = PAYLOAD_STAT_PROC;
		ps->n_payload = ts_get_section_length(packet) + 3;
		ps->recv_payload = ps->n_next_payload = ps->next_recv_payload = 0;
		ps->continuity_counter = ts_get_continuity_counter(packet);

		pos = ts_get_payload_data_pos(packet);
		/* �s���ȃp�P�b�g���ǂ����̃`�F�b�N */
		if (pos > 188) {
			ps->stat = PAYLOAD_STAT_INIT;
			output_message(MSG_PACKETERROR, L"Invalid payload data_byte offset! (pid=0x%02x)", ps->pid);
			return;
		}

		remain = 188 - pos;
		if (remain > ps->n_payload) {
			remain = ps->n_payload;
			ps->stat = PAYLOAD_STAT_FINISHED;
		}
		memcpy(ps->payload, &packet[pos], remain);
		ps->recv_payload += remain;
	} else if (ps->stat == PAYLOAD_STAT_PROC) {
		/* continuity_counter �̘A�������m�F */
		if ((ps->continuity_counter + 1) % 16 != ts_get_continuity_counter(packet)) {
			/* drop! */
			output_message(MSG_PACKETERROR, L"packet continuity_counter is discontinuous! (pid=0x%02x)", ps->pid);
			ps->n_payload = ps->recv_payload = 0;
			ps->stat = PAYLOAD_STAT_INIT;
			return;
		}
		ps->continuity_counter = ts_get_continuity_counter(packet);

		if (ts_get_payload_unit_start_indicator(packet)) {
			pos = ts_get_payload_pos(packet);
			pointer_field = packet[pos];
			pos++;

			/* �s���ȃp�P�b�g���ǂ����̃`�F�b�N */
			if (pos + pointer_field >= 188) {
				ps->stat = PAYLOAD_STAT_INIT;
				output_message(MSG_PACKETERROR, L"Invalid payload data_byte offset! (pid=0x%02x)", ps->pid);
				return;
			}

			ps->n_next_payload = ts_get_section_length(packet) + 3;
			ps->next_recv_payload = 188 - pos - pointer_field;
			if (ps->next_recv_payload > ps->n_next_payload) {
				ps->next_recv_payload = ps->n_next_payload;
			}
			memcpy(ps->next_payload, &packet[pos+pointer_field], ps->next_recv_payload);
			ps->stat = PAYLOAD_STAT_FINISHED;

			remain = pointer_field;
		} else {
			pos = ts_get_payload_data_pos(packet);
			/* �s���ȃp�P�b�g���ǂ����̃`�F�b�N */
			if (pos > 188) {
				ps->stat = PAYLOAD_STAT_INIT;
				output_message(MSG_PACKETERROR, L"Invalid payload data_byte offset! (pid=0x%02x)", ps->pid);
				return;
			}

			remain = 188 - pos;
		}

		if (remain >= ps->n_payload - ps->recv_payload) {
			remain = ps->n_payload - ps->recv_payload;
			ps->stat = PAYLOAD_STAT_FINISHED;
		}
		memcpy(&(ps->payload[ps->recv_payload]), &packet[pos], remain);
		ps->recv_payload += remain;
	}

	if (ps->stat == PAYLOAD_STAT_FINISHED) {
		ps->crc32 = get_payload_crc32(ps);
		unsigned __int32 crc = crc32(ps->payload, ps->n_payload - 4);
		if (ps->crc32 != crc) {
			ps->stat = PAYLOAD_STAT_INIT;
			output_message(MSG_PACKETERROR, L"Payload CRC32 mismatch! (pid=0x%02x)", ps->pid);
		}
	}
}

void clear_proginfo(proginfo_t *proginfo)
{
	proginfo->status &= PGINFO_GET_PAT;
	proginfo->last_desc = -1;
}

void init_proginfo(proginfo_t *proginfo)
{
	proginfo->status = 0;
	proginfo->last_desc = -1;
	proginfo->last_ready_time = gettime();
}

int parse_EIT_Sed(const uint8_t *desc, Sed_t *sed)
{
	const uint8_t *desc_end;

	//sed->descriptor_tag					= desc[0];
	sed->descriptor_length				= desc[1];

	desc_end							= &desc[2 + sed->descriptor_length];

	memcpy(sed->ISO_639_language_code,	  &desc[2], 3);
	sed->ISO_639_language_code[3]		= '\0';
	
	sed->event_name_length				= desc[5];
	sed->event_name_char				= &desc[6];
	if ( &sed->event_name_char[sed->event_name_length] >= desc_end ) {
		return 0;
	}

	sed->text_length					= sed->event_name_char[sed->event_name_length];
	sed->text_char						= &sed->event_name_char[sed->event_name_length + 1];
	if ( &sed->text_char[sed->text_length] > desc_end ) {
		return 0;
	}
	return 1;
}

int parse_EIT_Eed(const uint8_t *desc, Eed_t *eed)
{
	const uint8_t *desc_end;

	//eed->descriptor_tag					= desc[0];
	eed->descriptor_length				= desc[1];
	desc_end							= &desc[2+eed->descriptor_length];

	eed->descriptor_number				= get_bits(desc, 16, 4);
	eed->last_descriptor_number			= get_bits(desc, 20, 4);
	memcpy(eed->ISO_639_language_code,    &desc[3], 3);
	eed->ISO_639_language_code[3]		= '\0';
	eed->length_of_items				= desc[6];

	if ( &desc[7+eed->length_of_items] >= desc_end ) {
		return 0;
	}

	eed->text_length					= desc[7 + eed->length_of_items];
	eed->text_char						= &desc[8 + eed->length_of_items];
	if ( &eed->text_char[eed->text_length] > desc_end ) {
		return 0;
	}

	return 1;
}

int parse_EIT_Eed_item(const uint8_t *item, const uint8_t *item_end, Eed_item_t *eed_item)
{
	eed_item->item_description_length	= item[0];
	eed_item->item_description_char		= &item[1];
	if ( &eed_item->item_description_char[ eed_item->item_description_length ] >= item_end) {
		return 0;
	}

	eed_item->item_length				= eed_item->item_description_char[eed_item->item_description_length];
	eed_item->item_char					= &eed_item->item_description_char[eed_item->item_description_length+1];
	if ( &eed_item->item_char[eed_item->item_length] > item_end) {
		return 0;
	}
	return 1;
}

void parse_EIT_header(const uint8_t *payload, EIT_header_t *eit)
{
	eit->table_id						= payload[0];
	eit->section_syntax_indicator		= get_bits(payload, 8, 1);
	eit->section_length					= get_bits(payload, 12, 12);
	eit->service_id						= get_bits(payload, 24, 16);
	eit->version_number					= get_bits(payload, 42, 5);
	eit->current_next_indicator			= get_bits(payload, 47, 1);
	eit->section_number					= get_bits(payload, 48, 8);
	eit->last_section_number			= get_bits(payload, 56, 8);
	eit->transport_stream_id			= get_bits(payload, 64, 16);
	eit->original_network_id			= get_bits(payload, 80, 16);
	eit->segment_last_section_number	= get_bits(payload, 96, 8);
	eit->last_table_id					= get_bits(payload, 104, 8);
}

void parse_EIT_body(const uint8_t *body, EIT_body_t *eit_b)
{
	eit_b->event_id						= get_bits(body, 0, 16);
	eit_b->start_time_mjd				= get_bits(body, 16, 16);
	eit_b->start_time_jtc				= get_bits(body, 32, 24);
	eit_b->duration						= get_bits(body, 56, 24);
	eit_b->running_status				= get_bits(body, 80, 3);
	eit_b->free_CA_mode					= get_bits(body, 83, 1);
	eit_b->descriptors_loop_length		= get_bits(body, 84, 12);
}

void store_EIT_Sed(const Sed_t *sed, proginfo_t *proginfo)
{
	proginfo->event_name.aribstr_len = sed->event_name_length;
	memcpy(proginfo->event_name.aribstr, sed->event_name_char, sed->event_name_length);
	proginfo->event_text.aribstr_len = sed->text_length;
	memcpy(proginfo->event_text.aribstr, sed->text_char, sed->text_length);

	proginfo->event_name.str_len = 
		AribToString(proginfo->event_name.str, sizeof(proginfo->event_name.str),
					proginfo->event_name.aribstr, proginfo->event_name.aribstr_len);

	proginfo->event_text.str_len =
		AribToString(proginfo->event_text.str, sizeof(proginfo->event_text.str),
			proginfo->event_text.aribstr, proginfo->event_text.aribstr_len);
	proginfo->status |= PGINFO_GET_SHORT_TEXT;
}

void store_EIT_body(const EIT_body_t *eit_b, proginfo_t *proginfo)
{
	int y, m, d, k;
	double mjd;

	if (proginfo->status & PGINFO_GET_EVENT_INFO && proginfo->event_id != eit_b->event_id) {
		/* �O��̎擾����ԑg���؂�ւ���� */
		clear_proginfo(proginfo);
	}
	proginfo->event_id = eit_b->event_id;

	if (eit_b->start_time_mjd == 0xffff && eit_b->start_time_jtc == 0xffffff) {
		proginfo->start_year = 0;
		proginfo->start_month = 0;
		proginfo->start_day = 0;
		proginfo->start_hour = 0;
		proginfo->start_min = 0;
		proginfo->start_sec = 0;
		proginfo->status |= PGINFO_UNKNOWN_STARTTIME;
	} else {
		/* MJD -> YMD */
		/*�@2100�N2��28���܂ł̊ԗL���Ȍ����iARIB STD-B10 ��Q�����j�@*/
		mjd = (double)eit_b->start_time_mjd;
		y = (int)((mjd - 15078.2) / 365.25);
		m = (int)((mjd - 14956.1 - (int)((double)y*365.25)) / 30.6001);
		d = eit_b->start_time_mjd - 14956 - (int)((double)y*365.25) - (int)((double)m*30.6001);
		if (m == 14 || m == 15) {
			k = 1;
		}
		else {
			k = 0;
		}
		proginfo->start_year = 1900 + y + k;
		proginfo->start_month = m - 1 - k * 12;
		proginfo->start_day = d;

		proginfo->start_hour = (eit_b->start_time_jtc >> 20 & 0x0f) * 10 +
			((eit_b->start_time_jtc >> 16) & 0x0f);
		proginfo->start_min = (eit_b->start_time_jtc >> 12 & 0x0f) * 10 +
			((eit_b->start_time_jtc >> 8) & 0x0f);
		proginfo->start_sec = (eit_b->start_time_jtc >> 4 & 0x0f) * 10 +
			(eit_b->start_time_jtc & 0x0f);
		if (proginfo->start_hour >= 24) { proginfo->start_hour = 23; }
		if (proginfo->start_min >= 60) { proginfo->start_min = 59; }
		if (proginfo->start_sec >= 60) { proginfo->start_sec = 59; }

		proginfo->status &= ~PGINFO_UNKNOWN_STARTTIME;
	}

	if (eit_b->duration == 0xffffff) {
		proginfo->dur_hour = 0;
		proginfo->dur_min = 0;
		proginfo->dur_sec = 0;
		proginfo->status = PGINFO_UNKNOWN_DURATION;
	} else {
		proginfo->dur_hour = (eit_b->duration >> 20 & 0x0f) * 10 +
			((eit_b->duration >> 16) & 0x0f);
		proginfo->dur_min = (eit_b->duration >> 12 & 0x0f) * 10 +
			((eit_b->duration >> 8) & 0x0f);
		proginfo->dur_sec = (eit_b->duration >> 4 & 0x0f) * 10 +
			(eit_b->duration & 0x0f);
		if (proginfo->dur_hour >= 24) { proginfo->dur_hour = 23; }
		if (proginfo->dur_min >= 60) { proginfo->dur_min = 59; }
		if (proginfo->dur_sec >= 60) { proginfo->dur_sec = 59; }
		proginfo->status &= ~PGINFO_UNKNOWN_DURATION;
	}

	proginfo->status |= PGINFO_GET_EVENT_INFO;
}

void store_EIT_Eed_item(const Eed_t *eed, const Eed_item_t *eed_item, proginfo_t *proginfo)
{
	int i;
	int item_len;
	Eed_item_string_t *curr_item;

	if (proginfo->last_desc != -1) {
		/* �A�����`�F�b�N */
		if (proginfo->curr_desc == (int)eed->descriptor_number) {
			if (eed_item->item_description_length > 0) {
				/* �O��Ɠ���descriptor_number�ō��ږ�������̂ŕs�A�� */
				proginfo->last_desc = -1;
			} else {
				/* �O��̑��� */
				proginfo->curr_desc = eed->descriptor_number;
			}
		} else if( proginfo->curr_desc + 1 == (int)eed->descriptor_number ) {
			/* �O��̑��� */
			proginfo->curr_desc = eed->descriptor_number;
		} else {
			/* �s�A�� */
			proginfo->last_desc = -1;
		}
	}

	/* ������Ԃ��� */
	if (proginfo->last_desc == -1) {
		if (eed->descriptor_number == 0 && eed_item->item_description_length > 0) {
			proginfo->curr_desc = 0;
			proginfo->last_desc = eed->last_descriptor_number;
			proginfo->n_items = 0;
		} else {
			return;
		}
	}

	if (eed_item->item_description_length > 0) {
		/* �V�K���� */
		curr_item = &proginfo->items[proginfo->n_items];
		if (proginfo->n_items < sizeof(proginfo->items) / sizeof(proginfo->items[0])) {
			proginfo->n_items++;
			if (eed_item->item_description_length <= sizeof(curr_item->aribdesc)) {
				curr_item->aribdesc_len = eed_item->item_description_length;
			} else {
				/* �T�C�Y�I�[�o�[�Ȃ̂Ő؂�l�߂� */
				curr_item->aribdesc_len = sizeof(curr_item->aribdesc);
			}
			memcpy(curr_item->aribdesc, eed_item->item_description_char, eed_item->item_description_length);
			curr_item->aribitem_len = 0;
		} else {
			/* ����ȏ�item��ǉ��ł��Ȃ� */
			return;
		}
	} else {
		/* �O��̍��ڂ̑��� */
		if (proginfo->n_items == 0) {
			//curr_item = &proginfo->items[0];
			return;
		} else {
			curr_item = &proginfo->items[proginfo->n_items-1];
		}
	}

	item_len = curr_item->aribitem_len + eed_item->item_length;
	if ( item_len > sizeof(curr_item->aribitem) ) {
		/* �T�C�Y�I�[�o�[�Ȃ̂Ő؂�l�߂� */
		item_len = sizeof(curr_item->aribitem);
	}
	memcpy(&curr_item->aribitem[curr_item->aribitem_len], eed_item->item_char, eed_item->item_length);
	curr_item->aribitem_len = item_len;

	if (proginfo->curr_desc == proginfo->last_desc) {
		for (i = 0; i < proginfo->n_items; i++) {
			proginfo->items[i].desc_len = AribToString(
					proginfo->items[i].desc,
					sizeof(proginfo->items[i].desc),
					proginfo->items[i].aribdesc,
					proginfo->items[i].aribdesc_len
				);
			proginfo->items[i].item_len = AribToString(
					proginfo->items[i].item,
					sizeof(proginfo->items[i].item),
					proginfo->items[i].aribitem,
					proginfo->items[i].aribitem_len
				);
		}
		proginfo->status |= PGINFO_GET_EXTEND_TEXT;
	}
}

void parse_EIT_Cd(const uint8_t *desc, Cd_t *cd)
{
	int i;
	cd->n_items							= desc[1] / 2;
	if (cd->n_items > 8) {
		cd->n_items = 8;
	}

	for (i = 0; i < cd->n_items; i++) {
		cd->items[i].content_nibble_level_1 = get_bits(&desc[2+i*2], 0, 4);
		cd->items[i].content_nibble_level_2 = get_bits(&desc[2+i*2], 4, 4);
		cd->items[i].user_nibble_1 = get_bits(&desc[2+i*2], 8, 4);
		cd->items[i].user_nibble_2 = get_bits(&desc[2+i*2], 12, 4);
	}
}

proginfo_t *find_curr_service(proginfo_t *proginfos, int n_services, unsigned int service_id)
{
	int i;
	for (i = 0; i < n_services; i++) {
		if (service_id == proginfos[i].service_id) {
			return &proginfos[i];
		}
	}
	return NULL;
}

void parse_EIT(PSI_parse_t *payload_stat, const uint8_t *packet, proginfo_t *proginfo_all, int n_services)
{
	int len;
	EIT_header_t eit_h;
	EIT_body_t eit_b;
	uint8_t *p_eit_b, *p_eit_end;
	uint8_t *p_desc, *p_desc_end;
	uint8_t dtag, dlen;
	proginfo_t *curr_proginfo;

	parse_PSI(packet, payload_stat);

	if (payload_stat->stat != PAYLOAD_STAT_FINISHED || payload_stat->payload[0] != 0x4e) {
		return;
	}

	parse_EIT_header(payload_stat->payload, &eit_h);

	if (eit_h.section_number != 0) {
		/* ���̔ԑg�ł͂Ȃ� */
		return;
	}

	/* �Ώۂ̃T�[�r�XID���ǂ��� */
	curr_proginfo = find_curr_service(proginfo_all, n_services, eit_h.service_id);
	if(!curr_proginfo) {
		return;
	}

	len = payload_stat->n_payload - 14 - 4/*=sizeof(crc32)*/;
	p_eit_b = &payload_stat->payload[14];
	p_eit_end = &p_eit_b[len];
	while(&p_eit_b[12] < p_eit_end) {
		parse_EIT_body(p_eit_b, &eit_b); /* read 12bytes */
		store_EIT_body(&eit_b, curr_proginfo);

		p_desc = &p_eit_b[12];
		p_desc_end = &p_desc[eit_b.descriptors_loop_length];
		if (p_desc_end > p_eit_end) {
			break;
		}

		while( p_desc < p_desc_end ) {
			dtag = p_desc[0];
			dlen = p_desc[1];
			if ( &p_desc[2+dlen] > p_desc_end ) {
				break;
			}

			if (dtag == 0x4d) {
				Sed_t sed;
				if (parse_EIT_Sed(p_desc, &sed)) {
					store_EIT_Sed(&sed, curr_proginfo);
					if (PGINFO_READY(curr_proginfo->status)) {
						curr_proginfo->last_ready_time = gettime();
					}
				}
			} else if (dtag == 0x4e) {
				Eed_t eed;
				Eed_item_t eed_item;
				uint8_t *p_eed_item, *p_eed_item_end;
				if (parse_EIT_Eed(p_desc, &eed)) {
					p_eed_item = &p_desc[7];
					p_eed_item_end = &p_eed_item[eed.length_of_items];
					while (p_eed_item < p_eed_item_end) {
						if (parse_EIT_Eed_item(p_eed_item, p_eed_item_end, &eed_item)) {

							/* ARIB TR-B14 ��l�� �n��f�W�^���e���r�W�������� PSI/SI �^�p�K�� 12.2 �Z�N�V�����ւ̋L�q�q�̔z�u �́A
							Eed��������EIT�ɂ܂������đ��M����邱�Ƃ͖������Ƃ��K�肵�Ă���Ɖ��߂ł���B
							�����parse_EIT()�𔲂����Ƃ���curr_proginfo->items�ɑS�ẴA�C�e�������[����Ă��邱�Ƃ��ۏႳ���B
							���ɂ��̉��肪��������Ȃ��ꍇ�ł��A�P�ɒ��r���[�Ȕԑg��񂪌����Ă��܂��^�C�~���O�����݂��邾���ł��� */
							store_EIT_Eed_item(&eed, &eed_item, curr_proginfo);
						}
						p_eed_item += ( 2 + eed_item.item_description_length + eed_item.item_length );
					}
				}
			} else if (dtag == 0x54) {
				parse_EIT_Cd(p_desc, &curr_proginfo->genre_info);
				curr_proginfo->status |= PGINFO_GET_GENRE;
			}

			p_desc += (2+dlen);
		}
		p_eit_b = p_desc_end;
	}
}

int parse_SDT_Sd(const uint8_t *desc, Sd_t *sd)
{
	//sd->descriptor_tag					= desc[0];
	sd->descriptor_length				= desc[1];
	sd->service_type					= desc[2];

	sd->service_provider_name_length	= desc[3];
	sd->service_provider_name_char		= &desc[4];
	if ( &sd->service_provider_name_char[ sd->service_provider_name_length ] >
			&desc[ 2 + sd->descriptor_length ] ) {
		return 0;
	}

	sd->service_name_length				= sd->service_provider_name_char[ sd->service_provider_name_length ];
	sd->service_name_char				= &sd->service_provider_name_char[ sd->service_provider_name_length + 1 ];
	if ( &sd->service_name_char[ sd->service_name_length ] >
			&desc[ 2 + sd->descriptor_length ] ) {
		return 0;
	}
	return 1;
}

void parse_SDT_header(const uint8_t *payload, SDT_header_t *sdt_h)
{
	sdt_h->table_id						= payload[0];
	sdt_h->section_syntax_indicator 	= get_bits(payload,  8,  1);
	sdt_h->section_length 				= get_bits(payload, 12, 12);
	sdt_h->transport_stream_id 			= get_bits(payload, 24, 16);
	sdt_h->version_number 				= get_bits(payload, 42,  5);
	sdt_h->current_next_indicator 		= get_bits(payload, 47,  1);
	sdt_h->section_number 				= get_bits(payload, 48,  8);
	sdt_h->last_section_number 			= get_bits(payload, 56,  8);
	sdt_h->original_network_id 			= get_bits(payload, 64, 16);
}

void parse_SDT_body(const uint8_t *body, SDT_body_t *sdt_b)
{
	sdt_b->service_id					= get_bits(body,  0, 16);
	sdt_b->EIT_user_defined_flags		= get_bits(body, 19,  3);
	sdt_b->EIT_schedule_flag			= get_bits(body, 22,  1);
	sdt_b->EIT_present_following_flag	= get_bits(body, 23,  1);
	sdt_b->running_status				= get_bits(body, 24,  3);
	sdt_b->free_CA_mode					= get_bits(body, 27,  1);
	sdt_b->descriptors_loop_length 		= get_bits(body, 28, 12);
}

void store_SDT(const SDT_header_t *sdt_h, const Sd_t *sd, proginfo_t *proginfo)
{
	proginfo->network_id = sdt_h->original_network_id;
	proginfo->ts_id = sdt_h->transport_stream_id;

	proginfo->service_name.aribstr_len = sd->service_name_length;
	memcpy(proginfo->service_name.aribstr, sd->service_name_char, sd->service_name_length);
	proginfo->service_provider_name.aribstr_len = sd->service_provider_name_length;
	memcpy(proginfo->service_provider_name.aribstr, sd->service_provider_name_char, sd->service_provider_name_length);

	proginfo->service_name.str_len =
		AribToString(proginfo->service_name.str, sizeof(proginfo->service_name.str),
			proginfo->service_name.aribstr, proginfo->service_name.aribstr_len);

	proginfo->service_provider_name.str_len =
		AribToString(proginfo->service_provider_name.str, sizeof(proginfo->service_provider_name.str),
			proginfo->service_provider_name.aribstr, proginfo->service_provider_name.aribstr_len);

	proginfo->status |= PGINFO_GET_SERVICE_INFO;
}

void parse_SDT(PSI_parse_t *payload_stat, const uint8_t *packet, proginfo_t *proginfo_all, int n_services)
{
	int len;
	SDT_header_t sdt_h;
	SDT_body_t sdt_b;
	Sd_t sd;
	uint8_t *p_sdt_b, *p_sdt_end;
	uint8_t *p_desc, *p_desc_end;
	uint8_t dtag, dlen;
	proginfo_t *curr_proginfo;

	parse_PSI(packet, payload_stat);

	if (payload_stat->stat != PAYLOAD_STAT_FINISHED || payload_stat->payload[0] != 0x42) {
		return;
	}

	parse_SDT_header(payload_stat->payload, &sdt_h);

	len = payload_stat->n_payload - 11 - 4/*=sizeof(crc32)*/;
	p_sdt_b = &payload_stat->payload[11];
	p_sdt_end = &p_sdt_b[len];
	while(&p_sdt_b[5] < p_sdt_end) {
		parse_SDT_body(p_sdt_b, &sdt_b); /* read 5bytes */

		/* �Ώۂ̃T�[�r�XID���ǂ��� */
		curr_proginfo = find_curr_service(proginfo_all, n_services, sdt_b.service_id);
		if (!curr_proginfo) {
			continue;
		}

		p_desc = &p_sdt_b[5];
		p_desc_end = &p_desc[sdt_b.descriptors_loop_length];
		if (p_desc_end > p_sdt_end) {
			break;
		}
		while( p_desc < p_desc_end ) {
			dtag = p_desc[0];
			dlen = p_desc[1];
			if (dtag == 0x48) {
				if (parse_SDT_Sd(p_desc, &sd)) {
					store_SDT(&sdt_h, &sd, curr_proginfo);
				}
			}
			p_desc += (2+dlen);
		}
		p_sdt_b = p_desc_end;
	}
}

/* PMT: ISO 13818-1 2.4.4.8 Program Map Table */
void parse_PMT(uint8_t *packet, proginfo_t *proginfos, int n_services)
{
	int i;
	int pos, n_pids, len;
	uint16_t pid;
	uint8_t stream_type;
	uint8_t *payload;

	for (i = 0; i < n_services; i++) {
		parse_PSI(packet, &proginfos[i].PMT_payload);
		if (proginfos[i].PMT_payload.stat != PAYLOAD_STAT_FINISHED) {
			continue;
		}

		if ( proginfos[i].PMT_last_CRC != proginfos[i].PMT_payload.crc32 ) { /* CRC���O��ƈ�����Ƃ��̂ݕ\�� */
			output_message(MSG_DISP, L"<<< ------------- PMT ---------------\n"
				L"program_number: %d(0x%X), payload crc32: 0x%08X",
				proginfos[i].service_id, proginfos[i].service_id, proginfos[i].PMT_payload.crc32 );
		}

		len = proginfos[i].PMT_payload.n_payload - 4/*crc32*/;
		payload = proginfos[i].PMT_payload.payload;
		pos = 12 + get_bits(payload, 84, 12);
		n_pids = 0;
		while ( pos < len && n_pids <= MAX_PIDS_PER_SERVICE ) {
			stream_type = payload[pos];
			pid = (uint16_t)get_bits(payload, pos*8+11, 13);
			pos += get_bits(payload, pos*8+28, 12) + 5;
			if ( proginfos[i].PMT_last_CRC != proginfos[i].PMT_payload.crc32 ) { /* CRC���O��ƈ�����Ƃ��̂ݕ\�� */
				output_message(MSG_DISP, L"stream_type:0x%x(%S), elementary_PID:%d(0x%X)",
					stream_type, get_stream_type_str(stream_type), pid, pid);
			}
			proginfos[i].service_pids[n_pids].stream_type = stream_type;
			proginfos[i].service_pids[n_pids].pid = pid;
			n_pids++;
		}
		proginfos[i].n_service_pids = n_pids;
		proginfos[i].PMT_payload.stat = PAYLOAD_STAT_INIT;
		proginfos[i].status |= PGINFO_GET_PMT;
		proginfos[i].PMT_last_CRC = proginfos[i].PMT_payload.crc32;
		if ( proginfos[i].PMT_last_CRC != proginfos[i].PMT_payload.crc32 ) {
			output_message(MSG_DISP, L"---------------------------------- >>>");
		}
	}
}

void parse_PAT(PSI_parse_t *PAT_payload, const uint8_t *packet, proginfo_t *proginfos, const int n_services_max, int *n_services)
{
	int i, n, pn, pid;
	uint8_t *payload;

	parse_PSI(packet, PAT_payload);
	if (PAT_payload->stat == PAYLOAD_STAT_FINISHED) {
		n = (PAT_payload->n_payload - 4/*crc32*/ - 8/*fixed length*/) / 4;
		if (n > n_services_max) {
			n = n_services_max;
		}

		payload = &(PAT_payload->payload[8]);
		output_message(MSG_DISP, L"<<< ------------- PAT ---------------");
		*n_services = 0;
		for (i = 0; i < n; i++) {
			pn = get_bits(payload, i*32, 16);
			pid = get_bits(payload, i*32+19, 13);
			if (pn == 0) {
				output_message(MSG_DISP, L"network_PID:%d(0x%X)", pid, pid);
			} else {
				proginfos[*n_services].service_id = pn;
				proginfos[*n_services].PMT_payload.stat = PAYLOAD_STAT_INIT;
				proginfos[*n_services].PMT_payload.pid = pid;
				proginfos[*n_services].status |= PGINFO_GET_PAT;
				(*n_services)++;
				output_message(MSG_DISP, L"program_number:%d(0x%X), program_map_PID:%d(0x%X)", pn, pn, pid, pid);
			}
		}
		output_message(MSG_DISP, L"---------------------------------- >>>");
	}
}
