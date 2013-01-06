/*
 * Copyright (C) 2012 The Android Open Source Project
 * Copyright (C) 2012 The CyanogenMod Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define LOG_TAG "enrc2b PowerHAL"
#include <utils/Log.h>

#include <hardware/hardware.h>
#include <hardware/power.h>

static int boost_fd = -1;
static int boost_warned;

#define SCALING_GOVERNOR_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
#define ONDEMAND_GOVERNOR "ondemand"
#define INTERACTIVE_GOVERNOR "interactive"

#define ONDEMAND_SAMPLING_RATE "/sys/devices/system/cpu/cpufreq/ondemand/sampling_rate"
#define ONDEMAND_SAMPLING_RATE_SCREEN_ON "50000"
#define ONDEMAND_SAMPLING_RATE_SCREEN_OFF "500000"

static void sysfs_write(char *path, char *s)
{
    char buf[80];
    int len;
    int fd = open(path, O_WRONLY);

    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error opening %s: %s\n", path, buf);
        return;
    }

    len = write(fd, s, strlen(s));
    if (len < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error writing to %s: %s\n", path, buf);
    }

    close(fd);
}

static int sysfs_read(char *path, char *s, int num_bytes)
{
    char buf[80];
    int count;
    int ret = 0;
    int fd = open(path, O_RDONLY);

    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error opening %s: %s\n", path, buf);

        return -1;
    }

    if ((count = read(fd, s, num_bytes - 1)) < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error reading %s: %s\n", path, buf);

        ret = -1;
    } else {
        s[count] = '\0';
    }

    close(fd);

    return ret;
}

static int get_scaling_governor(char governor[], int size) {
    if (sysfs_read(SCALING_GOVERNOR_PATH, governor,
                size) == -1) {
        // Can't obtain the scaling governor. Return.
        return -1;
    } else {
        // Strip newline at the end.
        int len = strlen(governor);

        len--;

        while (len >= 0 && (governor[len] == '\n' || governor[len] == '\r'))
            governor[len--] = '\0';
    }

    return 0;
}
static void enrc2b_power_init(struct power_module *module)
{
	char governor[80];

	if(get_scaling_governor(governor, sizeof(governor))!=0)
		return;
	
    if (strncmp(governor, INTERACTIVE_GOVERNOR, strlen(INTERACTIVE_GOVERNOR)) == 0){
		ALOGI("enrc2b_power_init %s", governor);
		
		// permissions for interactive tweaks are set in ramdisk
		// because is the default govenor in the kernel
	   	sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/go_maxspeed_load", "85");
    	sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/boost_factor","0");
    	sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/max_boost","0");
    	sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/sustain_load","95");
    	sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/min_sample_time","30000");
    	sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/timer_rate","20000");
    	sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/input_boost","0");
    	sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/boost","0");
    	sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/above_hispeed_delay","20000");
    	sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/go_hispeed_load","85");
    	sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/hispeed_freq","1700000");
    	sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/io_is_busy","0");
	}
	else if (strncmp(governor, ONDEMAND_GOVERNOR, strlen(ONDEMAND_GOVERNOR)) == 0){
		// govener tweaks are set in the kernel because all other permissions
		// are not correct to be set here
		ALOGI("enrc2b_power_init %s", governor);
    }
}

static void enrc2b_power_set_interactive(struct power_module *module, int on)
{
	char governor[80];
		
	if(get_scaling_governor(governor, sizeof(governor))!=0)
		return;
		
    if (strncmp(governor, INTERACTIVE_GOVERNOR, strlen(INTERACTIVE_GOVERNOR)) == 0){
		ALOGI("enrc2b_power_set_interactive %s %d", governor, on);
		
        sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/input_boost", on ? "1" : "0");
    	sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/boost_factor", on ? "0" : "2");
    }
    else if (strncmp(governor, ONDEMAND_GOVERNOR, strlen(ONDEMAND_GOVERNOR)) == 0){
		ALOGI("enrc2b_power_set_interactive %s %d", governor, on);
		sysfs_write(ONDEMAND_SAMPLING_RATE, on ? ONDEMAND_SAMPLING_RATE_SCREEN_ON : ONDEMAND_SAMPLING_RATE_SCREEN_OFF);

    }
}

static void enrc2b_power_hint(struct power_module *module, power_hint_t hint,
                            void *data)
{
    char buf[80];
    int len;

    //ALOGI("enrc2b_power_hint :%d", hint);
    switch (hint) {
    case POWER_HINT_VSYNC:
    	//ALOGI("enrc2b_power_hint POWER_HINT_VSYNC");
        break;
    case POWER_HINT_INTERACTION:
        //ALOGI("enrc2b_power_hint POWER_HINT_INTERACTION");
        break;
    case POWER_HINT_VIDEO_ENCODE:
        //ALOGI("enrc2b_power_hint POWER_HINT_VIDEO_ENCODE");
        break;
    case POWER_HINT_CPU_BOOST:
        //ALOGI("enrc2b_power_hint POWER_HINT_CPU_BOOST");
        break;
    default:
        //ALOGI("enrc2b_power_hint unknown:%d", hint);
        break;
    }
}

static struct hw_module_methods_t power_module_methods = {
    .open = NULL,
};

struct power_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = POWER_MODULE_API_VERSION_0_2,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = POWER_HARDWARE_MODULE_ID,
        .name = "enrc2b Power HAL",
        .author = "max.weninger@gmail.com",
        .methods = &power_module_methods,
    },

    .init = enrc2b_power_init,
    .setInteractive = enrc2b_power_set_interactive,
    .powerHint = enrc2b_power_hint,
};
