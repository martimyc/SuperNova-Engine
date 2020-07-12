#include "PreCompiledHeader.hpp"
#include "Event.h"

Event::Event(EVENT_TYPE t_type, Module const & t_from): m_type(t_type), m_from(t_from)
{}

EVENT_TYPE const & Event::GetType() const
{
	return m_type;
}

Module const & Event::GetOrigin() const
{
	return m_from;
}

ModuleTerminationEvent::ModuleTerminationEvent(Module const & t_from): Event(E_MODULE_TERMINATION, t_from)
{}

CloseWindowEvent::CloseWindowEvent(Module const & t_from) : Event(E_CLOSE_WINDOW, t_from)
{}
