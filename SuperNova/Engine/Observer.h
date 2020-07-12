#ifndef OBSERVER
#define OBSERVER

#include "Event.h"

class Subject;
class Module;

class Observer
{
public:
	explicit Observer() = default;
	explicit Observer(const Observer&) = delete;
	explicit Observer(Observer&&) noexcept = default;
	Observer& operator = (const Observer&) = delete;
	Observer& operator = (Observer&&) noexcept = default;
	virtual ~Observer() = default;

	[[nodiscard]] Event const & GetFirstEvent() const;
	void PopEvent();
	void ReceiveEvent(Event e);
	[[nodiscard]] bool Empty() const;

private:
	std::queue<Event> m_events {};
};

#endif // !OBSERVER

