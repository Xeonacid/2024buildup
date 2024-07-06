enum dtype_score_compute {
	TYPE(SCORE_COMPUTE)=0x5221
};
enum subtype_score_compute {
	SUBTYPE(SCORE_COMPUTE,ITEM_STANDARD)=0x1,
	SUBTYPE(SCORE_COMPUTE,ITEM_STATE),
	SUBTYPE(SCORE_COMPUTE,EVENT_STANDARD),
	SUBTYPE(SCORE_COMPUTE,EVENT),
	SUBTYPE(SCORE_COMPUTE,SUM)
};
enum score_event_type
{
	SCORE_EVENT_START=0x01,
	SCORE_EVENT_ADD,
	SCORE_EVENT_DEC,
	SCORE_EVENT_EXIST,
	SCORE_EVENT_COUNT,
	SCORE_EVENT_VACANCY,
	SCORE_EVENT_RESET,
	SCORE_EVENT_BONUS,
	SCORE_EVENT_PENALTY,
	SCORE_EVENT_OUTPUT
};
enum score_event_result
{
	SCORE_RESULT_SUCCEED=0x01,
	SCORE_RESULT_FIND,
	SCORE_RESULT_PROTECT,
	SCORE_RESULT_WARNING,
	SCORE_RESULT_TRACE,
	SCORE_RESULT_HAPPEN,
	SCORE_RESULT_FAIL=0x100,
};
typedef struct score_compute_item_standard{
	char * name;
	int init_score;
	int lowest_score;
	int highest_score;
	int event_num;
	char * desc;
}__attribute__((packed)) RECORD(SCORE_COMPUTE,ITEM_STANDARD);

typedef struct score_compute_item_state{
	char * name;
	int curr_score;
	int bonus_score;
	int penalty_score;
	int event_count;
}__attribute__((packed)) RECORD(SCORE_COMPUTE,ITEM_STATE);

typedef struct score_compute_event_standard{
	char * name;
	char * item_name;
	UINT32 event_type;
	UINT32 result;
	int score;
	char * desc;
}__attribute__((packed)) RECORD(SCORE_COMPUTE,EVENT_STANDARD);

typedef struct score_compute_event{
	char * name;
	char * item_name;
	UINT32 result;
}__attribute__((packed)) RECORD(SCORE_COMPUTE,EVENT);
