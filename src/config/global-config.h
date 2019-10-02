/* Copyright (C) 2015-2019, Wazuh Inc.
 * Copyright (C) 2009 Trend Micro Inc.
 * All right reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */

#ifndef CCONFIG_H
#define CCONFIG_H

#include "shared.h"

/* Configuration structure */
typedef struct __Config {
    u_int8_t logall;
    u_int8_t logall_json;
    u_int8_t stats;
    u_int8_t integrity;
    u_int8_t syscheck_auto_ignore;
    int syscheck_ignore_frequency;
    int syscheck_ignore_time;
    u_int8_t syscheck_alert_new;
    u_int8_t rootcheck;
    u_int8_t hostinfo;
    u_int8_t mailbylevel;
    u_int8_t logbylevel;
    u_int8_t logfw;
    int decoder_order_size;

    /* Prelude support */
    u_int8_t prelude;
    /* which min. level the alert must be sent to prelude */
    u_int8_t prelude_log_level;
    /* prelude profile name */
    char *prelude_profile;

    /* GeoIP DB */
    char *geoipdb_file;

    /* ZEROMQ Export */
    u_int8_t zeromq_output;
    char *zeromq_output_uri;
    char *zeromq_output_server_cert;
    char *zeromq_output_client_cert;

    /* JSONOUT Export */
    u_int8_t jsonout_output;

    /* Standard alerts output */
    u_int8_t alerts_log;

    /* Not currently used */
    u_int8_t keeplogdate;

    /* Mail alerting */
    short int mailnotify;

    /* Custom Alert output*/
    short int custom_alert_output;
    char *custom_alert_output_format;

    /* For the active response */
    int ar;

    /* For the correlation */
    int memorysize;

    /* List of files to ignore (syscheck) */
    char **syscheck_ignore;

    /* List of ips to never block */
    os_ip **white_list;

    /* List of hostnames to never block */
    OSMatch **hostname_white_list;

    /* List of rules */
    char **includes;

    /* List of Lists */
    char **lists;

    /* List of decoders */
    char **decoders;

    /* Global rule hash */
    OSHash *g_rules_hash;

#ifdef LIBGEOIP_ENABLED
    /* GeoIP support */
    u_int8_t loggeoip;
    char *geoip_db_path;
    char *geoip6_db_path;
    int geoip_jsonout;
#endif

    wlabel_t *labels; /* null-ended label set */
    int label_cache_maxage;
    int show_hidden_labels;

    // Cluster configuration
    char *cluster_name;
    char *node_name;
    char *node_type;
    unsigned char hide_cluster_info;

    int rotate_interval;
    int min_rotate_interval;
    ssize_t max_output_size;

    // Rotation options for archives
    unsigned int archives_enabled:1;
    unsigned int archives_rotation_enabled:1;
    unsigned int archives_compress_rotation:1;
    unsigned int archives_log_json:1;
    unsigned int archives_log_plain:1;
    long int archives_max_size;
    long int archives_min_size;
    long int archives_interval;
    char archives_interval_units;
    char alerts_interval_units;
    int archives_rotate;
    int archives_maxage;
    rotation_list *log_archives_plain;
    rotation_list *log_archives_json;

    // Rotation options for alerts
    unsigned int alerts_enabled:1;
    unsigned int alerts_rotation_enabled:1;
    unsigned int alerts_compress_rotation:1;
    unsigned int alerts_log_json:1;
    unsigned int alerts_log_plain:1;
    OSList *alerts_rotation_files;
    long int alerts_max_size;
    long int alerts_min_size;
    long int alerts_interval;
    int alerts_rotate;
    int alerts_maxage;
    rotation_list *log_alerts_plain;
    rotation_list *log_alerts_json;

    long queue_size;
} _Config;


void config_free(_Config *config);

#endif /* CCONFIG_H */
