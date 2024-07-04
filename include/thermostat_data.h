enum dtype_thermostat_data {
	TYPE(THERMOSTAT_DATA)=0x6200
};
enum subtype_thermostat_data {
	SUBTYPE(THERMOSTAT_DATA,MACHINE_SWITCH)=0x1,
	SUBTYPE(THERMOSTAT_DATA,MACHINE_STATE),
	SUBTYPE(THERMOSTAT_DATA,TARGET_REG),
	SUBTYPE(THERMOSTAT_DATA,CURR_T),
	SUBTYPE(THERMOSTAT_DATA,HEATING_REG)
};
typedef struct machine_switch{
	BYTE coil;
	BYTE r_w;
}__attribute__((packed)) RECORD(THERMOSTAT_DATA,MACHINE_SWITCH);

typedef struct machine_state{
	BYTE machine_switch;
	UINT16 target_t;
	UINT16 current_t;
	BYTE heating_switch;
	BYTE heating_gear;
}__attribute__((packed)) RECORD(THERMOSTAT_DATA,MACHINE_STATE);

typedef struct target_register{
	UINT16 target_t;
	BYTE r_w;
}__attribute__((packed)) RECORD(THERMOSTAT_DATA,TARGET_REG);

typedef struct current_temperature{
	UINT16 current_t;
}__attribute__((packed)) RECORD(THERMOSTAT_DATA,CURR_T);

typedef struct heating_register{
	BYTE coil;
	BYTE gear;
	BYTE r_w;
}__attribute__((packed)) RECORD(THERMOSTAT_DATA,HEATING_REG);

