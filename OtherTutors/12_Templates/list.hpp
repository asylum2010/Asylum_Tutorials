
#ifndef _LIST_HPP_
#define _LIST_HPP_

#include <cassert>

template <typename T>
struct default_less
{
	inline bool operator ()(const T& a, const T& b) const {
		return a < b;
	}
};

template <typename value_type>
class list
{
protected:
	struct link {
		value_type value;
		link* next;
		link* prev;
	};

	link* head;
	size_t mysize;

public:
	typedef value_type value_type;

	class iterator;
	class const_iterator;

	list();
	list(const list& other);
	~list();

	explicit list(size_t size, const value_type& value = value_type());
	list& operator =(const list& other);

	void push_back(const value_type& item);
	void push_front(const value_type& item);
	void pop_front();
	void pop_back();
	void resize(size_t newsize, const value_type& value = value_type());
	void clear();
	void remove(const value_type& value);

	iterator insert(const iterator& pos, const value_type& value);
	iterator erase(iterator& pos);

	inline value_type& front() {
		assert(mysize > 0);
		return head->next->value;
	}

	inline const value_type& front() const {
		assert(mysize > 0);
		return head->next->value;
	}

	inline value_type& back() {
		assert(mysize > 0);
		return head->prev->value;
	}

	inline const value_type& back() const {
		assert(mysize > 0);
		return head->prev->value;
	}

	inline bool empty() const {
		return (mysize == 0);
	}

	inline size_t size() const {
		return mysize;
	}

	inline iterator begin() {
		return iterator(this, head->next);
	}

	inline iterator end() {
		return iterator(this, head);
	}

	inline const_iterator begin() const {
		return const_iterator(this, head->next);
	}

	inline const_iterator end() const {
		return const_iterator(this, head);
	}
};

#include "list_iterator.hpp"

template <typename value_type>
list<value_type>::list()
{
	head = new link();

	head->next = head->prev = head;
	mysize = 0;
}

template <typename value_type>
list<value_type>::list(size_t size, const value_type& value)
{
	head = new link();

	head->next = head->prev = head;
	mysize = 0;

	resize(size, value);
}

template <typename value_type>
list<value_type>::list(const list& other)
{
	head = new link();

	head->next = head->prev = head;
	mysize = 0;

	operator =(other);
}

template <typename value_type>
list<value_type>::~list()
{
	if (head) {
		clear();

		delete head;
		head = nullptr;
	}
}

template <typename value_type>
list<value_type>& list<value_type>::operator =(const list<value_type>& other)
{
	if (&other == this)
		return *this;

	clear();

	for (const_iterator it = other.begin(); it != other.end(); ++it)
		push_back(*it);

	return *this;
}

template <typename value_type>
void list<value_type>::push_back(const value_type& item)
{
	link* last = head->prev;

	last->next			= new link();
	last->next->prev	= last;
	last->next->next	= head;
	last->next->value	= item;

	head->prev			= last->next;
	++mysize;
}

template <typename value_type>
void list<value_type>::push_front(const value_type& item)
{
	link* first = head->next;

	head->next			= new link();
	head->next->prev	= head;
	head->next->next	= first;
	head->next->value	= item;

	first->prev			= head->next;
	++mysize;
}

template <typename value_type>
void list<value_type>::pop_front()
{
	assert(mysize > 0);

	link* second = head->next->next;
	delete head->next;

	head->next = second;
	second->prev = head;

	--mysize;
}

template <typename value_type>
void list<value_type>::pop_back()
{
	assert(mysize > 0);

	link* last = head->prev;
	head->prev = last->prev;

	last->prev->next = head;
	delete last;

	--mysize;
}

template <typename value_type>
void list<value_type>::resize(size_t newsize, const value_type& value)
{
	if (mysize < newsize) {
		while (mysize != newsize)
			push_back(value);
	} else {
		while (mysize != newsize)
			pop_back();
	}
}

template <typename value_type>
void list<value_type>::clear()
{
	link* q = head->next;
	link* p;

	while (q != head) {
		p = q;
		q = q->next;
		delete p;
	}

	head->next = head->prev = head;
	mysize = 0;
}

template <typename value_type>
void list<value_type>::remove(const value_type& value)
{
	link *p, *r;

	for (link* q = head->next; q != head; q = q->next) {
		if (q->value == value) {
			p = q->prev;
			r = q->next;

			p->next = r;
			r->prev = p;

			delete q;
			q = p;

			--mysize;
		}
	}
}

template <typename value_type>
typename list<value_type>::iterator list<value_type>::insert(const iterator& pos, const value_type& value)
{
	assert(pos.container == this);

	link* q = pos.ptr->prev;

	q->next			= new link();
	q->next->next	= pos.ptr;
	q->next->prev	= q;

	pos.ptr->prev	= q->next;
	q->next->value	= value;

	++mysize;
	return iterator(this, q->next);
}

template <typename value_type>
typename list<value_type>::iterator list<value_type>::erase(iterator& pos)
{
	assert(mysize > 0);
	assert(pos.container == this);
	assert(pos.ptr != head);

	link* q = pos.ptr->next;
	link* p = pos.ptr->prev;

	p->next = q;
	q->prev = p;

	delete pos.ptr;
	pos.ptr = head;

	--mysize;
	return iterator(this, q);
}

#endif
