
BUILD_DIR=$(HOME)/src/newrelic_agent_sdk_installation/

CC = gcc
CFLAGS = -g -Wall -I nr_agent_sdk-v0.16.1.0-beta.x86_64/include/
LDFLAGS = -L nr_agent_sdk-v0.16.1.0-beta.x86_64/lib/   -l  newrelic-transaction  -l  newrelic-common  -l newrelic-collector-client -l pthread


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
	   $(CC) $(CFLAGS) $(LDFLAGS) -o  test_newrelic_instrum_api  test_newrelic_instrum_api.c



run_a_test: test_newrelic_instrum_api
	cd $(BUILD_DIR) && \
	   export LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):$(BUILD_DIR)/nr_agent_sdk-v0.16.1.0-beta.x86_64/lib/  && \
	   ./test_newrelic_instrum_api     $(NEW_RELIC_LICENSE_KEY)


install_newrelic_agent_sdk:
	-mkdir -p $(BUILD_DIR)
	curl  http://download.newrelic.com/agent_sdk/nr_agent_sdk-v0.16.1.0-beta.x86_64.tar.gz -o  $(BUILD_DIR)/nr_agent_sdk-v0.16.1.0-beta.x86_64.tar.gz
	echo  -e "\n------ Extracting $(BUILD_DIR)/nr_agent_sdk-v0.16.1.0-beta.x86_64.tar.gz ...\n"
	cd $(BUILD_DIR) && \
	   tar  -xzf  nr_agent_sdk-v0.16.1.0-beta.x86_64.tar.gz
	echo  -e "\n------ $(BUILD_DIR) is ready.\n------ Preparing NewRelic logging settings in $(HOME)/.newrelic/log4cplus.properties ...\n"
	-mkdir  $(HOME)/.newrelic/
	cp   $(BUILD_DIR)/nr_agent_sdk-v0.16.1.0-beta.x86_64/config/log4cplus.properties  $(HOME)/.newrelic/
	-sed -i 's#log4cplus.logger.com.newrelic=info,#log4cplus.logger.com.newrelic=debug,#'   $(HOME)/.newrelic/log4cplus.properties



.PHONY : clean


clean:
	-rm -f $(BUILD_DIR)/test_newrelic_instrum_api


