#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h> 
#include <fcntl.h> 
#include <syslog.h>
#include <sys/wait.h>

#include "systemcalls.h"


/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/

    int ret = system(cmd);
    
    // Check if the return code is -1 which means syscall failed 
    if (ret != -1) {
        return true;
    } else {
        return false;
    }
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/
    
    syslog(LOG_DEBUG, "Starting execution of do_exec");
    // As we use the va_end at the end of the code, we can not just return from the function
    // so this will track the return code
    bool rc = false;

    int pid = fork();

    if (pid == 0) {
        // This is the child
        execv(command[0],command);

        // If we are executing this code, something had failed in the exec
        int err = errno;

         switch (err) {
             case EACCES:
                 syslog(LOG_ERR, "Error in access privilegies for the command\n"); 
                 exit(-1);
                 break;
             case ENOENT:
                 syslog(LOG_ERR, "The command specified is not found\n");
                 exit(-2);
                 break;
             case ENOEXEC:
                 syslog(LOG_ERR, "The spefified file is not executable\n");
                 exit(-3);
                 break;
             default:
                 syslog(LOG_ERR, "Error in the excution with return code (%d) %s\n", err, strerror(err));
                 exit(-4);
         }

    } else {
        // This is the parent
        int child_status;
        if (waitpid(pid, &child_status, 0) == -1) {
            int err = errno;
            syslog(LOG_ERR, "The waitpid() call failed with (%d) %s\n", err, strerror(err));
            rc = false;
        } else {
            if (child_status != 0) {
                syslog(LOG_ERR, "Child process exited with no zero return code\n");
                rc = false;
            } else {
                // Child returned with exit(0) so it was sucessfull
                rc = true;
            }
        }

    }

    va_end(args);

    return rc;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/

    // Redirect the file by calling dup2() to replace the stdout. The requiremnt is not telling as what to do with stdin and stderr

    syslog(LOG_DEBUG, "Starting execution of do_exec_redirect");
    // As we use the va_end at the end of the code, we can not just return from the function
    // so this will track the return code
    bool rc = false;

    int pid = fork();

    if (pid == 0) {
        // This is the child
        int new_stdout = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (new_stdout == -1) {
            int err = errno;
            switch (err) {
                case EACCES:
                    syslog(LOG_ERR, "Insufficient priviledges to opening the output file\n");
                    exit(-1);
                    break;
                case ENOENT:
                    syslog(LOG_ERR, "The path for the output file do not exit\n");
                    exit(-2);
                    break;
                default:
                    syslog(LOG_ERR, "Error while opening the output file (%d): %s\n",
                        err, strerror(err));
                    exit(-5);
            }

        } else {
            if (dup2(new_stdout, STDOUT_FILENO) == -1) {
                int err = errno;
                switch (err) {
                    case EBADF:
                        syslog(LOG_ERR, "Bad file descriptor in call to dup2()\n");
                        exit(-4);
                        break;
                    default:
                        syslog(LOG_ERR, "Error while duplicating file descriptor with dup2() (%d): %s\n",
                            err, strerror(err));
                        exit(-5);
                }
            } else {
                // Close the old stdout
                close(new_stdout);
            }
        }

        // File redirected, execute the command
        execv(command[0],command);

        // If we are executing this code, something had failed in the exec
        int err = errno;

         switch (err) {
             case EACCES:
                 syslog(LOG_ERR, "Error in access privilegies for the command\n"); 
                 exit(-1);
                 break;
             case ENOENT:
                 syslog(LOG_ERR, "The command specified is not found\n");
                 exit(-2);
                 break;
             case ENOEXEC:
                 syslog(LOG_ERR, "The spefified file is not executable\n");
                 exit(-3);
                 break;
             default:
                 syslog(LOG_ERR, "Error in the excution with return code (%d) %s\n", err, strerror(err));
                 exit(-5);
         }

    } else {
        // This is the parent
        int child_status;
        if (waitpid(pid, &child_status, 0) == -1) {
            int err = errno;
            syslog(LOG_ERR, "The waitpid() call failed with (%d) %s\n", err, strerror(err));
            rc = false;
        } else {
            if (child_status != 0) {
                syslog(LOG_ERR, "Child process exited with no zero return code\n");
                rc = false;
            } else {
                // Child returned with exit(0) so it was sucessfull
                rc = true;
            }
        }

    }

    va_end(args);

    return rc;
}
