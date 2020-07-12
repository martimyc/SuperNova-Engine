#include "PreCompiledHeader.hpp"
#include "Subject.h"
#include "Event.h"

Subject::Subject(std::initializer_list<Observer*> t_list)
{
	for (auto it = t_list.begin(); it != t_list.end(); it++)
	{
		m_observers.emplace_back(std::move(*it));
	}
}

void Subject::AddObserver(Observer* t_observer)
{
	m_observers.emplace_back(std::move(t_observer));
}

void Subject::BroadcastEvent(Event && t_event) const
{
	for (auto it = m_observers.begin(); it != m_observers.end(); it++)
	{
		(*it)->ReceiveEvent(std::move(t_event));
	}
}
