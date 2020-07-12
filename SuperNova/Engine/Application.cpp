#include "PreCompiledHeader.hpp"
#include "Application.hpp"
#include "date.h"
#include "Module.h"
#include "Renderer.h"

std::stringstream Application::m_log{}; 
std::ofstream Application::m_log_file{};

Application::Application():
	m_running{ true },
	m_modules{},
	m_observer{std::make_unique<Observer>()}
{
	constexpr auto num_modules = 1;
	m_modules.reserve(num_modules);
	
	m_modules.emplace_back(std::make_unique<Renderer>());
	m_modules.back()->AddObserver(m_observer.get());
}

Application::~Application() noexcept
{
	if (m_log_file.is_open()) {
		m_log_file.close();
	}
}

void Application::Start() noexcept
{
	for (auto it = m_modules.begin(); it != m_modules.end(); ++it) {
		StartWithErrorHandling(it);
	}
}

void Application::Update() noexcept
{
	HandleEvents();

	RemoveTerminatedModules();

	for (auto it = m_modules.begin(); it != m_modules.end(); ++it) {
		PreUpdateWithErrorHandling(it);
	}

	for (auto it = m_modules.begin(); it != m_modules.end(); ++it) {
		UpdateWithErrorHandling(it);
	}

	for (auto it = m_modules.begin(); it != m_modules.end(); ++it) {
		PostUpdateWithErrorHandling(it);
	}
}

void Application::CleanUp() noexcept
{
	for (auto it = m_modules.rbegin(); it != m_modules.rend(); ++it) {
		(*it)->CleanUp();
	}
}

void Application::Quit() noexcept
{
	m_running = false;
}

bool Application::IsRunning() const
{
	return m_running;
}

void Application::LogError(std::string const & t_log_message) noexcept
{
	m_log << t_log_message;
	LogCurrentError();
}

void Application::RemoveTerminatedModules() noexcept
{	
	auto erase_from = std::remove_if(m_modules.begin(), m_modules.end(), [](std::unique_ptr<Module>& t_module) { return !t_module.get()->IsActive(); });
	m_modules.erase(erase_from, m_modules.end());
}

void Application::HandleEvents()
{
	while (!m_observer->Empty())
	{
		decltype(auto) observer_event = m_observer->GetFirstEvent();

		switch (observer_event.GetType())
		{
		case E_CLOSE_WINDOW: m_running = false; break;
		default:
			break;
		}

		m_observer->PopEvent();
	}
}

void Application::StartWithErrorHandling(std::vector<std::unique_ptr<Module>>::iterator& t_mod) noexcept
{
	try {
		(*t_mod)->Start();
	}
	catch (std::exception & error) {
		m_log << "Exception thrown at module " << (*t_mod)->GetName() << " - Start: " << error.what() << '\n';
		LogCurrentError();
		(*t_mod)->Deactivate();
	}
}

void Application::PreUpdateWithErrorHandling(std::vector<std::unique_ptr<Module>>::iterator& t_mod) noexcept
{
	try {
		(*t_mod)->PreUpdate();
	}
	catch (std::exception & error) {
		m_log << "Exception thrown at module " << (*t_mod)->GetName() << " - PreUpdate: " << error.what() << '\n';
		LogCurrentError();
		(*t_mod)->Deactivate();
	}
}

void Application::UpdateWithErrorHandling(std::vector<std::unique_ptr<Module>>::iterator& t_mod) noexcept
{
	try {
		(*t_mod)->Update();
	}
	catch (std::exception & error) {
		m_log << "Exception thrown at module " << (*t_mod)->GetName() << " - Update: " << error.what() << '\n';
		LogCurrentError();
		(*t_mod)->Deactivate();
	}
}

void Application::PostUpdateWithErrorHandling(std::vector<std::unique_ptr<Module>>::iterator& t_mod) noexcept
{
	try {
		(*t_mod)->PostUpdate();
	}
	catch (std::exception & error) {
		m_log << "Exception thrown at module " << (*t_mod)->GetName() << " - PostUpdate: " << error.what() << '\n';
		LogCurrentError();
		(*t_mod)->Deactivate();
	}
}

void Application::WriteLogToLogFile() noexcept
{
	if (!m_log_file.is_open()) {
		CreateLogFile();
	}
	m_log_file << m_log.str() << std::flush;
}

void Application::CreateLogFile() noexcept
{
	try {
		using namespace std::chrono;
		using namespace std::filesystem;

		auto file_path = path{ current_path() };
		file_path /= "Logs";
		if (!std::filesystem::exists(file_path)) {
			if (!std::filesystem::create_directories(file_path)) {
				m_log << "Application - Logs directory could not be created\n";
				LogCurrentError();
				return;
			}
		}

		auto time = system_clock::now();
		auto daypoint = date::floor<date::days>(time);
		auto year_month_day = date::year_month_day{ daypoint };

		auto time_zone_dif = 2h;
		auto time_of_day = date::make_time(time - daypoint + time_zone_dif);

		std::stringstream s;		
		s << "Log_" << year_month_day << "_" <<
			time_of_day.hours().count() << "-" <<
			time_of_day.minutes().count() << "-" <<
			time_of_day.seconds().count() << ".txt";

		file_path /= s.str();
		m_log_file.open(file_path);
	}
	catch(std::exception & exception){
		std::cerr << "Could not create Log file: " << exception.what();
	}
}

void Application::LogCurrentError() noexcept
{
	m_log << '\n';
	std::cerr << m_log.str();
	WriteLogToLogFile();
	m_log.clear();
}