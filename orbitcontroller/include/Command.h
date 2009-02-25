/*
 * Command.h
 *
 *  Created on: Jan 28, 2009
 *      Author: chabotd
 */

#ifndef COMMAND_H_
#define COMMAND_H_

typedef void* CommandArg;

class Command {
public:
	Command(CommandArg c):arg(c){}
	virtual ~Command() {}
	virtual void execute()=0;

private:
	Command();
	Command(const Command&);
	const Command& operator=(const Command&);

	CommandArg arg;
};

#endif /* COMMAND_H_ */
