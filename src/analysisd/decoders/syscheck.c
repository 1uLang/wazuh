/*   $OSSEC, syscheck.c, v0.1, 2005/07/19, Daniel B. Cid$   */

/* Copyright (C) 2005 Daniel B. Cid <dcid@ossec.net>
 * All right reserved.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */


/* Syscheck decoder */

#include "eventinfo.h"
#include "os_regex/os_regex.h"
#include "config.h"
#include "alerts/alerts.h"


#define SYSCHECK_DIR    "/queue/syscheck"


typedef struct __sdb
{
    char buf[OS_MAXSTR + 1];
    char comment[OS_MAXSTR +1];
    char comment2[OS_MAXSTR +1];

    char size[OS_FLSIZE +1];
    char perm[OS_FLSIZE +1];
    char owner[OS_FLSIZE +1];
    char gowner[OS_FLSIZE +1];
    char md5[OS_FLSIZE +1];
    char sha1[OS_FLSIZE +1];

    char agent_cp[MAX_AGENTS +1][1];
    char *agent_ips[MAX_AGENTS +1];
    FILE *agent_fps[MAX_AGENTS +1];

    int db_err;

    /* Syscheck rule */
    RuleInfo *syscheck_rule;


    /* File search variables */
    fpos_t init_pos;
    
}_sdb; /* syscheck db information */


/* Global variable */
_sdb sdb;



/* SyscheckInit
 * Initialize the necessary information to process the syscheck information
 */
void SyscheckInit()
{
    int i = 0;

    sdb.db_err = 0;
    
    for(;i <= MAX_AGENTS;i++)
    {
        sdb.agent_ips[i] = NULL;
        sdb.agent_fps[i] = NULL;
        sdb.agent_cp[i][0] = '0';
    }

    /* Clearing db memory */
    memset(sdb.buf, '\0', OS_MAXSTR +1);
    memset(sdb.comment, '\0', OS_MAXSTR +1);
    memset(sdb.comment2, '\0', OS_MAXSTR +1);
    
    memset(sdb.size, '\0', OS_FLSIZE +1);
    memset(sdb.perm, '\0', OS_FLSIZE +1);
    memset(sdb.owner, '\0', OS_FLSIZE +1);
    memset(sdb.gowner, '\0', OS_FLSIZE +1);
    memset(sdb.md5, '\0', OS_FLSIZE +1);
    memset(sdb.sha1, '\0', OS_FLSIZE +1);


    /* Zeroring syscheck rule */
    sdb.syscheck_rule = zerorulemember(
                        SYSCHECK_PLUGIN,
                        Config.integrity,
                        0,0,0,0,0);

    if(!sdb.syscheck_rule)
    {
        ErrorExit(MEM_ERROR, ARGV0);
    }


    debug1("%s: SyscheckInit completed.", ARGV0);
    return;
}

/* DB_IsCompleted
 * Checks if the db is completed for that specific agent.
 */
#define DB_IsCompleted(x) (sdb.agent_cp[x][0] == '1')?1:0


void __setcompleted(char *agent)
{
    FILE *fp;
    
    /* Getting agent file */
    snprintf(sdb.buf, OS_MAXSTR , "%s/.%s.cpt", SYSCHECK_DIR, agent);

    fp = fopen(sdb.buf,"w");
    if(fp)
    {
        fprintf(fp, "#!X");
        fclose(fp);
    }
}


int __iscompleted(char *agent)
{
    FILE *fp;

    /* Getting agent file */
    snprintf(sdb.buf, OS_MAXSTR , "%s/.%s.cpt", SYSCHECK_DIR, agent);

    fp = fopen(sdb.buf,"r");
    if(fp)
    {
        fclose(fp);
        return(1);
    }
    return(0);
}


/* void DB_SetCompleted(Eventinfo *lf).
 * Set the database of a specific agent as completed.
 */
void DB_SetCompleted(Eventinfo *lf)
{
    int i = 0;

    /* Finding file pointer */
    while(sdb.agent_ips[i] != NULL)
    {
        if(strcmp(sdb.agent_ips[i], lf->location) == 0)
        {
            /* Return if already set as completed. */
            if(DB_IsCompleted(i))
            {
                return;
            }
            
            __setcompleted(lf->location);
        }

        i++;
    }
}


/* DB_File
 * Return the file pointer to be used to verify the integrity
 */
FILE *DB_File(char *agent, int *agent_id)
{
    int i = 0;

    /* Finding file pointer */
    while(sdb.agent_ips[i] != NULL)
    {
        if(strcmp(sdb.agent_ips[i], agent) == 0)
        {
            /* pointing to the beginning of the file */
            fseek(sdb.agent_fps[i],0, SEEK_SET);
            *agent_id = i;
            return(sdb.agent_fps[i]);
        }
        
        i++;    
    }

    /* If here, our agent wasn't found */
    os_strdup(agent, sdb.agent_ips[i]);


    /* Getting agent file */
    snprintf(sdb.buf, OS_MAXSTR , "%s/%s", SYSCHECK_DIR,agent);
    
        
    /* r+ to read and write. Do not truncate */
    sdb.agent_fps[i] = fopen(sdb.buf,"r+");
    if(!sdb.agent_fps[i])
    {
        /* try opening with a w flag, file probably does not exist */
        sdb.agent_fps[i] = fopen(sdb.buf, "w");
        if(sdb.agent_fps[i])
        {
            fclose(sdb.agent_fps[i]);
            sdb.agent_fps[i] = fopen(sdb.buf, "r+");
        }
    }
        
    /* Checking again */    
    if(!sdb.agent_fps[i])
    {
        merror("%s: Unable to open '%s'",ARGV0, sdb.buf);

        free(sdb.agent_ips[i]);
        sdb.agent_ips[i] = NULL;
        return(NULL);
    }


    /* Returning the opened pointer (the beginning of it) */
    fseek(sdb.agent_fps[i],0, SEEK_SET);
    *agent_id = i;
    
    
    /* Getting if the agent was completed */
    if(__iscompleted(agent))
    {
        sdb.agent_cp[i][0] = '1';    
    }

    return(sdb.agent_fps[i]);
}


/* DB_Search
 * Search the DB for any entry related to the file being received
 */
void DB_Search(char *f_name, char *c_sum, Eventinfo *lf)
{
    int p = 0;
    int sn_size;
    int agent_id;
    
    char *saved_sum;
    char *saved_name;
    
    FILE *fp;


    /* Getting db pointer */
    fp = DB_File(lf->location, &agent_id);
    if(!fp)
    {
        merror("%s: Error handling integrity database",ARGV0);
        sdb.db_err++; /* Increment db error */
        return;
    }


    /* Reads the integrity file and search for a possible
     * entry
     */
    if(fgetpos(fp, &sdb.init_pos) == -1)
    {
        merror("%s: Error handling integrity database (fgetpos).",ARGV0);
        return;
    }
    
    
    while(fgets(sdb.buf, OS_MAXSTR, fp) != NULL)
    {
        /* Ignore blank lines and lines with a comment */
        if(sdb.buf[0] == '\n' || sdb.buf[0] == '#')
        {
            fgetpos(fp, &sdb.init_pos); /* getting next location */
            continue;
        }
            
            
        saved_name = strchr(sdb.buf,' ');
        if(saved_name == NULL)
        {
            merror("%s: Invalid integrity message in the database",ARGV0);
            fgetpos(fp, &sdb.init_pos); /* getting next location */
            continue;
        }
        saved_name++;
        

        /* Removing the \n from saved_name */
        sn_size = strlen(saved_name);
        sn_size -= 1; /* 0 = \0, -1 = \n */
        if(saved_name[sn_size] == '\n')
            saved_name[sn_size] = '\0';
        
        
        if(strcmp(f_name,saved_name) == 0)
        {
            saved_name--;
            *saved_name = '\0';

            saved_sum = sdb.buf;


            /* First three bytes are for frequency check */
            saved_sum+=3;


            /* checksum match, we can just return and keep going */
            if(strcmp(saved_sum,c_sum) == 0)
                return;


            /* If we reached here, the checksum of the file has changed */
            if(saved_sum[-3] == '!')
            {
                p++;
                if(saved_sum[-2] == '!')
                {
                    p++;
                    if(saved_sum[-1] == '!')    
                        p++;
                    else if(saved_sum[-1] == '?')
                        p+=2;    
                }
            }
            
            
            /* Checking the number of changes */
            if(p >= 1)
            {
                if(p >= 2)
                {
                    if((p >= 3) && Config.syscheck_auto_ignore)
                    {
                        /* Ignoring it.. */
                        return;
                    }
                    
                    /* Third change */
                    snprintf(sdb.comment,OS_MAXSTR,
                                        "Integrity checksum of file '%s'"
                             " has changed again (third time or more).%s",
                             f_name, 
                             Config.syscheck_auto_ignore == 1?
                             "Ignoring it.":"");
                }
                else
                {
                    /* Second change */
                    snprintf(sdb.comment,OS_MAXSTR,
                                        "Integrity checksum of file '%s'"
                                        " has changed again (2nd time)",
                                        f_name);   
                }
            }
           
            /* First change */ 
            else
            {
                snprintf(sdb.comment,OS_MAXSTR,
                                    "Integrity checksum of file '%s' "
                                    "has changed.",f_name);
            }
      
      
            /* Adding new checksum to the database */
            /* Commenting the file entry and adding a new one latter */
            fsetpos(fp, &sdb.init_pos);
            fputc('#',fp);

            
            /* Adding the new entry at the end of the file */
            fseek(fp, 0, SEEK_END);
            fprintf(fp,"%c%c%c%s %s\n",
                        '!',
                        p >= 1? '!' : '+',
                        p == 2? '!' : (p > 2)?'?':'+',
                        c_sum,
                        f_name);
           
           
            /* File deleted */
            if(c_sum[0] == '-' && c_sum[1] == '1')
            {
                snprintf(sdb.comment2, OS_MAXSTR,
                            "File '%s' was deleted. Unable to retrieve "
                            "checksum.", f_name);
            }
            
            else    
            {
                /* Providing more info about the file change */
                char *oldsize = NULL, *newsize = NULL;
                char *olduid = NULL, *newuid = NULL;
                char *c_oldperm = NULL, *c_newperm = NULL;
                char *oldgid = NULL, *newgid = NULL;
                char *oldmd5 = NULL, *newmd5 = NULL;
                char *oldsha1 = NULL, *newsha1 = NULL;

                int oldperm = 0, newperm = 0;
                
                oldsize = saved_sum;
                newsize = c_sum;

                c_oldperm = strchr(saved_sum, ':');
                c_newperm = strchr(c_sum, ':');

                /* Get old/new permissions */
                if(c_oldperm && c_newperm)
                {
                    *c_oldperm = '\0';
                    c_oldperm++;

                    *c_newperm = '\0';
                    c_newperm++;

                    /* Get old/new uid/gid */
                    olduid = strchr(c_oldperm, ':');
                    newuid = strchr(c_newperm, ':');

                    if(olduid && newuid)
                    {
                        *olduid = '\0';
                        *newuid = '\0';

                        olduid++;
                        newuid++;

                        oldgid = strchr(olduid, ':');
                        newgid = strchr(newuid, ':');

                        if(oldgid && newgid)
                        {
                            *oldgid = '\0';
                            *newgid = '\0';

                            oldgid++;
                            newgid++;


                            /* Getting md5 */
                            oldmd5 = strchr(oldgid, ':');
                            newmd5 = strchr(newgid, ':');

                            if(oldmd5 && newmd5)
                            {
                                *oldmd5 = '\0';
                                *newmd5 = '\0';

                                oldmd5++;
                                newmd5++;
                                
                                /* getting sha1 */
                                oldsha1 = strchr(oldmd5, ':');
                                newsha1 = strchr(newmd5, ':');
                                
                                if(oldsha1 && newsha1)
                                {
                                    *oldsha1 = '\0';
                                    *newsha1 = '\0';

                                    oldsha1++;
                                    newsha1++;
                                }
                            }
                        }
                    }
                }

                /* Getting integer values */
                if(c_newperm && c_oldperm)
                {
                    newperm = atoi(c_newperm);
                    oldperm = atoi(c_oldperm);
                }
               
                /* Generating size message */
                if(!oldsize || !newsize || strcmp(oldsize, newsize) == 0)
                {
                    sdb.size[0] = '\0';
                }
                else
                {
                    snprintf(sdb.size, OS_FLSIZE,
                                       "Size changed from '%s' to '%s'\n",
                                       oldsize, newsize);
                }
                
                /* Permission message */
                if(oldperm == newperm)
                {
                    sdb.perm[0] = '\0';
                }
                else if(oldperm > 0 && newperm > 0)
                {
                    snprintf(sdb.perm, OS_FLSIZE, "Permissions changed from "
                            "'%c%c%c%c%c%c%c%c%c' "
                            "to '%c%c%c%c%c%c%c%c%c'\n",
                            (oldperm & S_IRUSR)? 'r' : '-',
                            (oldperm & S_IWUSR)? 'w' : '-',
                            (oldperm & S_IXUSR)? 'x' : '-',
                            (oldperm & S_IRGRP)? 'r' : '-',
                            (oldperm & S_IWGRP)? 'w' : '-',
                            (oldperm & S_IXGRP)? 'x' : '-',
                            (oldperm & S_IROTH)? 'r' : '-',
                            (oldperm & S_IWOTH)? 'w' : '-',
                            (oldperm & S_IXOTH)? 'x' : '-',

                            (newperm & S_IRUSR)? 'r' : '-',
                            (newperm & S_IWUSR)? 'w' : '-',
                            (newperm & S_IXUSR)? 'x' : '-',
                            (newperm & S_IRGRP)? 'r' : '-',
                            (newperm & S_IWGRP)? 'w' : '-',
                            (newperm & S_IXGRP)? 'x' : '-',
                            (newperm & S_IROTH)? 'r' : '-',
                            (newperm & S_IWOTH)? 'w' : '-',
                            (newperm & S_IXOTH)? 'x' : '-');
                }
                
                /* Ownership message */
                if(!newuid || !olduid || strcmp(newuid, olduid) == 0)
                {
                    sdb.owner[0] = '\0';
                }
                else
                {
                    snprintf(sdb.owner, OS_FLSIZE, "Ownership was '%s', "
                                              "now it is '%s'\n",
                                              olduid, newuid);
                }    
                
                /* group ownership message */
                if(!newgid || !oldgid || strcmp(newgid, oldgid) == 0)
                {
                    sdb.gowner[0] = '\0';
                }
                else
                {
                    snprintf(sdb.gowner, OS_FLSIZE,"Group ownership was '%s', "
                                               "now it is '%s'\n",
                                               oldgid, newgid);
                }
                
                /* md5 message */
                if(!newmd5 || !oldmd5 || strcmp(newmd5, oldmd5) == 0)
                {
                    sdb.md5[0] = '\0';
                }
                else
                {
                    snprintf(sdb.md5, OS_FLSIZE, "Old md5sum was: '%s'\n"
                                            "New md5sum is : '%s'\n",
                                            oldmd5, newmd5);
                }

                /* sha1 */
                if(!newsha1 || !oldsha1 || strcmp(newsha1, oldsha1) == 0)
                {
                    sdb.sha1[0] = '\0';
                }
                else
                {
                    snprintf(sdb.sha1, OS_FLSIZE, "Old sha1sum was: '%s'\n"
                                             "New sha1sum is : '%s'\n",
                                             oldsha1, newsha1);
                }
                                                                                                                                                        
                
                /* Provide information about the file */    
                snprintf(sdb.comment2,512,"Integrity checksum changed for: "
                        "'%s'\n"
                        "%s"
                        "%s"
                        "%s"
                        "%s"
                        "%s"
                        "%s",
                        f_name, 
                        sdb.size,
                        sdb.perm,
                        sdb.owner,
                        sdb.gowner,
                        sdb.md5,
                        sdb.sha1);
            }
            
            
            lf->generated_rule = sdb.syscheck_rule;
            sdb.syscheck_rule->comment = sdb.comment;


            /* Creating a new log message */
            free(lf->log);
            os_strdup(sdb.comment2, lf->log);
           
            OS_Log(lf);

            /* Removing pointer to rule */
            lf->generated_rule = NULL;
                
            return; 
        }
                       
        fgetpos(fp, &sdb.init_pos); /* getting next location */
        
    } /* continuiing... */


    /* If we reach here, this file is not present on our database */
    fseek(fp, 0, SEEK_END);
    
    fprintf(fp,"+++%s %s\n",c_sum,f_name);


    /* Alert if configured to notify on new files */
    if((Config.syscheck_alert_new == 1) && (DB_IsCompleted(agent_id)))
    {
        lf->generated_rule = sdb.syscheck_rule;
        sdb.syscheck_rule->comment = sdb.comment;


        /* New file message */
        snprintf(sdb.comment2,OS_MAXSTR,
                              "New file '%s' added to directory. Checksum: %s."
                              ,f_name,c_sum);
        

        /* Creating a new log message */
        free(lf->log);
        os_strdup(sdb.comment2, lf->log);

        OS_Log(lf);

        /* Removing pointer to rule */
        lf->generated_rule = NULL;

    }

    return;
}


/* Special decoder for syscheck
 * Not using the default decoding lib for simplicity
 * and to be less resource intensive
 */
void DecodeSyscheck(Eventinfo *lf)
{
    char *c_sum;
    char *f_name;
   
   
    /* Setting lf type */
    lf->type = SYSCHECK; 
   

    /* checking if we need to check it in here */
    if(!(sdb.syscheck_rule->alert_opts & DO_LOGALERT))
        return;
        
        
    /* Every syscheck message must be in the following format:
     * checksum filename     
     */
    f_name = strchr(lf->log, ' ');
    if(f_name == NULL)
    {
        /* If we don't have a valid syscheck message, it may be
         * a database completed message.
         */
        if(strcmp(lf->log, HC_SK_DB_COMPLETED) == 0)
        {
            DB_SetCompleted(lf);
            return;    
        }
         
        merror(SK_INV_MSG, ARGV0);
        return;
    }
    
    
    /* Zeroing to get the check sum */
    *f_name = '\0';
    f_name++;


    /* Checking if file is supposed to be ignored */
    if(Config.syscheck_ignore)
    {
        char **ff_ig = Config.syscheck_ignore;
        
        while(*ff_ig)
        {
            if(strncasecmp(*ff_ig, f_name, strlen(*ff_ig)) == 0)
            {
                return;
            }
            
            ff_ig++;
        }
    }
    
    
    /* Checksum is at the beginning of the log */
    c_sum = lf->log;
    
    
    /* Searching for file changes */
    DB_Search(f_name,c_sum,lf);
   
    return;
}

/* EOF */
