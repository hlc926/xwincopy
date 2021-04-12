
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "define.h"

#define MAX_LIST_WND 10

void getparam(int argc, char* argv[]);

int operate();

void list_wnds();
void copy_sync();

Window select_wnd();

/* 递归 */
void search_pid(Window wnd);
