
#include "interpreter.h"

void Interpreter::Execute_Print(void* arg1, void* arg2)
{
	std::string* str = reinterpret_cast<std::string*>(arg1);

	if (str)
		std::cout << *str;
}
