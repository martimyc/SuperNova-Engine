#ifndef OBSERVER
#define OBSERVER

enum Event
{
	NO_EVENT = 0,
	TEST
};

class Observer
{
public:	
	virtual ~Observer() {}
	virtual void OnNotify(Event e) = 0;

	bool operator == (Observer& observer)
	{
		if (&observer == this)
			return true;
		return false;
	}

private:

};

#endif // !OBSERVER

