#include "process_queries.h"

using namespace std;

vector<vector<Document>> ProcessQueries(const SearchServer &search_server, const vector<string> &queries)
{
    vector<vector<Document>> result(queries.size());
    transform(
        execution::par,
        queries.begin(), queries.end(),
        result.begin(),
        [&search_server](const string &query)
        { return search_server.FindTopDocuments(query); });
    return result;
}

list<Document> ProcessQueriesJoined(const SearchServer &search_server, const vector<string> &queries)
{
	const vector<vector<Document>> mid_result = ProcessQueries(search_server, queries);
    int count_mid_result = 0;
    for (const auto& i : mid_result)
    {
        count_mid_result += static_cast<int>(i.size());
    }
    list<Document> result(count_mid_result);
    auto iter = result.begin();
    for (auto i : mid_result)
    {
        move(i.begin(), i.end(), iter);
        iter = next(iter, static_cast<long long>(i.size()));
    }
    return result;
}