/* See LICENSE file for copyright and license details. */

/* interval between updates (in ms) */
const unsigned int interval = 3000;

/* text to show if no value can be retrieved */
static const char unknown_str[] = "n/a";

/* maximum output string length */
#define MAXLEN 2048

/*
 * function            description                     argument (example)
 *
 * battery_perc        battery percentage              battery name (BAT0)
 *                                                     NULL on OpenBSD/FreeBSD
 * battery_state       battery charging state          battery name (BAT0)
 *                                                     NULL on OpenBSD/FreeBSD
 * battery_remaining   battery remaining HH:MM         battery name (BAT0)
 *                                                     NULL on OpenBSD/FreeBSD
 * cpu_perc            cpu usage in percent            NULL
 * cpu_freq            cpu frequency in MHz            NULL
 * datetime            date and time                   format string (%F %T)
 * disk_free           free disk space in GB           mountpoint path (/)
 * disk_perc           disk usage in percent           mountpoint path (/)
 * disk_total          total disk space in GB          mountpoint path (/")
 * disk_used           used disk space in GB           mountpoint path (/)
 * entropy             available entropy               NULL
 * gid                 GID of current user             NULL
 * hostname            hostname                        NULL
 * ipv4                IPv4 address                    interface name (eth0)
 * ipv6                IPv6 address                    interface name (eth0)
 * kernel_release      `uname -r`                      NULL
 * keyboard_indicators caps/num lock indicators        format string (c?n?)
 *                                                     see keyboard_indicators.c
	{ cpu_perc,         "[ %s% ] ",       NULL },
 * keymap              layout (variant) of current     NULL
 *                     keymap
 * load_avg            load average                    NULL
 * netspeed_rx         receive network speed           interface name (wlan0)
 * netspeed_tx         transfer network speed          interface name (wlan0)
 * num_files           number of files in a directory  path
 *                                                     (/home/foo/Inbox/cur)
 * ram_free            free memory in GB               NULL
 * ram_perc            memory usage in percent         NULL
 * ram_total           total memory size in GB         NULL
 * ram_used            used memory in GB               NULL
 * run_command         custom shell command            command (echo foo)
 * swap_free           free swap in GB                 NULL
 * swap_perc           swap usage in percent           NULL
 * swap_total          total swap size in GB           NULL
 * swap_used           used swap in GB                 NULL
 * temp                temperature in degree celsius   sensor file
 *                                                     (/sys/class/thermal/...)
 *                                                     NULL on OpenBSD
 *                                                     thermal zone on FreeBSD
 *                                                     (tz0, tz1, etc.)
 * uid                 UID of current user             NULL
 * uptime              system uptime                   NULL
 * username            username of current user        NULL
 * vol_perc            OSS/ALSA volume in percent      mixer file (/dev/mixer)
 * wifi_perc           WiFi signal in percent          interface name (wlan0)
 * wifi_essid          WiFi ESSID                      interface name (wlan0)
 */
static const struct arg args[] = {
	/* function format          argument */
	/*
     * primary bar (topbar)
     */
    // slock
    { run_command,      "%s", "pgrep slock > /dev/null && echo '\uE26F' || echo ' '"},
    // Updates
    { run_command,      "   \uE00E %s", "cat /tmp/checkupdates"},
    // mails
    { run_command,      "   \uE073 %s", "echo '0'"},
    // volume
    { run_command,      "   \uE094 %s", "amixer get Master | grep 'Mono:' | awk -F'[][]' '{print $2}'"},
	{ run_command,      " [%s]", "amixer get Master | grep 'Mono:' | awk -F'[][]' '{print $6}'"},
	// brightness
    { run_command,      "   \uE1C3 %s%%", "xbacklight -get | awk '{print int($1+0.5)}'"},
	// bluetooth on / off
    { run_command,      "   \uE1B5 [%s]", "/usr/local/bin/radioctl --get bluetooth"},
	// wifi
	// { wifi_perc,        "  %s", "wlp1s0" },
    { wifi_essid,       "   \uE0F0 %s", "wlp1s0" },
	// wlan on / off
    { run_command,      " [%s]", "/usr/local/bin/radioctl --get wlan"},
    // ram
	{ ram_perc,         "   \uE020 %s%%", NULL },
	// cpu
    { cpu_perc,         "   \uE026 %s%%", NULL },
	// battery
    { battery_perc,     "   \uE238 %s", "BAT0" },
	{ battery_remaining,"%s", "BAT0" },
	// date
    { datetime,         "   \uE226 %s", "%d.%m.%Y" },
	// time
    { datetime,         "   \uE016 %s ", "%H:%M" },
	/*
     * DELIMITER ';'
     * xsetroot -name "top text;bottom text"
     */
    //{ run_command,      "%s", "echo ';'"},
	/*
     * secondary bar (bottom bar)
     */
};
