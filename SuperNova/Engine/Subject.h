#ifndef SUBJECT
#define SUBJECT

#include "Observer.h"

class Observer;
class Event;

class Subject
{
public:
	explicit Subject() = default;
	Subject(const Subject&) = delete;
	Subject(Subject&&) noexcept = default;
	Subject& operator = (const Subject&) = delete;
	Subject& operator = (Subject&&) noexcept = default;
	~Subject() = default;

	Subject(std::initializer_list<Observer*>);

	void AddObserver(Observer*);
	void BroadcastEvent(Event && e) const;

private:
	std::vector<Observer*> m_observers;
};

#endif // !SUBJECT

