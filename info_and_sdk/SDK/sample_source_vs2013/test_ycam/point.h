#pragma once

struct Point {
	int x,y;
	Point():x(),y(){}
	Point(int x, int y):x(x),y(y){}
};

struct pointRGB {
	unsigned char r,g,b;
	pointRGB() :r(), g(), b(){}
	pointRGB(unsigned char r, unsigned char g, unsigned char b):r(r),g(g),b(b){}
};
