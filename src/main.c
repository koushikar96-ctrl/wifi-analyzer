#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "utils.h"

char* ltrim(char* s) {
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

void extract_value(const char* line, const char* key, char* value) {
    const char* pos = strstr(line, key);
    if (pos) {
        pos += strlen(key);
        while ((*pos >= '0' && *pos <= '9') || *pos == ' ' || *pos == '\t') pos++;
        while (*pos == ':' || *pos == ' ' || *pos == '\t') pos++;
        strcpy(value, pos);
        size_t len = strlen(value);
        if (len > 0 && value[len - 1] == '\n') value[len - 1] = '\0';
    } else {
        value[0] = '\0';
    }
}

int parse_signal(const char* val) {
    char digits[10] = "";
    int j = 0;
    for (int i = 0; val[i] != '\0'; i++)
        if (val[i] >= '0' && val[i] <= '9') digits[j++] = val[i];
    digits[j] = '\0';
    return atoi(digits);
}

// Distance estimation using RSSI and frequency (in meters)
double estimate_distance(int rssi, int channel) {
    double freq_mhz = (channel >= 1 && channel <= 14) ? 2400.0 : 5000.0;
    double exp_val = (27.55 - (20 * log10(freq_mhz)) + fabs(rssi)) / 20.0;
    return pow(10.0, exp_val);
}

int main() {
    FILE* fp = _popen("netsh wlan show networks mode=bssid", "r");
    if (!fp) { printf("[]"); return 1; }

    WiFiNetwork networks[50];
    int count = 0;

    char line[1035];
    char current_ssid[100] = "";
    char current_security[50] = "";
    int current_channel = 0;

    while (fgets(line, sizeof(line), fp)) {
        char* trimmed = ltrim(line);

        if (strncmp(trimmed, "SSID ", 5) == 0 && strstr(trimmed, ":")) {
            extract_value(trimmed, "SSID", current_ssid);
            current_security[0] = '\0';
            current_channel = 0;
        } else if (strncmp(trimmed, "Authentication", 14) == 0 && strstr(trimmed, ":")) {
            extract_value(trimmed, "Authentication", current_security);
        } else if (strncmp(trimmed, "Channel", 7) == 0 && strstr(trimmed, ":")) {
            char val[10]; extract_value(trimmed, "Channel", val);
            current_channel = atoi(val);
        } else if (strncmp(trimmed, "BSSID ", 6) == 0 && strstr(trimmed, ":")) {
            char bssid[100] = "";
            extract_value(trimmed, "BSSID", bssid);

            int signal = 0;
            long prev_pos = ftell(fp);
            char next_line[1035];
            while (fgets(next_line, sizeof(next_line), fp)) {
                char* ntrim = ltrim(next_line);
                if (strncmp(ntrim, "Signal", 6) == 0 && strstr(ntrim, ":")) {
                    char sval[10]; extract_value(ntrim, "Signal", sval);
                    signal = parse_signal(sval);
                    break;
                }
            }
            fseek(fp, prev_pos, SEEK_SET);

            if (strlen(current_ssid) > 0 && strlen(bssid) > 0 && signal != 0) {
                WiFiNetwork net;
                strcpy(net.ssid, current_ssid);
                strcpy(net.bssid, bssid);
                net.signal = signal - 100; // negative dBm
                net.channel = current_channel;
                strcpy(net.security, current_security);
                net.quality = calculate_quality(net.signal);
                strcpy(net.protocol, get_protocol(current_channel));
                net.distance_m = estimate_distance(net.signal, current_channel);

                networks[count++] = net;
            }
        }
    }

    _pclose(fp);

    printf("[");
    for (int i = 0; i < count; i++) {
        printf("{\"ssid\":\"%s\",\"bssid\":\"%s\",\"signal\":%d,\"quality\":%d,"
               "\"channel\":%d,\"security\":\"%s\",\"protocol\":\"%s\",\"distance_m\":%.2f}",
               networks[i].ssid, networks[i].bssid, networks[i].signal, networks[i].quality,
               networks[i].channel, networks[i].security, networks[i].protocol, networks[i].distance_m);
        if (i < count - 1) printf(",");
    }
    printf("]");
    return 0;
}