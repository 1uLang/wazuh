/*
 * Wazuh Module for Task management.
 * Copyright (C) 2015-2020, Wazuh Inc.
 * July 13, 2020.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#ifndef CLIENT

#ifndef WM_TASK_MANAGER_DB
#define WM_TASK_MANAGER_DB

#define TASKS_PATH              "queue/tasks/"
#define TASKS_DB TASKS_PATH     "tasks.db"

#define TASKS_TABLE             "TASKS"
#define MAX_SQL_ATTEMPTS        1000

typedef enum _task_query {
    WM_TASK_INSERT_TASK,
    WM_TASK_GET_MAX_TASK_ID,
    WM_TASK_GET_LAST_AGENT_TASK,
    WM_TASK_GET_TASK_STATUS,
    WM_TASK_UPDATE_TASK_STATUS
} task_query;

typedef enum _task_status {
    WM_TASK_NEW = 0,
    WM_TASK_IN_PROGRESS,
    WM_TASK_DONE,
    WM_TASK_FAILED
} task_status;

extern char *schema_task_manager_sql;

/**
 * Create the tasks DB or check that it already exists.
 * @return 0 when succeed, -1 otherwise.
 * */
int wm_task_manager_check_db();

/**
 * Insert a new task in the tasks DB.
 * @param agent_id ID of the agent where the task will be executed.
 * @param module Name of the module where the message comes from.
 * @param command Command to be executed in the agent.
 * @return ID of the task recently created when succeed, <=0 otherwise.
 * */
int wm_task_manager_insert_task(int agent_id, const char *module, const char *command);

/**
 * Get the status of a task from the tasks DB.
 * @param agent_id ID of the agent where the task is being executed.
 * @param module Name of the module where the message comes from.
 * @param status String where the status of the task will be stored.
 * @return 0 when succeed, !=0 otherwise.
 * */
int wm_task_manager_get_task_status(int agent_id, const char *module, char **status);

/**
 * Update the status of a task in the tasks DB.
 * @param agent_id ID of the agent where the task is being executed.
 * @param module Name of the module where the message comes from.
 * @param status New status of the task.
 * @return 0 when succeed, !=0 otherwise.
 * */
int wm_task_manager_update_task_status(int agent_id, const char *module, const char *status);

#endif
#endif
