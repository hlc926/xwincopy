
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>

#include "xwincopy.h"

static void usage(char *program)
{
	fprintf(stderr, "xwincopy: Copy xlib's window and synchronize in real time.\n");
	fprintf(stderr, "Usage: %s [options] pid\n", program);
    fprintf(stderr, "   -l: only list windows for pid.\n");
    fprintf(stderr, "   -n <num>: copy the window(num, from -l) for pid, automatically selected by default.\n");
}

int g_copynum;
Bool g_listonly;
pid_t g_pid;

Display * g_display;
Window g_wnd_obj;
static Visual * g_visual;
static GC g_gc;

int g_width_src, g_height_src;
int g_width_obj, g_height_obj;
float g_zoomsize;

Atom g_atompid;

Bool g_exit = False;

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
	g_atompid = XInternAtom(g_display, "_NET_WM_PID", True);
	if(g_atompid == None) {
		logger("XInternAtom No such atom\n");
		return -1;
	}

	g_list_cnt = 0;
	search_pid(DefaultRootWindow(g_display));
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
	Window wnd_src = select_wnd();

	XWindowAttributes attr_src;
	XGetWindowAttributes(g_display, wnd_src, &attr_src);

	g_width_src = g_width_obj = attr_src.width;
	g_height_src = g_height_obj = attr_src.height;
	g_zoomsize = 1;

	logger("select: width[%d] height[%d] map[%d]\n", attr_src.width, attr_src.height, attr_src.map_state);

	g_visual = DefaultVisual(g_display, DefaultScreen(g_display));

	XSetWindowAttributes attr_obj;
	g_wnd_obj = XCreateWindow(g_display, DefaultRootWindow(g_display),
			0, 0, attr_src.width, attr_src.height, 
			attr_src.border_width, attr_src.depth, InputOutput,
			g_visual, CWBackPixel, &attr_obj);

	g_gc = XCreateGC(g_display, g_wnd_obj, 0, NULL);

	XStoreName(g_display, g_wnd_obj, "xwincopy");

	int pid = getpid();
	Atom atom = XInternAtom(g_display, "_NET_WM_PID", True);
	XChangeProperty(g_display, g_wnd_obj, atom,
			XA_CARDINAL, sizeof(pid_t) * 8, 
			PropModeReplace, (unsigned char *)&pid, 1);

	Atom kill_atom = XInternAtom(g_display, "WM_DELETE_WINDOW", True);
	XSetWMProtocols(g_display, g_wnd_obj, &kill_atom, 1);

	XSelectInput(g_display, g_wnd_obj, 
			ExposureMask | ButtonPressMask | PointerMotionMask);

	XMapWindow(g_display, g_wnd_obj);

	set_event_timer('I');
	
	Time lbtime = 0;
	int x_offset = 0, y_offset = 0;
	XEvent event;

	while(1) {

		XNextEvent(g_display, &event);

		XWindowAttributes attr_src;
		XGetWindowAttributes(g_display, wnd_src, &attr_src);

		XWindowAttributes attr_obj;
		XGetWindowAttributes(g_display, g_wnd_obj, &attr_obj);

		switch(event.type) {
			case Expose :
				{
					if (attr_src.width != attr_obj.width 
							|| attr_src.height != attr_obj.height) {
						set_event_timer('Z');
						zoom_wnd(wnd_src, attr_src, g_wnd_obj, attr_obj);
					} else {
						set_event_timer('C');
						XCopyArea(g_display, wnd_src, g_wnd_obj, g_gc, 
								0, 0,  attr_src.width, attr_src.height, 0, 0);
					}

					break;
				}
			case ButtonPress :
				{
					if (event.xbutton.button == Button1) {
						if (lbtime > 0 
								&& event.xbutton.time - lbtime < 500) {
							/* double click */
							lbtime = 0;

							g_width_src = g_width_obj = attr_src.width;
							g_height_src = g_height_obj = attr_src.height;
							g_zoomsize = 1;

							XResizeWindow(g_display, g_wnd_obj, 
									attr_src.width, attr_src.height); 
							XCopyArea(g_display, wnd_src, g_wnd_obj, g_gc, 
									0, 0,  attr_src.width, attr_src.height, 
									0, 0);
						} else { /* click */
							lbtime = event.xbutton.time;

							x_offset = event.xbutton.x;
							y_offset = event.xbutton.y;
						}
					}

					break;
				}
			case MotionNotify :
				{
					if (event.xmotion.state & Button1Mask) {
						XMoveWindow(g_display, g_wnd_obj,
								event.xmotion.x_root - x_offset,
								event.xmotion.y_root - y_offset - 50);
					}

					break;
				}
			case ClientMessage :
				{
					if (event.xclient.data.l[0] == (long) kill_atom) {
						g_exit = True;
					}

					break;
				}
		} // switch event

		if (g_exit == True) {
			break;
		}
	}

	XFreeGC(g_display, g_gc);

	XDestroyWindow(g_display, g_wnd_obj);

	logger("done.\n");
}


void signal_act(int sig)
{
	if (sig == SIGALRM) {
		XEvent xevent;
		xevent.type = Expose;
		XSendEvent(g_display, g_wnd_obj, False, ExposureMask, &xevent);
		XFlush(g_display);
	}
}

void set_event_timer(char ctype)
{
	if (ctype == 'I') {	// Init
		signal(SIGALRM, signal_act);
	}

	struct itimerval val;
	val.it_value.tv_sec = 0;
	if (ctype == 'Z') {	// Zoom
		val.it_value.tv_usec = 500000;
	} else {			// Copy or Init
		val.it_value.tv_usec = 100000;
	}
	val.it_interval = val.it_value;

	setitimer(ITIMER_REAL, &val, NULL);	// 定时发送 SIGALRM
}


void zoom_wnd(Window wnd_src, XWindowAttributes attr_src, Window wnd_obj, XWindowAttributes attr_obj)
{
	int width = g_width_obj;
	int height = g_height_obj;
	float zoomsize = g_zoomsize;

	if (g_width_src != attr_src.width || g_height_src != attr_src.height
			|| width != attr_obj.width || height != attr_obj.height ) {

		float zw = attr_obj.width / (float)attr_src.width;
		float zh = attr_obj.height / (float)attr_src.height;
		zoomsize = zw < zh ? zw : zh;

		g_width_src = attr_src.width;
		g_height_src = attr_src.height;
		g_zoomsize = zoomsize = ((int)(zoomsize * 1000)) / (float)1000;
		g_width_obj = width = (int)attr_src.width * zoomsize;
		g_height_obj = height = (int)attr_src.height * zoomsize;
	}

	XResizeWindow(g_display, wnd_obj, width, height); 


	XImage * imagesrc;
	imagesrc = XGetImage(g_display, wnd_src, 0, 0, attr_src.width, attr_src.height, AllPlanes, ZPixmap);

	int lenpixel = imagesrc->bits_per_pixel / 8;
	char * buf = malloc(width * height * lenpixel);

	zoom_data(imagesrc->data, imagesrc->width, buf, width, height, lenpixel, zoomsize);
	 
	char dummy;
	XImage * imageobj = XCreateImage( g_display, g_visual, imagesrc->depth, ZPixmap, 0, &dummy, width, height, imagesrc->bitmap_pad, 0 );
	imageobj->byte_order = imagesrc->byte_order;
	imageobj->bitmap_bit_order = imagesrc->bitmap_bit_order;
	imageobj->data = buf;

	XPutImage(g_display, wnd_obj, g_gc, imageobj, 0, 0, 0, 0, width, height);

	XDestroyImage(imageobj);
	XDestroyImage(imagesrc);
}

void zoom_data(const char * data, int datawidth, char * buf, int width, int height, int lenpixel, float zoomsize)
{
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int sx = (int)(x / zoomsize);
			int sy = (int)(y / zoomsize);

			memcpy(buf + (y * width + x) * lenpixel,
					data + (sy * datawidth + sx ) * lenpixel,
					lenpixel);
		}
	} 
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
		logger("window: num[%d] width[%d] height[%d] map_state[%d]\n", i, attr.width, attr.height, attr.map_state);
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

