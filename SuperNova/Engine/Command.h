#ifndef COMMAND
#define COMMAND

#include <assert.h>
#include "Globals.h"

class Command
{
public:
	virtual void Execute()
	{
		LOG("called command execute witout agent, it is not defined");
		assert(true);
	}

	virtual void Execute(void * ptr)
	{
		LOG("Called command execute with agent, it is not defined");
		assert(true);
	}

	virtual void Undo() = 0;

	virtual void ~Command()	{}

private:

}
#endif // !COMMAND

