
#include <iostream>
#include "interpreter.h"

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

int main()
{
	{
		Interpreter ip;

		//ip.Compile("../../Media/Scripts/helloworld.p");
		//ip.Compile("../../Media/Scripts/scopes.p");
		//ip.Compile("../../Media/Scripts/arithmetics.p");
		//ip.Compile("../../Media/Scripts/factorial.p");
		//ip.Compile("../../Media/Scripts/lnko.p");
		ip.Compile("../../Media/Scripts/primetest.p");
		//ip.Compile("../../Media/Scripts/bigtest.p");
		ip.Link();

		std::cout << "\n";
		ip.Disassemble();

		std::cout << "\n";
		ip.Run();
	}

	_CrtDumpMemoryLeaks();

	system("pause");
	return 0;
}
