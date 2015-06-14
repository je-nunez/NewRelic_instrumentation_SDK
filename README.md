# NewRelic Instrumentation SDK

Example of the `NewRelic Instrumentation SDK` in C, with a test application instrumented in the `NewRelic SDK embedded mode` (ie., no external NewRelic collector daemon to sample the application).

# WIP

This project is a *work in progress*. The implementation is *incomplete* and subject to change. The documentation can be inaccurate.

This is the first version of this document.

# Create a free NewRelic account

To run this test application, you need to create (or have already) a NewRelic account.
If you don't have one already, you may create a free NewRelic account:

    http://newrelic.com/application-monitoring/pricing

Choose the 'Lite' option.

# How to install the NewRelic Agent SDK and compile this application

To compile this test application you need to download and install first the `NewRelic Agent SDK` for C/C++:

    $ make install_newrelic_agent_sdk

This will download and install the NewRelic Agent SDK under `$HOME/src/newrelic_agent_sdk_installation/`, and create
also a new subdirectory `$HOME/.newrelic/` and file `$HOME/.newrelic/log4cplus.properties` with the debug log settings
(which write to `standard-error` and to `/tmp/newrelic-*.log` files).

To build the test application which calls the embedded NewRelic instrumenstation, run:

    $ make test_newrelic_instrum_api

It will copy the source file `test_embedded_newrelic_instrum.c` to `$HOME/src/newrelic_agent_sdk_installation/` and
compile it there.

To run the test application:

    $ export NEW_RELIC_LICENSE_KEY="the_code_of_my_NewRelic_License_Key"

    $ make run_a_test

Note that the environment variable `NEW_RELIC_LICENSE_KEY` is referred by the Makefile, but not by the source code
`test_embedded_newrelic_instrum.c` itself.

The test program what it does is a `find / -perm -100 -printf '%s %p'` (it could have been any other test action)
and then sorts it to find the biggest executable in the system. It records segments times in the NewRelic transaction
and also some custom Linux kernel measures.

More importantly, since this test program uses a `find / -perm -100 ...`, be careful with running this test program
on a system with too many files, or has mounted some remote filesystems (like NFS), etc. Or modify the test program
around the variable `external_command` inside it to run your test task you want to measure.

(Note: In reference to the custom Linux measures that this program sends to NewRelic, if you want, it is possible
that your test be a `perf record -g ...` instead of the two NewRelic segments here, `find / -perm -100 -printf '%s %p'`
and then sorting by size, so with `perf record -g ...` the segments inside the NewRelic transaction can be the
duration of each stack frame in the call stack, and then letting NewRelic do a flame-graph per segment (stack-frame).
The posibilities are endless, but this test program is just for the capabilities of instrumenting your application
with the `NewRelic Agent SDK in embedded mode`, so it can't be a complex program.)


# NewRelic SDK Documentation

The documentation for the NewRelic Agent SDK in C/C++ is near the bottom of:

     https://docs.newrelic.com/docs/agents

(In this URL, above SDK entries, are the documentation for the instrumentation into NewRelic of your Java, Python, PHP, .Net, and Ruby applications.)

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


