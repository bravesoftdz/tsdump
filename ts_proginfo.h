#define PGINFO_GET_PAT				1
#define PGINFO_GET_PMT				2
#define PGINFO_GET_SERVICE_INFO		4
#define PGINFO_GET_EVENT_INFO		8
#define PGINFO_GET_SHORT_TEXT		16
#define PGINFO_GET_EXTEND_TEXT		32
#define PGINFO_GET_GENRE			64
#define PGINFO_UNKNOWN_STARTTIME	128
#define PGINFO_UNKNOWN_DURATION		256

#define PGINFO_GET					(PGINFO_GET_PAT|PGINFO_GET_SERVICE_INFO|PGINFO_GET_EVENT_INFO|PGINFO_GET_SHORT_TEXT)
#define PGINFO_READY(status)		(( (status) & PGINFO_GET ) == PGINFO_GET)

#define MAX_PIDS_PER_SERVICE		64
#define MAX_SERVICES_PER_CH			32

#define ARIB_CHAR_SIZE_RATIO 1

typedef enum {
	PAYLOAD_STAT_INIT = 0,
	PAYLOAD_STAT_PROC,
	PAYLOAD_STAT_FINISHED
} PSI_stat_t;

typedef struct {
	unsigned int pid;
	PSI_stat_t stat;
	uint8_t payload[4096 + 3];
	uint8_t next_payload[188];
	int n_next_payload;
	int n_payload;
	int next_recv_payload;
	int recv_payload;
	unsigned int continuity_counter;
	uint32_t crc32;
} PSI_parse_t;

typedef struct {
	int aribstr_len;
	uint8_t aribstr[256];
	int str_len;
	WCHAR str[256*ARIB_CHAR_SIZE_RATIO];
} Sed_string_t;

typedef Sed_string_t Sd_string_t;

typedef struct {
	int aribdesc_len;
	uint8_t aribdesc[20]; /* ARIB TR-B14�ɂ����ď����16bytes�ƒ�߂��Ă��� */
	int desc_len;
	WCHAR desc[20*ARIB_CHAR_SIZE_RATIO+1];
	int aribitem_len;
	uint8_t aribitem[480]; /* ARIB TR-B14�ɂ����ď����440bytes�ƒ�߂��Ă��� */
	int item_len;
	WCHAR item[480*ARIB_CHAR_SIZE_RATIO+1];
} Eed_item_string_t;

typedef struct {
	unsigned int stream_type : 8;
	unsigned int pid : 16;
} PMT_pid_def_t;

/* �R���e���g�L�q�q (Content descriptor) */
typedef struct {
	unsigned int content_nibble_level_1 : 4;
	unsigned int content_nibble_level_2 : 4;
	unsigned int user_nibble_1 : 4;
	unsigned int user_nibble_2 : 4;
} Cd_t_item;

typedef struct {
	int n_items;
	Cd_t_item items[8]; /* TR-B14�̋K��ł͍ő�7 */
} Cd_t;

typedef struct {

	int status;

	/***** PAT,PMT *****/
	PSI_parse_t PMT_payload;
	uint32_t PMT_last_CRC;
	int n_service_pids;
	PMT_pid_def_t service_pids[MAX_PIDS_PER_SERVICE];
	unsigned int service_id : 16;

	/***** SDT *****/
	unsigned int network_id : 16;
	unsigned int ts_id : 16;

	Sd_string_t service_provider_name;
	Sd_string_t service_name;

	/***** EIT *****/
	int64_t last_ready_time;

	unsigned int event_id : 16;

	int curr_desc;
	int last_desc;

	int start_year;
	int start_month;
	int start_day;
	int start_hour;
	int start_min;
	int start_sec;
	int dur_hour;
	int dur_min;
	int dur_sec;

	/* �Z�`���C�x���g�L�q�q */
	Sed_string_t event_name;
	Sed_string_t event_text;

	/* �g���`���C�x���g�L�q�q */
	int n_items;
	Eed_item_string_t items[8];

	/* �R���e���g�L�q�q */
	Cd_t genre_info;

} proginfo_t;

MODULE_EXPORT_FUNC int get_extended_text(WCHAR *dst, size_t n, const proginfo_t *pi);
MODULE_EXPORT_FUNC void get_genre_str(const WCHAR **genre1, const WCHAR **genre2, Cd_t_item item);
MODULE_EXPORT_FUNC int proginfo_cmp(const proginfo_t *pi1, const proginfo_t *pi2);