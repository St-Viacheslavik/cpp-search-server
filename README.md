# SearchServer

Реализация поисковой системы.
Проект создан в учебных целях при поддержке команды Яндекс.Практикума (2022год).

# Функционал

Поисковая система имеет встроенный функционал парсинга входной строки с исключением стоп-слов, и подсчетом TF-IDF для каждого слова.
К каждому документу присваивается ID.
Реализован метод удаления документов по ID.
Сортировка документов по релевантности и актуальности, а также поиск и удаление дублирующих документов.
Управление осуществляется из командной строки.
Поиск документов может выполняться как в последовательном, так и в параллельных режимах 

# Сборка

Сборка осуществляется из командной строки. Инструкция универсальна как для ОС Linux и Windows
1. mkdir build 
2. cd build
3. cmake ..
4. cmake --build . --config Release

# Тестирование
Для проверки правильного функционирования поисковой системы можно использовать следующий код. 
*Изменение в main.cpp*
```C++
#include "process_queries.h"
#include "search_server.h"
#include <execution>
#include <iostream>
#include <string>
#include <vector>
using namespace std;
void PrintDocument(const Document& document) 
{
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s
         << endl;
}
int main()
{
    SearchServer search_server("and with"s);
    int id = 0;
    for
    (
        const string& text : 
        {
            "white cat and yellow hat"s,
            "curly cat curly tail"s,
            "nasty dog with big eyes"s,
            "nasty pigeon john"s,
        }
    ) 
    {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }
    cout << "ACTUAL by default:"s << endl;
    // последовательная версия
    for (const Document& document : search_server.FindTopDocuments("curly nasty cat"s)) 
    {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    // последовательная версия
    for (const Document& document : search_server.FindTopDocuments(execution::seq, "curly nasty cat"s, DocumentStatus::BANNED)) 
    {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    // параллельная версия
    for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) 
    {
        PrintDocument(document);
    }
    return 0;
}
```

Результат выполнения программы должен быть равен:
```json
ACTUAL by default:
{ document_id = 2, relevance = 0.866434, rating = 1 }
{ document_id = 4, relevance = 0.231049, rating = 1 }
{ document_id = 1, relevance = 0.173287, rating = 1 }
{ document_id = 3, relevance = 0.173287, rating = 1 }
BANNED:
Even ids:
{ document_id = 2, relevance = 0.866434, rating = 1 }
{ document_id = 4, relevance = 0.231049, rating = 1 } 
```

# Системные требования

+ CMake V3.22
+ С++17 (STL)
