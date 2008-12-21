/*
 * OrbitControlException.h
 *
 *  Created on: Dec 18, 2008
 *      Author: chabotd
 */

#ifndef ORBITCONTROLEXCEPTION_H_
#define ORBITCONTROLEXCEPTION_H_

#include <syslog.h>
#include <stdexcept>
using std::runtime_error;
using std::string;

class OrbitControlException : public runtime_error {
public:
	OrbitControlException(const string& msg = "", int rc=0)
	: runtime_error(msg),returncode(rc)
	{
		if(rc) { syslog(LOG_INFO, "\n\n%s rc=%d\n\n",msg.c_str(),returncode); }
		else { syslog(LOG_INFO, "\n\n%s\n\n",msg.c_str()); }
	}
private:
	int returncode;
};

#endif /* ORBITCONTROLEXCEPTION_H_ */
