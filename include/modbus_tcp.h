#ifndef MODBUS_TCP_H
#define MODBUS_TCP_H

enum dtype_modbus_tcp {
	TYPE(MODBUS_TCP)=0x21120
};
enum subtype_modbus_tcp {
	SUBTYPE(MODBUS_TCP,MBAP)=0x1,
	SUBTYPE(MODBUS_TCP,DATAGRAM)
};

enum modbus_protocol_id
{
     READ_COILS=0x01,
     READ_DISCRETE_INPUTS,
     READ_HOLDING_REGISTERS,
     READ_INPUT_REGISTERS,
     WRITE_SINGLE_COIL,
     WRITE_SINGLE_REGISTER,
     READ_EXCEPTION_STATUS,
     WRITE_MULTIPLE_COILS=0x0F,
     WRITE_MULTIPLE_REGISTERS=0x10,
     REPORT_SLAVE_ID=0x11,
     MASK_WRITE_REGISTER=0x16,
     WRITE_AND_READ_REGISTERS=0x17

};

typedef struct record_modbus_mbap{
	UINT16 trans_id;
	UINT16 protocol_id;
	BYTE length_hi;
	BYTE length_lo;
}__attribute__((packed)) RECORD(MODBUS_TCP,MBAP);

typedef struct record_modbus_datagram{
	BYTE datasize;
	BYTE unit_id;
	BYTE function;
	BYTE * data;
}__attribute__((packed)) RECORD(MODBUS_TCP,DATAGRAM);

#endif 
