
#ifndef _ORDEREDMULTIARRAY_HPP_
#define _ORDEREDMULTIARRAY_HPP_

#include <functional>
#include <algorithm>
#include <cstdint>

template <typename value_type, typename compare = std::less<value_type> >
class OrderedMultiArray
{
private:
	value_type*	data;
	size_t		capacity;
	size_t		size;

	size_t _find(const value_type& value) const;

public:
	typedef std::pair<size_t, size_t> pairii;

	static const size_t npos = SIZE_MAX;

	compare comp;

	OrderedMultiArray();
	OrderedMultiArray(const OrderedMultiArray& other);
	~OrderedMultiArray();

	size_t Insert(const value_type& value);

	void Clear();
	void Destroy();
	void Erase(const value_type& value);
	void PopBack();
	void Reserve(size_t newcap);

	pairii EqualRange(const value_type& value) const;

	size_t Find(const value_type& value) const;
	size_t LowerBound(const value_type& value) const;
	size_t UpperBound(const value_type& value) const;

	OrderedMultiArray& operator =(const OrderedMultiArray& other);
	
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
	friend std::ostream& operator <<(std::ostream& os, OrderedMultiArray<T, U>& arr);
};

template <typename value_type, typename compare>
OrderedMultiArray<value_type, compare>::OrderedMultiArray()
{
	data		= nullptr;
	size		= 0;
	capacity	= 0;
}

template <typename value_type, typename compare>
OrderedMultiArray<value_type, compare>::OrderedMultiArray(const OrderedMultiArray& other)
{
	data		= nullptr;
	size		= 0;
	capacity	= 0;

	this->operator =(other);
}

template <typename value_type, typename compare>
OrderedMultiArray<value_type, compare>::~OrderedMultiArray()
{
	Destroy();
}

template <typename value_type, typename compare>
size_t OrderedMultiArray<value_type, compare>::_find(const value_type& value) const
{
	size_t low	= 0;
	size_t high	= size;
	size_t mid	= (low + high) / 2;

	while (low < high) {
		if (comp(data[mid], value))
			low = mid + 1;
		else if (comp(value, data[mid]))
			high = mid;
		else
			return mid;

		mid = (low + high) / 2;
	}
	
	return low;
}

template <typename value_type, typename compare>
size_t OrderedMultiArray<value_type, compare>::Insert(const value_type& value)
{
	size_t i = 0;

	Reserve(size + 1);

	if (size > 0) {
		i = _find(value);

		size_t count = (size - i);
		new(data + size) value_type();

		for (size_t j = count; j > 0; --j)
			data[i + j] = data[i + j - 1];
	} else {
		new(data) value_type();
	}

	data[i] = value;
	++size;

	return i;
}

template <typename value_type, typename compare>
void OrderedMultiArray<value_type, compare>::Erase(const value_type& value)
{
	pairii p = EqualRange(value);

	if (p.first != npos) {
		size_t count = (size - p.second);

		for (size_t j = 0; j < count; ++j)
			data[p.first + j] = data[p.second + j];

		for (size_t i = p.first + count; i < size; ++i)
			(data + i)->~value_type();

		size -= (p.second - p.first);

		if (size == 0)
			Clear();
	}
}

template <typename value_type, typename compare>
void OrderedMultiArray<value_type, compare>::Reserve(size_t newcap)
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
void OrderedMultiArray<value_type, compare>::Clear()
{
	for (size_t i = 0; i < size; ++i)
		(data + i)->~value_type();

	size = 0;
}

template <typename value_type, typename compare>
void OrderedMultiArray<value_type, compare>::Destroy()
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
void OrderedMultiArray<value_type, compare>::PopBack()
{
	if (size > 0) {
		--size;
		(data + size)->~value_type();
	}
}

template <typename value_type, typename compare>
typename OrderedMultiArray<value_type, compare>::pairii
OrderedMultiArray<value_type, compare>::EqualRange(const value_type& value) const
{
	pairii range;

	range.first = npos;
	range.second = npos;

	if (size > 0) {
		range.first = _find(value);

		if (range.first < size) {
			if (comp(data[range.first], value) || comp(value, data[range.first])) {
				// doesn't exist
				range.first = npos;
			} else {
				range.second = range.first + 1;

				// look for first and last index
				while (range.first > 0) {
					if (comp(data[range.first - 1], value))
						break;

					--range.first;
				}

				while (range.second < size) {
					if (comp(value, data[range.second]))
						break;

					++range.second;
				}
			}
		} else {
			range.first = npos;
		}
	}

	return range;
}
	
template <typename value_type, typename compare>
size_t OrderedMultiArray<value_type, compare>::Find(const value_type& value) const
{
	return LowerBound(value);
}
	
template <typename value_type, typename compare>
size_t OrderedMultiArray<value_type, compare>::LowerBound(const value_type& value) const
{
	// returns the first that is greater or equal
	if (size == 0)
		return 0;

	size_t ind = _find(value);

	if (ind < size) {
		if (comp(value, data[ind])) {
			// first greater
			return ind;
		} else if (!comp(data[ind], value)) {
			// equal, find first equal
			do {
				if (ind == 0 || comp(data[ind - 1], value))
					break;

				--ind;
			} while( true );
				
			return ind;
		}
	}

	return npos;
}
	
template <typename value_type, typename compare>
size_t OrderedMultiArray<value_type, compare>::UpperBound(const value_type& value) const
{
	// returns the first that is greater
	if (size == 0)
		return 0;

	size_t ind = _find(value);

	if (ind < size) {
		if (comp(value, data[ind])) {
			// first greater
			return ind;
		} else if( !comp(data[ind], value)) {
			// equal, find first greater
			++ind;

			while (ind < size) {
				if (comp(value, data[ind]))
					break;

				++ind;
			}

			if (ind != size)
				return ind;
		}
	}

	return npos;
}
	
template <typename value_type, typename compare>
OrderedMultiArray<value_type, compare>& OrderedMultiArray<value_type, compare>::operator =(const OrderedMultiArray& other)
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
std::ostream& operator <<(std::ostream& os, OrderedMultiArray<value_type, compare>& arr)
{
	for (size_t i = 0; i < oa.size(); ++i)
		os << arr.data[i] << " ";

	return os;
}

#endif
