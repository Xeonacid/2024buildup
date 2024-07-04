enum dtype_modbus_cmd {
	TYPE(MODBUS_CMD)=0x21150,
	TYPE(MODBUS_DATA)=0x21160
};
enum subtype_modbus_cmd {
	SUBTYPE(MODBUS_CMD,READ_COILS)=0x1,
	SUBTYPE(MODBUS_CMD,READ_DISCRETE_INPUTS),
	SUBTYPE(MODBUS_CMD,READ_HOLDING_REGISTERS),
	SUBTYPE(MODBUS_CMD,READ_INPUT_REGISTERS),
	SUBTYPE(MODBUS_CMD,WRITE_SINGLE_COIL),
	SUBTYPE(MODBUS_CMD,WRITE_SINGLE_REGISTER),
	SUBTYPE(MODBUS_CMD,READ_EXCEPTION_STATUS),
	SUBTYPE(MODBUS_CMD,WRITE_MULTIPLE_COILS)=0xf,
	SUBTYPE(MODBUS_CMD,WRITE_MULTIPLE_REGISTERS),
	SUBTYPE(MODBUS_CMD,REPORT_SLAVE_ID),
	SUBTYPE(MODBUS_CMD,MASK_WRITE_REGISTER)=0x16,
	SUBTYPE(MODBUS_CMD,WRITE_AND_READ_REGISTERS)
};

typedef struct modbus_cmd_read_coils{
	UINT16 start_addr;
	UINT16 reg_num;
}__attribute__((packed)) RECORD(MODBUS_CMD,READ_COILS);

typedef struct modbus_cmd_read_holding_reg{
	UINT16 start_addr;
	UINT16 reg_num;
}__attribute__((packed)) RECORD(MODBUS_CMD,READ_HOLDING_REGISTERS);

typedef struct modbus_cmd_read_input_reg{
	UINT16 start_addr;
	UINT16 reg_num;
}__attribute__((packed)) RECORD(MODBUS_CMD,READ_INPUT_REGISTERS);

typedef struct modbus_cmd_write_single_coil{
	UINT16 start_addr;
	UINT16 convert_value;
}__attribute__((packed)) RECORD(MODBUS_CMD,WRITE_SINGLE_COIL);

typedef struct modbus_cmd_write_single_reg{
	UINT16 start_addr;
	UINT16 convert_value;
}__attribute__((packed)) RECORD(MODBUS_CMD,WRITE_SINGLE_REGISTER);

enum subtype_modbus_data {
	SUBTYPE(MODBUS_DATA,READ_COILS)=0x1,
	SUBTYPE(MODBUS_DATA,READ_DISCRETE_INPUTS),
	SUBTYPE(MODBUS_DATA,READ_HOLDING_REGISTERS),
	SUBTYPE(MODBUS_DATA,READ_INPUT_REGISTERS),
	SUBTYPE(MODBUS_DATA,WRITE_SINGLE_COIL),
	SUBTYPE(MODBUS_DATA,WRITE_SINGLE_REGISTER),
	SUBTYPE(MODBUS_DATA,READ_EXCEPTION_STATUS),
	SUBTYPE(MODBUS_DATA,WRITE_MULTIPLE_COILS)=0xf,
	SUBTYPE(MODBUS_DATA,WRITE_MULTIPLE_REGISTERS),
	SUBTYPE(MODBUS_DATA,REPORT_SLAVE_ID),
	SUBTYPE(MODBUS_DATA,MASK_WRITE_REGISTER)=0x16,
	SUBTYPE(MODBUS_DATA,WRITE_AND_READ_REGISTERS)
};
typedef struct modbus_data_read_coils{
	BYTE area_bytes;
	BYTE * value;
}__attribute__((packed)) RECORD(MODBUS_DATA,READ_COILS);

typedef struct modbus_data_read_holding_reg{
	BYTE area_bytes;
	BYTE * value;
}__attribute__((packed)) RECORD(MODBUS_DATA,READ_HOLDING_REGISTERS);

typedef struct modbus_data_read_input_reg{
	BYTE area_bytes;
	BYTE * value;
}__attribute__((packed)) RECORD(MODBUS_DATA,READ_INPUT_REGISTERS);

typedef struct modbus_data_write_single_coil{
	UINT16 start_addr;
	UINT16 convert_value;
}__attribute__((packed)) RECORD(MODBUS_DATA,WRITE_SINGLE_COIL);

typedef struct modbus_data_write_single_reg{
	UINT16 start_addr;
	UINT16 convert_value;
}__attribute__((packed)) RECORD(MODBUS_DATA,WRITE_SINGLE_REGISTER);

