
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include "define.h"

#define MAX_LIST_WND 10

void getparam(int argc, char* argv[]);

int operate();

void list_wnds();
void copy_sync();

Window select_wnd();
void set_event_timer();

void zoom_wnd(Window wnd_src, XWindowAttributes attr_src, Window wnd_obj, XWindowAttributes attr_obj);
void zoom_data(const char * data, int datawidth, char * buf, int width, int height, int lenpixel, float zoomsize);

/* 递归 */
void search_pid(Window wnd);

