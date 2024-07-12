dirs= file_replace  login_hacker modbus_monitor modbus_zuc_crypt monitor_shield operator_shield role_access role_verify sm2sign
all:
	$(foreach N,$(dirs),make -C $(N) || exit "$$?";)
clean:
	rm -f ../plugin/*.so ../plugin/*.cfg
	$(foreach N,$(dirs),make clean -C $(N);)
	rm -f *~


