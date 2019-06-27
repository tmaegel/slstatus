/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <string.h>

#include "../util.h"

/* battery style
 * 1 = Show only percent
 * 2 = Show only battery symbol
 * 3 = Show both (symbol + percent)
 */
const unsigned int battery_style = 3;

#if defined(__linux__)
	#include <limits.h>
	#include <stdint.h>
	#include <unistd.h>

    /*
     * only available with the status2d patch
     */
    static const char
    *d_bar(unsigned char p)
    {
        const char *s[] = {
            "^c#ac444d^^r0,10,25,12^^c#4b555a^^r1,11,23,10^^f25^^c#ac444d^^r0,13,2,6^^f4^^c#8b9499^",//  0-10%
            "^c#d0661c^^r0,10,25,12^^c#4b555a^^r1,11,21,10^^f25^^c#d0661c^^r0,13,2,6^^f4^^c#8b9499^",// 10-20%
            "^c#8b9499^^r0,10,25,12^^c#4b555a^^r1,11,18,10^^f25^^c#8b9499^^r0,13,2,6^^f4^",// 30-40%
            "^c#8b9499^^r0,10,25,12^^c#4b555a^^r1,11,15,10^^f25^^c#8b9499^^r0,13,2,6^^f4^",// 40-50%
            "^c#8b9499^^r0,10,25,12^^c#4b555a^^r1,11,12,10^^f25^^c#8b9499^^r0,13,2,6^^f4^",// 50-60%
            "^c#8b9499^^r0,10,25,12^^c#4b555a^^r1,11,9,10^^f25^^c#8b9499^^r0,13,2,6^^f4^",// 60-70%
            "^c#8b9499^^r0,10,25,12^^c#4b555a^^r1,11,6,10^^f25^^c#8b9499^^r0,13,2,6^^f4^",// 70-80%
            "^c#8b9499^^r0,10,25,12^^c#4b555a^^r1,11,3,10^^f25^^c#8b9499^^r0,13,2,6^^f4^",// 80-90%
            "^c#8b9499^^r0,10,25,12^^c#4b555a^^r1,11,0,10^^f25^^c#8b9499^^r0,13,2,6^^f4^" // 90-100%
        };

        return s[((8 * p) / 100)];
    }

	static const char *
	pick(const char *bat, const char *f1, const char *f2, char *path,
	     size_t length)
	{
		if (esnprintf(path, length, f1, bat) > 0 &&
		    access(path, R_OK) == 0) {
			return f1;
		}

		if (esnprintf(path, length, f2, bat) > 0 &&
		    access(path, R_OK) == 0) {
			return f2;
		}

		return NULL;
	}

	const char *
	battery_perc(const char *bat)
	{
		int perc;
		char path[PATH_MAX];

		if (esnprintf(path, sizeof(path),
		              "/sys/class/power_supply/%s/capacity", bat) < 0) {
			return NULL;
		}
		if (pscanf(path, "%d", &perc) != 1) {
			return NULL;
		}

        if (battery_style == 1)  {
		    return bprintf("%3d%%", perc);
        } else if (battery_style == 2) {
		    return bprintf("%s", d_bar(perc));
        } else if (battery_style == 3) {
		    return bprintf("%s %3d%%", d_bar(perc), perc);
        } else {
            // fallback: if battery_style not specified
		    return bprintf("%3d%%", perc);
        }
	}

	const char *
	battery_state(const char *bat)
	{
		static struct {
			char *state;
			char *symbol;
		} map[] = {
			{ "Charging",    "+" },
			{ "Discharging", "-" },
		};
		size_t i;
		char path[PATH_MAX], state[12];

		if (esnprintf(path, sizeof(path),
		              "/sys/class/power_supply/%s/status", bat) < 0) {
			return NULL;
		}
		if (pscanf(path, "%12s", state) != 1) {
			return NULL;
		}

		for (i = 0; i < LEN(map); i++) {
			if (!strcmp(map[i].state, state)) {
				break;
			}
		}
		return (i == LEN(map)) ? "?" : map[i].symbol;
	}

	const char *
	battery_remaining(const char *bat)
	{
		uintmax_t charge_now, current_now, m, h;
		double timeleft;
		char path[PATH_MAX], state[12];

		if (esnprintf(path, sizeof(path),
		              "/sys/class/power_supply/%s/status", bat) < 0) {
			return NULL;
		}
		if (pscanf(path, "%12s", state) != 1) {
			return NULL;
		}

		if (!pick(bat, "/sys/class/power_supply/%s/charge_now",
		          "/sys/class/power_supply/%s/energy_now", path,
		          sizeof(path)) ||
		    pscanf(path, "%ju", &charge_now) < 0) {
			return NULL;
		}

		if (!strcmp(state, "Discharging")) {
			if (!pick(bat, "/sys/class/power_supply/%s/current_now",
			          "/sys/class/power_supply/%s/power_now", path,
			          sizeof(path)) ||
			    pscanf(path, "%ju", &current_now) < 0) {
				return NULL;
			}

			if (current_now == 0) {
				return NULL;
			}

			timeleft = (double)charge_now / (double)current_now;
			h = timeleft;
			m = (timeleft - (double)h) * 60;

			return bprintf(" [ %juh %jum ]", h, m);
		}

		return "";
	}
#elif defined(__OpenBSD__)
	#include <fcntl.h>
	#include <machine/apmvar.h>
	#include <sys/ioctl.h>
	#include <unistd.h>

	static int
	load_apm_power_info(struct apm_power_info *apm_info)
	{
		int fd;

		fd = open("/dev/apm", O_RDONLY);
		if (fd < 0) {
			warn("open '/dev/apm':");
			return 0;
		}

		memset(apm_info, 0, sizeof(struct apm_power_info));
		if (ioctl(fd, APM_IOC_GETPOWER, apm_info) < 0) {
			warn("ioctl 'APM_IOC_GETPOWER':");
			close(fd);
			return 0;
		}
		return close(fd), 1;
	}

	const char *
	battery_perc(const char *unused)
	{
		struct apm_power_info apm_info;

		if (load_apm_power_info(&apm_info)) {
			return bprintf("%d", apm_info.battery_life);
		}

		return NULL;
	}

	const char *
	battery_state(const char *unused)
	{
		struct {
			unsigned int state;
			char *symbol;
		} map[] = {
			{ APM_AC_ON,      "+" },
			{ APM_AC_OFF,     "-" },
		};
		struct apm_power_info apm_info;
		size_t i;

		if (load_apm_power_info(&apm_info)) {
			for (i = 0; i < LEN(map); i++) {
				if (map[i].state == apm_info.ac_state) {
					break;
				}
			}
			return (i == LEN(map)) ? "?" : map[i].symbol;
		}

		return NULL;
	}

	const char *
	battery_remaining(const char *unused)
	{
		struct apm_power_info apm_info;

		if (load_apm_power_info(&apm_info)) {
			if (apm_info.ac_state != APM_AC_ON) {
				return bprintf("%uh %02um",
			                       apm_info.minutes_left / 60,
				               apm_info.minutes_left % 60);
			} else {
				return "";
			}
		}

		return NULL;
	}
#elif defined(__FreeBSD__)
	#include <sys/sysctl.h>

	const char *
	battery_perc(const char *unused)
	{
		int cap;
		size_t len;

		len = sizeof(cap);
		if (sysctlbyname("hw.acpi.battery.life", &cap, &len, NULL, 0) == -1
				|| !len)
			return NULL;

		return bprintf("%d", cap);
	}

	const char *
	battery_state(const char *unused)
	{
		int state;
		size_t len;

		len = sizeof(state);
		if (sysctlbyname("hw.acpi.battery.state", &state, &len, NULL, 0) == -1
				|| !len)
			return NULL;

		switch(state) {
			case 0:
			case 2:
				return "+";
			case 1:
				return "-";
			default:
				return "?";
		}
	}

	const char *
	battery_remaining(const char *unused)
	{
		int rem;
		size_t len;

		len = sizeof(rem);
		if (sysctlbyname("hw.acpi.battery.time", &rem, &len, NULL, 0) == -1
				|| !len
				|| rem == -1)
			return NULL;

		return bprintf("%uh %02um", rem / 60, rem % 60);
	}
#endif
