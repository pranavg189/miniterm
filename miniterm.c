#define _XOPEN_SOURCE 600
#include <gtk/gtk.h>
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcelanguage.h>
#include <gtksourceview/gtksourcelanguagemanager.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <termios.h> /* Unix terminal API */
#include <sys/ioctl.h> /* required for winsize struct and ioctl() */
#include <sys/types.h> /* required for pid_t datatype */
#include <sys/select.h>
#include <unistd.h>
#include <stdio.h>

#define BUF_SIZE 256
#define MAX_SNAME 1000
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#define MAX_EVENTS 1000

void destroy(GtkWidget *window);
gboolean delete_event(GtkWidget*, GdkEvent*);
void empty_stream();
GtkWidget *window, *txtInput;
static gboolean txtinput_key_press_event(GtkWidget *widget, GdkEventKey *event);
void gtk_text_buffer_append_output(GtkTextBuffer *buffer, char *text, int len);
gboolean read_masterFd(GIOChannel *channel, GIOCondition condition,gpointer data);
int min_cursor_offset = 0;

GThread *thread;

char slaveName[MAX_SNAME];
char *shell;
int masterFd, scriptFd;
struct winsize ws;		/* winsize is a data structure defined in sys/ioctl.h and used to store info about window size */
fd_set inFds;			/* fd_set is a fixed size buffer */
char buf[BUF_SIZE];
ssize_t numRead; 		/* ssize_t data type is used to represent the sizes of block that can be read or written in a single operation. */
pid_t childPid; 		/* pid_t data type represents process IDs */

struct termios ttyOrig;

pid_t ptyFork(int *masterFd, char *slaveName, size_t snLen, struct termios *slaveTermios, const struct winsize *slaveWS)
{
	int mfd, slaveFd, savedErrno;
	pid_t childPid;
	char slname[100];
	char *p;
	mfd = posix_openpt(O_RDWR | O_NOCTTY); /* creates a new pseudo terminal */
	grantpt(mfd);  /* grant required permissions */
	unlockpt(mfd);
	p = ptsname(mfd); /* get pseudo terminal name */
	strncpy(slname, p, 100);
	if (slaveName != NULL)
	{
		/* Return slave name to caller */
		if(strlen(slname) < snLen)
		{
			strncpy(slaveName,slname,snLen);
		}
	}
	childPid = fork();
	if(childPid != 0)
	{
		*masterFd = mfd;
		return childPid;
	}
	setsid();
	close(mfd);
	slaveFd = open(slname, O_RDWR);
	#ifdef TIOCSCTTY
	ioctl(slaveFd, TIOCSCTTY, 0);
	#endif
	if(slaveTermios != NULL)
	{
		slaveTermios->c_lflag &= ~ECHO;
		slaveTermios->c_lflag |= ECHOE;
		slaveTermios->c_lflag |= ECHOK;
		//slaveTermios->c_lflag |= ECHOKE;
		//slaveTermios->c_lflag |= ECHOCTL;
		slaveTermios->c_lflag |= ICANON;
		slaveTermios->c_lflag |= ISIG;
		slaveTermios->c_lflag |= IEXTEN;
		slaveTermios->c_lflag |= HUPCL;
		slaveTermios->c_iflag |= BRKINT;
		slaveTermios->c_iflag |= ICRNL;
		slaveTermios->c_iflag |= IXON;
		slaveTermios->c_iflag |= IXANY;
		slaveTermios->c_iflag |= IUTF8;
		slaveTermios->c_iflag |= IMAXBEL;
		slaveTermios->c_oflag |= OPOST;
		slaveTermios->c_oflag |= ONLCR;
		cfsetspeed(slaveTermios, B38400);
		slaveTermios->c_cc[VMIN]=1;
		if (tcsetattr(slaveFd, TCSANOW, slaveTermios) == -1);
	}

	if(slaveWS != NULL)
		if(ioctl(slaveFd, TIOCSWINSZ, slaveWS) == -1);

	dup2(slaveFd, 0);
	dup2(slaveFd, 1);
	dup2(slaveFd, 2);

	if(slaveFd > 2)
		close(slaveFd);
	return 0;
}

static void ttyReset(void)
{
	if(tcsetattr(STDIN_FILENO), TCSANOW, &ttyOrig) == -1);
}

int main()
{
	return 0;
}