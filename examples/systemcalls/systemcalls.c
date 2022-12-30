#include "systemcalls.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

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
    int retval = system(cmd);
    if (cmd == NULL) {
        return retval != 0 ? true : false;
    } else {
        if (WIFEXITED(retval) && !WEXITSTATUS(retval)) {
            return true;
        }
    }
    return false;
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
    int i, retval;
    pid_t pid;

    /* Open the system log that's for records if something goes wrong */
    openlog(NULL, LOG_PID, LOG_SYSLOG);

    /* Initialize the command array with 0 */
    memset(command, 0, sizeof(command));

    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/
    if ((pid = fork()) == -1) {
        syslog(LOG_ERR | LOG_SYSLOG, "[%s] err: %d, %s. line: %d", __func__, errno, strerror(errno), __LINE__);
        goto ERROR;
    } else if (!pid) {
        if (execv(command[0], command) == -1) {
            /* In this situation, it needs to call to exit with EXIT_FAILURE to avoid
             * WEXITSTATUS() misunderstanding the return status of a child process. */
            syslog(LOG_ERR | LOG_SYSLOG, "[%s] err: %d, %s. line: %d", __func__, errno, strerror(errno), __LINE__);
            va_end(args);
            closelog();
            exit(EXIT_FAILURE);
        }
    } else if (waitpid(pid, &retval, 0) != -1) {
        if (WIFEXITED(retval)) {
            if (!WEXITSTATUS(retval)) {
                goto SUCCESS;
            } else {
                syslog(LOG_ERR | LOG_SYSLOG, "[%s] err: %d, %s. line: %d", __func__, errno, strerror(errno), __LINE__);
                goto ERROR;
            }
        }
    } else {
        syslog(LOG_ERR | LOG_SYSLOG, "[%s] err: %d, %s. line: %d", __func__, errno, strerror(errno), __LINE__);
        goto ERROR;
    }


ERROR:;
    va_end(args);
    closelog();
    return false;
SUCCESS:;
    va_end(args);
    closelog();
    return true;
}

/* file_close: clase a file descriptor */
void file_close(int fd) {
    if (fd != -1) {
        close(fd);
	}
	return;
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
    int i, retval, fd;
    pid_t pid;

    /* Initialize the command array with 0 */
    memset(command, 0, sizeof(command));
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }

    /* Open the system log that's for records if something goes wrong */
    openlog(NULL, LOG_PID, LOG_SYSLOG);

/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/
    fd = open(outputfile, O_CREAT | O_RDWR);
    if (fd == -1) {
        syslog(LOG_ERR | LOG_SYSLOG, "[%s] err: %d, %s. line: %d", __func__, errno, strerror(errno), __LINE__);
        goto ERROR;
    }

    if ((pid = fork()) == -1) {
        syslog(LOG_ERR | LOG_SYSLOG, "[%s] err: %d, %s. line: %d", __func__, errno, strerror(errno), __LINE__);
        goto ERROR;
    } else if (!pid) {
        retval = dup2(fd, STDOUT_FILENO);
        if (retval == -1 || execv(command[0], command) == -1) {
            syslog(LOG_ERR | LOG_SYSLOG, "[%s] err: %d, %s. line: %d", __func__, errno, strerror(errno), __LINE__);
            va_end(args);
            file_close(fd);
            closelog();
            exit(EXIT_FAILURE);
        }
    } else if (waitpid(pid, &retval, 0) != -1) {
        if (WIFEXITED(retval) && !WEXITSTATUS(retval)) {
            goto SUCCESS;
        } else {
            syslog(LOG_ERR | LOG_SYSLOG, "[%s] err: %d, %s. line: %d", __func__, errno, strerror(errno), __LINE__);
            goto ERROR;
        }
    } else {
        syslog(LOG_ERR | LOG_SYSLOG, "[%s] err: %d, %s. line: %d", __func__, errno, strerror(errno), __LINE__);
    }

ERROR:;
    va_end(args);
    file_close(fd);
    closelog();
    return false;
SUCCESS:;
    va_end(args);
    file_close(fd);
    closelog();
    return true;
}
