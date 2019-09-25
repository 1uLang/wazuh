/* Copyright (C) 2015-2019, Wazuh Inc.
 * June 13, 2017.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */

#include "shared.h"
#include "agentd.h"
#include "monitord/monitord.h"

#ifdef WIN32
#define localtime_r(x, y) localtime_s(y, x)
#endif

monitor_config mond;

static void init_conf()
{
    mond.enabled = 0;
    mond.max_size = 0;
    mond.interval = 0;
    mond.rotate = -1;
    mond.rotation_enabled = 1;
    mond.compress_rotation = 1;
    mond.ossec_log_plain = 0;
    mond.ossec_log_json = 0;
    mond.size_rotate = 0;
    mond.interval_rotate = 0;
    mond.interval_units = 's';
    mond.size_units = 'B';
    mond.maxage = 31;
    mond.day_wait = 10;
    mond.log_level = 0;

    return;
}

static void read_internal()
{
    int aux;

    if ((aux = getDefine_Int("monitord", "rotate_log", 0, 1)) != INT_OPT_NDEF)
        mond.rotation_enabled = aux;
    if ((aux = getDefine_Int("monitord", "size_rotate", 0, 4096)) != INT_OPT_NDEF) {
        mond.max_size = (unsigned long) aux * 1024 * 1024;
        mond.size_rotate = (unsigned long) aux;
        mond.size_units = 'M';              // Internal options has only MBytes available
    }
    if ((aux = getDefine_Int("monitord", "compress", 0, 1)) != INT_OPT_NDEF)
        mond.compress_rotation = aux;
    if ((aux = getDefine_Int("monitord", "day_wait", 0, MAX_DAY_WAIT)) != INT_OPT_NDEF)
        mond.day_wait = (short) aux;
    if ((aux = getDefine_Int("monitord", "keep_log_days", 0, 500)) != INT_OPT_NDEF)
        mond.maxage = aux;
    if ((aux = getDefine_Int("monitord", "debug", 0, 2)) != INT_OPT_NDEF)
        mond.log_level = aux;

    return;
}

// Thread to rotate internal log
void * w_rotate_log_thread(__attribute__((unused)) void * arg) {
    char path[PATH_MAX];
    char path_json[PATH_MAX];
    struct stat buf;
    off_t size, size_json;
    time_t n_time, n_time_json, now = time(NULL);
    struct tm tm, *rot = NULL;
    int today_log, today_json;
    char *new_path;

    localtime_r(&now, &tm);
    today_log = tm.tm_mday;
    today_json = today_log;

#ifdef WIN32
    // ossec.log
    snprintf(path, PATH_MAX, "%s", LOGFILE);
    // ossec.json
    snprintf(path_json, PATH_MAX, "%s", LOGJSONFILE);
#else
    // /var/ossec/logs/ossec.log
    snprintf(path, PATH_MAX, "%s%s", isChroot() ? "" : DEFAULTDIR, LOGFILE);
    // /var/ossec/logs/ossec.json
    snprintf(path_json, PATH_MAX, "%s%s", isChroot() ? "" : DEFAULTDIR, LOGJSONFILE);
#endif

    init_conf();

    const char *cfg = (isChroot() ? OSSECCONF : DEFAULTCPATH);
    int c;
    c = 0;
    c |= CROTMONITORD;
    if (ReadConfig(c, cfg, &mond, NULL) < 0) {
        merror_exit(CONFIG_ERROR, cfg);
    }

    read_internal();

    // If module is disabled, exit
    if (mond.rotation_enabled) {
        mdebug1("Log rotating thread started.");
    } else {
        mdebug1("Log rotating disabled. Exiting.");
        pthread_exit(NULL);
    }

    mwarn("The following internal options will be deprecated in the next version: compress, keep_log_days, day_wait, size_rotate_read and daily_rotations."
          "Please, use the 'logging' configuration block instead.");

    /* Calculate when is the next rotation */
    n_time = mond.interval ? calc_next_rotation(now, rot, mond.interval_units, mond.interval) : 0;
    n_time_json = n_time;

    // Initializes the rotation lists
    mond.log_list_plain = get_rotation_list("logs", ".log");
    mond.log_list_json = get_rotation_list("logs", ".json");
    purge_rotation_list(mond.log_list_plain, mond.rotate);
    purge_rotation_list(mond.log_list_json, mond.rotate);

    while (1) {
        if (mond.enabled){

            now = time(NULL);
            localtime_r(&now, &tm);

            if (mond.rotation_enabled) {

                if (mond.min_size > 0 && mond.interval > 0) {
                    if ((stat(path, &buf) == 0) && mond.ossec_log_plain) {
                        size = buf.st_size;
                        if (mond.interval > 0 && now > n_time && (long) size >= mond.min_size) {
                            if(mond.log_list_plain && mond.log_list_plain->last && today_log == mond.log_list_plain->last->first_value) {
                                new_path = w_rotate_log(path, mond.compress_rotation, mond.maxage, today_log != tm.tm_mday ? 1 : 0, 0, mond.daily_rotations, mond.log_list_plain->last->second_value);
                            } else {
                                new_path = w_rotate_log(path, mond.compress_rotation, mond.maxage, today_log != tm.tm_mday ? 1 : 0, 0, mond.daily_rotations, -1);
                            }
                            if(new_path) {
                                add_new_rotation_node(mond.log_list_plain, new_path, mond.rotate);
                            }
                            os_free(new_path);
                            today_log = today_log != tm.tm_mday ? tm.tm_mday : today_log;
                            n_time = calc_next_rotation(now, rot, mond.interval_units, mond.interval);
                        }
                    }
                    if ((stat(path_json, &buf) == 0) && mond.ossec_log_json) {
                        size_json = buf.st_size;
                        if (mond.interval > 0 && now > n_time_json && (long) size_json >= mond.min_size) {
                            if(mond.log_list_json && mond.log_list_json->last && today_json == mond.log_list_json->last->first_value) {
                                new_path = w_rotate_log(path_json, mond.compress_rotation, mond.maxage, today_json != tm.tm_mday ? 1 : 0, 1, mond.daily_rotations, mond.log_list_json->last->second_value);
                            } else {
                                new_path = w_rotate_log(path_json, mond.compress_rotation, mond.maxage, today_json != tm.tm_mday ? 1 : 0, 1, mond.daily_rotations, -1);
                            }
                            if(new_path) {
                                add_new_rotation_node(mond.log_list_json, new_path, mond.rotate);
                            }
                            os_free(new_path);
                            today_json = today_json != tm.tm_mday ? tm.tm_mday : today_json;
                            n_time_json = calc_next_rotation(now, rot, mond.interval_units, mond.interval);
                        }
                    }
                } else {
                    if (mond.max_size > 0) {
                        if ((stat(path, &buf) == 0) && mond.ossec_log_plain) {
                            size = buf.st_size;
                            /* If log file reachs maximum size, rotate ossec.log */
                            if ( (long) size >= mond.max_size) {
                                today_log = today_log != tm.tm_mday ? tm.tm_mday : today_log;
                                if(mond.log_list_plain && mond.log_list_plain->last && today_log == mond.log_list_plain->last->first_value) {
                                    new_path = w_rotate_log(path, mond.compress_rotation, mond.maxage, today_log != tm.tm_mday ? 1 : 0, 0, mond.daily_rotations, mond.log_list_plain->last->second_value);
                                } else {
                                    new_path = w_rotate_log(path, mond.compress_rotation, mond.maxage, today_log != tm.tm_mday ? 1 : 0, 0, mond.daily_rotations, -1);
                                }
                                if(new_path) {
                                    add_new_rotation_node(mond.log_list_plain, new_path, mond.rotate);
                                }
                                os_free(new_path);
                            }
                        }
                        if ((stat(path_json, &buf) == 0) && mond.ossec_log_json) {
                            size = buf.st_size;
                            /* If log file reachs maximum size, rotate ossec.json */
                            if ( (long) size >= mond.max_size) {
                                today_json = today_json != tm.tm_mday ? tm.tm_mday : today_json;
                                if(mond.log_list_json && mond.log_list_json->last && today_json == mond.log_list_json->last->first_value) {
                                    new_path = w_rotate_log(path_json, mond.compress_rotation, mond.maxage, today_json != tm.tm_mday ? 1 : 0, 1, mond.daily_rotations, mond.log_list_json->last->second_value);
                                } else{
                                    new_path = w_rotate_log(path_json, mond.compress_rotation, mond.maxage, today_json != tm.tm_mday ? 1 : 0, 1, mond.daily_rotations, -1);
                                }
                                if(new_path) {
                                    add_new_rotation_node(mond.log_list_json, new_path, mond.rotate);
                                }
                                os_free(new_path);
                            }
                        }
                    }
                    if (mond.rotation_enabled && mond.interval > 0 && now > n_time) {
                        if(mond.ossec_log_plain) {
                            if(mond.log_list_plain && mond.log_list_plain->last && today_log == mond.log_list_plain->last->first_value) {
                                new_path = w_rotate_log(path, mond.compress_rotation, mond.maxage, today_log != tm.tm_mday ? 1 : 0, 0, mond.daily_rotations, mond.log_list_plain->last->first_value == tm.tm_mday ? mond.log_list_json->last->second_value : -1);
                            } else {
                                new_path = w_rotate_log(path, mond.compress_rotation, mond.maxage, today_log != tm.tm_mday ? 1 : 0, 0, mond.daily_rotations, -1);
                            }
                            if(new_path) {
                                add_new_rotation_node(mond.log_list_plain, new_path, mond.rotate);
                            }
                            os_free(new_path);
                        }
                        if (mond.ossec_log_json) {
                            if (mond.log_list_json && mond.log_list_json->last && today_json == mond.log_list_json->last->first_value) {
                                new_path = w_rotate_log(path_json, mond.compress_rotation, mond.maxage, today_json != tm.tm_mday ? 1 : 0, 1, mond.daily_rotations, mond.log_list_json->last->first_value == tm.tm_mday ? mond.log_list_json->last->second_value : -1);
                            } else {
                                new_path = w_rotate_log(path_json, mond.compress_rotation, mond.maxage, today_json != tm.tm_mday ? 1 : 0, 1, mond.daily_rotations, -1);
                            }
                            if (new_path) {
                                add_new_rotation_node(mond.log_list_json, new_path, mond.rotate);
                            }
                            os_free(new_path);
                        }
                        today_log = today_log != tm.tm_mday ? tm.tm_mday : today_log;
                        today_json = today_json != tm.tm_mday ? tm.tm_mday : today_json;
                        n_time = calc_next_rotation(now, rot, mond.interval_units, mond.interval);
                    }
                }
            }
        }
        sleep(1);
    }
}
