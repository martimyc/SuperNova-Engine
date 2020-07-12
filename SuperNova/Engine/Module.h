#ifndef MODULE
#define MODULE

#include "Subject.h"

class Module
{
public:
	explicit Module() = default;
	Module(Module const &) = delete;
	Module(Module&&) noexcept = default;
	Module& operator = (Module const &) = delete;
	Module& operator = (Module&&) noexcept = default;
	virtual ~Module() noexcept = default;

	explicit Module(std::string const &);
	explicit Module(std::string&&) noexcept;

	virtual void Start() = 0;
	virtual void PreUpdate() = 0;
	virtual void Update() = 0;
	virtual void PostUpdate() = 0;
	virtual void CleanUp() noexcept = 0;

	void Deactivate() noexcept;

	[[nodiscard]] std::string const & GetName() const noexcept;
	[[nodiscard]] bool IsActive() const noexcept;
	void AddObserver(Observer*);

protected:
	bool m_active {true};
	const std::string m_name{};
	Subject m_subject{};
};

#endif //!MODULE