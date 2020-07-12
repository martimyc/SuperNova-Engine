#ifndef EVENTS
#define EVENTS

class Module;

enum EVENT_TYPE
{
	E_NO_TYPE = 0,
	E_MODULE_TERMINATION,
	E_CLOSE_WINDOW
};

class Event
{
public:
	explicit Event(EVENT_TYPE, Module const &);
	explicit Event() = delete;
	Event(Event const & ) = delete;
	Event(Event &&) noexcept = default;
	Event & operator = (Event const &) = delete;
	Event & operator = (Event &&) noexcept = default;
	virtual ~Event() noexcept = default;

	[[nodiscard]] EVENT_TYPE const & GetType() const;
	[[nodiscard]] Module const & GetOrigin() const;

protected:
	EVENT_TYPE m_type{E_NO_TYPE};
	Module const & m_from;
};

class ModuleTerminationEvent : public Event
{
public:
	ModuleTerminationEvent(Module const &);
	~ModuleTerminationEvent() noexcept = default;

private:

};

class CloseWindowEvent : public Event
{
public:
	CloseWindowEvent(Module const &);
	~CloseWindowEvent() noexcept = default;

private:

};

#endif // !EVENTS