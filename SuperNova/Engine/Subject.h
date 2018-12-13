#ifndef SUBJECT
#define SUBJECT

#include <vector>

#include "Observer.h"

class Subject
{
public:
	virtual ~Subject() {}

	void AddObserver(Observer* observer)
	{
		observers.push_back(observer);
	}

	void RemoveObserver(Observer* observer)
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

protected:
	void Notify(Event e)
	{
		for (auto it = observers.begin(); it != observers.end(); ++it)
			(*it)->OnNotify(e);
	}

private:
	std::vector<Observer*> observers;

};

#endif // !SUBJECT

