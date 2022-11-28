#pragma once
#include <ostream>
#include <vector>

// ==================== Page =========================
template <typename Iterator>
class Page
{
public:
	Page(Iterator begin, Iterator end)
		:begin_(begin)
		, end_(end)
		, size_(distance(begin, end))
	{  }

	[[nodiscard]] Iterator begin() const
	{
		return begin_;
	}

	[[nodiscard]] Iterator end() const
	{
		return end_;
	}

	[[nodiscard]] size_t size() const
	{
		return size_;
	}

private:
	Iterator begin_, end_;
	size_t size_;
};

template <typename Iterator>
inline std::ostream& operator<<(std::ostream& out, const Page<Iterator>& range)
{
	for (Iterator it = range.begin(); it != range.end(); ++it)
	{
		out << *it;
	}
	return out;
}

// ==================== Paginator =========================

template <typename Iterator>
class Paginator
{
public:
	Paginator(Iterator begin, Iterator end, size_t page_size)
	{
		while (begin != end)
		{
			if (static_cast<size_t>(distance(begin, end)) > page_size)
			{
				pages_.push_back(Page<Iterator>(begin, begin + page_size));
				begin += page_size;
			}
			else
			{
				pages_.push_back(Page<Iterator>(begin, begin + distance(begin, end)));
				begin += distance(begin, end);
			}
		}
	}

	[[nodiscard]] auto begin() const
	{
		return pages_.begin();
	}

	[[nodiscard]] auto end() const
	{
		return pages_.end();
	}

	[[nodiscard]] size_t size() const
	{
		return pages_.size();
	}

private:
	std::vector<Page<Iterator>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size)
{
	return Paginator(begin(c), end(c), page_size);
}