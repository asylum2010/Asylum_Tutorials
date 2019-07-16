
#ifndef _VARIADIC_POINTER_SET_HPP_
#define _VARIADIC_POINTER_SET_HPP_

#include <set>

class VariadicPointerSet
{
	struct BaseItem
	{
		virtual ~BaseItem() {}

		virtual void Tidy() = 0;
		virtual void* Ptr() = 0;
	};

	template <typename value_type>
	struct Item;

	struct Compare
	{
		bool operator ()(BaseItem* a, BaseItem* b) const {
			return (a->Ptr() < b->Ptr());
		}
	};

	typedef std::set<BaseItem*, Compare> Container;

private:
	Container values;

public:
	VariadicPointerSet();
	~VariadicPointerSet();

	void Clear();
	void Erase(void* ptr);
	
	template <typename value_type>
	void Insert(value_type* ptr);
};

template <typename value_type>
struct VariadicPointerSet::Item : VariadicPointerSet::BaseItem
{
	value_type* value;

	Item(value_type* ptr);

	void Tidy() override;
	void* Ptr() override;
};

template <typename value_type>
VariadicPointerSet::Item<value_type>::Item(value_type* ptr)
	: value(ptr)
{
}

template <typename value_type>
void VariadicPointerSet::Item<value_type>::Tidy()
{
	delete value;
	value = nullptr;
}

template <typename value_type>
void* VariadicPointerSet::Item<value_type>::Ptr()
{
	return value;
}

template <typename value_type>
void VariadicPointerSet::Insert(value_type* ptr)
{
	BaseItem* bi = new Item<value_type>(ptr);
	auto it = values.find(bi);

#if defined(_MSC_VER) && defined(_DEBUG)
	// If the program stops here it means that you
	// used 'delete' instead of 'interpreter->Deallocate()'.
	if (it != values.end())
		__debugbreak();
#endif

	values.insert(bi);
}

#endif

