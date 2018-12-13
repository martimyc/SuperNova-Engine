#ifndef SUBJECT
#define SUBJECT

#include <vector>
#include <queue>

//Forward declarations
class Observer;
enum Event;

class Subject
{
public:
	virtual ~Subject() {}

	void AddObserver(Observer* observer);
	void RemoveObserver(Observer* observer);
	void Update();
	void Unregister();

protected:
	void Notify(Event e);

private:
	std::vector<Observer*> observers;
	std::queue<Event> events;
};

#endif // !SUBJECT

