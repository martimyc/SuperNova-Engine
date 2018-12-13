#include "Observer.h"

bool Observer::operator==(Observer & observer)
{
	if (&observer == this)
		return true;
	return false;
}

void Observer::UnregisterSubject(Subject* subject)
{
	for (auto it = subjects_observed.begin(); it != subjects_observed.end(); ++it)
	{
		if (*it == subject)
		{
			subjects_observed.erase(it);
			break;
		}
	}
}
