dirs= modbus_channel modbus_master modbus_slave thermostat_device login_user login_server logic_upload logic_store \
      	plc_upload monitor_manage audit_store audit_query cmd_process
all:
	$(foreach N,$(dirs),make -C $(N) || exit "$$?";)
clean:
	rm -f ../plugin/*.so ../plugin/*.cfg
	$(foreach N,$(dirs),make clean -C $(N);)
	rm -f *~


