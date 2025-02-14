/* Copyright (C) 2015, Wazuh Inc.
 * Copyright (C) 2009 Trend Micro Inc.
 * All right reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */

#include "shared.h"
#include "rules.h"
#include "eventinfo.h"
#include "analysisd.h"

#ifdef WAZUH_UNIT_TESTING
// Remove STATIC qualifier from tests
#define STATIC
#else
#define STATIC static
#endif


/* _OS_Addrule: Internal AddRule */
STATIC RuleNode *_OS_AddRule(RuleNode *_rulenode, RuleInfo *read_rule);
STATIC int _AddtoRule(int sid, int level, int none, const char *group,
               RuleNode *r_node, RuleInfo *read_rule);


RuleNode *os_analysisd_rulelist;

/* Create the RuleList */
void OS_CreateRuleList() {
    os_analysisd_rulelist = NULL;
}

/* Get first node from rule */
RuleNode *OS_GetFirstRule()
{
    RuleNode *rulenode_pt = os_analysisd_rulelist;
    return (rulenode_pt);
}

/* Search all rules, including children */
STATIC int _AddtoRule(int sid, int level, int none, const char *group,
               RuleNode *r_node, RuleInfo *read_rule)
{
    int r_code = 0;

    /* If we don't have the first node, start from
     * the beginning of the list
     */
    if (!r_node) {
        return r_code;
    }

    while (r_node) {
        /* Check if the sigid matches */
        if (sid) {
            if (r_node->ruleinfo->sigid == sid) {
                /* Assign the category of this rule to the child
                 * as they must match
                 */
                read_rule->category = r_node->ruleinfo->category;

                r_node->child =
                    _OS_AddRule(r_node->child, read_rule);
                return (1);
            }
        }

        /* Check if the group matches */
        else if (group) {
            if (OS_WordMatch(group, r_node->ruleinfo->group) &&
                    (r_node->ruleinfo->sigid != read_rule->sigid)) {
                /* Loop over all rules until we find it */
                r_node->child =
                    _OS_AddRule(r_node->child, read_rule);
                r_code = 1;
            }
        }

        /* Check if the level matches */
        else if (level) {
            if ((r_node->ruleinfo->level >= level) &&
                    (r_node->ruleinfo->sigid != read_rule->sigid)) {
                r_node->child =
                    _OS_AddRule(r_node->child, read_rule);
                r_code = 1;
            }
        }

        /* If we are not searching for the sid/group, the category must
         * be the same
         */
        else if (read_rule->category != r_node->ruleinfo->category) {
            r_node = r_node->next;
            continue;
        }

        /* If none of them are set, add for the category */
        else {
            /* Set the parent category to it */
            read_rule->category = r_node->ruleinfo->category;
            r_node->child =
                _OS_AddRule(r_node->child, read_rule);
            return (1);
        }

        /* Check if the child has a rule */
        if (r_node->child) {
            if (_AddtoRule(sid, level, none, group, r_node->child, read_rule)) {
                r_code = 1;
            }
        }

        r_node = r_node->next;
    }

    return (r_code);
}

/* Add a child */
int OS_AddChild(RuleInfo *read_rule, RuleNode **r_node, OSList* log_msg)
{
    if (read_rule == NULL) {
        smwarn(log_msg, ANALYSISD_NULL_RULE);
        return -1;
    }

    /* Adding for if_sid */
    if (read_rule->if_sid != NULL) {
        int val = 0;
        const char * sid_ptr = read_rule->if_sid;

        /* Loop to read all the rules (comma or space separated) */
        do {
            if ((*sid_ptr == ',') || (*sid_ptr == ' ')) {
                val = 0;
                continue;
            } else if ((isdigit((int)*sid_ptr)) || (*sid_ptr == '\0')) {

                if (val == 0) {

                    int if_sid_rule_id = atoi(sid_ptr);

                    if (_AddtoRule(if_sid_rule_id, 0, 0, NULL, *r_node, read_rule) == 0) {

                        smwarn(log_msg, ANALYSISD_SIG_ID_NOT_FOUND,
                               if_sid_rule_id, read_rule->if_matched_sid != 0 ?
                                                    "if_matched_sid" : "if_sid",
                               read_rule->sigid);
                        return -1;
                    }
                    val = 1;
                }
            } else {

                smwarn(log_msg, ANALYSISD_INV_SIG_ID,
                        read_rule->if_matched_sid != 0 ? "if_matched_sid"
                                                       : "if_sid",
                        read_rule->sigid);
                return -1;
            }
        } while (*sid_ptr++ != '\0');
    }

    /* Adding for if_level */
    else if (read_rule->if_level != NULL) {

        int ilevel = atoi(read_rule->if_level);

        if (ilevel == 0) {
            smwarn(log_msg, ANALYSISD_INV_IF_LEVEL, read_rule->if_level, read_rule->sigid);
            return -1;
        }

        ilevel *= 100;

        if (_AddtoRule(0, ilevel, 0, NULL, *r_node, read_rule) == 0) {
            smwarn(log_msg, ANALYSISD_LEVEL_NOT_FOUND, ilevel, read_rule->sigid);
            return -1;
        }
    }

    /* Adding for if_group */
    else if (read_rule->if_group != NULL) {
        if (_AddtoRule(0, 0, 0, read_rule->if_group, *r_node, read_rule) == 0) {
            smwarn(log_msg, ANALYSISD_GROUP_NOT_FOUND, read_rule->if_group, read_rule->sigid);
            return -1;
        }
    }

    /* Just add based on the category */
    else {
        if (_AddtoRule(0, 0, 0, NULL, *r_node, read_rule) == 0) {
            smwarn(log_msg, ANALYSISD_CATEGORY_NOT_FOUND, read_rule->sigid);
            return -1;
        }
    }

    /* done over here */
    return (0);
}

/* Add a rule in the chain */
STATIC RuleNode *_OS_AddRule(RuleNode *_rulenode, RuleInfo *read_rule)
{
    RuleNode *tmp_rulenode = _rulenode;

    if (tmp_rulenode != NULL) {
        int middle_insertion = 0;
        RuleNode *prev_rulenode = NULL;
        RuleNode *new_rulenode = NULL;

        while (tmp_rulenode != NULL) {
            if (read_rule->level > tmp_rulenode->ruleinfo->level) {
                middle_insertion = 1;
                break;
            }
            prev_rulenode = tmp_rulenode;
            tmp_rulenode = tmp_rulenode->next;
        }

        new_rulenode = (RuleNode *)calloc(1, sizeof(RuleNode));

        if (!new_rulenode) {
            merror_exit(MEM_ERROR, errno, strerror(errno));
        }

        if (middle_insertion == 1) {
            if (prev_rulenode == NULL) {
                _rulenode = new_rulenode;
            } else {
                prev_rulenode->next = new_rulenode;
            }

            new_rulenode->next = tmp_rulenode;
            new_rulenode->ruleinfo = read_rule;
            new_rulenode->child = NULL;
        } else {
            prev_rulenode->next = new_rulenode;
            prev_rulenode->next->ruleinfo = read_rule;
            prev_rulenode->next->next = NULL;
            prev_rulenode->next->child = NULL;
        }
    } else {
        _rulenode = (RuleNode *)calloc(1, sizeof(RuleNode));
        if (_rulenode == NULL) {
            merror_exit(MEM_ERROR, errno, strerror(errno));
        }

        _rulenode->ruleinfo = read_rule;
        _rulenode->next = NULL;
        _rulenode->child = NULL;
    }

    return (_rulenode);
}

/* External AddRule */
int OS_AddRule(RuleInfo *read_rule, RuleNode **r_node)
{
    *r_node = _OS_AddRule(*r_node, read_rule);

    return (0);
}

/* Update rule info for overwritten ones */
int OS_AddRuleInfo(RuleNode *r_node, RuleInfo *newrule, int sid)
{
    /* If no r_node is given, get first node */
    if (r_node == NULL) {
        return -1;
    }

    if (sid == 0) {
        return (0);
    }

    while (r_node) {
        /* Check if the sigid matches */
        if (r_node->ruleinfo->sigid == sid) {
            os_remove_ruleinfo(r_node->ruleinfo);
            r_node->ruleinfo = newrule;
            return (1);
        }

        /* Check if the child has a rule */
        if (r_node->child) {
            if (OS_AddRuleInfo(r_node->child, newrule, sid)) {
                return (1);
            }
        }

        r_node = r_node->next;
    }

    return (0);
}

/* Mark rules that match specific id (for if_matched_sid) */
int OS_MarkID(RuleNode *r_node, RuleInfo *orig_rule)
{
    /* If no r_node is given, get first node */
    if (r_node == NULL) {
        return -1;
    }

    while (r_node) {
        if (r_node->ruleinfo->sigid == orig_rule->if_matched_sid) {
            /* If child does not have a list, create one */
            if (!r_node->ruleinfo->sid_prev_matched) {
                r_node->ruleinfo->sid_prev_matched = OSList_Create();
                if (!r_node->ruleinfo->sid_prev_matched) {
                    merror_exit(MEM_ERROR, errno, strerror(errno));
                }
            }

            /* Assign the parent pointer to it */
            orig_rule->sid_search = r_node->ruleinfo->sid_prev_matched;
        }

        /* Check if the child has a rule */
        if (r_node->child) {
            OS_MarkID(r_node->child, orig_rule);
        }

        r_node = r_node->next;
    }

    return (0);
}

/* Mark rules that match specific group (for if_matched_group) */
int OS_MarkGroup(RuleNode *r_node, RuleInfo *orig_rule)
{
    /* If no r_node is given, get first node */
    if (r_node == NULL) {
        return -1;
    }

    while (r_node) {
        if (OSMatch_Execute(r_node->ruleinfo->group,
                            strlen(r_node->ruleinfo->group),
                            orig_rule->if_matched_group)) {
            unsigned int rule_g = 0;
            if (r_node->ruleinfo->group_prev_matched) {
                while (r_node->ruleinfo->group_prev_matched[rule_g]) {
                    rule_g++;
                }
            }

            os_realloc(r_node->ruleinfo->group_prev_matched,
                       (rule_g + 2)*sizeof(OSList *),
                       r_node->ruleinfo->group_prev_matched);

            r_node->ruleinfo->group_prev_matched[rule_g] = NULL;
            r_node->ruleinfo->group_prev_matched[rule_g + 1] = NULL;

            /* Set the size */
            r_node->ruleinfo->group_prev_matched_sz = rule_g + 1;

            r_node->ruleinfo->group_prev_matched[rule_g] =
                orig_rule->group_search;
        }

        /* Check if the child has a rule */
        if (r_node->child) {
            OS_MarkGroup(r_node->child, orig_rule);
        }

        r_node = r_node->next;
    }

    return (0);
}

void os_remove_rules_list(RuleNode *node) {

    RuleInfo **rules;
    int pos = 0;
    int num_rules = 0;

    os_count_rules(node, &num_rules);

    os_calloc(num_rules + 1, sizeof(RuleInfo *), rules);

    os_remove_rulenode(node, rules, &pos, &num_rules);

    for (int i = 0; i <= pos; i++) {
        os_remove_ruleinfo(rules[i]);
    }

    os_free(rules);
}


void os_remove_rulenode(RuleNode *node, RuleInfo **rules, int *pos, int *max_size) {

    RuleNode *tmp;

    while (node) {

        if (node->child) {
            os_remove_rulenode(node->child, rules, pos, max_size);
        }

        tmp = node;
        node = node->next;

        if (tmp->ruleinfo->internal_saving == false && *pos <= *max_size) {

            tmp->ruleinfo->internal_saving = true;
            rules[*pos] = tmp->ruleinfo;
            (*pos)++;
        }

        os_free(tmp);
    }
}

void os_remove_ruleinfo(RuleInfo *ruleinfo) {

    if (!ruleinfo) {
        return;
    }

    if (ruleinfo->ignore_fields) {
        for (int i = 0; ruleinfo->ignore_fields[i]; i++) {
            os_free(ruleinfo->ignore_fields[i]);
        }
    }

    if (ruleinfo->ckignore_fields) {
        for (int i = 0; ruleinfo->ckignore_fields[i]; i++) {
            os_free(ruleinfo->ckignore_fields[i]);
        }
    }

    w_free_expression_t(&ruleinfo->srcip);
    w_free_expression_t(&ruleinfo->dstip);

    if (ruleinfo->fields) {
        for (int i = 0; ruleinfo->fields[i]; i++) {
            os_free(ruleinfo->fields[i]->name);
            w_free_expression_t(&ruleinfo->fields[i]->regex);
            os_free(ruleinfo->fields[i]);
        }
    }

    if (ruleinfo->info_details) {
        RuleInfoDetail *tmp;
        while (ruleinfo->info_details) {
            tmp = ruleinfo->info_details;
            ruleinfo->info_details = ruleinfo->info_details->next;
            os_free(tmp->data);
            os_free(tmp);
        }
    }

    if (ruleinfo->ar) {
        for (int i = 0; ruleinfo->ar[i]; i++) {
            os_free(ruleinfo->ar[i]->name);
            os_free(ruleinfo->ar[i]->command);
            os_free(ruleinfo->ar[i]->agent_id);
            os_free(ruleinfo->ar[i]->rules_id);
            os_free(ruleinfo->ar[i]->rules_group);
            os_free(ruleinfo->ar[i]->ar_cmd->name);
            os_free(ruleinfo->ar[i]->ar_cmd->executable);
            os_free(ruleinfo->ar[i]->ar_cmd->extra_args);
            os_free(ruleinfo->ar[i]->ar_cmd);
            os_free(ruleinfo->ar[i]);
        }
    }

    if (ruleinfo->lists) {
        os_remove_cdbrules(&ruleinfo->lists);
    }

    if (ruleinfo->same_fields) {
        for (int i = 0; ruleinfo->same_fields[i]; i++) {
            os_free(ruleinfo->same_fields[i]);
        }
    }

    if (ruleinfo->not_same_fields) {
        for (int i = 0; ruleinfo->not_same_fields[i]; i++) {
            os_free(ruleinfo->not_same_fields[i]);
        }
    }

    if (ruleinfo->mitre_id) {
        for (int i = 0; ruleinfo->mitre_id[i]; i++) {
            os_free(ruleinfo->mitre_id[i]);
        }
    }

    if (ruleinfo->mitre_tactic_id) {
        for (int i = 0; ruleinfo->mitre_tactic_id[i]; i++) {
            os_free(ruleinfo->mitre_tactic_id[i]);
        }
    }

    if (ruleinfo->mitre_technique_id) {
        for (int i = 0; ruleinfo->mitre_technique_id[i]; i++) {
            os_free(ruleinfo->mitre_technique_id[i]);
        }
    }

    w_free_expression_t(&ruleinfo->match);
    w_free_expression_t(&ruleinfo->regex);
    w_free_expression_t(&ruleinfo->srcgeoip);
    w_free_expression_t(&ruleinfo->dstgeoip);
    w_free_expression_t(&ruleinfo->srcport);
    w_free_expression_t(&ruleinfo->dstport);
    w_free_expression_t(&ruleinfo->user);
    w_free_expression_t(&ruleinfo->url);
    w_free_expression_t(&ruleinfo->id);
    w_free_expression_t(&ruleinfo->status);
    w_free_expression_t(&ruleinfo->hostname);
    w_free_expression_t(&ruleinfo->program_name);
    w_free_expression_t(&ruleinfo->data);
    w_free_expression_t(&ruleinfo->extra_data);
    w_free_expression_t(&ruleinfo->location);
    w_free_expression_t(&ruleinfo->system_name);
    w_free_expression_t(&ruleinfo->protocol);
    w_free_expression_t(&ruleinfo->action);

    if (ruleinfo->if_matched_regex) OSRegex_FreePattern(ruleinfo->if_matched_regex);
    os_free(ruleinfo->if_matched_regex);

    if (ruleinfo->if_matched_group) OSMatch_FreePattern(ruleinfo->if_matched_group);
    os_free(ruleinfo->if_matched_group);

    os_free(ruleinfo->sid_prev_matched);
    os_free(ruleinfo->group_search);
    os_free(ruleinfo->group_prev_matched);

    os_free(ruleinfo->ignore_fields);
    os_free(ruleinfo->ckignore_fields);
    os_free(ruleinfo->srcip);
    os_free(ruleinfo->dstip);
    os_free(ruleinfo->fields);
    os_free(ruleinfo->group);
    os_free(ruleinfo->day_time);
    os_free(ruleinfo->week_day);
    os_free(ruleinfo->comment);
    os_free(ruleinfo->info);
    os_free(ruleinfo->cve);
    os_free(ruleinfo->if_sid);
    os_free(ruleinfo->if_level);
    os_free(ruleinfo->if_group);
    os_free(ruleinfo->ar);
    os_free(ruleinfo->file);
    os_free(ruleinfo->same_fields);
    os_free(ruleinfo->not_same_fields);
    os_free(ruleinfo->mitre_id);
    os_free(ruleinfo->mitre_tactic_id);
    os_free(ruleinfo->mitre_technique_id);

    os_free(ruleinfo);
}

void os_count_rules(RuleNode *node, int *num_rules) {

    while (node) {

        if (node->child) {
            os_count_rules(node->child, num_rules);
        }

        (*num_rules)++;

        node = node->next;
    }
}
