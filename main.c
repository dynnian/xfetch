#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE_LENGTH 256

// Helper function to safely open a file
FILE* open_file(const char* path, const char* mode) {
    FILE* file = fopen(path, mode);
    if (file == NULL) {
        perror("Error opening file");
    }
    return file;
}

// Function to extract a string enclosed in quotes
char* extract_quoted_string(const char* line) {
    char* start = strchr(line, '"');
    char* end = strrchr(line, '"');
    if (start != NULL && end != NULL && end > start) {
        size_t name_len = end - start - 1;
        char* result = (char*)malloc(name_len + 1);
        if (result == NULL) {
            perror("Memory allocation failed");
            return NULL;
        }
        strncpy(result, start + 1, name_len);
        result[name_len] = '\0'; // Null-terminate the string
        return result;
    }
    return NULL;
}

// Function to get OS name from /etc/os-release
char* get_os_name() {
    FILE* file = open_file("/etc/os-release", "r");
    if (file == NULL) {
        return NULL;
    }

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file) != NULL) {
        if (strncmp(line, "PRETTY_NAME=", 12) == 0) { // Fixed key length comparison
            fclose(file);
            return extract_quoted_string(line);
        }
    }

    fclose(file);
    return NULL; // Return NULL if PRETTY_NAME is not found
}

// Function to get the desktop environment
char* get_desktop() {
    char* xdgDesktop = getenv("XDG_CURRENT_DESKTOP");
    if (xdgDesktop == NULL) {
        return NULL;
    }
    return xdgDesktop;
}

// Function to get the session type (Wayland/X11) and capitalize it
char* get_session() {
    char* xdgSession = getenv("XDG_SESSION_TYPE");
    if (xdgSession == NULL) {
        return NULL;
    }

    // Capitalize the first letter without modifying the environment variable
    char* session_type = strdup(xdgSession);
    if (session_type == NULL) {
        perror("Memory allocation failed");
        return NULL;
    }
    session_type[0] = toupper(session_type[0]);
    return session_type;
}

// Function to get the kernel information by executing 'uname'
char* get_kernel() {
    FILE* os_fp = popen("uname -s", "r");
    FILE* kernel_fp = popen("uname -r", "r");
    if (os_fp == NULL || kernel_fp == NULL) {
        perror("Error executing 'uname'");
        if (os_fp) pclose(os_fp);
        if (kernel_fp) pclose(kernel_fp);
        return NULL;
    }

    char os_str[MAX_LINE_LENGTH];
    char kernel_str[MAX_LINE_LENGTH];

    if (fgets(os_str, sizeof(os_str), os_fp) == NULL || fgets(kernel_str, sizeof(kernel_str), kernel_fp) == NULL) {
        perror("Failed to read output of system command");
        pclose(os_fp);
        pclose(kernel_fp);
        return NULL;
    }

    // Remove trailing newlines
    os_str[strcspn(os_str, "\n")] = '\0';
    kernel_str[strcspn(kernel_str, "\n")] = '\0';

    pclose(os_fp);
    pclose(kernel_fp);

    // Concatenate OS and kernel version
    char* kernel_info = (char*)malloc(strlen(os_str) + strlen(kernel_str) + 2); // +2 for space and null terminator
    if (kernel_info == NULL) {
        perror("Memory allocation failed");
        return NULL;
    }

    snprintf(kernel_info, strlen(os_str) + strlen(kernel_str) + 2, "%s %s", os_str, kernel_str);
    return kernel_info;
}

// Function to extract the first version number from a string
char* extract_version_number(char* version_output) {
    char* version_start = version_output;
    while (*version_start && !isdigit(*version_start)) {
        version_start++;
    }

    char* version_only = (char*)malloc(128); // Allocate memory for the version number
    if (version_only == NULL) {
        perror("Memory allocation failed");
        return NULL;
    }

    int i = 0;
    while (*version_start && (isdigit(*version_start) || *version_start == '.')) {
        version_only[i++] = *version_start++;
    }
    version_only[i] = '\0'; // Null-terminate the version string
    return version_only;
}

// Function to get the shell and its version
char* get_shell_with_version() {
    pid_t ppid = getppid(); // Get the parent process ID
    char proc_path[256];
    char shell_name[256];
    static char result[512]; // To store shell name and version

    snprintf(proc_path, sizeof(proc_path), "/proc/%d/comm", ppid);
    FILE* fp = open_file(proc_path, "r");
    if (fp == NULL) {
        return NULL;
    }

    if (fgets(shell_name, sizeof(shell_name), fp) != NULL) {
        shell_name[strcspn(shell_name, "\n")] = '\0'; // Remove newline character
    } else {
        fclose(fp);
        return NULL;
    }
    fclose(fp);

    // Construct the version command
    snprintf(result, sizeof(result), "%s --version 2>&1", shell_name);
    fp = popen(result, "r");
    if (fp == NULL) {
        return NULL;
    }

    if (fgets(result, sizeof(result), fp) != NULL) {
        result[strcspn(result, "\n")] = '\0'; // Remove newline character
        char* version_only = extract_version_number(result);
        pclose(fp);
        if (version_only == NULL) {
            return NULL;
        }
        snprintf(result, sizeof(result), "%s %s", shell_name, version_only); // Combine shell name and version
        free(version_only);
        return result;
    }
    pclose(fp);
    return NULL;
}

int main() {
    char* os_name = get_os_name();
    char* desktop_name = get_desktop();
    char* session_type = get_session();
    char* kernel = get_kernel();
    char* shell = get_shell_with_version();

    if (os_name != NULL) {
        printf("Operating System: %s\n", os_name);
        free(os_name);
    } else {
        printf("Operating System not found.\n");
    }

    if (desktop_name != NULL) {
        printf("Desktop Environment / WM: %s\n", desktop_name);
    } else {
        printf("Desktop Environment not recognized.\n");
    }

    if (session_type != NULL) {
        printf("Session Type: %s\n", session_type);
        free(session_type);
    } else {
        printf("Desktop session not recognized.\n");
    }

    if (kernel != NULL) {
        printf("Kernel: %s\n", kernel);
        free(kernel);
    } else {
        printf("Kernel not recognized.\n");
    }

    if (shell != NULL) {
        printf("Shell: %s\n", shell);
    } else {
        printf("Shell not recognized.\n");
    }

    return 0;
}
