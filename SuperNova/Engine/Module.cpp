#include "PreCompiledHeader.hpp"
#include "Module.h"
#include "Event.h"

Module::Module(const std::string & t_name) : m_name(t_name)
{}

Module::Module(std::string && t_name) noexcept : m_name(std::move(t_name))
{}

void Module::Deactivate() noexcept
{
	m_active = false;
}

std::string const & Module::GetName() const noexcept
{
	return m_name;
}

bool Module::IsActive() const noexcept
{
	return m_active;
}

void Module::AddObserver(Observer* t_observer)
{
	m_subject.AddObserver(t_observer);
}
