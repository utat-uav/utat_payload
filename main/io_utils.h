#include <iostream>
#include <fstream>
#include <string>

#include "gps_struct.h"

#define CONFIG_FILE "/home/odroid/uav/utat_payload/options.cfg"
#define LOG_FILE "/home/odroid/Pictures/uav_gps.log"

extern int cameratype, usegps, saveimg, view, start_delay, strm;
extern double sizefac;

bool parseConfig();
int checkLog();
void openLogtoWrite(int n_saved);
void writeImageInfo(struct position latest_gps, std::string image);
void closeLog();
