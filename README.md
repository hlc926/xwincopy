# xwincopy
Copy xlib's window and synchronize in real time

![image](doc/xwincopy.png)

### 功能:
拷贝xlib的窗口并同步。  
可以zoom放大缩小窗口。  
双击拷贝窗口重置为源窗口大小。  
拖放移动窗口。  

### 使用:
直接运行`xwincopy`可得到使用帮助:

	xwincopy: Copy xlib's window and synchronize in real time.
	Usage: ./xwincopy [options] pid
	   -l: only list windows for pid.
	   -n <num>: copy the window(num, from -l) for pid, automatically selected by default.
	
	xwincopy pid  	# 对进程pid的窗口复制，默认选择第一个(map_state == 2)的。
					# 可以过-n <num> 改变， -l 可以列出该进程所有窗口及num

源窗口和拷贝窗口大小相同时，有最好的性能和画质。  
缩放时，性能和画质下降，因需循环取缩放后的像素点，暂无更好解决方案。  
如缩放时觉得性能和画质不能接受，可以先调整源窗口的大小，然后双击拷贝窗口重置，这样就达到最好的性能和画质。

### Next:
后来发现有个 xwininfo 可以直接点选窗口，或者将来改为这样的模式，而不是通过pid。  

