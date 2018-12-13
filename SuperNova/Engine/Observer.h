#ifndef OBSERVER
#define OBSERVER

#include <vector>

//Forward declarations
class Subject;

enum Event
{
	NO_EVENT = 0,
	TEST
};

class Observer
{
public:	
	virtual ~Observer() {}

	virtual void OnNotify(Event e) = 0;

	bool operator == (Observer& observer);
	void UnregisterSubject(Subject* subject);

private:
	std::vector<Subject*> subjects_observed;
};

#endif // !OBSERVER

