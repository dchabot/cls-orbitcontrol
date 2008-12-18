/*
 * OrbitControlException.h
 *
 *  Created on: Dec 18, 2008
 *      Author: chabotd
 */

#ifndef ORBITCONTROLEXCEPTION_H_
#define ORBITCONTROLEXCEPTION_H_

#include <stdexcept>
using std::runtime_error;
using std::string;

class OrbitControlException : public runtime_error {
public:
	OrbitControlException(const string& msg = "") : runtime_error(msg) {}
};

#endif /* ORBITCONTROLEXCEPTION_H_ */
