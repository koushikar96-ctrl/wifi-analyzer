#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "utils.h"

int calculate_quality(int signal) {
    if (signal >= -50) return 100;
    else if (signal >= -60) return 85;
    else if (signal >= -70) return 70;
    else if (signal >= -80) return 55;
    else if (signal >= -90) return 40;
    else return 25;
}

// Return Wi-Fi protocol based on frequency/channel
char* get_protocol(int channel) {
    if (channel <= 11)
        return "802.11b/g/n";
    else
        return "802.11ac";
}

// Simulate number of connected devices
int get_connected_devices() {
    srand(time(NULL));
    return rand() % 10 + 1; // between 1 and 10
}
