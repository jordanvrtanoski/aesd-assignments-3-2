#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>

/**
 * dir_exists
 *
 * Use POSIX function to verify if a folder exists
 */
int dir_exists(char *dir) {
    struct stat st;

    if (stat(dir, &st) != 0) {
        /* stat failed, dir does not exist */
        int err = errno; // Not really needed but a good practice to save the error
        switch (err) {
            case ENOENT:   // No such file or directory
                syslog(LOG_ERR, "Directory does not exist, we need to create it: %s\n", dir);
                return false;
                break;
            case EACCES:   // Permission denied
                syslog(LOG_ERR, "Permission denied: %s\n", dir);
                exit(1);
                break;
            case ENOTDIR:  // A component of the dir is not a directory
                syslog(LOG_ERR, "Invalid dir component: %s\n", dir);
                exit(1);
                break;
            default:
                syslog(LOG_ERR, "Other error (%d) for dir: %s\n", err, dir);
                exit(1);

        }
    }
    return S_ISDIR(st.st_mode);
}

int make_dir(char *dir) {
    if (mkdir(dir, 0755) != 0) {
        int err = errno;
        switch (err) {
            case EEXIST:
            syslog(LOG_ERR, "Directory already exists\n");
            exit(1);
            break;
        case EACCES:
            syslog(LOG_ERR, "Permission denied\n");
            exit(1);
            break;
        case ENOENT:
            syslog(LOG_ERR, "A parent directory does not exist\n");
            exit(1);
            break;
        default:
            syslog(LOG_ERR, "Other error (%d)\n", err);
            exit(1);
        }
    }
    return 0;
}

/**
 * check_arguments
 *
 * Check if the user passed 2 arguments
 */
int check_arguments(int argc, char *argv[]) {
    if (argc != 3) {
        syslog(LOG_ERR, "The call to %s requires 2 arguments\n", argv[0]);
        exit(1);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    // Check the number of arguments
    check_arguments(argc, argv);

    char *file_dir_name = argv[1];
    char *str_to_write = argv[2];

    char *dirn = dirname(strdup(file_dir_name));
    char *filen = basename(strdup(file_dir_name));

    // Check if folder exists
    if (!dir_exists(dirn)) {
        // Create if not existing
        if(make_dir(dirn) != 0) {
            syslog(LOG_ERR, "Directory creation failed\n"); // We should never see this  as the make_dir uses `exit(1)` to break the execution
        }
    }

    // Open the file with creation and truncation
    int fd = open(file_dir_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        int err = errno;
        switch (err) {
            case EACCES:
                syslog(LOG_ERR, "Permission denied: %s\n", filen);
                exit(1);
                break;
            case ENOENT:
                syslog(LOG_ERR, "A directory in the path does not exist: %s\n", dirn);
                exit(1);
                break;
            case EMFILE:
            case ENFILE:
                syslog(LOG_ERR, "System Error, too many open files, consider modifying ulimit\n");
                exit(1);
                break;
            default:
                syslog(LOG_ERR, "Error opening file %s: %s\n", file_dir_name, strerror(err));
                exit(1);
        }
    }

    // Write to file
    syslog(LOG_DEBUG, "Writing %s to %s", filen, str_to_write);
    ssize_t n = write(fd, str_to_write, strlen(str_to_write));
    if (n == -1) {
        int err = errno;
        syslog(LOG_ERR, "Error writing to file %s: %s\n", filen, strerror(err));
        close(fd);
        exit(1);
    }

    // Close the file
    if (close(fd) == -1) {
        syslog(LOG_ERR, "Error closing file %s: %s\n", filen, strerror(errno));
        exit(1);
    }

    return 0;
}

