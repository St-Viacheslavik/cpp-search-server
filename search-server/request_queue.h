#pragma once
#include <deque>
#include "search_server.h"

class RequestQueue
{
public:
    explicit RequestQueue(const SearchServer &search_server);
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string &raw_query, DocumentPredicate document_predicate)
    {
        const auto search_results = search_server_.FindTopDocuments(raw_query, document_predicate);
        AddRequest(search_results, raw_query);
        return search_results;
    }
    std::vector<Document> AddFindRequest(const std::string &raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string &raw_query);
    [[nodiscard]] int GetNoResultRequests() const;

private:
    struct QueryResult
    {
        const std::string &raw_query;
        bool result;
    };
    std::deque<QueryResult> requests_;
    constexpr static int min_in_day = 1440;
    const SearchServer &search_server_;
    void AddRequest(const std::vector<Document>& search_results, const std::string &raw_query);
};