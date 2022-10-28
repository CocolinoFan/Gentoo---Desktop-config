/*
 * Copy me if you can.
 * by 20h
 */

#define _BSD_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <X11/Xlib.h>

char *tzUK = "Europe/London";
char *tzutc = "UTC";

static Display *dpy;

char *
smprintf(char *fmt, ...)
{
	va_list fmtargs;
	char *ret;
	int len;

	va_start(fmtargs, fmt);
	len = vsnprintf(NULL, 0, fmt, fmtargs);
	va_end(fmtargs);

	ret = malloc(++len);
	if (ret == NULL) {
		perror("malloc");
		exit(1);
	}

	va_start(fmtargs, fmt);
	vsnprintf(ret, len, fmt, fmtargs);
	va_end(fmtargs);

	return ret;
}

void
settz(char *tzname)
{
	setenv("TZ", tzname, 1);
}

char *
mktimes(char *fmt, char *tzname)
{
	char buf[129];
	time_t tim;
	struct tm *timtm;

	settz(tzname);
	tim = time(NULL);
	timtm = localtime(&tim);
	if (timtm == NULL)
		return smprintf("");

	if (!strftime(buf, sizeof(buf)-1, fmt, timtm)) {
		fprintf(stderr, "strftime == 0\n");
		return smprintf("");
	}

	return smprintf("%s", buf);
}

void
setstatus(char *str)
{
	XStoreName(dpy, DefaultRootWindow(dpy), str);
	XSync(dpy, False);
}

char *
loadavg(void)
{
	double avgs[2];

	if (getloadavg(avgs, 2) < 0)
		return smprintf("");

	return smprintf("%.2f %.2f", avgs[0], avgs[1]);
}

char *
readfile(char *base, char *file)
{
	char *path, line[513];
	FILE *fd;

	memset(line, 0, sizeof(line));

	path = smprintf("%s/%s", base, file);
	fd = fopen(path, "r");
	free(path);
	if (fd == NULL)
		return NULL;

	if (fgets(line, sizeof(line)-1, fd) == NULL)
		return NULL;
	fclose(fd);

	return smprintf("%s", line);
}

char *
gettemperature(char *base, char *sensor)
{
	char *co;

	co = readfile(base, sensor);
	if (co == NULL)
		return smprintf("");
	return smprintf("%02.0f°C", atof(co) / 1000);
}

char *
execscript(char *cmd)
{
	FILE *fp;
	char retval[1025], *rv;

	memset(retval, 0, sizeof(retval));

	fp = popen(cmd, "r");
	if (fp == NULL)
		return smprintf("");

	rv = fgets(retval, sizeof(retval), fp);
	pclose(fp);
	if (rv == NULL)
		return smprintf("");
	retval[strlen(retval)-1] = '\0';

	return smprintf("%s", retval);
}

char *
PKTwallet(void)
{
//	int yourH = atoi(execscript("pktctl --wallet getinfo | awk '/CurrentHeight/ {print $2-1}'"));
//	int backH = atoi(execscript("pktctl --wallet getinfo | awk '/BackendHeight/ {print $2-1}'"));
	return execscript("pktctl --wallet getbalance | awk '{printf \"%.3f\", $0}'");
}


char*
Weather(char *city)
{
	char fn[128] = {0};
	strcat(fn, "curl wttr.in/");
	strcat(fn, city);
	strcat(fn, "?format=\"%c%t.%w\\n\"");
	return execscript(fn);
}

int
main(void)
{
	char *status;
	char *avgs;
	char *tmbln;
	char *t0;
	char *t1;
	char *pkt;
	char *weather;

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "dwmstatus: cannot open display.\n");
		return 1;
	}

	for (;;sleep(30)) {
		avgs = loadavg();
		tmbln = mktimes("%a %d %b %Y %H:%M", tzUK);
		t0 = gettemperature("/sys/devices/virtual/thermal/thermal_zone0", "temp");
		t1 = gettemperature("/sys/devices/virtual/thermal/thermal_zone1", "temp");
		pkt = PKTwallet();
		weather = Weather("Coventry");
		status = smprintf("|🪙%s |🌡️%s 🌡️:%s |:%s |%s |%s |",
				     pkt, t0, t1, avgs, tmbln, weather);
		setstatus(status);

		free(t0);
		free(t1);
		free(avgs);
		free(tmbln);
		free(status);
		free(pkt);
		free(weather);
	}

	XCloseDisplay(dpy);

	return 0;
}

