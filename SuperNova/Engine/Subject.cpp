#include "Subject.h"
#include "Observer.h"

void Subject::AddObserver(Observer * observer)
{
	observers.push_back(observer);
}

void Subject::RemoveObserver(Observer * observer)
{
	for (auto it = observers.begin(); it != observers.end(); ++it)
	{
		if (*it == observer)
		{
			observers.erase(it);
			break;
		}
	}
}

void Subject::Update()
{
	while (!events.empty())
	{
		for (auto it = observers.begin(); it != observers.end(); ++it)
			(*it)->OnNotify(events.front());
		events.pop();
	}
}

void Subject::Unregister()
{
	for (auto it = observers.begin(); it != observers.end(); ++it)
		(*it)->UnregisterSubject(this);
}

void Subject::Notify(Event e)
{
	events.push(e);
}
