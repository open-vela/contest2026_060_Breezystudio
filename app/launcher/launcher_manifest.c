/****************************************************************************
 * contest2026_060_Breezystudio/app/launcher/launcher_manifest.c
 *
 * Minimal JSON manifest parser - no external dependencies.
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#include "launcher_manifest.h"

/****************************************************************************
 * Private Helpers
 ****************************************************************************/

/* Extract a JSON string value for a given key from raw JSON text.
 * Simple and robust - handles escaped quotes and whitespace.
 * Returns pointer into json_text (static), or NULL if not found.
 */
static const char *json_get_string(const char *json, const char *key)
{
    char search[128];
    const char *p;
    static char value[256];
    int i;

    /* Build search pattern: "key" */
    snprintf(search, sizeof(search), "\"%s\"", key);
    p = strstr(json, search);
    if (!p) return NULL;

    /* Move past the key */
    p += strlen(search);

    /* Skip whitespace and colon */
    while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
    if (*p != ':') return NULL;
    p++;

    /* Skip whitespace before value */
    while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;

    /* Expect opening quote for string */
    if (*p != '"') return NULL;
    p++;

    /* Extract string value (handle escaped chars) */
    i = 0;
    while (*p && *p != '"' && i < (int)sizeof(value) - 1) {
        if (*p == '\\' && *(p + 1)) {
            p++;
            switch (*p) {
                case '"':  value[i++] = '"';  break;
                case '\\': value[i++] = '\\'; break;
                case '/':  value[i++] = '/';  break;
                case 'n':  value[i++] = '\n'; break;
                case 't':  value[i++] = '\t'; break;
                case 'r':  value[i++] = '\r'; break;
                default:   value[i++] = '\\'; value[i++] = *p; break;
            }
        } else {
            value[i++] = *p;
        }
        p++;
    }
    value[i] = '\0';
    return value;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int launcher_manifest_parse(const char *filepath, const char *base_dir,
                            launcher_manifest_t *manifest)
{
    FILE *fp;
    long fsize;
    char *json_buf;
    const char *val;
    size_t nread;

    if (!filepath || !manifest) return -1;

    fp = fopen(filepath, "r");
    if (!fp) {
        printf("Launcher: cannot open manifest %s: %d\n", filepath, errno);
        return -1;
    }

    /* Get file size */
    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
    if (fsize <= 0 || fsize > 4096) {
        fclose(fp);
        return -1;
    }
    fseek(fp, 0, SEEK_SET);

    json_buf = (char *)malloc(fsize + 1);
    if (!json_buf) {
        fclose(fp);
        return -1;
    }

    nread = fread(json_buf, 1, fsize, fp);
    fclose(fp);
    json_buf[nread] = '\0';

    /* Parse fields */
    memset(manifest, 0, sizeof(*manifest));

    val = json_get_string(json_buf, "name");
    if (val) strncpy(manifest->name, val, MAX_APP_NAME_LEN - 1);

    val = json_get_string(json_buf, "exec");
    if (val) strncpy(manifest->exec_path, val, MAX_EXEC_PATH_LEN - 1);

    val = json_get_string(json_buf, "type");
    if (val) {
        if (strcmp(val, "elf") == 0) {
            manifest->type = APP_TYPE_ELF;
        } else if (strcmp(val, "quickapp") == 0) {
            manifest->type = APP_TYPE_QUICKAPP;
        } else {
            manifest->type = APP_TYPE_BUILTIN;
        }
    }

    val = json_get_string(json_buf, "icon");
    if (val) strncpy(manifest->icon_path, val, MAX_EXEC_PATH_LEN - 1);

    free(json_buf);

    /* Validate: must have name and exec */
    if (manifest->name[0] == '\0' || manifest->exec_path[0] == '\0') {
        printf("Launcher: manifest %s missing name or exec\n", filepath);
        return -1;
    }

    printf("Launcher: parsed app '%s' type=%d exec=%s\n",
           manifest->name, manifest->type, manifest->exec_path);
    return 0;
}

int launcher_manifest_scan(const char *dir_path,
                           launcher_manifest_t *manifests,
                           int max_manifests)
{
    DIR *dir;
    struct dirent *entry;
    char manifest_path[MAX_MANIFEST_PATH];
    char subdir_path[MAX_MANIFEST_PATH];
    struct stat st;
    int count = 0;

    if (!dir_path || !manifests) return 0;

    dir = opendir(dir_path);
    if (!dir) {
        printf("Launcher: cannot open apps dir %s: %d\n", dir_path, errno);
        return 0;
    }

    while ((entry = readdir(dir)) != NULL && count < max_manifests) {
        /* Skip dot files */
        if (entry->d_name[0] == '.') continue;

        snprintf(subdir_path, sizeof(subdir_path), "%s/%s", dir_path, entry->d_name);

        /* Check if it's a directory */
        if (stat(subdir_path, &st) != 0 || !S_ISDIR(st.st_mode)) continue;

        /* Look for manifest.json inside */
        snprintf(manifest_path, sizeof(manifest_path), "%s/%s",
                 subdir_path, SDCARD_MANIFEST_FILE);

        if (launcher_manifest_parse(manifest_path, subdir_path,
                                    &manifests[count]) == 0) {
            count++;
        }
    }

    closedir(dir);
    printf("Launcher: found %d apps in %s\n", count, dir_path);
    return count;
}
