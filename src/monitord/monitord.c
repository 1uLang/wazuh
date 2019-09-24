/* Copyright (C) 2015-2019, Wazuh Inc.
 * Copyright (C) 2009 Trend Micro Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */

#include "shared.h"
#include "monitord.h"

/* Global variables */
monitor_config mond;
const char * MONTHS[] = {
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec"
};

void Monitord()
{
    time_t tm, n_time, n_time_json;
    struct tm *p;
    struct tm rot;
    int counter = 0;

    char path_ossec[PATH_MAX];
    char path_ossec_json[PATH_MAX];

    struct stat buf;
    off_t size, size_json;

    int today = 0;
    int thismonth = 0;
    int thisyear = 0;

    char *new_path;
    char str[OS_SIZE_1024 + 1];

    /* Wait a few seconds to settle */
    sleep(10);

    memset(str, '\0', OS_SIZE_1024 + 1);

    /* Get current time before starting */
    tm = time(NULL);
    p = localtime(&tm);

    today = p->tm_mday;
    thismonth = p->tm_mon;
    thisyear = p->tm_year + 1900;

    /* Calculate when is the next rotation */
    n_time = mond.interval ? calc_next_rotation(tm, &rot, mond.interval_units, mond.interval) : 0;
    n_time_json = n_time;

    /* Set internal log path to rotate them */
#ifdef WIN32
    // ossec.log
    snprintf(path, PATH_MAX, "%s", LOGFILE);
    // ossec.json
    snprintf(path_json, PATH_MAX, "%s", LOGJSONFILE);
#else
    // /var/ossec/logs/ossec.log
    snprintf(path_ossec, PATH_MAX, "%s%s", isChroot() ? "" : DEFAULTDIR, LOGFILE);
    // /var/ossec/logs/ossec.json
    snprintf(path_ossec_json, PATH_MAX, "%s%s", isChroot() ? "" : DEFAULTDIR, LOGJSONFILE);
#endif

    /* Connect to the message queue or exit */
    if ((mond.a_queue = StartMQ(DEFAULTQUEUE, WRITE)) < 0) {
        merror_exit(QUEUE_FATAL, DEFAULTQUEUE);
    }

    /* Send startup message */
    snprintf(str, OS_SIZE_1024 - 1, OS_AD_STARTED);
    if (SendMSG(mond.a_queue, str, ARGV0,
                LOCALFILE_MQ) < 0) {
        merror(QUEUE_SEND);
    }

    // Start com request thread
    w_create_thread(moncom_main, NULL);

    mwarn("The following internal options will be deprecated in the next version: compress, rotate_log, keep_log_days, day_wait, size_rotate_read and daily_rotations."
          "Please, use the 'logging' configuration block instead.");

    // Initializes the rotation lists
    mond.log_list_plain = get_rotation_list("logs", ".log");
    mond.log_list_json = get_rotation_list("logs", ".json");
    purge_rotation_list(mond.log_list_plain, mond.rotate);
    purge_rotation_list(mond.log_list_json, mond.rotate);

    if (mond.min_size > 0 && mond.max_size > 0) {
        mwarn("'max_size' and 'min_size' options cannot be used together for the same log rotation. Disabling 'min_size' option...");
        mond.min_size = 0;
        mond.min_size_rotate = 0;
    }

    /* Main monitor loop */
    while (1) {
        tm = time(NULL);
        p = localtime(&tm);
        counter++;

#ifndef LOCAL
        /* Check for unavailable agents, every two minutes */
        if (mond.monitor_agents && counter >= 120) {
            monitor_agents();
            counter = 0;
        }
#endif

        if (mond.enabled) {
            if (mond.rotation_enabled) {
                if (mond.min_size > 0 && mond.interval > 0) {
                    if ((stat(path_ossec, &buf) == 0) && mond.ossec_log_plain) {
                        size = buf.st_size;
                        if (mond.interval > 0 && tm > n_time && (long) size >= mond.min_size) {
                            if(mond.log_list_plain && mond.log_list_plain->last && today == mond.log_list_plain->last->first_value) {
                                new_path = w_rotate_log(path_ossec, mond.compress_rotation, mond.maxage, today != p->tm_mday ? 1 : 0, 0, mond.daily_rotations, mond.log_list_plain->last->second_value);
                            } else {
                                new_path = w_rotate_log(path_ossec, mond.compress_rotation, mond.maxage, today != p->tm_mday ? 1 : 0, 0, mond.daily_rotations, -1);
                            }
                            if(new_path) {
                                add_new_rotation_node(mond.log_list_plain, new_path, mond.rotate);
                            }
                            os_free(new_path);
                            n_time = calc_next_rotation(tm, &rot, mond.interval_units, mond.interval);
                        }
                    }
                    if ((stat(path_ossec_json, &buf) == 0) && mond.ossec_log_json) {
                        size_json = buf.st_size;
                        if (mond.interval > 0 && tm > n_time_json && (long) size_json >= mond.min_size) {
                            if(mond.log_list_json && mond.log_list_json->last && today == mond.log_list_json->last->first_value) {
                                new_path = w_rotate_log(path_ossec_json, mond.compress_rotation, mond.maxage, today != p->tm_mday ? 1 : 0, 1, mond.daily_rotations, mond.log_list_json->last->second_value);
                            } else {
                                new_path = w_rotate_log(path_ossec_json, mond.compress_rotation, mond.maxage, today != p->tm_mday ? 1 : 0, 1, mond.daily_rotations, -1);
                            }
                            if(new_path) {
                                add_new_rotation_node(mond.log_list_json, new_path, mond.rotate);
                            }
                            os_free(new_path);
                            n_time_json = calc_next_rotation(tm, &rot, mond.interval_units, mond.interval);
                        }
                    }
                    if (today != p->tm_mday) {
                        /* Generate reports */
                        /*
                        generate_reports(today, thismonth, thisyear, p);
                        manage_files(today, thismonth, thisyear);
                        */
                        today = p->tm_mday;
                        thismonth = p->tm_mon;
                        thisyear = p->tm_year + 1900;
                    }
                } else {
                    if (mond.max_size > 0) {
                        if ((stat(path_ossec, &buf) == 0) && mond.ossec_log_plain) {
                            size = buf.st_size;
                            /* If log file reachs maximum size, rotate ossec.log */
                            if ( (long) size >= mond.max_size) {
                                if(mond.log_list_plain && mond.log_list_plain->last && today == mond.log_list_plain->last->first_value) {
                                    new_path = w_rotate_log(path_ossec, mond.compress_rotation, mond.maxage, 0, 0, mond.daily_rotations, mond.log_list_plain->last->second_value);
                                } else {
                                    new_path = w_rotate_log(path_ossec, mond.compress_rotation, mond.maxage, 0, 0, mond.daily_rotations, -1);
                                }
                                if(new_path) {
                                    add_new_rotation_node(mond.log_list_plain, new_path, mond.rotate);
                                }
                                os_free(new_path);
                            }
                        }
                        if ((stat(path_ossec_json, &buf) == 0) && mond.ossec_log_json) {
                            size = buf.st_size;
                            /* If log file reachs maximum size, rotate ossec.json */
                            if ( (long) size >= mond.max_size) {
                                if (mond.log_list_json && mond.log_list_json->last && today == mond.log_list_json->last->first_value) {
                                    new_path = w_rotate_log(path_ossec_json, mond.compress_rotation, mond.maxage, 0, 1, mond.daily_rotations, mond.log_list_json->last->second_value);
                                } else {
                                    new_path = w_rotate_log(path_ossec_json, mond.compress_rotation, mond.maxage, 0, 1, mond.daily_rotations, -1);
                                }
                                if(new_path) {
                                    add_new_rotation_node(mond.log_list_json, new_path, mond.rotate);
                                }
                                os_free(new_path);
                            }
                        }
                    }
                    if (mond.rotation_enabled && mond.interval > 0 && tm > n_time) {
                        if (mond.ossec_log_plain) {
                            if (mond.log_list_plain && mond.log_list_plain->last && today == mond.log_list_plain->last->first_value) {
                                new_path = w_rotate_log(path_ossec, mond.compress_rotation, mond.maxage, today != p->tm_mday ? 1 : 0, 0, mond.daily_rotations, mond.log_list_plain->last->second_value);
                            } else {
                                new_path = w_rotate_log(path_ossec, mond.compress_rotation, mond.maxage, today != p->tm_mday ? 1 : 0, 0, mond.daily_rotations, -1);
                            }
                            if(new_path) {
                                add_new_rotation_node(mond.log_list_plain, new_path, mond.rotate);
                            }
                            os_free(new_path);
                        }
                        if (mond.ossec_log_json) {
                            if (mond.log_list_json && mond.log_list_json->last && today == mond.log_list_json->last->first_value) {
                                new_path = w_rotate_log(path_ossec_json, mond.compress_rotation, mond.maxage, today != p->tm_mday ? 1 : 0, 1, mond.daily_rotations, mond.log_list_json->last->second_value);
                            } else {
                                new_path = w_rotate_log(path_ossec_json, mond.compress_rotation, mond.maxage, today != p->tm_mday ? 1 : 0, 1, mond.daily_rotations, -1);
                            }
                            if(new_path) {
                                add_new_rotation_node(mond.log_list_json, new_path, mond.rotate);
                            }
                            os_free(new_path);
                        }
                        if (today != p->tm_mday) {
                            /* Generate reports */
                            /*generate_reports(today, thismonth, thisyear, p);
                            manage_files(today, thismonth, thisyear);
                            */
                            today = p->tm_mday;
                            thismonth = p->tm_mon;
                            thisyear = p->tm_year + 1900;
                        }
                        n_time = calc_next_rotation(tm, &rot, mond.interval_units, mond.interval);
                    }
                }
            }
        }
        sleep(1);
    }
}

cJSON *getMonitorInternalOptions(void) {

    cJSON *root = cJSON_CreateObject();
    cJSON *monconf = cJSON_CreateObject();

    cJSON_AddNumberToObject(monconf, "monitor_agents", mond.monitor_agents);
    cJSON_AddNumberToObject(monconf, "delete_old_agents", mond.delete_old_agents);

    cJSON_AddItemToObject(root, "monitord", monconf);

    return root;
}


cJSON *getReportsOptions(void) {

    cJSON *root = cJSON_CreateObject();
    unsigned int i;

    if (mond.reports) {
        cJSON *arr = cJSON_CreateArray();
        for (i=0;mond.reports[i];i++) {
            cJSON *rep = cJSON_CreateObject();
            if (mond.reports[i]->title) cJSON_AddStringToObject(rep,"title",mond.reports[i]->title);
            if (mond.reports[i]->r_filter.group) cJSON_AddStringToObject(rep,"group",mond.reports[i]->r_filter.group);
            if (mond.reports[i]->r_filter.rule) cJSON_AddStringToObject(rep,"rule",mond.reports[i]->r_filter.rule);
            if (mond.reports[i]->r_filter.level) cJSON_AddStringToObject(rep,"level",mond.reports[i]->r_filter.level);
            if (mond.reports[i]->r_filter.srcip) cJSON_AddStringToObject(rep,"srcip",mond.reports[i]->r_filter.srcip);
            if (mond.reports[i]->r_filter.user) cJSON_AddStringToObject(rep,"user",mond.reports[i]->r_filter.user);
            if (mond.reports[i]->r_filter.show_alerts) cJSON_AddStringToObject(rep,"showlogs","yes"); else cJSON_AddStringToObject(rep,"showlogs","no");
            if (mond.reports[i]->emailto) {
                unsigned int j = 0;
                cJSON *email = cJSON_CreateArray();
                while (mond.reports[i]->emailto[j]) {
                    cJSON_AddItemToArray(email, cJSON_CreateString(mond.reports[i]->emailto[j]));
                    j++;
                }
                cJSON_AddItemToObject(rep,"email_to",email);
            }
            cJSON_AddItemToArray(arr, rep);
        }
        cJSON_AddItemToObject(root,"reports",arr);
    }

    return root;
}

cJSON *getMonitorLogging(void) {
    char *json_format = "json_format";
    char *plain_format = "plain_format";
    char *compress_rotation = "compress_rotation";
    char *rotation_interval = "rotation_interval";
    char *saved_rotations = "saved_rotations";
    char *size_rotation = "size_rotation";
    char *maxage = "maxage";
    char *day_wait = "day_wait";
    char *min_size_rotation = "min_size_rotation";
    cJSON *root;
    cJSON *logging;
    char aux[50];


    root = cJSON_CreateObject();
    logging = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "logging", logging);

    if (mond.enabled) {
        cJSON_AddStringToObject(logging, plain_format, mond.ossec_log_plain ? "yes" : "no");
        cJSON_AddStringToObject(logging, json_format, mond.ossec_log_json ? "yes" : "no");
        if (mond.rotation_enabled) {
            cJSON_AddStringToObject(logging, compress_rotation, mond.compress_rotation ? "yes" : "no");
            cJSON_AddNumberToObject(logging, saved_rotations, mond.rotate);
            cJSON_AddNumberToObject(logging, rotation_interval, mond.interval);
            snprintf(aux, 50, "%ld %c", mond.interval_rotate, mond.interval_units);
            cJSON_AddStringToObject(logging, rotation_interval, mond.interval ? aux : "no");
            snprintf(aux, 50, "%ld %c", mond.size_rotate, mond.size_units);
            cJSON_AddStringToObject(logging, size_rotation, mond.size_rotate ? aux : "no");
            snprintf(aux, 50, "%ld %c", mond.min_size_rotate, mond.min_size_units);
            cJSON_AddStringToObject(logging, min_size_rotation, mond.min_size_rotate ? aux : "no");
            cJSON_AddNumberToObject(logging, maxage, mond.maxage);
            cJSON_AddNumberToObject(logging, day_wait, mond.day_wait);
        }
    }

    return root;
}