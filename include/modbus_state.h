enum dtype_modbus_state {
	TYPE(MODBUS_STATE)=0x21101
};
enum subtype_modbus_state {
	SUBTYPE(MODBUS_STATE,MASTER)=0x1,
	SUBTYPE(MODBUS_STATE,SLAVE),
	SUBTYPE(MODBUS_STATE,SERVER),
	SUBTYPE(MODBUS_STATE,CHANNEL),
	SUBTYPE(MODBUS_STATE,CLIENT),
	SUBTYPE(MODBUS_STATE,RELATE)
};

enum enum_modbus_state_machine
{
   MODBUS_START=0x01,
   MODBUS_CONNECT,
   MODBUS_QUERY,
   MODBUS_RESPONSE,
   ERROR=0x10
};
typedef struct modbus_master_state{
	char master_name[32];
	BYTE unit_num;
	BYTE * slave_uuid;
}__attribute__((packed)) RECORD(MODBUS_STATE,MASTER);

typedef struct modbus_slave_state{
	char slave_name[32];
	BYTE ip[4];
    UINT16 port;
	BYTE unit_addr;
	UINT16 state_machine;
	BYTE slave_key[32];
}__attribute__((packed)) RECORD(MODBUS_STATE,SLAVE);

typedef struct modbus_server_state{
	char server_name[32];
	BYTE unit_addr;
	UINT16 state_machine;
	BYTE server_key[32];
}__attribute__((packed)) RECORD(MODBUS_STATE,SERVER);

typedef struct modbus_channel_state{
	char client_name[32];
	BYTE unit_addr;
	BYTE client_ip[4];
	UINT16 port;
	UINT16 state_machine;
    UINT16 no;
	BYTE client_key[32];
}__attribute__((packed)) RECORD(MODBUS_STATE,CHANNEL);

typedef struct modbus_client_state{
	char client_name[32];
	char server_name[32];
	BYTE unit_addr;
	UINT16 state_machine;
    UINT16 no;
	BYTE client_key[32];
}__attribute__((packed)) RECORD(MODBUS_STATE,CLIENT);

typedef struct modbus_client_relate{
	BYTE unit_addr;
    	UINT16 no;
	BYTE msg_uuid[32];
}__attribute__((packed)) RECORD(MODBUS_STATE,RELATE);
