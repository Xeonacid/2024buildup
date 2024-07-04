
enum dtype_plc_engineer {
	TYPE(PLC_ENGINEER)=0x22010,
	TYPE(PLC_OPERATOR)=0x22011,
	TYPE(PLC_MONITOR)=0x22012
};
enum subtype_plc_engineer {
	SUBTYPE(PLC_ENGINEER,LOGIC_CODE)=0x1,
	SUBTYPE(PLC_ENGINEER,LOGIC_BIN),
	SUBTYPE(PLC_ENGINEER,LOGIC_UPLOAD),
	SUBTYPE(PLC_ENGINEER,LOGIC_RETURN)
};

enum enum_plc_file_type
{
    FILE_CODE=1,
    FILE_LOGIC,
    FILE_DESC=0x10,
    FILE_RULE=0x20

};

typedef struct plc_logic_code{
	char * plc_devname;
	char * logic_filename;
	BYTE code_uuid[32];
	char * author;
	int time;
}__attribute__((packed)) RECORD(PLC_ENGINEER,LOGIC_CODE);

typedef struct plc_logic_bin{
	char * plc_devname;
	char * logic_filename;
	BYTE code_uuid[32];
	BYTE bin_uuid[32];
	char * author;
	int time;
}__attribute__((packed)) RECORD(PLC_ENGINEER,LOGIC_BIN);

typedef struct plc_logic_upload{
	char * plc_devname;
	char * logic_filename;
	int  type;
	BYTE uuid[32];
	char * author;
	char * uploader;
	int time;
}__attribute__((packed)) RECORD(PLC_ENGINEER,LOGIC_UPLOAD);

typedef struct plc_logic_return{
	char * plc_devname;
	char * logic_filename;
	BYTE uuid[32];
	char * author;
	int result;
	int time;
}__attribute__((packed)) RECORD(PLC_ENGINEER,LOGIC_RETURN);

enum subtype_plc_operator {
	SUBTYPE(PLC_OPERATOR,PLC_CMD)=0x1,
	SUBTYPE(PLC_OPERATOR,PLC_RETURN)
};

enum enum_plc_operator_action
{
    ACTION_ON=1,
    ACTION_OFF,
    ACTION_MONITOR,
    ACTION_ADJUST

};
typedef struct plc_operator_cmd{
	char * plc_devname;
	UINT32 action;
	char * action_desc;
	int value;
	char * plc_operator;
	int time;
}__attribute__((packed)) RECORD(PLC_OPERATOR,PLC_CMD);

typedef struct plc_operator_return{
	char * plc_devname;
	UINT32 action;
	char * action_desc;
	int value;
	int result;
	int time;
}__attribute__((packed)) RECORD(PLC_OPERATOR,PLC_RETURN);

enum subtype_plc_monitor {
	SUBTYPE(PLC_MONITOR,PLC_CMD)=0x1,
	SUBTYPE(PLC_MONITOR,PLC_AUDIT),
	SUBTYPE(PLC_MONITOR,PLC_RETURN),
};

enum enum_plc_monitor_action
{
    ACTION_WARNING=0x100,
    ACTION_AUDIT,
    ACTION_CONTROL
};
typedef struct plc_monitor_cmd{
	char * plc_devname;
	UINT32 action;
	char * action_desc;
	int value1;
	int value2;
	char * plc_monitor;
	int time;
}__attribute__((packed)) RECORD(PLC_MONITOR,PLC_CMD);

typedef struct plc_monitor_audit{
	char plc_devname[DIGEST_SIZE];
	int user_type;
	int start_time;
	int event_num;
	int end_time; 
}__attribute__((packed)) RECORD(PLC_MONITOR,PLC_AUDIT);

typedef struct plc_monitor_return{
	char * plc_devname;
	UINT32 action;
	char * action_desc;
	int value;
	int result;
	int time;
}__attribute__((packed)) RECORD(PLC_MONITOR,PLC_RETURN);

