#include "request_queue.h"

using namespace std;
RequestQueue::RequestQueue(const SearchServer &search_server) : search_server_(search_server)
{
}

vector<Document> RequestQueue::AddFindRequest(const string &raw_query, DocumentStatus status)
{
    const auto search_results = search_server_.FindTopDocuments(raw_query, status);
    AddRequest(search_results, raw_query);
    return search_results;
}

vector<Document> RequestQueue::AddFindRequest(const string &raw_query)
{
    const auto search_results = search_server_.FindTopDocuments(raw_query);
    AddRequest(search_results, raw_query);
    return search_results;
}

int RequestQueue::GetNoResultRequests() const
{
    return count_if(requests_.begin(), requests_.end(), [](QueryResult lhs)
                    { return lhs.result == false; });
}

void RequestQueue::AddRequest(const vector<Document>& search_results, const string &raw_query)
{
    if (requests_.size() >= min_in_day)
    {
        requests_.pop_front();
    }
    if (search_results.empty())
    {
        requests_.push_back({raw_query, false});
    }
    else
    {
        requests_.push_back({raw_query, true});
    }
}