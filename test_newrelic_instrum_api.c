
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/stat.h>


#include "newrelic_common.h"
#include "newrelic_transaction.h"
#include "newrelic_collector_client.h"


/* The maximum length of a New Relic identifier.
 *
 * In:
 *
 *  https://docs.newrelic.com/docs/agents/php-agent/configuration/php-agent-api
 *
 * it is mentioned:
 *
 *  newrelic_record_custom_event:
 *   ...
 *   The attributes parameter is expected to be an associative array: the keys
 *   should be the attribute names (which may be up to 255 characters in length)
 *   ...
 *
 * so we limit our custom attributes also to be at most 255 chars in length
 *
 * The URL above also mentions on the NAMING CONVENTIONS for your custom
 * attributes:
 *
 *  function newrelic_custom_metric (...)
 *
 *    ... Your custom metrics can then be used in custom dashboards and custom
 *    views in the New Relic user interface. Name your custom metrics with a
 *    Custom/ prefix (for example, Custom/MyMetric). This will make them easily
 *    usable in custom dashboards...
 *
 * Also, on the NAMING CONVENTIONS for your custom attributes (URL below exceeds
 * 80 columns):
 *
 *  https://docs.newrelic.com/docs/insights/new-relic-insights/decorating-events/insights-custom-attributes#keywords
 *
 * it lists some reserved words that New Relic understands, which should not be
 * used as attribute names:
 *
 *     The following words are used by NRQL and by Insights. Avoid using
 *     them as names for attributes. Otherwise the results may not be
 *     what was expected.
 *      .... [List of reserved words by New Relic] ....
 *
 * we add the preffix "ct_" to our attribute names to avouid any accidental
 * conflict with New Relic of some identifier returned by the Linux Performance
 * Counters.
 */


int
usage_and_exit(void);


void*
main_worker_function(void* p);


void
record_newrelic_metric_from_first_value_of_kernel_stat_file(char * kernel_file,
                char * newrelic_metric);


int
generate_lists_of_files(const char* tmp_file, long newrelic_transaction);


int
process_lists_of_files(const char* tmp_file, long newrelic_transaction,
                       long* max_exec_size_found, char* max_exec_fname);


int
main(int argc, char** argv)
{
    if (argc != 2 ||
          strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
        usage_and_exit();

    char newrelic_license_key[256];
    strncpy(newrelic_license_key, argv[1], sizeof newrelic_license_key);

    newrelic_register_message_handler(newrelic_message_handler);

    newrelic_init(newrelic_license_key, "My Application", "C", "4.8");
    // newrelic_enable_instrumentation(1);  /* 1 is enable */


    /* create the app worker thread to execute "main_worker_function()" */

    /*
     * Our pthread_create() -commented, below- doesn't seem to be strictly
     * necessary: it is the embedded mode of the NewRelic SDK the one
     * which starts the SDK engine in a separate pthread (at least in the
     * NewRelic Agent SDK version 0.16.1), and tracing reveals that this
     * NewRelic thread is the one which connects to the NewRelic
     * collector site ("collector*.newrelic.com") and sends the stats
     * to it.
     *

    pthread_t worker_thread;
    int err = pthread_create(&worker_thread, NULL, main_worker_function, NULL);
    if (err != 0)) {
        fprintf(stderr, "ERROR: couldn't create our worker thread:"
                        " error code %d\n", err);
        return 1;
    }
    */

    main_worker_function(NULL);

    /* wait for the application worker thread to finish */
    /*
    if(pthread_join(worker_thread, NULL)) {
        fprintf(stderr, "Error joining thread\n");
        return 2;
    }
    */

    return 0;
}


void
send_error_notice_to_NewRelic(long transaction_id, const char *exception_type,
                              const char *error_message)
{
    fprintf(stderr, "ERROR: %s: %s\n", exception_type, error_message);

    int err_code;
    err_code = newrelic_transaction_notice_error(transaction_id, exception_type,
                                                 error_message, "", "");
    if (err_code != 0)
        fprintf(stderr, "ERROR: Couldn't send error message to New Relic: "
                        "returned error %d\n", err_code);
}


void*
main_worker_function(void *p)
{
    int return_code;

    fprintf(stderr, "DEBUG: about to call newrelic_transaction_begin()\n");
    long newr_transxtion_id = newrelic_transaction_begin();
    if (newr_transxtion_id < 0) {
        fprintf(stderr, "ERROR: newrelic_transaction_begin() returned %ld\n",
                        newr_transxtion_id);
        exit(1);  /* No point continuing without a NewRelic transaction */
    }

    /* Naming transactions is optional.
     * Use caution when naming transactions. Issues come about when the
     * granularity of transaction names is too fine, resulting in hundreds if
     * not thousands of different transactions. If you name transactions
     * poorly, your application may get blacklisted due to too many metrics
     * being sent to New Relic.
     */
    return_code = newrelic_transaction_set_type_other(newr_transxtion_id);
    if (return_code < 0)
       fprintf(stderr, "ERROR: newrelic_transaction_set_type_other() "
                       "returned %d\n", return_code);

    return_code = newrelic_transaction_set_name(newr_transxtion_id,
                                               "find_biggest_executable");
    if (return_code < 0)
       fprintf(stderr, "ERROR: newrelic_transaction_set_name() "
                       "returned %d\n", return_code);

    return_code = newrelic_transaction_set_category(newr_transxtion_id,
                                                "BackendTrans/Stat/find");
    if (return_code < 0)
       fprintf(stderr, "ERROR: newrelic_transaction_set_category() "
                       "returned %d\n", return_code);

    /* Record an attribute for this transaction: the start time */
    char start_time[16];
    snprintf(start_time, sizeof start_time, "%u",
               (unsigned)time(NULL));

    return_code = newrelic_transaction_add_attribute(newr_transxtion_id,
                                           "ct_tx_start_time", start_time);

    /*
     * We can have used:
     *    find_pipe = popen("/usr/bin/find / -perm -100 ...", "r");
     *    if (find_pipe) {
     *       ...
     *        ret = pclose(find_pipe);
     *        if (ret < 0) {
     *                // find failed
     *        }
     *
     * but for this NewRelic example we would like to split the pipe
     * in two disjoint NewRelic segments, one NR segment finding the
     * files, the other processing the output
     */

    char* temp_file = tempnam("/tmp", "myapp_");
    return_code = generate_lists_of_files(temp_file, newr_transxtion_id);
    // fprintf(stderr, "generate_lists_of_files() returned %d\n", return_code);

    if (return_code == 0) {
        long max_fsize;
        char max_fname[PATH_MAX];
        process_lists_of_files(temp_file, newr_transxtion_id, &max_fsize,
                               max_fname);
    }

    /* Clean-up: remove temporary file */
    struct stat buf;
    if (stat(temp_file, &buf) == 0) {
       return_code = unlink(temp_file);
       if (return_code != 0) {
          char err_msg[256];
          strerror_r(errno, err_msg, sizeof err_msg);
          send_error_notice_to_NewRelic(newr_transxtion_id, "unlink_temp_file",
                                        err_msg);
       }
    }
    free(temp_file);

    /* Finnish the NewRelic transaction */
    if (newr_transxtion_id >= 0) {
         fprintf(stderr, "DEBUG: about to call newrelic_transaction_end()\n");
         return_code = newrelic_transaction_end(newr_transxtion_id);
         if (return_code < 0)
         fprintf(stderr, "ERROR: newrelic_transaction_end() "
                         "returned %d\n", return_code);
    }

    return NULL;
}


int
generate_lists_of_files(const char* tmp_file, long newrelic_transaction)
{
    long newr_segm_external_find = 0;
    if (newrelic_transaction >= 0) {
        newr_segm_external_find =
                newrelic_segment_external_begin(newrelic_transaction,
                                                NEWRELIC_ROOT_SEGMENT,
                                                "localhost", "find");
        if (newr_segm_external_find < 0)
            fprintf(stderr, "ERROR: newrelic_segment_external_begin() "
                            "returned %ld\n", newr_segm_external_find);
    }

    char external_command[32*1024];
    snprintf(external_command, sizeof external_command,
             "/usr/bin/find  / -type f -perm -100 -printf '%%s %%p\\n' "
             "> %s 2>/dev/null", tmp_file);

    fprintf(stderr, "Running: %s\n", external_command);

    int ext_cmd_ret = system(external_command);

    /* Just after running the "external_command" (find / -perm -100 ...)
     * record two custom measures up to NewRelic, as an example.
     * Ie., how busy was the cache left.
     * TODO: the load left on the VM/disks. But the disks and
     * filesystems can be different, so this is not an simple
     * task for this NewRelic SDK test.
     */

     record_newrelic_metric_from_first_value_of_kernel_stat_file(
            "/proc/sys/fs/dentry-state",
            "Custom/ct_cached_dentries_metric"
          );

     record_newrelic_metric_from_first_value_of_kernel_stat_file(
            "/proc/sys/fs/inode-state",
            "Custom/ct_cached_inodes_metric"
          );

    /* end this NewRelic segment */
    if (newrelic_transaction >= 0 && newr_segm_external_find >= 0) {
        int ret_code =  newrelic_segment_end(newrelic_transaction,
                                             newr_segm_external_find);
        if (ret_code < 0)
            fprintf(stderr, "ERROR: newrelic_segment_end() returned %d\n",
                    ret_code);
    }

    /* The `find` command will exit non-zero if it doesn't have permission
     * to enter into a subdirectory, and similar cases with permissions
     * -that is why the `find` command above is executed with '2>/dev/null'.
     * So WEXITSTATUS(ext_cmd_ret), for the `find` case, may return errors
     * which the user normally ignores.
     */

    return ( ext_cmd_ret != -1 ) ?  0: -1;
}


void
record_newrelic_metric_from_first_value_of_kernel_stat_file(char * kernel_file,
                char * newrelic_metric)
{
     int error_code;

     FILE* f = fopen(kernel_file, "r");
     if (!f)  {
         char err_msg[1024];
         strerror_r(errno, err_msg, sizeof err_msg);
         fprintf(stderr, "ERROR: fopen(%s) returned: %s\n", kernel_file,
                         err_msg);
         return;
     }

     char line[1024];
     if (fgets(line, sizeof line, f) != NULL) {
             long interesting_kernel_stat;
             char rest[1024];
             sscanf (line, "%ld %s", &interesting_kernel_stat, rest);

             error_code = newrelic_record_metric(newrelic_metric,
                                                 interesting_kernel_stat);
             if (error_code != 0)
                 fprintf(stderr, "ERROR: newrelic_record_metric(%s) "
                                 "returned %d\n", newrelic_metric,
                                 error_code);
     }

     fclose(f);
}


int
process_lists_of_files(const char* tmp_file, long newrelic_transaction,
                       long* max_exec_size_found, char* max_exec_fname)
{
    long newr_segm_internal_proc = 0;
    if (newrelic_transaction >= 0) {
        newr_segm_internal_proc =
                newrelic_segment_generic_begin(newrelic_transaction,
                                              NEWRELIC_ROOT_SEGMENT,
                                           "processing_list_execs");
        if (newr_segm_internal_proc < 0)
            fprintf(stderr, "ERROR: newrelic_segment_external_begin() "
                            "returned %ld\n", newr_segm_internal_proc);
    }

    long curr_max_fsize = -1;
    char curr_max_fname[PATH_MAX];

    FILE *f;
    f = fopen(tmp_file, "r");
    if (f) {
        char line[PATH_MAX+32];
        long fsize;
        char fname[PATH_MAX];
        long numb_lines = 0;

        while (fgets(line, sizeof line, f) != NULL) {
             sscanf (line, "%ld %s", &fsize, fname);
             if (fsize > curr_max_fsize) {
                 curr_max_fsize = fsize;
                 strncpy(curr_max_fname, fname, sizeof curr_max_fname);
             }
             numb_lines++;  /* one line per file, as per output of `find` */
        }

        fclose(f);

        if (newrelic_transaction >= 0) {
            char total_numb_executables[32];
            snprintf(total_numb_executables, sizeof total_numb_executables,
                     "%ld", numb_lines);
            int ret_code =
                 newrelic_transaction_add_attribute(newrelic_transaction,
                                                "Custom/ct_NUMBER_EXECUTABLES",
                                                total_numb_executables);
            if (ret_code < 0)
                fprintf(stderr, "ERROR: newrelic_transaction_add_attribute() "
                                "returned %d\n", ret_code);
        }
    } else
        curr_max_fname[0] = '\0';

    *max_exec_size_found = curr_max_fsize;
    strncpy(max_exec_fname, curr_max_fname, sizeof curr_max_fname);

    if (newrelic_transaction >= 0 && newr_segm_internal_proc >= 0) {
        int ret_code =  newrelic_segment_end(newrelic_transaction,
                                             newr_segm_internal_proc);
        if (ret_code < 0)
            fprintf(stderr, "ERROR: newrelic_segment_end() returned %d\n",
                    ret_code);
    }

    return (curr_max_fname[0] != 0) ? 0: -1 ;
}



int
usage_and_exit(void)
{
    printf("Usage:\n"
           "      test_embedded_newrelic_instrum   newrelic_license_key\n"
           "                                        Run and record performance"
                                                  " under this N.R. key\n"
           "      test_embedded_newrelic_instrum   [-h|--help]\n"
           "                                        Show this usage help\n");
    exit(1);
}

