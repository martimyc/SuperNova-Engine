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
	virtual ~Subject();

	void AddObserver(Observer* observer);
	void RemoveObserver(Observer* observer);
	void Update();									//Update on pre update so that every event is dealt with before updates start

protected:
	void Notify(Event e);

private:
	void Unregister();

	std::vector<Observer*> observers;
	std::queue<Event> events;
};

#endif // !SUBJECT

