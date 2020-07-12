#include "PreCompiledHeader.hpp"
#include "Observer.h"
#include "Event.h"

Event const & Observer::GetFirstEvent() const
{
	return m_events.front();
}

 void Observer::PopEvent()
{
	m_events.pop();
}

void Observer::ReceiveEvent(Event e)
{
	m_events.emplace(std::move(e));
}

bool Observer::Empty() const
{
	return m_events.empty();
}
