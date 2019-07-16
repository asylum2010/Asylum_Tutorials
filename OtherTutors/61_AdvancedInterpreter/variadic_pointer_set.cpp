
#include "variadic_pointer_set.hpp"

// --- VariadicPointerSet impl ------------------------------------------------

VariadicPointerSet::VariadicPointerSet()
{
}

VariadicPointerSet::~VariadicPointerSet()
{
	Clear();
}

void VariadicPointerSet::Clear()
{
	for (auto it = values.begin(); it != values.end(); ++it) {
		(*it)->Tidy();
		delete (*it);
	}

	values.clear();
}

void VariadicPointerSet::Erase(void* ptr)
{
	Item<void> tmp(ptr);
	auto it = values.find(&tmp);

	if (it != values.end()) {
		BaseItem* bi = (*it);
		values.erase(it);

		bi->Tidy();
		delete bi;
	}
}
