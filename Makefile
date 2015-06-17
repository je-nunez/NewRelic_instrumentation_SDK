
BUILD_DIR=$(HOME)/src/newrelic_agent_sdk_installation/

CC = gcc
CFLAGS = -g -Wall -I nr_agent_sdk_base_dir/include/
LDFLAGS = -L nr_agent_sdk_base_dir/lib/   -l  newrelic-transaction  -l  newrelic-common  -l newrelic-collector-client -l pthread


.SILENT:  help


help:
	echo "Makefile help"	
	echo -e "Possible make targets:\n"	
	echo "    make test_newrelic_instrum_api"	
	echo -e "         Build the test program with the embedded NewRelic instrumentation.\n"	
	echo "    make run_a_test"	
	echo -e "         Build the test program -if necessary- and run it (Requires before that the environment variable NEW_RELIC_LICENSE_KEY had been externally set and exported"	
	echo -e "         like in an: 'export NEW_RELIC_LICENSE_KEY=my_NewRelic_License_KEy' in sh/bash, before running 'make run_a_test'.)\n"	
	echo "    make install_newrelic_agent_sdk"	
	echo -e "         Install the NewRelic Agent SDK in $(BUILD_DIR)\n"
	echo "    make clean"	
	echo -e "         Remove compiled and binary-object files.\n"	
	echo "    make help"	
	echo -e "         Offers these help instructions.\n"	


test_newrelic_instrum_api: test_newrelic_instrum_api.c
	cp test_newrelic_instrum_api.c $(BUILD_DIR)/
	cd $(BUILD_DIR) && \
	   $(CC)  $(CFLAGS) -o  test_newrelic_instrum_api  test_newrelic_instrum_api.c  $(LDFLAGS)



run_a_test: test_newrelic_instrum_api
	cd $(BUILD_DIR) && \
	   export LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):$(BUILD_DIR)/nr_agent_sdk_base_dir/lib/  && \
	   ./test_newrelic_instrum_api     $(NEW_RELIC_LICENSE_KEY)


install_newrelic_agent_sdk:
	-download_NewRelic_Agent_SDK.sh


.PHONY : clean


clean:
	-rm -f $(BUILD_DIR)/test_newrelic_instrum_api


