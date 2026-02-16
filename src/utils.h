#ifndef UTILS_H
#define UTILS_H

typedef struct {
    char ssid[50];
    char bssid[20];
    int signal;
    int quality;
    int channel;
    char security[20];
    char protocol[20];
    int connected_devices;
    double distance_m;
} WiFiNetwork;

int calculate_quality(int signal);
char* get_protocol(int channel);
int get_connected_devices();

#endif
