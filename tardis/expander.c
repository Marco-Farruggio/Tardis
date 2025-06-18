#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <windows.h> // For GetTempPathA, CreateDirectoryA

#define TARDIS_MARKER "TARDIS_MARKER"

#define MAX_CONFIG_ENTRIES 16
#define MAX_PATH_LEN 260
#define MAX_KEY_LEN 64
#define MAX_VAL_LEN 64

typedef struct {
    char key[MAX_KEY_LEN];
    char value[MAX_VAL_LEN];
} ConfigEntry;

typedef struct {
    ConfigEntry entries[MAX_CONFIG_ENTRIES];
    int entries_count;
} Config;

bool split_by_delimiter(const char *line, const char *delimiter, char *left, size_t left_size, char *right, size_t right_size) {
    char *pos = strstr(line, delimiter);
    if (pos == NULL) {
        // Delimiter not found
        return false;
    }

    size_t left_len = pos - line;
    if (left_len >= left_size) {
        // Left buffer too small
        return false;
    }
    strncpy(left, line, left_len);
    left[left_len] = '\0';

    // Copy right part
    const char *right_part = pos + strlen(delimiter);
    if (strlen(right_part) >= right_size) {
        // Right buffer too small
        return false;
    }
    strcpy(right, right_part);

    return true;
}

int starts_with(const char *buffer, const char *prefix) {
    size_t len = strlen(prefix);
    return strncmp(buffer, prefix, len) == 0;
}

void cleanup_temp_zipfile(const char *zip_path) {
    if (remove(zip_path) == 0) {
        // Successfull removal
        fprintf(stdout, "[Tardis] [Expander] [C] Successfully deleted temporary zip file %s\n", zip_path);
    } else {
        // Error returned
        fprintf(stdout, "[Tardis] [Expander] [C] [Warning] Unable to delete temp zip file %s\n", zip_path);
    }
}

int parse_config_file(const char *filepath, Config* config) {
    fprintf(stdout, "[Tardis] [Expander] [C] Attempting to parse expected tardis.config file in extracted payload\n");

    FILE* config_file = fopen(filepath, "r");
    if (!config_file) {
        fprintf(stdout, "[Tardis] [Expander] [C] [Error] Failed to open the tardis.config file.\n");
        return 1;
    }

    char line[256];
    while (fgets(line, sizeof(line), config_file)) {
        // Trim leading whitespace
        char* ptr = line;
        while (isspace(*ptr)) ptr++;

        // Skip empty or comment lines
        if (*ptr == '\0' || *ptr == '\n' || starts_with(ptr, "#") || starts_with(ptr, "[")) {
            continue;
        }

        // Trim trailing newline and spaces
        char* end = ptr + strlen(ptr) - 1;
        while (end > ptr && isspace(*end)) {
            *end = '\0';
            end--;
        }

        char left[MAX_KEY_LEN];
        char right[MAX_VAL_LEN];

        if (split_by_delimiter(ptr, " = ", left, sizeof(left), right, sizeof(right))) {
            if (config->entries_count < MAX_CONFIG_ENTRIES) {
                strncpy(config->entries[config->entries_count].key, left, MAX_KEY_LEN);
                config->entries[config->entries_count].key[MAX_KEY_LEN - 1] = '\0';

                strncpy(config->entries[config->entries_count].value, right, MAX_VAL_LEN);
                config->entries[config->entries_count].value[MAX_VAL_LEN - 1] = '\0';

                config->entries_count++;
            } else {
                fprintf(stdout, "[Tardis] [Expander] [C] [Warning] Config entries exceeded max limit\n");
            }
        } else {
            fprintf(stdout, "[Tardis] [Expander] [C] [Warning] Invalid config line: %s\n", ptr);
        }
    }

    fclose(config_file);
    fprintf(stdout, "[Tardis] [Expander] [C] Successfully parsed tardis.config\n");
    return 0;
}

// Function to find the marker in the file and return offset after the marker
long find_marker_offset(FILE *exe_file) {
    int marker_length = (int)strlen(TARDIS_MARKER);
    long marker_offset = -1;
    char buffer[1024];
    size_t read;
    long pos = 0;

    // Make sure we start reading at the beginning
    fseek(exe_file, 0, SEEK_SET);

    while ((read = fread(buffer, 1, sizeof(buffer), exe_file)) > 0) {
        for (size_t i = 0; i < read - marker_length + 1; i++) {
            if (memcmp(buffer + i, TARDIS_MARKER, marker_length) == 0) {
                marker_offset = pos + i + marker_length;
                return marker_offset; // Found marker, return immediately
            }
        }
        pos += read;
        fseek(exe_file, pos, SEEK_SET);
    }

    // Marker not found
    return -1;
}

int main(int argc, char *argv[])
{   
    // Attempt to open the file itself (.exe)
    FILE *exe_file = fopen(argv[0], "rb");
    if (exe_file == NULL) {
        fprintf(stdout, "[Tardis] [Expander] [C] [Error] Unable to open myself.\n");
        return 1;
    }

    long marker_offset = find_marker_offset(exe_file);
    if (marker_offset == -1) {
        fprintf(stdout, "[Tardis] [Expander] [C] [Error] Tardis marker not found in executable.\n");
        fclose(exe_file);
        return 1;
    }

    // Get Windows temp path
    char temp_path[MAX_PATH];
    if (GetTempPathA(MAX_PATH, temp_path) == 0) {
        fprintf(stdout, "[Tardis] [Expander] [C] [Error] Failed to fetch windows temp path.\n");
        return 1;
    }

    // Build temp zip file path
    char temp_zip_path[MAX_PATH];
    snprintf(temp_zip_path, sizeof(temp_zip_path), "%spayload.zip", temp_path);

    // Write embedded ZIP payload to temp zip file
    FILE *payload = fopen(temp_zip_path, "wb");
    if (payload == NULL) {
        fprintf(stdout, "[Tardis] [Expander] [C] [Error] Unable to create payload.zip at %s\n", temp_zip_path);
        return 1;
    }
    fseek(exe_file, marker_offset, SEEK_SET); // Locate beginning of zip payload
    int ch;
    while ((ch = fgetc(exe_file)) != EOF) {
        fputc(ch, payload);
    }
    fclose(exe_file);
    fclose(payload);

    // Create extraction folder path inside temp folder %TEMP%\_TARDIS_EXTRACTION
    char extract_folder[MAX_PATH];
    snprintf(extract_folder, sizeof(extract_folder), "%s_TARDIS_EXTRACTION", temp_path);

    // Create the extraction folder
    if (!CreateDirectoryA(extract_folder, NULL)) {
        DWORD err = GetLastError();
        if (err != ERROR_ALREADY_EXISTS) {
            fprintf(stdout, "[Tardis] [Expander] [C] [Error] Failed to create extraction directory %s (Error %lu)\n", extract_folder, err);
            return 1;
        }
    }

    // Build system command to extract zip into extraction folder
    char command[2 * MAX_PATH + 50];
    snprintf(command, sizeof(command), "tar -xf \"%s\" -C \"%s\"", temp_zip_path, extract_folder);

    int cmd_ret = system(command);
    if (cmd_ret != 0) {
        return cmd_ret;
    }

    // Delete the temp ZIP file after extraction
    cleanup_temp_zipfile(temp_zip_path);

    // Construct full path to tardis.config
    char config_filepath[MAX_PATH_LEN];
    snprintf(config_filepath, sizeof(config_filepath), "%s\\%s", extract_folder, "tardis.config");

    Config tardis_config = { .entries_count = 0 };

    if (parse_config_file(config_filepath, &tardis_config) != 0) {
        return 1;
    }

    // Print all key value pairs
    for (int i = 0; i < tardis_config.entries_count; ++i) {
        printf("[Tardis] [Config] %s = %s\n", tardis_config.entries[i].key, tardis_config.entries[i].value);
    }

    // Execute code from entry point

    // Success
    return 0;
}