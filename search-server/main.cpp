#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <numeric>
#include <utility>
#include <vector>

#include "search_server.h"

using namespace std;

/*
   Подставьте сюда вашу реализацию макросов
   ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST
*/
template <typename T>
ostream& operator <<(ostream& out, vector<T> value)
{
    out << "[";
    bool is_first = true;
    for (const auto& element : value)
    {
        if (!is_first)
        {
            out << ", ";
        }
        is_first = false;
        out << element;
    }
    out << "]";
    return out;
}

template <typename T>
ostream& operator <<(ostream& out, set<T> value)
{
    out << "{";
    bool is_first = true;
    for (const auto& element : value)
    {
        if (!is_first)
        {
            out << ", ";
        }
        is_first = false;
        out << element;
    }
    out << "}";
    return out;
}

template <typename K, typename T>
ostream& operator <<(ostream& out, map<K, T> value)
{
    out << "{";
    bool is_first = true;
    for (const auto& element : value)
    {
        if (!is_first)
        {
            out << ", ";
        }
        is_first = false;
        out << element.first << ": " << element.second;
    }
    out << "}";
    return out;
}

//проверка двух чисел
template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint)
{
    if (t != u)
    {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty())
        {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

//проверка одного числа
void AssertEqualImpl(bool value, const string& val_str, const string& file,
    const string& func, unsigned line, const string& hint = ""s)
{
    if (!value)
    {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << val_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

template <typename T>
void RunTestImpl(const T& element, const string& func)
{
    /* Напишите недостающий код */
    auto result = element;
    if (result)
    {
        cerr << func + " OK" << endl;
    }

}

// -------- Начало модульных тестов поисковой системы ----------

//Тест добавления документа в систему
void TestAddingDocument()
{
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    SearchServer server;
    ASSERT(server.GetDocumentCount() == 0);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    ASSERT(server.GetDocumentCount() == 1);
}

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent()
{
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT(found_docs.size() == 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("in"s).empty());
    }
}

//Тест добавления минус слов
void TestExcludeMinusWords()
{
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    // Сначала убеждаемся, что поиск слова, не входящего в список минус-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat"s);
        ASSERT(found_docs.size() == 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список минус-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        const auto new_content = "-cat in the city"s;
        server.AddDocument(doc_id, new_content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("cat"s).empty());
    }
}

void TestFindTopDocuments()
{
    SearchServer server;
    const vector ratings = { 1, 2, 3 };
    //проверяем что сейчас ничего не найдет в топе документов
    ASSERT(server.FindTopDocuments("Test"s).empty());
    for (int cnt = 1; cnt < 7; ++cnt)
    {
        string doc_name = "Test doc " + to_string(cnt);
        server.AddDocument(cnt, doc_name, DocumentStatus::ACTUAL, ratings);
    }
    //Убеждаемся что функция возвращает только 5 документов
    ASSERT_EQUAL(server.FindTopDocuments("Test").size(), 5);
}

//Проверка функции матчинга
void TestMatchingDocs()
{
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    //проверка нормального условия
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto [words, status] = server.MatchDocument("cat", doc_id);
        ASSERT_HINT(words[0] == "cat"s && status == DocumentStatus::ACTUAL,
            "Слово должно быть cat и должен быть только актуальный документ"s);
    }
    //проверка пустого запроса
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto [words, status] = server.MatchDocument("", doc_id);
        ASSERT(words.empty());
    }
    //проверка минус слова
    {
        SearchServer server;
        const string minus_content = "-cat in the city"s;
        server.AddDocument(doc_id, minus_content, DocumentStatus::ACTUAL, ratings);
        const auto [words, status] = server.MatchDocument("cat", doc_id);
        ASSERT(words.empty());
    }
}

void TestSortingDocs()
{
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);

    search_server.AddDocument(0, "пушистый белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });

    auto found = search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
    //проверка сортировки по статусу
    ASSERT(found.size() == 1);
    found = search_server.FindTopDocuments("пушистый ухоженный кот"s,
        [](const int document_id, DocumentStatus status, int rating)
        { return document_id % 2 == 0; });
    //проверка сортировки по id
    ASSERT(found[0].id == 0 && found[1].id == 2);
    //проверка сортировки по релевантности
    ASSERT_HINT(found[0].relevance > found[1].relevance,
        "Сортировка должны быть по убыванию релевантности");
}

void TestMathFunctions()
{
    SearchServer search_server;
    int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    search_server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    vector<Document> found_docs = search_server.FindTopDocuments("cat"s);
    ASSERT(found_docs.size() == 1);
    const Document& doc0 = found_docs[0];
    double rating = accumulate(ratings.begin(), ratings.end(), 0) / static_cast<double>(ratings.size());
    //проверка рейтинга
    ASSERT_EQUAL_HINT(doc0.rating, rating, "Рейтинг должен быть одинаковый");

    doc_id = 43;
    const string add_content = "dog in the country"s;
    search_server.AddDocument(doc_id, add_content, DocumentStatus::ACTUAL, ratings);
    double tf = 1 / static_cast<double>(4);
    double idf = log(2 * 1.0 / static_cast<double>(1));
    double relevance = tf * idf;
    found_docs = search_server.FindTopDocuments("cat"s);
    ASSERT_EQUAL_HINT(found_docs[0].relevance, relevance, "Релевантность должны быть одинаковой");


}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer()
{
    RUN_TEST(TestAddingDocument);
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeMinusWords);
    RUN_TEST(TestFindTopDocuments);
    RUN_TEST(TestMatchingDocs);
    RUN_TEST(TestSortingDocs);
    RUN_TEST(TestMathFunctions);
}


// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}