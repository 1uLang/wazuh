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

#ifndef WM_TASK_MANAGER_H
#define WM_TASK_MANAGER_H

#define WM_TASK_MANAGER_LOGTAG ARGV0 ":" TASK_MANAGER_WM_NAME

typedef struct _wm_task_manager {
    int enabled:1;
} wm_task_manager;

typedef enum _json_key {
    WM_TASK_MODULE = 0,
    WM_TASK_COMMAND,
    WM_TASK_AGENT_ID,
    WM_TASK_TASK_ID,
    WM_TASK_STATUS,
    WM_TASK_ERROR,
    WM_TASK_ERROR_DATA
} json_key;

typedef enum _module_list {
    WM_TASK_UPGRADE_MODULE = 0
} module_list;

typedef enum _command_list {
    WM_TASK_UPGRADE = 0,
    WM_TASK_UPGRADE_CUSTOM,
    WM_TASK_UPGRADE_GET_STATUS,
    WM_TASK_UPGRADE_UPDATE_STATUS
} command_list;

typedef enum _error_code {
    WM_TASK_SUCCESS = 0,
    WM_TASK_INVALID_MESSAGE,
    WM_TASK_INVALID_MODULE,
    WM_TASK_INVALID_COMMAND,
    WM_TASK_INVALID_AGENT_ID,
    WM_TASK_INVALID_TASK_ID,
    WM_TASK_INVALID_STATUS,
    WM_TASK_DATABASE_NO_TASK,
    WM_TASK_DATABASE_ERROR,
    WM_TASK_UNKNOWN_ERROR
} error_code;

extern const wm_context WM_TASK_MANAGER_CONTEXT;   // Context

// Parse XML configuration
int wm_task_manager_read(xml_node **nodes, wmodule *module);

/**
 * Do all the analysis of the incomming message and returns a response.
 * @param msg Incomming message from a connection.
 * @param response Response to be sent to the connection.
 * @return Size of the response string.
 * */
size_t wm_task_manager_dispatch(const char *msg, char **response);

/**
 * Parse the incomming message and return a JSON with the message.
 * @param msg Incomming message from a connection.
 * @return JSON array when succeed, NULL otherwise.
 * */
cJSON* wm_task_manager_parse_message(const char *msg);

/**
 * Analyze a task by module and command. Update the tasks DB when necessary.
 * @param task_object JSON object with a task to be analyzed.
 * @param error_code Variable to store an error code if something is wrong.
 * @return JSON object with the response for this task.
 * */
cJSON* wm_task_manager_analyze_task(const cJSON *task_object, int *error_code);

/**
 * Build a JSON object response.
 * 
 * Example 1: Success
 * 
 * {
 *     "error":0,
 *     "data":"Success",
 *     "agent":4,
 *     "task_id":2
 * }
 * 
 * Example 2: Invalid message
 * 
 * {
 *     "error":1,
 *     "data":"Invalid message"
 * }
 * 
 * 
 * Example 3: Invalid module
 * 
 * {
 *     "error":2,
 *     "data":"Invalid module",
 *     "task_id":45
 * }
 * 
 * Example 4: Invalid command
 * 
 * {
 *     "error":3,
 *     "data":"Invalid command",
 *     "agent":15
 * }
 * 
 * @param error_code Code of the error.
 * @param agent_id ID of the agent when receiving a request for a specific agent.
 * @param task_id ID of the task when receiving a request for a specific task.
 * @param status Status of the task when receiving a request for a specific status.
 * @return JSON object.
 * */
cJSON* wm_task_manager_build_response(int error_code, int agent_id, int task_id, char *status);

#endif
#endif
