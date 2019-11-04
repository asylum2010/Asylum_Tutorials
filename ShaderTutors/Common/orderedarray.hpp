
#ifndef _ORDEREDARRAY_HPP_
#define _ORDEREDARRAY_HPP_

#include <functional>
#include <algorithm>
#include <ostream>
#include <cstdint>

template <typename value_type, typename compare = std::less<value_type> >
class OrderedArray
{
private:
	value_type*	data;
	size_t		capacity;
	size_t		size;

	size_t _find(const value_type& value) const;

public:
	typedef std::pair<size_t, bool> pairib;

	static const size_t npos = SIZE_MAX;

	compare comp;

	OrderedArray();
	OrderedArray(const OrderedArray& other);
	~OrderedArray();
	
	void Clear();
	void Destroy();
	void Erase(const value_type& value);
	void EraseAt(size_t index);
	void FastCopy(const OrderedArray& other);
	void Reserve(size_t newcap);
	void Swap(OrderedArray& other);

	pairib Insert(const value_type& value);

	size_t Find(const value_type& value) const;
	size_t LowerBound(const value_type& value) const;
	size_t UpperBound(const value_type& value) const;

	OrderedArray& operator =(const OrderedArray& other);
	
	inline const value_type& operator [](size_t index) const {
		return data[index];
	}

	inline size_t Size() const {
		return size;
	}

	inline size_t Capacity() const {
		return capacity;
	}

	template <typename T, typename U>
	friend std::ostream& operator <<(std::ostream& os, const OrderedArray<T, U>& arr);
};

template <typename value_type, typename compare>
OrderedArray<value_type, compare>::OrderedArray()
{
	data		= nullptr;
	size		= 0;
	capacity	= 0;
}
	
template <typename value_type, typename compare>
OrderedArray<value_type, compare>::OrderedArray(const OrderedArray& other)
{
	data		= nullptr;
	size		= 0;
	capacity	= 0;

	this->operator =(other);
}
	
template <typename value_type, typename compare>
OrderedArray<value_type, compare>::~OrderedArray()
{
	Destroy();
}
	
template <typename value_type, typename compare>
size_t OrderedArray<value_type, compare>::_find(const value_type& value) const
{
	size_t low	= 0;
	size_t high	= size;
	size_t mid	= (low + high) / 2;

	while (low < high) {
		if (comp(data[mid], value))
			low = mid + 1;
		else if( comp(value, data[mid]) )
			high = mid;
		else
			return mid;

		mid = (low + high) / 2;
	}
	
	return low;
}
	
template <typename value_type, typename compare>
typename OrderedArray<value_type, compare>::pairib OrderedArray<value_type, compare>::Insert(const value_type& value)
{
	size_t i = 0;

	Reserve(size + 1);

	if (size > 0) {
		i = _find(value);

		if (i < size && !(comp(data[i], value) || comp(value, data[i])))
			return pairib(SIZE_MAX, false);

		size_t count = (size - i);
		new(data + size) value_type;

		for (size_t j = count; j > 0; --j)
			data[i + j] = data[i + j - 1];
	} else {
		new(data) value_type;
	}

	data[i] = value;
	++size;

	return pairib(i, true);
}
	
template <typename value_type, typename compare>
void OrderedArray<value_type, compare>::Erase(const value_type& value)
{
	size_t i = Find(value);

	if (i != npos) {
		size_t count = (size - i) - 1;

		for (size_t j = 0; j < count; ++j)
			data[i + j] = data[i + j + 1];

		(data + i + count)->~value_type();
		--size;

		if (size == 0)
			Clear();
	}
}
	
template <typename value_type, typename compare>
void OrderedArray<value_type, compare>::EraseAt(size_t index)
{
	if (index < size) {
		size_t count = (size - index) - 1;

		for (size_t j = 0; j < count; ++j)
			data[index + j] = data[index + j + 1];

		(data + index + count)->~value_type();
		--size;

		if (size == 0)
			Clear();
	}
}

template <typename value_type, typename compare>
void OrderedArray<value_type, compare>::Reserve(size_t newcap)
{
	if (capacity < newcap) {
		size_t diff = newcap - capacity;
		diff = std::max<size_t>(diff, 10);

		value_type* newdata = (value_type*)malloc((capacity + diff) * sizeof(value_type));

		for (size_t i = 0; i < size; ++i)
			new(newdata + i) value_type(data[i]);

		for (size_t i = 0; i < size; ++i)
			(data + i)->~value_type();

		if (data)
			free(data);

		data = newdata;
		capacity = capacity + diff;
	}
}

template <typename value_type, typename compare>
void OrderedArray<value_type, compare>::Clear()
{
	for( size_t i = 0; i < size; ++i )
		(data + i)->~value_type();

	size = 0;
}

template <typename value_type, typename compare>
void OrderedArray<value_type, compare>::Destroy()
{
	if (data) {
		for (size_t i = 0; i < size; ++i)
			(data + i)->~value_type();

		free(data);
	}

	data		= nullptr;
	size		= 0;
	capacity	= 0;
}

template <typename value_type, typename compare>
size_t OrderedArray<value_type, compare>::Find(const value_type& value) const
{
	if (size > 0) {
		size_t ind = _find(value);

		if (ind < size) {
			if (!(comp(data[ind], value) || comp(value, data[ind])))
				return ind;
		}
	}

	return npos;
}

template <typename value_type, typename compare>
size_t OrderedArray<value_type, compare>::LowerBound(const value_type& value) const
{
	// NOTE: returns the index of the first element which is not greater
	if (size > 0) {
		size_t ind = _find(value);

		if (!(comp(data[ind], value) || comp(value, data[ind])))
			return ind;
		else if (ind > 0)
			return ind - 1;
	}

	return npos;
}

template <typename value_type, typename compare>
size_t OrderedArray<value_type, compare>::UpperBound(const value_type& value) const
{
	// NOTE: returns the index of the first element which is not smaller
	return _find(value);
}
	
template <typename value_type, typename compare>
OrderedArray<value_type, compare>& OrderedArray<value_type, compare>::operator =(const OrderedArray& other)
{
	if (&other == this)
		return *this;

	Clear();
	Reserve(other.capacity);

	size = other.size;

	for (size_t i = 0; i < size; ++i)
		new(data + i) value_type(other.data[i]);

	return *this;
}

template <typename value_type, typename compare>
void OrderedArray<value_type, compare>::FastCopy(const OrderedArray& other)
{
	if (&other != this) {
		Reserve(other.capacity);

		size = other.size;
		memcpy(data, other.data, size * sizeof(value_type));
	}
}

template <typename value_type, typename compare>
void OrderedArray<value_type, compare>::Swap(OrderedArray& other)
{
	if (&other == this)
		return;

	std::swap(capacity, other.capacity);
	std::swap(size, other.size);
	std::swap(data, other.data);
}

template <typename value_type, typename compare>
std::ostream& operator <<(std::ostream& os, const OrderedArray<value_type, compare>& arr)
{
	for (size_t i = 0; i < arr.size(); ++i)
		os << arr[i] << " ";

	return os;
}

#endif
