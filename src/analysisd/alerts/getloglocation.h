/* Copyright (C) 2015-2019, Wazuh Inc.
 * Copyright (C) 2009 Trend Micro Inc.
 * All right reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */

#ifndef GETLL_H
#define GETLL_H

#include "eventinfo.h"
#include "analysisd.h"

#define SECONDS_PER_DAY 86400

/* Start the log location (need to be called before getlog) */
void OS_InitLog(void);
void OS_InitFwLog(void);

/* Get the log file based on the date/logtype
 * Returns 0 on success or -1 on error
 */
int OS_GetLogLocation(int day,int year,char *mon);

/* Global declarations */
extern FILE *_eflog;
extern FILE *_ejflog;
extern FILE *_aflog;
extern FILE *_fflog;
extern FILE *_jflog;
extern FILE *_ejflog;

void OS_RotateLogs(int day,int year,char *mon);
void OS_SignLog(const char *logfile, const char *logfile_old, const char * ext);
void sign_firewall_logs();
void sign_log(const char * logdir, const char *logfile, time_t *last_rot,
                int last_counter, const char * tag, const char * ext);

#endif /* GETLL_H */
