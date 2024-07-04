
enum dtype_plc_device {
	TYPE(PLC_DEVICE)=0x22020
};
enum subtype_plc_device {
	SUBTYPE(PLC_DEVICE,REGISTER)=0x1,
};

enum enum_modbus_register_type
{
    COILS=1,
    DISCRETE_INPUT,
    INPUT_REGISTER,
    HOLDING_REGISTER

};

typedef struct plc_device_register{
	char * plc_devname;
	enum enum_modbus_register_type type;
	int  addr;
	char * register_desc;
}__attribute__((packed)) RECORD(PLC_DEVICE,REGISTER);
