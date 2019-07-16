
#include <iostream>
#include <string>

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#include "list.hpp"
#include "metalist.hpp"

#define DEBUG_METHOD(x)		std::cout << #x << "\n"; x;
#define ASSERT(x)			{ if( !(x) ) { std::cout << "ASSERTION FAILED: " << #x << "\n"; } }
#define SECTION(x)			std::cout << "\n\n" << x << "\n---------------------------------\n\n";

template <typename container_t>
void write(const std::string& name, const container_t& cont)
{
	std::cout << name << ": ";

	for (container_t::const_iterator it = cont.begin(); it != cont.end(); ++it)
		std::cout << *it << " ";

	std::cout << "\n";
}

int main()
{
	{
		SECTION("list: simple insertion");

		list<int> l1;

		l1.push_back(10);
		l1.push_back(5);
		l1.push_back(1);
		l1.push_back(-7);
		l1.push_back(8);

		ASSERT(l1.front() == 10);
		ASSERT(l1.back() == 8);

		write("l1", l1);

		DEBUG_METHOD(l1.clear());
		write("l1", l1);

		SECTION("list: insert at front/back");

		l1.push_front(10);
		l1.push_front(3);
		l1.push_back(5);
		l1.push_front(6);
		l1.push_back(-1);

		write("l1", l1);

		SECTION("list: resize & copy");

		DEBUG_METHOD(l1.resize(10, 25));
		write("l1", l1);

		DEBUG_METHOD(list<int> l2(l1));
		list<int> l3;

		l3.push_back(34);
		l3.push_back(11);

		ASSERT(l2.front() == l1.front());
		ASSERT(l2.back() == l1.back());
		ASSERT(l3.back() == 11);

		DEBUG_METHOD(l2.push_front(2));
		DEBUG_METHOD(l3 = l1);

		write("l2", l2);
		write("l3", l3);

		SECTION("list: random insertion");
		{
			DEBUG_METHOD(l2.insert(l2.begin(), 55));
			DEBUG_METHOD(l2.insert(l2.end(), 44));

			ASSERT(l2.front() == 55);
			ASSERT(l2.back() == 44);

			write("l2", l2);

			list<int>::iterator it = l2.begin();

			for( int i = 0; i < 4; ++i )
				++it;

			std::cout << "*it == " << *it << "\n";

			DEBUG_METHOD(l2.insert(it, 3));
			DEBUG_METHOD(l2.insert(it, 5));
			DEBUG_METHOD(l2.insert(it, 1));

			write("l2", l2);
		}

		SECTION("list: iterator errors");
		{
			list<int>::iterator it1, it2;

			it1 = l1.begin();
			it2 = l2.end();

			//DEBUG_METHOD(it1 == it2);	// incompaitble
			//DEBUG_METHOD(it1 != it2);	// incompaitble
			//DEBUG_METHOD(*it2);		// not dereferencable
			//DEBUG_METHOD(++it2);		// not incrementable
			//DEBUG_METHOD(it2++);		// not incrementable
			//DEBUG_METHOD(--it1);		// not decrementable
			//DEBUG_METHOD(it1--);		// not decrementable
		}

		SECTION("list: erase & remove");
		{
			write("l2", l2);

			DEBUG_METHOD(l2.erase(l2.begin()));
			//DEBUG_METHOD(l2.erase(l2.end()));	// invalid iterator

			write("l2", l2);

			list<int>::iterator it = l2.begin();

			for( int i = 0; i < 4; ++i )
				++it;

			std::cout << "*it == " << *it << "\n";

			DEBUG_METHOD(l2.erase(it));
			//DEBUG_METHOD(l2.erase(it));	// invalid iterator

			write("l2", l2);

			DEBUG_METHOD(l2.remove(25));
			write("l2", l2);
		}

	}

	{
		SECTION("list: types");

		class Apple {
		private:
			int i;

		public:
			Apple()			{ i = 0; }
			Apple(int j)	{ i = j; }

			void foo() {
				std::cout << "Apple::foo(): i == " << i << "\n";
			}
		};

		typedef list<Apple> applelist;
		typedef list<Apple*> appleptrlist;

		applelist apples;
		appleptrlist appleptrs;

		apples.push_back(6);
		apples.push_back(10);
		apples.push_front(-4);
		apples.push_back(5);

		appleptrs.push_back(new Apple(20));
		appleptrs.push_back(new Apple(1));
		appleptrs.push_front(new Apple(-4));
		appleptrs.push_back(new Apple(-3));
		appleptrs.push_front(new Apple(6));

		std::cout << "apples:\n";

		for( applelist::iterator it = apples.begin(); it != apples.end(); ++it )
			it->foo();

		std::cout << "\nappleptrs:\n";

		for (appleptrlist::iterator it = appleptrs.begin(); it != appleptrs.end(); ++it) {
			(*it)->foo();
			delete (*it);
		}
	}

	SECTION("multi-type list:");
	{
		struct Apple {
			std::string name;
		};

		typedef multilist<TL3(int, float, Apple)> meta3_ifA;
		meta3_ifA l1;

		Apple a;
		a.name = "apple";

		l1.push_back(a);
		l1.push_back(5);
		l1.push_back(3.5f);
		l1.push_back(2);
		l1.push_back(0.1f);

		int cnt = 0;

		for (meta3_ifA::iterator it = l1.begin(); it != l1.end(); ++it) {
			if (cnt == 0)
				std::cout << it.get<Apple>().name << "\n";
			if (cnt == 1 || cnt == 3)
				std::cout << it.get<int>() << "\n";
			else if (cnt == 2 || cnt == 4)
				std::cout << it.get<float>() << "\n";

			++cnt;
		}
	}

	_CrtDumpMemoryLeaks();

	system("pause");
	return 0;
}
