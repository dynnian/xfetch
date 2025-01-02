#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <wayland-client.h>
#include <X11/Xlib.h>

#define MAX_LINE_LENGTH 256

// A helper function to handle errors and exit
void handle_error(const char* message) {
    perror(message);
    exit(EXIT_FAILURE);
}

// Function to initialize and retrieve system information
struct utsname get_system_info() {
    struct utsname sys_info;
    if (uname(&sys_info) == -1) {
        handle_error("Error initializing system information");
    }
    return sys_info;
}

// Function to retrieve runtime system information
struct sysinfo get_system_runtime_info() {
    struct sysinfo sys_runtime_info;
    if (sysinfo(&sys_runtime_info) == -1) {
        handle_error("Error retrieving system runtime information");
    }
    return sys_runtime_info;
}

// Function to read environment variables or fallback to a default
const char* get_env_or_default(const char* var, const char* fallback) {
    const char* value = getenv(var);
    return value ? value : fallback;
}

// A pure function to extract a quoted string from a line
char* extract_quoted_string(const char* line) {
    const char* start = strchr(line, '"');
    const char* end = strrchr(line, '"');
    if (start && end && end > start) {
        size_t length = end - start - 1;
        char* result = malloc(length + 1);
        if (!result) handle_error("Memory allocation failed");
        strncpy(result, start + 1, length);
        result[length] = '\0';
        return result;
    }
    return NULL;
}

// A pure function to get the OS name from /etc/os-release
char* get_os_name() {
    FILE* file = fopen("/etc/os-release", "r");
    if (!file) return NULL;

    char line[MAX_LINE_LENGTH];
    char* result = NULL;

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
            result = extract_quoted_string(line);
            break;
        }
    }

    fclose(file);
    return result;
}

// A pure function to concatenate OS and kernel version
char* get_kernel_info(const struct utsname* sys_info) {
    size_t length = strlen(sys_info->sysname) + strlen(sys_info->release) + 2;
    char* kernel_info = malloc(length);
    if (!kernel_info) handle_error("Memory allocation failed");
    snprintf(kernel_info, length, "%s %s", sys_info->sysname, sys_info->release);
    return kernel_info;
}

// A pure function to capitalize the first character of a string
char* capitalize_first(const char* str) {
    if (!str) return NULL;
    char* capitalized = strdup(str);
    if (!capitalized) handle_error("Memory allocation failed");
    capitalized[0] = toupper(capitalized[0]);
    return capitalized;
}

// A pure function to detect the session type
char* get_session_type() {
    const char* session_type = getenv("XDG_SESSION_TYPE");
    if (session_type) {
        return capitalize_first(session_type);
    }

    // Fallbacks
    if (XOpenDisplay(NULL)) return strdup("X11");
    if (wl_display_connect(NULL)) return strdup("Wayland");

    return strdup("Unknown");
}

// A pure function to detect the desktop environment
char* get_desktop_environment() {
    const char* xdg_desktop = getenv("XDG_CURRENT_DESKTOP");
    if (xdg_desktop) return strdup(xdg_desktop);

    const char* session = getenv("DESKTOP_SESSION");
    if (session) return strdup(session);

    return strdup("Unknown");
}

// A pure function to detect the window manager or compositor
char* get_window_manager() {
    const char* session_type = getenv("XDG_SESSION_TYPE");

    if (session_type && strcmp(session_type, "wayland") == 0) {
        const char* desktop = getenv("XDG_CURRENT_DESKTOP");
        const char* session = getenv("DESKTOP_SESSION");

        if (desktop && strstr(desktop, "GNOME")) return strdup("Mutter (Wayland)");
        if (desktop && strstr(desktop, "KDE") && session &&
            (strstr(session, "plasma") || strstr(session, "kde"))) {
            return strdup("KWin (Wayland)");
        }

        return strdup("Wayland Compositor");
    }

    if (session_type && strcmp(session_type, "x11") == 0) {
        Display* display = XOpenDisplay(NULL);
        if (!display) return strdup("Unknown WM");

        char* wm_name = NULL;
        Atom wm_atom = XInternAtom(display, "_NET_WM_NAME", True);
        Atom utf8_string = XInternAtom(display, "UTF8_STRING", True);
        Window root = DefaultRootWindow(display);

        if (wm_atom != None && utf8_string != None) {
            Atom actual_type;
            int actual_format;
            unsigned long nitems, bytes_after;
            unsigned char* prop = NULL;

            if (XGetWindowProperty(display, root, wm_atom, 0, 1024, False, utf8_string,
                                   &actual_type, &actual_format, &nitems, &bytes_after, &prop) == Success &&
                prop) {
                wm_name = strdup((char*)prop);
                XFree(prop);
            }
        }

        XCloseDisplay(display);
        return wm_name ? wm_name : strdup("Unknown WM");
    }

    return strdup("Unknown");
}

// A pure function to get system uptime
char* get_uptime(const struct sysinfo* sys_runtime_info) {
    long uptime_seconds = sys_runtime_info->uptime;
    int days = uptime_seconds / (60 * 60 * 24);
    int hours = (uptime_seconds / (60 * 60)) % 24;
    int minutes = (uptime_seconds / 60) % 60;

    // Format uptime into a string
    char* uptime_str = malloc(64); // Enough space for a formatted string
    if (!uptime_str) handle_error("Memory allocation failed");

    if (days > 0) {
        snprintf(uptime_str, 64, "%d days, %d hours, %d minutes", days, hours, minutes);
    } else {
        snprintf(uptime_str, 64, "%d hours, %d minutes", hours, minutes);
    }

    return uptime_str;
}

// Main function
int main() {
    struct utsname sys_info = get_system_info();
    struct sysinfo sys_runtime_info = get_system_runtime_info();

    char* os_name = get_os_name();
    char* kernel_info = get_kernel_info(&sys_info);
    char* session_type = get_session_type();
    char* desktop_env = get_desktop_environment();
    char* window_manager = get_window_manager();
    char* uptime = get_uptime(&sys_runtime_info);
    char* hostname = strdup(sys_info.nodename);

    printf("Hostname: %s\n", hostname);
    printf("Operating System: %s\n", os_name ? os_name : "Unknown");
    printf("Kernel: %s\n", kernel_info);
    printf("Session Type: %s\n", session_type);
    printf("Desktop Environment: %s\n", desktop_env);
    printf("Window Manager/Compositor: %s\n", window_manager);
    printf("Uptime: %s\n", uptime);

    // Free allocated memory
    free(os_name);
    free(kernel_info);
    free(session_type);
    free(desktop_env);
    free(window_manager);
    free(uptime);
    free(hostname);

    return 0;
}
