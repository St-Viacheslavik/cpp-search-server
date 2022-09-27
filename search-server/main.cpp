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

/* Подставьте вашу реализацию класса SearchServer сюда */
const double MAX_EPSILON = 1e-6;
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
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty())
        {
            cout << " Hint: "s << hint;
        }
        cout << endl;
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

//Тест добавления документов в систему
void TestAddingDocument()
{
    const vector<int> ratings = { 1, 2, 3 };
    SearchServer server;
    ASSERT(server.GetDocumentCount() == 0);
    for (int cnt = 1; cnt < 5; ++cnt)
    {
        string doc_name = "Test doc " + to_string(cnt);
        server.AddDocument(cnt, doc_name, DocumentStatus::ACTUAL, ratings);
    }
    ASSERT_EQUAL_HINT(server.GetDocumentCount(), 4, "Должно быть 4 документа");
    const auto found_docs = server.FindTopDocuments("Test"s);
    for (int cnt = 0; cnt < server.GetDocumentCount(); ++cnt)
    {
        ASSERT_EQUAL_HINT(found_docs[cnt].id, cnt + 1, "ID должны совпадать");
    }
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
    ASSERT_EQUAL(server.FindTopDocuments("Test").size(), 5);
}

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
    //Тест сортировки по ID в предикате и убыванию релевантности
    {
        SearchServer search_server;
        search_server.SetStopWords("и в на"s);

        search_server.AddDocument(0, "пушистый белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
        search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
        search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
        search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
        const auto found = search_server.FindTopDocuments("пушистый ухоженный кот"s,
            [](const int document_id, DocumentStatus status, int rating)
            { return document_id % 2 == 0; });
        //проверка сортировки по id
        ASSERT(found[0].id == 0 && found[1].id == 2);
        //проверка сортировки по релевантности
        ASSERT_HINT(found[0].relevance > found[1].relevance,
            "Сортировка должны быть по убыванию релевантности");
    }
    //Тест сортировки по всем статусам в документе
    {
        SearchServer search_server;
        search_server.SetStopWords("и в на"s);
        //Добавим:
        /*
         *ACTUAL - 2 документа
         *IRRELEVANT - 1 документ
         *BANNED - 0 документов
         *REMOVED - 3 документа
         */
        search_server.AddDocument(0, "Актуальный кот"s, DocumentStatus::ACTUAL, { 8, -3 });
        search_server.AddDocument(1, "Второй актуальный кот"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
        search_server.AddDocument(2, "Неуместный кот"s, DocumentStatus::IRRELEVANT, { 5, -12, 2, 1 });
        search_server.AddDocument(3, "Удаленный кот"s, DocumentStatus::REMOVED, { 9 });
        search_server.AddDocument(4, "Второй удаленный кот"s, DocumentStatus::REMOVED, { -5, 4, 1, 2 });
        search_server.AddDocument(5, "Третий удаленный кот"s, DocumentStatus::REMOVED, { 15, 16, -6, 10 });
        //ищем
        auto found = search_server.FindTopDocuments("кот"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL_HINT(found.size(), 2, "Было добавлено два актуальных кота");
        found = search_server.FindTopDocuments("кот"s, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL_HINT(found.size(), 1, "Был добавлен один неуместный кот");
        found = search_server.FindTopDocuments("кот"s, DocumentStatus::BANNED);
        ASSERT_HINT(found.empty(), "Забаненных котов не было");
        found = search_server.FindTopDocuments("кот"s, DocumentStatus::REMOVED);
        ASSERT_EQUAL_HINT(found.size(), 3, "Было добавлено три удаленных кота");
    }
    //Тест сортировки по полному предикату
    {
        SearchServer search_server;
        search_server.SetStopWords("и в на"s);

        search_server.AddDocument(0, "пушистый белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
        search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
        search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
        search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
        const auto found = search_server.FindTopDocuments("пушистый ухоженный кот"s,
            [](const int document_id, DocumentStatus status, int rating)
            { return document_id % 2 == 1 && status == DocumentStatus::ACTUAL && rating > 4; });
        ASSERT_EQUAL_HINT(found.size(), 1, "По предикату должен был найтись один документ");
    }


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
    ASSERT_HINT(abs(found_docs[0].relevance - relevance) < MAX_EPSILON, "Модуль разности релевантностей не должен быть больше погрешности");


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