
#ifndef _METALIST_HPP_
#define _METALIST_HPP_

#include "typelist.hpp"

/**
 * \brief Recursive multi-type list (internal)
 */
template <typename type_list, int index>
class _metalist : public _metalist<type_list, index - 1>
{
	typedef typename type_at<type_list, index>::value value_type;

public:
	// make the overloaded method public
	using _metalist<type_list, index - 1>::push_back;

	void push_back(const value_type& value);
};

template <typename type_list>
class _metalist<type_list, 0>
{
	typedef typename type_at<type_list, 0>::value value_type;

protected:
	struct metalink_base {
		virtual ~metalink_base() {}

		metalink_base* next;
		metalink_base* prev;
	};

	template <typename value_type>
	struct metalink : metalink_base {
		value_type value;
	};

	metalink_base* head;

public:
	_metalist();
	virtual ~_metalist();

	void push_back(const value_type& value);
};

template <typename type_list>
_metalist<type_list, 0>::_metalist()
{
	head = new metalink_base();
	head->next = head;
	head->prev = head;
}

template <typename type_list>
_metalist<type_list, 0>::~_metalist()
{
	metalink_base* p = head->next;
	metalink_base* q;

	while (p != head) {
		q = p->next;
		delete p;

		p = q;
	}

	delete head;
	head = 0;
}

template <typename type_list>
void _metalist<type_list, 0>::push_back(const value_type& value)
{
	metalink<value_type>* ml = new metalink<value_type>();

	ml->value = value;
	ml->next = head;
	ml->prev = head->prev;

	head->prev->next = ml;
	head->prev = ml;
}

template <typename type_list, int index>
void _metalist<type_list, index>::push_back(const value_type& value)
{
	metalink<value_type>* ml = new metalink<value_type>();

	ml->value = value;
	ml->next = head;
	ml->prev = head->prev;

	head->prev->next = ml;
	head->prev = ml;
}

/**
 * \brief Multi-type list
 */
template <typename type_list>
class multilist : public _metalist<type_list, length<type_list>::value - 1>
{
public:
	class iterator;

	iterator begin() {
		return iterator(this, head->next);
	}

	iterator end() {
		return iterator(this, head);
	}
};

/**
 * \brief Multi-type list iterator
 */
template <typename type_list>
class multilist<type_list>::iterator
{
	typedef multilist<type_list> container_t;
	typedef typename multilist<type_list>::metalink_base link_t;

	friend class multilist<type_list>;

private:
	container_t* cont;
	link_t* ptr;

	iterator(container_t* _cont, link_t* _ptr)
		: cont(_cont), ptr(_ptr)
	{
	}

public:
	iterator() {
		cont = 0;
		ptr = 0;
	}

	iterator(const iterator& other) {
		cont = other.cont;
		ptr = other.ptr;
	}

	template <typename deref_type>
	deref_type& get()
	{
		metalink<deref_type>* cp = dynamic_cast<metalink<deref_type>*>(ptr);

		if (!cp)
			throw 1;

		return cp->value;
	}

	template <typename deref_type>
	deref_type* get_ptr()
	{
		metalink<deref_type>* cp = dynamic_cast<metalink<deref_type>*>(ptr);

		if (!cp)
			return 0;

		return &cp->value;
	}

	iterator& operator =(const iterator& other)
	{
		if (&other != this) {
			cont = other.cont;
			ptr = other.ptr;
		}

		return *this;
	}

	inline bool operator !=(const iterator& other) {
		return ptr != other.ptr;
	}

	inline iterator& operator ++() {
		ptr = ptr->next;
		return *this;
	}

	inline iterator operator ++(int) {
		iterator tmp(*this);
		ptr = ptr->next;

		return tmp;
	}
};

#endif
