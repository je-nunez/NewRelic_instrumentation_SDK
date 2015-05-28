# NewRelic_instrumentation_SDK

Example of the `NewRelic Instrumentation SDK` in C

# WIP

This project is a *work in progress*. The implementation is *incomplete* and subject to change. The documentation can be inaccurate.

This is the first version of this document.

# How to install

Download the `NewRelic Instrumentation SDK` for C/C++:

    NR_DEST_DIR=~/src/newrelic_agent_sdk_installation/
    
    mkdir  -p "${NR_DEST_DIR}"
    curl  http://download.newrelic.com/agent_sdk/nr_agent_sdk-v0.16.1.0-beta.x86_64.tar.gz \
          -o  "${NR_DEST_DIR}"/nr_agent_sdk-v0.16.1.0-beta.x86_64.tar.gz
    
    cd   "${NR_DEST_DIR}"
    tar  xzf  nr_agent_sdk-v0.16.1.0-beta.x86_64.tar.gz

Copy the NewRelic SDK's `log4cplus.properties` to standard location (could be left in current directory where the instrumented program runs):

     mkdir  ~/.newrelic/
     cp   "${NR_DEST_DIR}"/nr_agent_sdk-v0.16.1.0-beta.x86_64/config/log4cplus.properties \
          ~/.newrelic/
      
     # Optionally, increase the debug level in this 'log4cplus.properties'
     # (debug messages are written by default to files in /tmp/ -besides to stdout)
     #
     ##     Substitute in ~/.newrelic/log4cplus.properties:
     ##                log4cplus.logger.com.newrelic=info, stdout
     ##     to:
     ##                log4cplus.logger.com.newrelic=debug, stdout

Compile the test instrumented program (`Makefile` will be due shortly):

     cc  -o  test_embedded_newrelic_instrum  \
             test_embedded_newrelic_instrum.c  \
          -I  "${NR_DEST_DIR}"/nr_agent_sdk-v0.16.1.0-beta.x86_64/include/  \
          -L  "${NR_DEST_DIR}"/nr_agent_sdk-v0.16.1.0-beta.x86_64/lib/  \
          -l  newrelic-transaction  -l  newrelic-common  -l newrelic-collector-client 

Run the instrumented program:

     export LD_LIBRARY_PATH="$LD_LIBRARY_PATH":"${NR_DEST_DIR}"/nr_agent_sdk-v0.16.1.0-beta.x86_64/lib/
     ./test_embedded_newrelic_instrum


Optional:

     rpm -Uvh https://yum.newrelic.com/pub/newrelic/el5/x86_64/newrelic-repo-5-3.noarch.rpm
     yum install newrelic-sysmond


# Some scattered notes about the NewRelic SDK in the first version of this document


In the New Relic Agent SDK .pdf document (which is inside the tar-ball downloaded in the `How to install` section above):

"In order to maintain communication with New Relic, the Agent SDK requires a thread to be started that harvests data once per minute. This means that your app will need to be able to start a thread, which will run continuously as long as transactions are being executed.

For example, if you are running perl apps through Apache, then each perl script is a short-lived process that exits upon completion of the script. There is no ability to startup a thread that lives beyond the life of the script.

To solve this problem, we designed the SDK so that your agent can run in two different modes: daemon-mode or embedded-mode. In daemon-mode, you'll run the newrelic-collector-client-daemonthat came with the Agent SDK as a stand-alone process. The daemon will collect and send data to New Relic. In embedded-mode your application will start a thread, which will have the same responsibilities as the daemon but will be running within the same process as your transactions.

All functions in the SDK return integers or longs. Any positive number indicates success and any negative number indicates a failure.
"


Apdex T:

Apdex T is the response time threshold used to measure customer satisfaction. The transaction trace threshold is calculated as four times the Apdex T value. The default Apex T value is 0.5 seconds.
You can change the Apdex T used across all transactions of your application using the environment variable NEWRELIC_APP_SERVER_APDEX_T. If you don't set this environment variable, the Apdex T will default to 0.5 seconds.
If you want to change the Apdex T for a specific transaction, go to New Relic and make it a Key Transaction. This will give you the option of changing its Apdex T. Once it's been saved, your agent will be notified the next time it's ready to send data up to New Relic.


Transaction traces are automatically generated when transactions exceed the transaction trace threshold (4 x ApdexT). ApdexT has a default value of 0.5 seconds, so the default trace threshold for a transaction is 2 seconds.
The process to create a Transaction Trace is simple:
* If the transaction duration is longer than the transaction trace threshold, then a trace will be collected.
* Note: a maximum of two traces will be sent to New Relic per minute.

