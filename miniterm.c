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

int main(int argc, char *argv[])
{
	tcgetattr(STDIN_FILENO, &ttyOrig);
	ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);
	childPid = ptyFork(&masterFd, slaveName, MAX_SNAME, &ttyOrig, &ws);
	if(childPid == 0)
	{
		shell = "/bin/bash";
		execlp(shell, shell, (char*) NULL);
	}
	atexit(ttyReset);

	/* gtk code start */
	
	GtkSourceBuffer *buffer;
	GtkWidget *scrollwin;

	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	buffer = gtk_source_buffer_new(NULL);
	txtInput = gtk_source_view_new_with_buffer(buffer);
	gtk_text_buffer_append_output(GTK_TEXT_BUFFER(buffer),buf,-1);
	srollwin = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrollwin), txtInput);

	GIOChannel *masterFd_channel = g_io_channel_unix_new(masterFd);
	g_io_add_watch(masterFd_channel, G_IO_IN, (GIOFunc)read_masterFd, NULL);

	gtk_container_add(GTK_CONTAINER(window),scrollwin);
	gtk_widget_set_size_request(window, 600, 400);
	gtk_window_set_title(GTK_WINDOW(window), "Terminal");

	g_signal_connect(G_OBJECT(txtInput),"key-press-event",G_CALLBACK(txtinput_key_press_event),NULL);
	g_signal_connect(G_OBJECT(window),"destroy",G_CALLBACK(destroy), NULL);
	g_signal_connect(G_OBJECT(window),"delete_event", G_CALLBACK(delete_event), NULL);

	gtk_widget_show_all(window);

	gtk_main();  /* gtk event loop */

	/* gtk code end */
	return 0;
}

