
你也可以访问我的博客 http://www.cnblogs.com/ningci

2017-05-07 实现设置中的可以设置定时时间

2017-06-07 实现文件夹进入，向上，翻页，多种文件类型ICON区分

2017-06-07 9:54 实现在文件浏览时可以点击运行 .nes 游戏

2017-06-07 17:43 实现 BMP JPG 图片查看 （需要编译 jpegsrc.v9b.tar.gz 使用 jpeg_mem_src）

2017-06-16 17:52 实现 GIF 播放 （需要编译 libgif 库）

2017-06-30 10:51 实现 usb camrea 实时显示图片 和 拍照 （基于 v4l2 MJPEG）

使用说明
本程序需要 icon 中的图标文件 和 msyh.ttf 和 make 以后生成的可执行文件 digitpic

可以将以上的文件复制到ARM文件系统中来运行

如果 要使用鼠标，请配置内核 支持 HID 鼠标

另外，因为使用触摸屏，所以也需要加载 TS 驱动支持，及配置好 TSLIB 的环境变量


