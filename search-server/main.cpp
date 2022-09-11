#include <algorithm>
#include <iostream>
#include <map>
#include <cmath>
#include <set>
#include <string>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine()
{
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber()
{
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text)
{
    vector<string> words;
    string word;
    for (const char c : text)
    {
        if (c == ' ')
        {
            if (!word.empty())
            {
                words.push_back(word);
                word.clear();
            }
        }
        else
            word += c;
    }
    if (!word.empty())
        words.push_back(word);

    return words;
}

struct Document
{
    int id;
    double relevance;
};

class SearchServer
{
public:
    void SetStopWords(const string& text)
    {
        for (const string& word : SplitIntoWords(text))
            stop_words_.insert(word);
    }

    void AddDocument(const int& document_id, const string& document)
    {
        const vector<string> words = SplitIntoWordsNoStop(document);

        ++document_count_;
        //вычисляем TF для слова
        const double word_tf = 1 / static_cast<double>(words.size());

        for (const string& word : words)
            //прибавляем TF к словарю. Если слово есть, то тф увеличивается.
            word_to_document_freqs_[word][document_id] += word_tf;
    }

    vector<Document> FindTopDocuments(const string& raw_query) const
    {
        const QueryWords query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs)
            {
                return lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);

        return matched_documents;
    }

private:

    struct QueryWords
    {
        set<string> plus_word;
        set<string> minus_word;
    };

    /**
     * string - слова в документах
     * int - id doc
     * double - TF world
     */
    map<string, map<int, double>> word_to_document_freqs_;

    set<string> stop_words_;

    int document_count_ = 0;

    bool IsStopWord(const string& word) const
    {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const
    {
        vector<string> words;
        for (const string& word : SplitIntoWords(text))
        {
            if (!IsStopWord(word))
                words.push_back(word);
        }
        return words;
    }

    QueryWords ParseQuery(const string& text) const
    {
        QueryWords query_words;
        for (const string& word : SplitIntoWordsNoStop(text))
        {
            if (word[0] == '-')
                query_words.minus_word.insert(word.substr(1));
            else
                query_words.plus_word.insert(word);
        }
        return query_words;
    }

    double Calculate_IDF(const string& word) const
    {
        return log(document_count_ / static_cast<double>(word_to_document_freqs_.at(word).size()));
    }

    vector<Document> FindAllDocuments(const QueryWords& query_words) const
    {
        //словарь id документа к частоте встречания
        map<int, double> id_relevance;

        for (const auto& word : query_words.plus_word)
        {

            if (!word_to_document_freqs_.count(word))
                continue;

            for (const auto& [doc_id, doc_tf] : word_to_document_freqs_.at(word))
                id_relevance[doc_id] += Calculate_IDF(word) * doc_tf;
        }

        for (const auto& word : query_words.minus_word)
        {
            if (!word_to_document_freqs_.count(word))
                continue;

            for (const auto& [doc_id, _] : word_to_document_freqs_.at(word))
                id_relevance.erase(doc_id);
        }

        vector<Document> matched_documents;

        matched_documents.reserve(id_relevance.size());
        for (const auto& [id, relevance] : id_relevance)
            matched_documents.push_back({ id, relevance });

        return matched_documents;
    }
};

SearchServer CreateSearchServer()
{
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    int doc_cnt = ReadLineWithNumber();
    for (int document_id = 0; document_id < doc_cnt; ++document_id)
        search_server.AddDocument(document_id, ReadLine());

    return search_server;
}

int main()
{
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query))
        cout << "{ document_id = "s << document_id << ", " << "relevance = "s << relevance << " }"s << endl;
}