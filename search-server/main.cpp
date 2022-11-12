#include <iostream>
#include <ostream>

#include "request_queue.h"
#include "search_server.h"
#include "paginator.h"

std::ostream& operator<<(std::ostream& out, const Document& document)
{
	out << "{ document_id = " << document.id << ", relevance = " << document.relevance << ", rating = " << document.rating << " }";
	return out;
}

template <typename Iterator>
std::ostream& operator<<(std::ostream& out, const Page<Iterator>& range)
{
	for (Iterator it = range.begin(); it != range.end(); ++it)
	{
		out << *it;
	}
	return out;
}

// ==================== для примера =========================

template <typename Container>
auto Paginate(const Container& c, size_t page_size)
{
	return Paginator(begin(c), end(c), page_size);
}

int main() {
	SearchServer search_server(std::string("and in at"));
	RequestQueue request_queue(search_server);
	search_server.AddDocument(1, std::string("curly cat curly tail"), DocumentStatus::ACTUAL, { 7, 2, 7 });
	search_server.AddDocument(2, std::string("curly dog and fancy collar"), DocumentStatus::ACTUAL, { 1, 2, 3 });
	search_server.AddDocument(3, std::string("big cat fancy collar "), DocumentStatus::ACTUAL, { 1, 2, 8 });
	search_server.AddDocument(4, std::string("big dog sparrow Eugene"), DocumentStatus::ACTUAL, { 1, 3, 2 });
	search_server.AddDocument(5, std::string("big dog sparrow Vasiliy"), DocumentStatus::ACTUAL, { 1, 1, 1 });
	// 1439 запросов с нулевым результатом
	for (int i = 0; i < 1439; ++i) {
		request_queue.AddFindRequest(std::string("empty request"));
	}
	// все еще 1439 запросов с нулевым результатом
	request_queue.AddFindRequest(std::string("curly dog"));
	// новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
	request_queue.AddFindRequest(std::string("big collar"));
	// первый запрос удален, 1437 запросов с нулевым результатом
	request_queue.AddFindRequest(std::string("sparrow"));
	std::cout << "Total empty requests: " << request_queue.GetNoResultRequests() << std::endl;
	return 0;
}