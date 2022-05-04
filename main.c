#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/eventfd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wiringPi.h>

#define MY_PIN_BACKLIGHT	0
#define MY_STATUS_LIGHTON	1
#define MY_STATUS_LIGHTOFF	0
#define MY_BACKLIGHT_CTRL_FILE_PATH	"/sys/class/backlight/soc:backlight/brightness"
#define MY_BACKLIGHT_CTRL_CMD_ON	"1\n"
#define MY_BACKLIGHT_CTRL_CMD_OFF	"0\n"

static int g_status = MY_STATUS_LIGHTON;
static int g_evtfd = -1;

static int _light_cmd(int cmd)
{
	int fd;
	ssize_t wret;
	const char *cmd_str;

	fd = open(MY_BACKLIGHT_CTRL_FILE_PATH, O_WRONLY);
	if (fd < 0)
	{
		printf("failed to open file at %s, errno=%d\n", MY_BACKLIGHT_CTRL_FILE_PATH, errno);
		return -1;
	}

	if (MY_STATUS_LIGHTON == cmd)
		cmd_str = MY_BACKLIGHT_CTRL_CMD_ON;
	else
		cmd_str = MY_BACKLIGHT_CTRL_CMD_OFF;

	wret = write(fd, cmd_str, strlen(cmd_str));
	if (wret != strlen(cmd_str))
	{
		printf("expected write: %u, actually: %u\n", strlen(cmd_str), wret);
		close(fd);
		return -1;
	}

	close(fd);

	return 0;
}

static void _light_off()
{
	int ret;

	printf("do light off\n");
	ret = _light_cmd(MY_STATUS_LIGHTOFF);
	if (0 != ret)
		printf("failed to switch backlight to OFF.\n");
	g_status = MY_STATUS_LIGHTOFF;
	return;
}

static void _light_on()
{
	int ret;

	printf("do light on\n");
	ret = _light_cmd(MY_STATUS_LIGHTON);
	if (0 != ret)
		printf("failed to switch backlight to ON.\n");
	g_status = MY_STATUS_LIGHTON;
	return;
}

static void _isr_evt_proc()
{
	if (MY_STATUS_LIGHTON == g_status)
		_light_off();
        else
                _light_on();
	return;
}

static void _isr_callback()
{
	unsigned long long evt;
	ssize_t wret;

	evt = 1;
	wret = write(g_evtfd, &evt, sizeof(evt));
	if (wret != sizeof(evt))
		printf("failed to write event fd, erno=%d\n", errno);

	return;
}

int main(int argc, char *argv[])
{
	int ret;
	unsigned long long evt;
	ssize_t rret;

	g_evtfd = eventfd(0, EFD_CLOEXEC | EFD_SEMAPHORE);
	if (g_evtfd < 0)
	{
		printf("failed to create event fd, errno=%d\n", errno);
		return -1;
	}

	ret = wiringPiSetup();
	if (0 != ret)
	{
		printf("Faied to initialize wiringPi\n");
		goto end;
	}

	//pinMode(MY_PIN_BACKLIGHT, OUTPUT);
	//digitalWrite(MY_PIN_BACKLIGHT, HIGH);
	pullUpDnControl(MY_PIN_BACKLIGHT, PUD_UP);
	pinMode(MY_PIN_BACKLIGHT, INPUT);

	wiringPiISR(MY_PIN_BACKLIGHT, INT_EDGE_RISING, &_isr_callback);

	while(1)
	{
		rret = read(g_evtfd, &evt, sizeof(evt));
		if (rret == sizeof(evt))
		{
			_isr_evt_proc();
		}
	}

end:
	if (g_evtfd >= 0)
		close(g_evtfd);
	g_evtfd = -1;

	return ret;
}
