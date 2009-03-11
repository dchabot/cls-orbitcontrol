/*
 * Publisher.h
 *
 *  Created on: Mar 11, 2009
 *      Author: chabotd
 */

#ifndef PUBLISHER_H_
#define PUBLISHER_H_

#include <Command.h>
#include <vector>
using std::vector;
using std::iterator;

/*
 * This is NOT a thread-safe Observer pattern: what happens if one thread calls
 * Publisher::execute() while another is calling Publisher::(un)subscribe() ???
 *
 * 						!!! TROUBLE !!! That's what...
 */
class Publisher {
public:
	Publisher() { }
	~Publisher() {
		for(uint32_t i=0; i<cbList.size(); i++) {
			if(cbList[i]) { delete cbList[i]; }
		}
		cbList.clear();
	}

	void subscribe(Command* cmd) {
		for(uint32_t i=0; i<cbList.size(); i++) {
			if(cbList[i]==cmd) { return; /*already in list*/ }
		}
		cbList.push_back(cmd);
	}

	void unsubscribe(Command* cmd) {
		for(uint32_t i=0; i<cbList.size(); i++) {
			if(cbList[i]==cmd) {
				removeCallback(i);
				return;
			}
		}
	}

	void publish() {
		for(uint32_t i=0; i<cbList.size(); i++) {
			cbList[i]->execute();
		}
	}

protected:
	void removeCallback(uint32_t cbIndex) {
		delete cbList[cbIndex]; //nuke the callback
		cbList.erase(cbList.begin()+cbIndex); //remove from our list
	}

private:
	Publisher(Publisher&);
	const Publisher& operator=(const Publisher&);

	vector<Command*> cbList;
};

#endif /* PUBLISHER_H_ */
