#include <cstdlib>
#include <iostream>

#include "Globals.h"
#include "Observer.h"
#include "Subject.h"

int main(int argc, char** argv)
{
	class ob1 : public Observer
	{
	public:
		void OnNotify(Event e)
		{
			if (e == TEST)
				LOG("Observer works");
		}
	};

	class s : public Subject
	{
	public:
		void TestNotify()
		{
			Notify(TEST);
		}
	};

	s test;

	ob1 o;

	test.AddObserver(&o);

	std::cout << "Loop Started";

	for (int i = 4; i >= 0; i--)
	{
		std::cout << "Looping\n";
		test.TestNotify();
		test.Update();
	}

	std::cout << "Loop end";

	system("pause");
	return EXIT_SUCCESS;
}