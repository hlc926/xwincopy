
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "xwincopy.h"

static void usage(char *program)
{
	fprintf(stderr, "xwincopy: Copy xlib's window and synchronize in real time.\n");
	fprintf(stderr, "Usage: %s [options] pid\n", program);
    fprintf(stderr, "   -l: only list windows for pid.\n");
    fprintf(stderr, "   -n <num>: copy the number(n) window for pid, automatically selected by default.\n");
}

int g_copynum;
Bool g_listonly;
pid_t g_pid;

Display *g_display;

Atom g_atompid;

Window g_list_wnd[MAX_LIST_WND];
int g_list_cnt;

int main(int argc, char* argv[])
{

	getparam(argc, argv);

	if (argc - optind != 1) {
		usage(argv[0]);
		return -1;
	}

	g_pid = atoi(argv[optind]);
	if (g_pid < 1) {
		logger("pid[%d] < 1\n", g_pid);
		return -1;
	}

	g_display = XOpenDisplay(NULL);	// /usr/include/X11/Xlib.h
	if (g_display == NULL) {
        logger( "failed to open X11 display: %s", XDisplayName(NULL));
        return -1;
    }

	operate();

	XCloseDisplay(g_display);

	return 0;
}


void getparam(int argc, char* argv[])
{
	g_copynum = -1;
	g_listonly = False;

	char c;
	while ((c = getopt(argc, argv, "ln:")) != -1) {
		switch (c)
		{
			case 'l':
				g_listonly = True;
				break;

			case 'n':
				g_copynum = atoi(optarg);
				break;

			default:
				usage(argv[0]);
				exit(-1);
		}
	}
}

int operate()
{
	g_atompid = XInternAtom(g_display, "_NET_WM_PID", False);
	if(g_atompid == None) {
		logger("XInternAtom No such atom\n");
		return -1;
	}

	Window rootwnd = DefaultRootWindow(g_display);

	g_list_cnt = 0;
	search_pid(rootwnd);
	if (g_list_cnt < 1) {
		logger("pid[%d] do not have windows\n", g_pid);
		return -1;
	}

	if (g_listonly == True) {
		list_wnds();
	} else {
		copy_sync();
	}

	return 0;
}

void copy_sync()
{
	Window wnd_sel = select_wnd();

	XWindowAttributes attr_sel;
	XGetWindowAttributes(g_display, wnd_sel, &attr_sel);

	logger("select: width[%d] height[%d] map[%d]\n", attr_sel.width, attr_sel.height, attr_sel.map_state);
}

Window select_wnd()
{
	if (g_copynum >= 0 && g_copynum < g_list_cnt) {
		return g_list_wnd[g_copynum];
	}

	for (int i = 0; i < g_list_cnt; i++) {
		Window wnd = g_list_wnd[i];
		XWindowAttributes attr;
		XGetWindowAttributes(g_display, wnd, &attr);
		if (attr.map_state == IsViewable) {
			return wnd;
		}
	}

	return g_list_wnd[0];
}

void list_wnds()
{
	for (int i = 0; i < g_list_cnt; i++) {
		Window wnd = g_list_wnd[i];
		XWindowAttributes attr;
		XGetWindowAttributes(g_display, wnd, &attr);
		logger("found: i[%d] width[%d] height[%d] map[%d]\n", i, attr.width, attr.height, attr.map_state);
	}
}

/* 递归 */
void search_pid(Window wnd)
{
	// Get the PID for the current Window.
	Atom           type;
	int            format;
	unsigned long  nitems;
	unsigned long  bytesafter;
	unsigned char *proppid = NULL;

	if(XGetWindowProperty(g_display, wnd, g_atompid, 0, 1, False, XA_CARDINAL, &type, &format, &nitems, &bytesafter, &proppid) == Success) {
		if(proppid != NULL) {
			if(g_pid == *((pid_t *)proppid)) {
				if (g_list_cnt < MAX_LIST_WND) {
					g_list_wnd[g_list_cnt++] = wnd;
				} else {
					logger("skip for cnt[%d] >= max[%d]\n", g_list_cnt, MAX_LIST_WND);
				}
			}
			XFree(proppid);
		}
	}

    Window root_return, parent_return;
    Window * child_list = NULL;
    unsigned int child_num = 0;

    XQueryTree(g_display, wnd, &root_return, &parent_return, &child_list, &child_num);

	for(unsigned int i = 0; i < child_num; i++) {
		search_pid(child_list[i]);
	}

    XFree(child_list);
}

