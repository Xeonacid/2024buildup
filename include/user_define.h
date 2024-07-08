enum dtype_user_define {
	TYPE(USER_DEFINE)=0x1010,
};
enum subtype_user_define {
	SUBTYPE(USER_DEFINE,LOGIN)=0x1,
	SUBTYPE(USER_DEFINE,CLIENT_STATE),
	SUBTYPE(USER_DEFINE,SERVER_STATE),
	SUBTYPE(USER_DEFINE,RETURN),
  SUBTYPE(USER_DEFINE,LOGOUT)
};

enum enum_plc_role_type
{
	PLC_ENGINEER=0x01,
	PLC_OPERATOR,
	PLC_MONITOR,
	PLC_DEVICE

};
enum enum_user_state
{
	WAIT=0x01,
	REQUEST,
	RESPONSE,
	LOGIN,
	CONNECT,
  LOGOUT,
	ERROR
};

enum enum_login_state
{
	SUCCEED=0x01,
	CHALLENGE,
	INVALID,
	NOUSER,
	AUTHFAIL,
  LOGIN_SUCCEED,
  LOGIN_FAILED,
  LOGOUT_SUCCEED,
  LOGOUT_FAILED,
  NOAUTH,
	NOACCESS
};

typedef struct user_define_login{
	char * user_name;
	char passwd[32];
	char proc_name[32];
	BYTE machine_uuid[32];
}__attribute__((packed)) RECORD(USER_DEFINE,LOGIN);

typedef struct user_define_client_state{
	char * user_name;
	UINT32 curr_state;
  	BYTE nonce[32];
	char * user_info;
}__attribute__((packed)) RECORD(USER_DEFINE,CLIENT_STATE);

typedef struct user_define_server_state{
	char * user_name;
	enum enum_plc_role_type role;
	BYTE node_uuid[32];
	char proc_name[32];
	char * passwd;
	enum enum_login_state curr_state;
  BYTE nonce[32];
}__attribute__((packed)) RECORD(USER_DEFINE,SERVER_STATE);

typedef struct user_define_return{
	enum enum_login_state return_code;
	char * return_info;
  	BYTE nonce[32];
}__attribute__((packed)) RECORD(USER_DEFINE,RETURN);

typedef struct user_define_logout{
	char * user_name;
}__attribute__((packed)) RECORD(USER_DEFINE,LOGOUT);
