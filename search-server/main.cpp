#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <string>
#include <vector>

using namespace std;

//----------------------------Constants----------------------------
const int MAX_RESULT_DOCUMENT_COUNT = 5;
//--------------------------End Constants--------------------------

//----------------------------Parsers----------------------------
string ReadLine()
{
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber()
{
    int result;
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
        {
            word += c;
        }
    }
    if (!word.empty())
    {
        words.push_back(word);
    }

    return words;
}
//--------------------------End Parsers--------------------------

//----------------------------Structures----------------------------
struct Document
{
    int id;
    double relevance;
    int rating;
};
//--------------------------End Structures--------------------------

//----------------------------Enums----------------------------
enum class DocumentStatus
{
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};
//--------------------------End Enums--------------------------

class SearchServer
{
public:
    // ==================== BEGIN PUBLIC =========================
    //Добавляем стоп слова в множество stop_words_
    void SetStopWords(const string& text)
    {
        for (const string& word : SplitIntoWords(text))
        {
            stop_words_.insert(word);
        }
    }

    //Добавляем документ в словарь documents_
    //Рассчитываем TF для каждого слова в документе 
    void AddDocument(int document_id, const string& document, DocumentStatus status,
        const vector<int>& ratings)
    {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / static_cast<double>(words.size());
        for (const string& word : words)
        {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }

        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    }

    //Найти 5 документов с статусом ACTUAL
    vector<Document> FindTopDocuments(const string& raw_query)	const
    {
        //теперь return можно сократить
        return FindTopDocuments(raw_query,
            [](const int document_id, const DocumentStatus status, int rating)
            {
                return status == DocumentStatus::ACTUAL;
            });
    }

    //Найти 5 документов с заданным статусом
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus set_status)	const
    {
        //теперь return можно сократить
        return FindTopDocuments(raw_query,
            [set_status](const int document_id, DocumentStatus status, int rating)
            {
                return status == set_status;
            });
    }

    //Найти 5 документов с фильтром через Предикат
    template <typename  FindPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, FindPredicate predicate)	const
    {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, predicate);

        sort(matched_documents.begin(), matched_documents.end(),
            [this](const Document& lhs, const Document& rhs)
            {
                if (abs(lhs.relevance - rhs.relevance) < MAX_EPSILON_)
                {
                    return lhs.rating > rhs.rating;
                }
                return lhs.relevance > rhs.relevance;
            });

        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
        {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    }

    //----------------------------Additional Functions----------------------------
    //Метод должен быть публичным из-за тренажера
    int GetDocumentCount() const
    {
        return static_cast<int>(documents_.size());
    }

    //Метод должен быть публичным из-за тренажера
    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
        const int document_id) const
    {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words)
        {
            if (!word_to_document_freqs_.count(word))
            {
                continue;
            }

            if (word_to_document_freqs_.at(word).count(document_id))
            {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words)
        {
            if (!word_to_document_freqs_.count(word))
            {
                continue;
            }

            if (word_to_document_freqs_.at(word).count(document_id))
            {
                matched_words.clear();
                break;
            }
        }
        return { matched_words, documents_.at(document_id).status };
    }
    //--------------------------End Additional Functions--------------------------
    // ==================== END PUBLIC =========================

private:
    // ==================== BEGIN PRIVATE =========================

    //----------------------------Constants----------------------------
    const double MAX_EPSILON_ = 1e-6;
    //--------------------------End Constants--------------------------

    //----------------------------Structures----------------------------
    struct DocumentData
    {
        int rating;
        DocumentStatus status;
    };

    struct Query
    {
        set<string> plus_words;
        set<string> minus_words;
    };

    struct QueryWord
    {
        string data;
        bool is_minus;
        bool is_stop;
    };
    //--------------------------End Structures--------------------------

    //----------------------------Private Variables----------------------------
    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    //--------------------------End Private Variables--------------------------

    //----------------------------Parsers----------------------------
    vector<string> SplitIntoWordsNoStop(const string& text) const
    {
        vector<string> words;
        for (const string& word : SplitIntoWords(text))
        {
            if (!IsStopWord(word))
            {
                words.push_back(word);
            }
        }
        return words;
    }

    QueryWord ParseQueryWord(string text) const
    {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, IsStopWord(text) };
    }

    Query ParseQuery(const string& text) const
    {
        Query query;
        for (const string& word : SplitIntoWords(text))
        {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop)
            {
                if (query_word.is_minus)
                {
                    query.minus_words.insert(query_word.data);
                }
                else
                {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }
    //--------------------------End Parsers--------------------------

    //----------------------------Additional Functions----------------------------
    bool IsStopWord(const string& word) const
    {
        return stop_words_.count(word) > 0;
    }

    static int ComputeAverageRating(const vector<int>& ratings)
    {
        if (ratings.empty())
        {
            return 0;
        }
        const int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
        return rating_sum / static_cast<int>(ratings.size());
    }

    double ComputeWordInverseDocumentFreq(const string& word) const
    {
        return log(GetDocumentCount() * 1.0 / static_cast<double>(word_to_document_freqs_.at(word).size()));
    }

    //--------------------------End Additional Functions--------------------------

    template <typename  FindPredicate>
    vector<Document> FindAllDocuments(const Query& query, FindPredicate predicate) const
    {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }

            //считаем IDF
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word))
            {
                if (const auto& [rating, status] = documents_.at(document_id); predicate(document_id, status, rating))
                {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }

            }
        }

        for (const string& word : query.minus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }

            for (const auto& [document_id, _] : word_to_document_freqs_.at(word))
            {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        //Резервируем память
        matched_documents.reserve(document_to_relevance.size());
        for (const auto& [document_id, relevance] : document_to_relevance)
        {
            matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }

    // ==================== END PRIVATE =========================
};

// ==================== DEBUG =========================

void PrintDocument(const Document& document)
{
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating
        << " }"s << endl;
}

int main()
{
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });

    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s))
    {
        PrintDocument(document);
    }

    cout << "BANNED:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s,
        DocumentStatus::BANNED))
    {
        PrintDocument(document);
    }

    cout << "Even ids:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s,
        [](const int document_id, DocumentStatus status, int rating)
        { return document_id % 2 == 0; }))
    {
        PrintDocument(document);
    }

    return 0;
}