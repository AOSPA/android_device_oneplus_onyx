/*
 *  Copyright (C) 2015 The OmniROM Project
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#define LOG_TAG "mac-update"
#include <cutils/log.h>

static char mac_string[256];
static char mac[6];

#define MAC_FILE "/data/oemnvitems/4678"
#define EMPTY_MAC "000000000000"

/* control messages to wcnss driver */
#define WCNSS_USR_CTRL_MSG_START    0x00000000
#define WCNSS_USR_SERIAL_NUM        (WCNSS_USR_CTRL_MSG_START + 1)
#define WCNSS_USR_HAS_CAL_DATA      (WCNSS_USR_CTRL_MSG_START + 2)
#define WCNSS_USR_WLAN_MAC_ADDR     (WCNSS_USR_CTRL_MSG_START + 3)

#define WCNSS_CTRL      "/dev/wcnss_ctrl"
#define WCNSS_MAX_CMD_LEN  (128)
#define BYTE_0  0
#define BYTE_1  8

int read_mac(const char *filename)
{
    char raw[6];
    int fd = -1;
    int numtries = 0;
    int ret;

    memset(raw, 0, 6);
    memset(mac, 0, 6);

    //Try to open the file once every 4 seconds for 120 seconds
    while(fd < 0 && numtries++ < 30) {
        fd = open(filename, O_RDONLY);
        if(fd < 0) {
            sleep(4);
        }
    }

    //if it's still not open, bomb out
    if (fd < 0)
        return ENOENT;

    ret = read(fd, raw, 6);
    close(fd);

    // swap bytes
    mac[0] = raw[5];
    mac[1] = raw[4];
    mac[2] = raw[3];
    mac[3] = raw[2];
    mac[4] = raw[1];
    mac[5] = raw[0];

    sprintf(mac_string, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    return 0;
}

int main(int argc, char **argv)
{
    int pos = 0;
    int fd = 0;
    char msg[WCNSS_MAX_CMD_LEN];

    if (read_mac(MAC_FILE)) {
        ALOGE("Failed to read MAC");
        exit(EINVAL);
    }

    if (!strcmp(mac_string, EMPTY_MAC)) {
        ALOGE("MAC empty");
        exit(EINVAL);
    }
    
    ALOGI("Found MAC address %s\n", mac_string);
    
    if (argc == 2 && !strcmp(argv[1], "-v")){
        exit(0);
    }

    fd = open(WCNSS_CTRL, O_WRONLY);
    if (fd < 0) {
        ALOGE("Failed to open %s : %s\n", WCNSS_CTRL, strerror(errno));
        exit(EINVAL);
    }

    msg[pos++] = WCNSS_USR_WLAN_MAC_ADDR >> BYTE_1;
    msg[pos++] = WCNSS_USR_WLAN_MAC_ADDR >> BYTE_0;
    msg[pos++] = mac[0];
    msg[pos++] = mac[1];
    msg[pos++] = mac[2];
    msg[pos++] = mac[3];
    msg[pos++] = mac[4];
    msg[pos++] = mac[5];

    if (write(fd, msg, pos) < 0) {
        ALOGE("Failed to write to %s : %s\n", WCNSS_CTRL, strerror(errno));
    }

    close(fd);
    return 0;
}
