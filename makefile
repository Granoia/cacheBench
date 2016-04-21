client: client.c workload_sim.c
	gcc -o work -I. -I./tools workload_sim.c client.c nanomsg/.libs/libnanomsg.a jsmn/libjsmn.a -lanl -pthread
