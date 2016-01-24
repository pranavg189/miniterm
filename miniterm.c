#define _XOPEN_SOURCE 600
#include <gtk/gtk.h>
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcelanguage.h>
#include <gtksourceview/gtksourcelanguagemanager.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <termios.h> /* unix terminal API */
#include <sys/ioctl.h>
#include <sys/types.h>
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


int main()
{
	return 0;
}