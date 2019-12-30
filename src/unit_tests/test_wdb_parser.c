/*
 * Copyright (C) 2015-2019, Wazuh Inc.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../wazuh_db/wdb.h"

int __wrap__minfo()
{
    return 0;
}

int __wrap__merror()
{
    return 0;
}

int __wrap__mwarn()
{
    return 0;
}

void __wrap__mdebug1(const char * file, int line, const char * func, const char *msg, ...)
{
    char formatted_msg[OS_MAXSTR];
    va_list args;

    va_start(args, msg);
    vsnprintf(formatted_msg, OS_MAXSTR, msg, args);
    va_end(args);

    check_expected(formatted_msg);
}

wdb_t * __wrap_wdb_open_agent2(int agent_id)
{
    wdb_t * wdb = NULL;
    if (mock()) {
        char sagent_id[64];
        snprintf(sagent_id, sizeof(sagent_id), "%03d", agent_id);
        os_calloc(1, sizeof(wdb_t), wdb);
        w_mutex_init(&wdb->mutex, NULL);
        wdb->agent_id = strdup(sagent_id);
    }
    return wdb;
}

void __wrap_wdb_leave(wdb_t * wdb)
{
    if (wdb) {
        free(wdb->agent_id);
        w_mutex_destroy(&wdb->mutex);
        free(wdb);
    }
}

int __wrap_wdb_inventory_save_hw(wdb_t * wdb, const char * payload)
{
    check_expected(payload);
    return mock();
}

int __wrap_wdb_inventory_save_os(wdb_t * wdb, const char * payload)
{
    check_expected(payload);
    return mock();
}

int __wrap_wdb_inventory_save_network(wdb_t * wdb, const char * payload)
{
    check_expected(payload);
    return mock();
}

int __wrap_wdb_inventory_delete_network(wdb_t * wdb, const char * payload)
{
    check_expected(payload);
    return mock();
}

int __wrap_wdb_inventory_save_program(wdb_t * wdb, const char * payload)
{
    check_expected(payload);
    return mock();
}

int __wrap_wdb_inventory_delete_program(wdb_t * wdb, const char * payload)
{
    check_expected(payload);
    return mock();
}

int __wrap_wdb_inventory_save_hotfix(wdb_t * wdb, const char * payload)
{
    check_expected(payload);
    return mock();
}

int __wrap_wdb_inventory_delete_hotfix(wdb_t * wdb, const char * payload)
{
    check_expected(payload);
    return mock();
}

int __wrap_wdb_inventory_save_port(wdb_t * wdb, const char * payload)
{
    check_expected(payload);
    return mock();
}

int __wrap_wdb_inventory_delete_port(wdb_t * wdb, const char * payload)
{
    check_expected(payload);
    return mock();
}

int __wrap_wdb_inventory_save_process(wdb_t * wdb, const char * payload)
{
    check_expected(payload);
    return mock();
}

int __wrap_wdb_inventory_delete_process(wdb_t * wdb, const char * payload)
{
    check_expected(payload);
    return mock();
}

int __wrap_wdb_inventory_save_scan_info(wdb_t * wdb, const char * inventory, const char * payload)
{
    check_expected(inventory);
    check_expected(payload);
    return mock();
}

void test_parse_no_input(void **state)
{
    char output[OS_MAXSTR + 1];
    *output = '\0';

    expect_string(__wrap__mdebug1, formatted_msg, "Empty input query.");

    int ret = wdb_parse(NULL, output);

    assert_int_equal(ret, -1);
    assert_null(*output);
}

void test_parse_invalid_actor(void **state)
{
    char * input1 = strdup("abcdef");
    char * output1 = calloc(1, OS_MAXSTR + 1);
    *output1 = '\0';

    expect_string(__wrap__mdebug1, formatted_msg, "Invalid DB query syntax.");

    int ret = wdb_parse(input1, output1);

    assert_int_equal(ret, -1);
    assert_string_equal(output1, "err Invalid DB query syntax, near 'abcdef'");

    free(input1);
    free(output1);

    char * input2 = strdup("manager 000");
    char * output2 = calloc(1, OS_MAXSTR + 1);
    *output2 = '\0';

    expect_string(__wrap__mdebug1, formatted_msg, "DB() Invalid DB query actor: manager");

    ret = wdb_parse(input2, output2);

    assert_int_equal(ret, -1);
    assert_string_equal(output2, "err Invalid DB query actor: 'manager'");

    free(input2);
    free(output2);
}

void test_parse_invalid_agent_id(void **state)
{
    char * input1 = strdup("agent 000");
    char * output1 = calloc(1, OS_MAXSTR + 1);
    *output1 = '\0';

    expect_string(__wrap__mdebug1, formatted_msg, "Invalid DB query syntax.");

    int ret = wdb_parse(input1, output1);

    assert_int_equal(ret, -1);
    assert_string_equal(output1, "err Invalid DB query syntax, near '000'");

    free(input1);
    free(output1);

    char * input2 = strdup("agent abc test");
    char * output2 = calloc(1, OS_MAXSTR + 1);
    *output2 = '\0';

    expect_string(__wrap__mdebug1, formatted_msg, "Invalid agent ID 'abc'");

    ret = wdb_parse(input2, output2);

    assert_int_equal(ret, -1);
    assert_string_equal(output2, "err Invalid agent ID 'abc'");

    free(input2);
    free(output2);

    char * input3 = strdup("agent 000 test");
    char * output3 = calloc(1, OS_MAXSTR + 1);
    *output3 = '\0';

    will_return(__wrap_wdb_open_agent2, 0);

    ret = wdb_parse(input3, output3);

    assert_int_equal(ret, -1);
    assert_string_equal(output3, "err Couldn't open DB for agent 0");

    free(input3);
    free(output3);
}

void test_parse_inventory_invalid_type(void **state)
{
    char * input1 = strdup("agent 000 inventory");
    char * output1 = calloc(1, OS_MAXSTR + 1);
    *output1 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax.");

    int ret = wdb_parse(input1, output1);

    assert_int_equal(ret, -1);
    assert_string_equal(output1, "err Invalid inventory query syntax, near 'inventory'");

    free(input1);
    free(output1);

    char * input2 = strdup("agent 000 inventory drivers");
    char * output2 = calloc(1, OS_MAXSTR + 1);
    *output2 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: drivers");

    ret = wdb_parse(input2, output2);

    assert_int_equal(ret, -1);
    assert_string_equal(output2, "err Invalid inventory query syntax, near 'drivers'");

    free(input2);
    free(output2);
}

void test_parse_inventory_network_invalid_query(void **state)
{
    char * input1 = strdup("agent 000 inventory network");
    char * output1 = calloc(1, OS_MAXSTR + 1);
    *output1 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: network");

    int ret = wdb_parse(input1, output1);

    assert_int_equal(ret, -1);
    assert_string_equal(output1, "err Invalid inventory query syntax, near 'network'");

    free(input1);
    free(output1);

    char * input2 = strdup("agent 000 inventory network save");
    char * output2 = calloc(1, OS_MAXSTR + 1);
    *output2 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: save");

    ret = wdb_parse(input2, output2);

    assert_int_equal(ret, -1);
    assert_string_equal(output2, "err Invalid inventory query syntax, near 'save'");

    free(input2);
    free(output2);

    char * input3 = strdup("agent 000 inventory network create {}");
    char * output3 = calloc(1, OS_MAXSTR + 1);
    *output3 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: create");

    ret = wdb_parse(input3, output3);

    assert_int_equal(ret, -1);
    assert_string_equal(output3, "err Invalid inventory query syntax, near 'create'");

    free(input3);
    free(output3);
}

void test_parse_inventory_network_save(void **state)
{
    char * input = strdup("agent 000 inventory network save {\"type\":\"added\"}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_save_network, payload, "{\"type\":\"added\"}");
    will_return(__wrap_wdb_inventory_save_network, 0);

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, 0);
    assert_string_equal(output, "ok");

    free(input);
    free(output);
}

void test_parse_inventory_network_save_error(void **state)
{
    char * input = strdup("agent 000 inventory network save {\"type\":\"added\"}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_save_network, payload, "{\"type\":\"added\"}");
    will_return(__wrap_wdb_inventory_save_network, -1);

    expect_string(__wrap__mdebug1, formatted_msg, "Cannot save network information.");

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, -1);
    assert_string_equal(output, "err Cannot save network information.");

    free(input);
    free(output);
}

void test_parse_inventory_network_delete(void **state)
{
    char * input = strdup("agent 000 inventory network delete {\"type\":\"deleted\"}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_delete_network, payload, "{\"type\":\"deleted\"}");
    will_return(__wrap_wdb_inventory_delete_network, 0);

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, 0);
    assert_string_equal(output, "ok");

    free(input);
    free(output);
}

void test_parse_inventory_network_delete_error(void **state)
{
    char * input = strdup("agent 000 inventory network delete {\"type\":\"deleted\"}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_delete_network, payload, "{\"type\":\"deleted\"}");
    will_return(__wrap_wdb_inventory_delete_network, -1);

    expect_string(__wrap__mdebug1, formatted_msg, "Cannot delete old network information.");

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, -1);
    assert_string_equal(output, "err Cannot delete old network information.");

    free(input);
    free(output);
}

void test_parse_inventory_os_invalid_query(void **state)
{
    char * input1 = strdup("agent 000 inventory OS");
    char * output1 = calloc(1, OS_MAXSTR + 1);
    *output1 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: OS");

    int ret = wdb_parse(input1, output1);

    assert_int_equal(ret, -1);
    assert_string_equal(output1, "err Invalid inventory query syntax, near 'OS'");

    free(input1);
    free(output1);

    char * input2 = strdup("agent 000 inventory OS save");
    char * output2 = calloc(1, OS_MAXSTR + 1);
    *output2 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: save");

    ret = wdb_parse(input2, output2);

    assert_int_equal(ret, -1);
    assert_string_equal(output2, "err Invalid inventory query syntax, near 'save'");

    free(input2);
    free(output2);

    char * input3 = strdup("agent 000 inventory OS install {}");
    char * output3 = calloc(1, OS_MAXSTR + 1);
    *output3 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: install");

    ret = wdb_parse(input3, output3);

    assert_int_equal(ret, -1);
    assert_string_equal(output3, "err Invalid inventory query syntax, near 'install'");

    free(input3);
    free(output3);
}

void test_parse_inventory_os_save(void **state)
{
    char * input = strdup("agent 000 inventory OS save {\"type\":\"modified\"}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_save_os, payload, "{\"type\":\"modified\"}");
    will_return(__wrap_wdb_inventory_save_os, 0);

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, 0);
    assert_string_equal(output, "ok");

    free(input);
    free(output);
}

void test_parse_inventory_os_save_error(void **state)
{
    char * input = strdup("agent 000 inventory OS save {\"type\":\"modified\"}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_save_os, payload, "{\"type\":\"modified\"}");
    will_return(__wrap_wdb_inventory_save_os, -1);

    expect_string(__wrap__mdebug1, formatted_msg, "Cannot save OS information.");

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, -1);
    assert_string_equal(output, "err Cannot save OS information.");

    free(input);
    free(output);
}

void test_parse_inventory_hw_invalid_query(void **state)
{
    char * input1 = strdup("agent 000 inventory hardware");
    char * output1 = calloc(1, OS_MAXSTR + 1);
    *output1 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: hardware");

    int ret = wdb_parse(input1, output1);

    assert_int_equal(ret, -1);
    assert_string_equal(output1, "err Invalid inventory query syntax, near 'hardware'");

    free(input1);
    free(output1);

    char * input2 = strdup("agent 000 inventory hardware save");
    char * output2 = calloc(1, OS_MAXSTR + 1);
    *output2 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: save");

    ret = wdb_parse(input2, output2);

    assert_int_equal(ret, -1);
    assert_string_equal(output2, "err Invalid inventory query syntax, near 'save'");

    free(input2);
    free(output2);

    char * input3 = strdup("agent 000 inventory hardware add {}");
    char * output3 = calloc(1, OS_MAXSTR + 1);
    *output3 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: add");

    ret = wdb_parse(input3, output3);

    assert_int_equal(ret, -1);
    assert_string_equal(output3, "err Invalid inventory query syntax, near 'add'");

    free(input3);
    free(output3);
}

void test_parse_inventory_hw_save(void **state)
{
    char * input = strdup("agent 000 inventory hardware save {\"type\":\"added\"}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_save_hw, payload, "{\"type\":\"added\"}");
    will_return(__wrap_wdb_inventory_save_hw, 0);

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, 0);
    assert_string_equal(output, "ok");

    free(input);
    free(output);
}

void test_parse_inventory_hw_save_error(void **state)
{
    char * input = strdup("agent 000 inventory hardware save {\"type\":\"added\"}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_save_hw, payload, "{\"type\":\"added\"}");
    will_return(__wrap_wdb_inventory_save_hw, -1);

    expect_string(__wrap__mdebug1, formatted_msg, "Cannot save HW information.");

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, -1);
    assert_string_equal(output, "err Cannot save HW information.");

    free(input);
    free(output);
}

void test_parse_inventory_program_invalid_query(void **state)
{
    char * input1 = strdup("agent 000 inventory program");
    char * output1 = calloc(1, OS_MAXSTR + 1);
    *output1 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: program");

    int ret = wdb_parse(input1, output1);

    assert_int_equal(ret, -1);
    assert_string_equal(output1, "err Invalid inventory query syntax, near 'program'");

    free(input1);
    free(output1);

    char * input2 = strdup("agent 000 inventory program save");
    char * output2 = calloc(1, OS_MAXSTR + 1);
    *output2 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: save");

    ret = wdb_parse(input2, output2);

    assert_int_equal(ret, -1);
    assert_string_equal(output2, "err Invalid inventory query syntax, near 'save'");

    free(input2);
    free(output2);

    char * input3 = strdup("agent 000 inventory program update {}");
    char * output3 = calloc(1, OS_MAXSTR + 1);
    *output3 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: update");

    ret = wdb_parse(input3, output3);

    assert_int_equal(ret, -1);
    assert_string_equal(output3, "err Invalid inventory query syntax, near 'update'");

    free(input3);
    free(output3);
}

void test_parse_inventory_program_save(void **state)
{
    char * input = strdup("agent 000 inventory program save {\"type\":\"modified\"}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_save_program, payload, "{\"type\":\"modified\"}");
    will_return(__wrap_wdb_inventory_save_program, 0);

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, 0);
    assert_string_equal(output, "ok");

    free(input);
    free(output);
}

void test_parse_inventory_program_save_error(void **state)
{
    char * input = strdup("agent 000 inventory program save {\"type\":\"added\"}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_save_program, payload, "{\"type\":\"added\"}");
    will_return(__wrap_wdb_inventory_save_program, -1);

    expect_string(__wrap__mdebug1, formatted_msg, "Cannot save program information.");

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, -1);
    assert_string_equal(output, "err Cannot save program information.");

    free(input);
    free(output);
}

void test_parse_inventory_program_delete(void **state)
{
    char * input = strdup("agent 000 inventory program delete {\"type\":\"deleted\"}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_delete_program, payload, "{\"type\":\"deleted\"}");
    will_return(__wrap_wdb_inventory_delete_program, 0);

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, 0);
    assert_string_equal(output, "ok");

    free(input);
    free(output);
}

void test_parse_inventory_program_delete_error(void **state)
{
    char * input = strdup("agent 000 inventory program delete {\"type\":\"deleted\"}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_delete_program, payload, "{\"type\":\"deleted\"}");
    will_return(__wrap_wdb_inventory_delete_program, -1);

    expect_string(__wrap__mdebug1, formatted_msg, "Cannot delete old program information.");

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, -1);
    assert_string_equal(output, "err Cannot delete old program information.");

    free(input);
    free(output);
}

void test_parse_inventory_hotfix_invalid_query(void **state)
{
    char * input1 = strdup("agent 000 inventory hotfix");
    char * output1 = calloc(1, OS_MAXSTR + 1);
    *output1 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: hotfix");

    int ret = wdb_parse(input1, output1);

    assert_int_equal(ret, -1);
    assert_string_equal(output1, "err Invalid inventory query syntax, near 'hotfix'");

    free(input1);
    free(output1);

    char * input2 = strdup("agent 000 inventory hotfix save");
    char * output2 = calloc(1, OS_MAXSTR + 1);
    *output2 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: save");

    ret = wdb_parse(input2, output2);

    assert_int_equal(ret, -1);
    assert_string_equal(output2, "err Invalid inventory query syntax, near 'save'");

    free(input2);
    free(output2);

    char * input3 = strdup("agent 000 inventory hotfix upgrade {}");
    char * output3 = calloc(1, OS_MAXSTR + 1);
    *output3 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: upgrade");

    ret = wdb_parse(input3, output3);

    assert_int_equal(ret, -1);
    assert_string_equal(output3, "err Invalid inventory query syntax, near 'upgrade'");

    free(input3);
    free(output3);
}

void test_parse_inventory_hotfix_save(void **state)
{
    char * input = strdup("agent 000 inventory hotfix save {\"type\":\"added\"}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_save_hotfix, payload, "{\"type\":\"added\"}");
    will_return(__wrap_wdb_inventory_save_hotfix, 0);

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, 0);
    assert_string_equal(output, "ok");

    free(input);
    free(output);
}

void test_parse_inventory_hotfix_save_error(void **state)
{
    char * input = strdup("agent 000 inventory hotfix save {\"type\":\"added\"}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_save_hotfix, payload, "{\"type\":\"added\"}");
    will_return(__wrap_wdb_inventory_save_hotfix, -1);

    expect_string(__wrap__mdebug1, formatted_msg, "Cannot save hotfix information.");

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, -1);
    assert_string_equal(output, "err Cannot save hotfix information.");

    free(input);
    free(output);
}

void test_parse_inventory_hotfix_delete(void **state)
{
    char * input = strdup("agent 000 inventory hotfix delete {\"type\":\"deleted\"}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_delete_hotfix, payload, "{\"type\":\"deleted\"}");
    will_return(__wrap_wdb_inventory_delete_hotfix, 0);

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, 0);
    assert_string_equal(output, "ok");

    free(input);
    free(output);
}

void test_parse_inventory_hotfix_delete_error(void **state)
{
    char * input = strdup("agent 000 inventory hotfix delete {\"type\":\"deleted\"}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_delete_hotfix, payload, "{\"type\":\"deleted\"}");
    will_return(__wrap_wdb_inventory_delete_hotfix, -1);

    expect_string(__wrap__mdebug1, formatted_msg, "Cannot delete old hotfix information.");

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, -1);
    assert_string_equal(output, "err Cannot delete old hotfix information.");

    free(input);
    free(output);
}

void test_parse_inventory_port_invalid_query(void **state)
{
    char * input1 = strdup("agent 000 inventory port");
    char * output1 = calloc(1, OS_MAXSTR + 1);
    *output1 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: port");

    int ret = wdb_parse(input1, output1);

    assert_int_equal(ret, -1);
    assert_string_equal(output1, "err Invalid inventory query syntax, near 'port'");

    free(input1);
    free(output1);

    char * input2 = strdup("agent 000 inventory port save");
    char * output2 = calloc(1, OS_MAXSTR + 1);
    *output2 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: save");

    ret = wdb_parse(input2, output2);

    assert_int_equal(ret, -1);
    assert_string_equal(output2, "err Invalid inventory query syntax, near 'save'");

    free(input2);
    free(output2);

    char * input3 = strdup("agent 000 inventory port open {}");
    char * output3 = calloc(1, OS_MAXSTR + 1);
    *output3 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: open");

    ret = wdb_parse(input3, output3);

    assert_int_equal(ret, -1);
    assert_string_equal(output3, "err Invalid inventory query syntax, near 'open'");

    free(input3);
    free(output3);
}

void test_parse_inventory_port_save(void **state)
{
    char * input = strdup("agent 000 inventory port save {\"type\":\"modified\"}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_save_port, payload, "{\"type\":\"modified\"}");
    will_return(__wrap_wdb_inventory_save_port, 0);

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, 0);
    assert_string_equal(output, "ok");

    free(input);
    free(output);
}

void test_parse_inventory_port_save_error(void **state)
{
    char * input = strdup("agent 000 inventory port save {\"type\":\"added\"}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_save_port, payload, "{\"type\":\"added\"}");
    will_return(__wrap_wdb_inventory_save_port, -1);

    expect_string(__wrap__mdebug1, formatted_msg, "Cannot save port information.");

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, -1);
    assert_string_equal(output, "err Cannot save port information.");

    free(input);
    free(output);
}

void test_parse_inventory_port_delete(void **state)
{
    char * input = strdup("agent 000 inventory port delete {\"type\":\"deleted\"}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_delete_port, payload, "{\"type\":\"deleted\"}");
    will_return(__wrap_wdb_inventory_delete_port, 0);

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, 0);
    assert_string_equal(output, "ok");

    free(input);
    free(output);
}

void test_parse_inventory_port_delete_error(void **state)
{
    char * input = strdup("agent 000 inventory port delete {\"type\":\"deleted\"}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_delete_port, payload, "{\"type\":\"deleted\"}");
    will_return(__wrap_wdb_inventory_delete_port, -1);

    expect_string(__wrap__mdebug1, formatted_msg, "Cannot delete old port information.");

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, -1);
    assert_string_equal(output, "err Cannot delete old port information.");

    free(input);
    free(output);
}

void test_parse_inventory_process_invalid_query(void **state)
{
    char * input1 = strdup("agent 000 inventory process");
    char * output1 = calloc(1, OS_MAXSTR + 1);
    *output1 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: process");

    int ret = wdb_parse(input1, output1);

    assert_int_equal(ret, -1);
    assert_string_equal(output1, "err Invalid inventory query syntax, near 'process'");

    free(input1);
    free(output1);

    char * input2 = strdup("agent 000 inventory process save");
    char * output2 = calloc(1, OS_MAXSTR + 1);
    *output2 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: save");

    ret = wdb_parse(input2, output2);

    assert_int_equal(ret, -1);
    assert_string_equal(output2, "err Invalid inventory query syntax, near 'save'");

    free(input2);
    free(output2);

    char * input3 = strdup("agent 000 inventory process start {}");
    char * output3 = calloc(1, OS_MAXSTR + 1);
    *output3 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: start");

    ret = wdb_parse(input3, output3);

    assert_int_equal(ret, -1);
    assert_string_equal(output3, "err Invalid inventory query syntax, near 'start'");

    free(input3);
    free(output3);
}

void test_parse_inventory_process_save(void **state)
{
    char * input = strdup("agent 000 inventory process save {\"type\":\"added\"}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_save_process, payload, "{\"type\":\"added\"}");
    will_return(__wrap_wdb_inventory_save_process, 0);

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, 0);
    assert_string_equal(output, "ok");

    free(input);
    free(output);
}

void test_parse_inventory_process_save_error(void **state)
{
    char * input = strdup("agent 000 inventory process save {\"type\":\"added\"}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_save_process, payload, "{\"type\":\"added\"}");
    will_return(__wrap_wdb_inventory_save_process, -1);

    expect_string(__wrap__mdebug1, formatted_msg, "Cannot save process information.");

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, -1);
    assert_string_equal(output, "err Cannot save process information.");

    free(input);
    free(output);
}

void test_parse_inventory_process_delete(void **state)
{
    char * input = strdup("agent 000 inventory process delete {\"type\":\"deleted\"}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_delete_process, payload, "{\"type\":\"deleted\"}");
    will_return(__wrap_wdb_inventory_delete_process, 0);

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, 0);
    assert_string_equal(output, "ok");

    free(input);
    free(output);
}

void test_parse_inventory_process_delete_error(void **state)
{
    char * input = strdup("agent 000 inventory process delete {\"type\":\"deleted\"}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_delete_process, payload, "{\"type\":\"deleted\"}");
    will_return(__wrap_wdb_inventory_delete_process, -1);

    expect_string(__wrap__mdebug1, formatted_msg, "Cannot delete old process information.");

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, -1);
    assert_string_equal(output, "err Cannot delete old process information.");

    free(input);
    free(output);
}

void test_parse_inventory_network_scan_invalid_query(void **state)
{
    char * input1 = strdup("agent 000 inventory network_scan");
    char * output1 = calloc(1, OS_MAXSTR + 1);
    *output1 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: network_scan");

    int ret = wdb_parse(input1, output1);

    assert_int_equal(ret, -1);
    assert_string_equal(output1, "err Invalid inventory query syntax, near 'network_scan'");

    free(input1);
    free(output1);

    char * input2 = strdup("agent 000 inventory network_scan update");
    char * output2 = calloc(1, OS_MAXSTR + 1);
    *output2 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: update");

    ret = wdb_parse(input2, output2);

    assert_int_equal(ret, -1);
    assert_string_equal(output2, "err Invalid inventory query syntax, near 'update'");

    free(input2);
    free(output2);

    char * input3 = strdup("agent 000 inventory network_scan save {}");
    char * output3 = calloc(1, OS_MAXSTR + 1);
    *output3 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: save");

    ret = wdb_parse(input3, output3);

    assert_int_equal(ret, -1);
    assert_string_equal(output3, "err Invalid inventory query syntax, near 'save'");

    free(input3);
    free(output3);
}

void test_parse_inventory_network_scan_save(void **state)
{
    char * input = strdup("agent 000 inventory network_scan update {\"timestamp\":12345}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_save_scan_info, inventory, "network");
    expect_string(__wrap_wdb_inventory_save_scan_info, payload, "{\"timestamp\":12345}");
    will_return(__wrap_wdb_inventory_save_scan_info, 0);

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, 0);
    assert_string_equal(output, "ok");

    free(input);
    free(output);
}

void test_parse_inventory_network_scan_save_error(void **state)
{
    char * input = strdup("agent 000 inventory network_scan update {\"timestamp\":12345}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_save_scan_info, inventory, "network");
    expect_string(__wrap_wdb_inventory_save_scan_info, payload, "{\"timestamp\":12345}");
    will_return(__wrap_wdb_inventory_save_scan_info, -1);

    expect_string(__wrap__mdebug1, formatted_msg, "Cannot save network scan information.");

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, -1);
    assert_string_equal(output, "err Cannot save network scan information.");

    free(input);
    free(output);
}

void test_parse_inventory_os_scan_invalid_query(void **state)
{
    char * input1 = strdup("agent 000 inventory OS_scan");
    char * output1 = calloc(1, OS_MAXSTR + 1);
    *output1 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: OS_scan");

    int ret = wdb_parse(input1, output1);

    assert_int_equal(ret, -1);
    assert_string_equal(output1, "err Invalid inventory query syntax, near 'OS_scan'");

    free(input1);
    free(output1);

    char * input2 = strdup("agent 000 inventory OS_scan update");
    char * output2 = calloc(1, OS_MAXSTR + 1);
    *output2 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: update");

    ret = wdb_parse(input2, output2);

    assert_int_equal(ret, -1);
    assert_string_equal(output2, "err Invalid inventory query syntax, near 'update'");

    free(input2);
    free(output2);

    char * input3 = strdup("agent 000 inventory OS_scan save {}");
    char * output3 = calloc(1, OS_MAXSTR + 1);
    *output3 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: save");

    ret = wdb_parse(input3, output3);

    assert_int_equal(ret, -1);
    assert_string_equal(output3, "err Invalid inventory query syntax, near 'save'");

    free(input3);
    free(output3);
}

void test_parse_inventory_os_scan_save(void **state)
{
    char * input = strdup("agent 000 inventory OS_scan update {\"timestamp\":12345}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_save_scan_info, inventory, "OS");
    expect_string(__wrap_wdb_inventory_save_scan_info, payload, "{\"timestamp\":12345}");
    will_return(__wrap_wdb_inventory_save_scan_info, 0);

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, 0);
    assert_string_equal(output, "ok");

    free(input);
    free(output);
}

void test_parse_inventory_os_scan_save_error(void **state)
{
    char * input = strdup("agent 000 inventory OS_scan update {\"timestamp\":12345}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_save_scan_info, inventory, "OS");
    expect_string(__wrap_wdb_inventory_save_scan_info, payload, "{\"timestamp\":12345}");
    will_return(__wrap_wdb_inventory_save_scan_info, -1);

    expect_string(__wrap__mdebug1, formatted_msg, "Cannot save OS scan information.");

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, -1);
    assert_string_equal(output, "err Cannot save OS scan information.");

    free(input);
    free(output);
}

void test_parse_inventory_hw_scan_invalid_query(void **state)
{
    char * input1 = strdup("agent 000 inventory hardware_scan");
    char * output1 = calloc(1, OS_MAXSTR + 1);
    *output1 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: hardware_scan");

    int ret = wdb_parse(input1, output1);

    assert_int_equal(ret, -1);
    assert_string_equal(output1, "err Invalid inventory query syntax, near 'hardware_scan'");

    free(input1);
    free(output1);

    char * input2 = strdup("agent 000 inventory hardware_scan update");
    char * output2 = calloc(1, OS_MAXSTR + 1);
    *output2 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: update");

    ret = wdb_parse(input2, output2);

    assert_int_equal(ret, -1);
    assert_string_equal(output2, "err Invalid inventory query syntax, near 'update'");

    free(input2);
    free(output2);

    char * input3 = strdup("agent 000 inventory hardware_scan save {}");
    char * output3 = calloc(1, OS_MAXSTR + 1);
    *output3 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: save");

    ret = wdb_parse(input3, output3);

    assert_int_equal(ret, -1);
    assert_string_equal(output3, "err Invalid inventory query syntax, near 'save'");

    free(input3);
    free(output3);
}

void test_parse_inventory_hw_scan_save(void **state)
{
    char * input = strdup("agent 000 inventory hardware_scan update {\"timestamp\":12345}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_save_scan_info, inventory, "hardware");
    expect_string(__wrap_wdb_inventory_save_scan_info, payload, "{\"timestamp\":12345}");
    will_return(__wrap_wdb_inventory_save_scan_info, 0);

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, 0);
    assert_string_equal(output, "ok");

    free(input);
    free(output);
}

void test_parse_inventory_hw_scan_save_error(void **state)
{
    char * input = strdup("agent 000 inventory hardware_scan update {\"timestamp\":12345}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_save_scan_info, inventory, "hardware");
    expect_string(__wrap_wdb_inventory_save_scan_info, payload, "{\"timestamp\":12345}");
    will_return(__wrap_wdb_inventory_save_scan_info, -1);

    expect_string(__wrap__mdebug1, formatted_msg, "Cannot save hardware scan information.");

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, -1);
    assert_string_equal(output, "err Cannot save hardware scan information.");

    free(input);
    free(output);
}

void test_parse_inventory_program_scan_invalid_query(void **state)
{
    char * input1 = strdup("agent 000 inventory program_scan");
    char * output1 = calloc(1, OS_MAXSTR + 1);
    *output1 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: program_scan");

    int ret = wdb_parse(input1, output1);

    assert_int_equal(ret, -1);
    assert_string_equal(output1, "err Invalid inventory query syntax, near 'program_scan'");

    free(input1);
    free(output1);

    char * input2 = strdup("agent 000 inventory program_scan update");
    char * output2 = calloc(1, OS_MAXSTR + 1);
    *output2 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: update");

    ret = wdb_parse(input2, output2);

    assert_int_equal(ret, -1);
    assert_string_equal(output2, "err Invalid inventory query syntax, near 'update'");

    free(input2);
    free(output2);

    char * input3 = strdup("agent 000 inventory program_scan save {}");
    char * output3 = calloc(1, OS_MAXSTR + 1);
    *output3 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: save");

    ret = wdb_parse(input3, output3);

    assert_int_equal(ret, -1);
    assert_string_equal(output3, "err Invalid inventory query syntax, near 'save'");

    free(input3);
    free(output3);
}

void test_parse_inventory_program_scan_save(void **state)
{
    char * input = strdup("agent 000 inventory program_scan update {\"timestamp\":12345}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_save_scan_info, inventory, "program");
    expect_string(__wrap_wdb_inventory_save_scan_info, payload, "{\"timestamp\":12345}");
    will_return(__wrap_wdb_inventory_save_scan_info, 0);

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, 0);
    assert_string_equal(output, "ok");

    free(input);
    free(output);
}

void test_parse_inventory_program_scan_save_error(void **state)
{
    char * input = strdup("agent 000 inventory program_scan update {\"timestamp\":12345}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_save_scan_info, inventory, "program");
    expect_string(__wrap_wdb_inventory_save_scan_info, payload, "{\"timestamp\":12345}");
    will_return(__wrap_wdb_inventory_save_scan_info, -1);

    expect_string(__wrap__mdebug1, formatted_msg, "Cannot save program scan information.");

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, -1);
    assert_string_equal(output, "err Cannot save program scan information.");

    free(input);
    free(output);
}

void test_parse_inventory_hotfix_scan_invalid_query(void **state)
{
    char * input1 = strdup("agent 000 inventory hotfix_scan");
    char * output1 = calloc(1, OS_MAXSTR + 1);
    *output1 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: hotfix_scan");

    int ret = wdb_parse(input1, output1);

    assert_int_equal(ret, -1);
    assert_string_equal(output1, "err Invalid inventory query syntax, near 'hotfix_scan'");

    free(input1);
    free(output1);

    char * input2 = strdup("agent 000 inventory hotfix_scan update");
    char * output2 = calloc(1, OS_MAXSTR + 1);
    *output2 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: update");

    ret = wdb_parse(input2, output2);

    assert_int_equal(ret, -1);
    assert_string_equal(output2, "err Invalid inventory query syntax, near 'update'");

    free(input2);
    free(output2);

    char * input3 = strdup("agent 000 inventory hotfix_scan save {}");
    char * output3 = calloc(1, OS_MAXSTR + 1);
    *output3 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: save");

    ret = wdb_parse(input3, output3);

    assert_int_equal(ret, -1);
    assert_string_equal(output3, "err Invalid inventory query syntax, near 'save'");

    free(input3);
    free(output3);
}

void test_parse_inventory_hotfix_scan_save(void **state)
{
    char * input = strdup("agent 000 inventory hotfix_scan update {\"timestamp\":12345}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_save_scan_info, inventory, "hotfix");
    expect_string(__wrap_wdb_inventory_save_scan_info, payload, "{\"timestamp\":12345}");
    will_return(__wrap_wdb_inventory_save_scan_info, 0);

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, 0);
    assert_string_equal(output, "ok");

    free(input);
    free(output);
}

void test_parse_inventory_hotfix_scan_save_error(void **state)
{
    char * input = strdup("agent 000 inventory hotfix_scan update {\"timestamp\":12345}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_save_scan_info, inventory, "hotfix");
    expect_string(__wrap_wdb_inventory_save_scan_info, payload, "{\"timestamp\":12345}");
    will_return(__wrap_wdb_inventory_save_scan_info, -1);

    expect_string(__wrap__mdebug1, formatted_msg, "Cannot save hotfix scan information.");

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, -1);
    assert_string_equal(output, "err Cannot save hotfix scan information.");

    free(input);
    free(output);
}

void test_parse_inventory_port_scan_invalid_query(void **state)
{
    char * input1 = strdup("agent 000 inventory port_scan");
    char * output1 = calloc(1, OS_MAXSTR + 1);
    *output1 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: port_scan");

    int ret = wdb_parse(input1, output1);

    assert_int_equal(ret, -1);
    assert_string_equal(output1, "err Invalid inventory query syntax, near 'port_scan'");

    free(input1);
    free(output1);

    char * input2 = strdup("agent 000 inventory port_scan update");
    char * output2 = calloc(1, OS_MAXSTR + 1);
    *output2 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: update");

    ret = wdb_parse(input2, output2);

    assert_int_equal(ret, -1);
    assert_string_equal(output2, "err Invalid inventory query syntax, near 'update'");

    free(input2);
    free(output2);

    char * input3 = strdup("agent 000 inventory port_scan save {}");
    char * output3 = calloc(1, OS_MAXSTR + 1);
    *output3 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: save");

    ret = wdb_parse(input3, output3);

    assert_int_equal(ret, -1);
    assert_string_equal(output3, "err Invalid inventory query syntax, near 'save'");

    free(input3);
    free(output3);
}

void test_parse_inventory_port_scan_save(void **state)
{
    char * input = strdup("agent 000 inventory port_scan update {\"timestamp\":12345}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_save_scan_info, inventory, "port");
    expect_string(__wrap_wdb_inventory_save_scan_info, payload, "{\"timestamp\":12345}");
    will_return(__wrap_wdb_inventory_save_scan_info, 0);

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, 0);
    assert_string_equal(output, "ok");

    free(input);
    free(output);
}

void test_parse_inventory_port_scan_save_error(void **state)
{
    char * input = strdup("agent 000 inventory port_scan update {\"timestamp\":12345}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_save_scan_info, inventory, "port");
    expect_string(__wrap_wdb_inventory_save_scan_info, payload, "{\"timestamp\":12345}");
    will_return(__wrap_wdb_inventory_save_scan_info, -1);

    expect_string(__wrap__mdebug1, formatted_msg, "Cannot save port scan information.");

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, -1);
    assert_string_equal(output, "err Cannot save port scan information.");

    free(input);
    free(output);
}

void test_parse_inventory_process_scan_invalid_query(void **state)
{
    char * input1 = strdup("agent 000 inventory process_scan");
    char * output1 = calloc(1, OS_MAXSTR + 1);
    *output1 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: process_scan");

    int ret = wdb_parse(input1, output1);

    assert_int_equal(ret, -1);
    assert_string_equal(output1, "err Invalid inventory query syntax, near 'process_scan'");

    free(input1);
    free(output1);

    char * input2 = strdup("agent 000 inventory process_scan update");
    char * output2 = calloc(1, OS_MAXSTR + 1);
    *output2 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: update");

    ret = wdb_parse(input2, output2);

    assert_int_equal(ret, -1);
    assert_string_equal(output2, "err Invalid inventory query syntax, near 'update'");

    free(input2);
    free(output2);

    char * input3 = strdup("agent 000 inventory process_scan save {}");
    char * output3 = calloc(1, OS_MAXSTR + 1);
    *output3 = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap__mdebug1, formatted_msg, "DB(000) Invalid inventory query syntax: save");

    ret = wdb_parse(input3, output3);

    assert_int_equal(ret, -1);
    assert_string_equal(output3, "err Invalid inventory query syntax, near 'save'");

    free(input3);
    free(output3);
}

void test_parse_inventory_process_scan_save(void **state)
{
    char * input = strdup("agent 000 inventory process_scan update {\"timestamp\":12345}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_save_scan_info, inventory, "process");
    expect_string(__wrap_wdb_inventory_save_scan_info, payload, "{\"timestamp\":12345}");
    will_return(__wrap_wdb_inventory_save_scan_info, 0);

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, 0);
    assert_string_equal(output, "ok");

    free(input);
    free(output);
}

void test_parse_inventory_process_scan_save_error(void **state)
{
    char * input = strdup("agent 000 inventory process_scan update {\"timestamp\":12345}");
    char * output = calloc(1, OS_MAXSTR + 1);
    *output = '\0';

    will_return(__wrap_wdb_open_agent2, 1);

    expect_string(__wrap_wdb_inventory_save_scan_info, inventory, "process");
    expect_string(__wrap_wdb_inventory_save_scan_info, payload, "{\"timestamp\":12345}");
    will_return(__wrap_wdb_inventory_save_scan_info, -1);

    expect_string(__wrap__mdebug1, formatted_msg, "Cannot save process scan information.");

    int ret = wdb_parse(input, output);

    assert_int_equal(ret, -1);
    assert_string_equal(output, "err Cannot save process scan information.");

    free(input);
    free(output);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_parse_no_input),
        cmocka_unit_test(test_parse_invalid_actor),
        cmocka_unit_test(test_parse_invalid_agent_id),
        cmocka_unit_test(test_parse_inventory_invalid_type),
        cmocka_unit_test(test_parse_inventory_network_invalid_query),
        cmocka_unit_test(test_parse_inventory_network_save),
        cmocka_unit_test(test_parse_inventory_network_save_error),
        cmocka_unit_test(test_parse_inventory_network_delete),
        cmocka_unit_test(test_parse_inventory_network_delete_error),
        cmocka_unit_test(test_parse_inventory_os_invalid_query),
        cmocka_unit_test(test_parse_inventory_os_save),
        cmocka_unit_test(test_parse_inventory_os_save_error),
        cmocka_unit_test(test_parse_inventory_hw_invalid_query),
        cmocka_unit_test(test_parse_inventory_hw_save),
        cmocka_unit_test(test_parse_inventory_hw_save_error),
        cmocka_unit_test(test_parse_inventory_program_invalid_query),
        cmocka_unit_test(test_parse_inventory_program_save),
        cmocka_unit_test(test_parse_inventory_program_save_error),
        cmocka_unit_test(test_parse_inventory_program_delete),
        cmocka_unit_test(test_parse_inventory_program_delete_error),
        cmocka_unit_test(test_parse_inventory_hotfix_invalid_query),
        cmocka_unit_test(test_parse_inventory_hotfix_save),
        cmocka_unit_test(test_parse_inventory_hotfix_save_error),
        cmocka_unit_test(test_parse_inventory_hotfix_delete),
        cmocka_unit_test(test_parse_inventory_hotfix_delete_error),
        cmocka_unit_test(test_parse_inventory_port_invalid_query),
        cmocka_unit_test(test_parse_inventory_port_save),
        cmocka_unit_test(test_parse_inventory_port_save_error),
        cmocka_unit_test(test_parse_inventory_port_delete),
        cmocka_unit_test(test_parse_inventory_port_delete_error),
        cmocka_unit_test(test_parse_inventory_process_invalid_query),
        cmocka_unit_test(test_parse_inventory_process_save),
        cmocka_unit_test(test_parse_inventory_process_save_error),
        cmocka_unit_test(test_parse_inventory_process_delete),
        cmocka_unit_test(test_parse_inventory_process_delete_error),
        cmocka_unit_test(test_parse_inventory_network_scan_invalid_query),
        cmocka_unit_test(test_parse_inventory_network_scan_save),
        cmocka_unit_test(test_parse_inventory_network_scan_save_error),
        cmocka_unit_test(test_parse_inventory_os_scan_invalid_query),
        cmocka_unit_test(test_parse_inventory_os_scan_save),
        cmocka_unit_test(test_parse_inventory_os_scan_save_error),
        cmocka_unit_test(test_parse_inventory_hw_scan_invalid_query),
        cmocka_unit_test(test_parse_inventory_hw_scan_save),
        cmocka_unit_test(test_parse_inventory_hw_scan_save_error),
        cmocka_unit_test(test_parse_inventory_program_scan_invalid_query),
        cmocka_unit_test(test_parse_inventory_program_scan_save),
        cmocka_unit_test(test_parse_inventory_program_scan_save_error),
        cmocka_unit_test(test_parse_inventory_hotfix_scan_invalid_query),
        cmocka_unit_test(test_parse_inventory_hotfix_scan_save),
        cmocka_unit_test(test_parse_inventory_hotfix_scan_save_error),
        cmocka_unit_test(test_parse_inventory_port_scan_invalid_query),
        cmocka_unit_test(test_parse_inventory_port_scan_save),
        cmocka_unit_test(test_parse_inventory_port_scan_save_error),
        cmocka_unit_test(test_parse_inventory_process_scan_invalid_query),
        cmocka_unit_test(test_parse_inventory_process_scan_save),
        cmocka_unit_test(test_parse_inventory_process_scan_save_error)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
