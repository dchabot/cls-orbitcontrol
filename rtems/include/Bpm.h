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
	Bpm(string str) : id(str),enabled(true) { }
	~Bpm() { }
	string getId() const;
	double getX() const;
	void setX(double x);
	double getY() const;
	void setY(double y);
	double getXRef() const;
	void setXRef(double x);
	double getYRef() const;
	void setYRef(double y);
	double getXOffs() const;
	void setXOffs(double x);
	double getYOffs() const;
	void setYOffs(double y);
	bool isEnabled() const;
private:
	Bpm();
	Bpm(const Bpm&);
	string id;
	double x;
	double y;
	double xRef;
	double yRef;
	double xOffs;
	double yOffs;
	bool enabled;
};

inline string Bpm::getId() const { return id; }
inline double Bpm::getX() const { return x; }
inline double Bpm::getY() const { return y; }
inline double Bpm::getXRef() const { return xRef; }
inline double Bpm::getYRef() const { return yRef; }
inline double Bpm::getXOffs() const { return xOffs; }
inline double Bpm::getYOffs() const { return yOffs; }

inline void Bpm::setX(double x) { this->x = x; }
inline void Bpm::setY(double y) { this->y = y; }
inline void Bpm::setXRef(double x) { xRef = x; }
inline void Bpm::setYRef(double y) { yRef = y; }
inline void Bpm::setXOffs(double x) { xOffs = x; }
inline void Bpm::setYOffs(double y) { yOffs = y; }

/** In the context of Orbit Control, isEnabled()==true
 *  indicates that the BPM (x,y) are included in the orbit
 *  control algorithm and calculation(s).
 *
 * @return
 */
inline bool Bpm::isEnabled() const { return enabled; }

#endif /* BPM_H_ */

