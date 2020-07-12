#ifndef APPLICATION
#define APPLICATION

class Module;
class Window;
class Observer;

class Application
{
public:
	explicit Application();
	Application(Application const &) = delete;
	Application(Application&&) noexcept = default;
	Application & operator = (Application const &) = delete;
	Application & operator = (Application&&) = default;
	~Application() noexcept;

	void Start() noexcept;
	void Update() noexcept;
	void CleanUp() noexcept;
	void Quit() noexcept;

	bool IsRunning() const;

	static void LogError(std::string const &) noexcept;

private:
	void RemoveTerminatedModules() noexcept;
	void HandleEvents();

	void StartWithErrorHandling(std::vector<std::unique_ptr<Module>>::iterator&) noexcept;
	void PreUpdateWithErrorHandling(std::vector<std::unique_ptr<Module>>::iterator&) noexcept;
	void UpdateWithErrorHandling(std::vector<std::unique_ptr<Module>>::iterator&) noexcept;
	void PostUpdateWithErrorHandling(std::vector<std::unique_ptr<Module>>::iterator&) noexcept;

	static void WriteLogToLogFile() noexcept;
	static void CreateLogFile() noexcept;
	static void LogCurrentError() noexcept;

	static std::ofstream m_log_file;
	static std::stringstream m_log;

	bool m_running{ false };
	std::vector<std::unique_ptr<Module>> m_modules{};
	std::unique_ptr<Observer> m_observer;
};

#endif // !APPLICATION

