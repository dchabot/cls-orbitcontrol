/*
 * Bpm.h
 *
 *  Created on: Jan 6, 2009
 *      Author: chabotd
 */

#ifndef BPM_H_
#define BPM_H_

#include <syslog.h>
#include <string>
using std::string;


class Bpm {
public:
	Bpm(const string& str) : id(str),enabled(true) { }
	~Bpm() { }
	string getId() const { return id; }
	double getX() const { return x; }
	void setX(double x) { this->x = x; }
	double getY() const { return y; }
	void setY(double y) { this->y = y; }
	double getXRef() const { return xRef; }
	void setXRef(double x) { xRef = x; }
	double getYRef() const { return yRef; }
	void setYRef(double y) { yRef = y; }
	double getXOffs() const { return xOffs; }
	void setXOffs(double x) { xOffs = x; }
	double getYOffs() const { return yOffs; }
	void setYOffs(double y) { yOffs = y; }
	double getXVoltsPerMilli() const { return xVoltsPerMilli; }
	void setXVoltsPerMilli(double val) { this->xVoltsPerMilli=val; }
	double getYVoltsPerMilli() const { return yVoltsPerMilli; }
	void setYVoltsPerMilli(double val) { yVoltsPerMilli=val; }
	void setEnabled(bool b) { enabled = b; }
	bool isEnabled() const { return enabled; }

private:
	Bpm();
	Bpm(const Bpm&);
	const string id;
	double x;
	double y;
	double xRef;
	double yRef;
	double xOffs;
	double yOffs;
	double xVoltsPerMilli;
	double yVoltsPerMilli;
	bool enabled;
};

#endif /* BPM_H_ */

