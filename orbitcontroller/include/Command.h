/*
 * Command.h
 *
 *  Created on: Jan 28, 2009
 *      Author: chabotd
 */

#ifndef COMMAND_H_
#define COMMAND_H_

typedef void* CommandArg;
typedef void (*CommandFunc)(CommandArg);

class Command {
public:
	Command(CommandFunc f, CommandArg c) : func(f),arg(c) { }
	virtual ~Command() { }
	void execute() {
		this->func(arg);
	}

private:
	Command();
	Command(const Command&);
	const Command& operator=(const Command&);

	CommandFunc func;
	CommandArg arg;
};

#endif /* COMMAND_H_ */
