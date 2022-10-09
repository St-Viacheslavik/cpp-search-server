#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <stdexcept>
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
	int result;
	cin >> result;
	ReadLine();
	return result;
}

struct Document
{
	Document() = default;

	Document(int id, double relevance, int rating)
		: id(id)
		, relevance(relevance)
		, rating(rating) {
	}

	int id = 0;
	double relevance = 0.0;
	int rating = 0;
};

[[nodiscard]] static vector<string> SplitIntoWords(const string& text)
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

enum class DocumentStatus
{
	ACTUAL,
	IRRELEVANT,
	BANNED,
	REMOVED,
};

class SearchServer
{
public:
	template <typename StringContainer>
	explicit SearchServer(const StringContainer& stop_words)
		:stop_words_(MakeUniqueNonEmptyStrings(stop_words))
	{
	}

	explicit SearchServer(const string& stop_words_text)
		:SearchServer(SplitIntoWords(stop_words_text))
	{
	}

	void AddDocument(int document_id, const string& document,
		const DocumentStatus status, const vector<int>& ratings)
	{
		if (document_id < 0 || documents_.count(document_id) > 0 || !IsValidWord(document))
		{
			throw invalid_argument("Error while adding document");
		}

		const vector<string> words = SplitIntoWordsNoStop(document);
		const double inv_word_count = 1.0 / words.size();
		for (const string& word : words)
		{
			word_to_document_freqs_[word][document_id] += inv_word_count;
		}
		documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
		document_ids_.push_back(document_id);
	}

	template <typename DocumentPredicate>
	[[nodiscard]]
	vector<Document> FindTopDocuments(const string& raw_query,
		DocumentPredicate document_predicate) const
	{
		if (!IsValidWord(raw_query))
		{
			throw invalid_argument("Invalid query to find documents");
		}
		const Query query = ParseQuery(raw_query);

		vector<Document> topDocuments = FindAllDocuments(query, document_predicate);
		sort(topDocuments.begin(), topDocuments.end(),
			[](const Document& lhs, const Document& rhs)
			{
				if (abs(lhs.relevance - rhs.relevance) < 1e-6)
				{
					return lhs.rating > rhs.rating;
				}
				return lhs.relevance > rhs.relevance;
			});
		if (topDocuments.size() > MAX_RESULT_DOCUMENT_COUNT)
		{
			topDocuments.resize(MAX_RESULT_DOCUMENT_COUNT);
		}
		return topDocuments;
	}

	[[nodiscard]] vector<Document> FindTopDocuments(const string& raw_query, const DocumentStatus status) const
	{
		return FindTopDocuments(
			raw_query, [status](int document_id, const DocumentStatus document_status, int rating)
			{
				return document_status == status;
			});
	}

	[[nodiscard]] vector<Document> FindTopDocuments(const string& raw_query) const
	{
		return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
	}

	[[nodiscard]] int GetDocumentCount() const
	{
		return static_cast<int>(documents_.size());
	}

	[[nodiscard]] tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, const int document_id) const
	{
		if (!IsValidWord(raw_query) || document_id < 0)
		{
			throw invalid_argument("Invalid query or doc_id for matching document");
		}
		const Query query = ParseQuery(raw_query);

		vector<string> matched_words;
		for (const string& word : query.plus_words)
		{
			if (word_to_document_freqs_.count(word) == 0)
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
			if (word_to_document_freqs_.count(word) == 0)
			{
				continue;
			}
			if (word_to_document_freqs_.at(word).count(document_id))
			{
				matched_words.clear();
				break;
			}
		}
		tuple<vector<string>, DocumentStatus> matched_docs = { matched_words, documents_.at(document_id).status };
		return matched_docs;
	}

	[[nodiscard]] int GetDocumentId(const int index) const
	{
		if (documents_.count(index) <= 0)
		{
			throw out_of_range("Error while get document ID by index");
		}
		return document_ids_[index];
	}

private:
	struct DocumentData
	{
		int rating;
		DocumentStatus status;
	};

	struct QueryWord
	{
		string data;
		bool is_minus;
		bool is_stop;
	};

	struct Query
	{
		set<string> plus_words;
		set<string> minus_words;
	};

	set<string> stop_words_;
	map<string, map<int, double>> word_to_document_freqs_;
	map<int, DocumentData> documents_;
	vector<int> document_ids_;

	[[nodiscard]] bool IsStopWord(const string& word) const
	{
		return stop_words_.count(word) > 0;
	}

	[[nodiscard]] vector<string> SplitIntoWordsNoStop(const string& text) const
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

	[[nodiscard]] static bool IsValidWord(const string& word)
	{
		// A valid word must not contain special characters
		return none_of(word.begin(), word.end(), [](char c)
			{
				// "\37" - octal = 31 decimal
				return c >= '\0' && c <= '\37';
			});
	}

	template <typename StringContainer>
	[[nodiscard]] static set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings)
	{
		set<string> non_empty_strings;
		for (const string& str : strings)
		{
			if (!IsValidWord(str))
			{
				throw invalid_argument("Error while parsing stop words");
			}
			if (!str.empty())
			{
				non_empty_strings.insert(str);
			}
		}
		return non_empty_strings;
	}

	[[nodiscard]] static int ComputeAverageRating(const vector<int>& ratings)
	{
		if (ratings.empty())
		{
			return 0;
		}
		int rating_sum = 0;
		for (const int rating : ratings)
		{
			rating_sum += rating;
		}
		return rating_sum / static_cast<int>(ratings.size());
	}

	[[nodiscard]] QueryWord ParseQueryWord(string text) const
	{
		bool is_minus = false;
		// Word shouldn't be empty
		if (text[0] == '-')
		{
			is_minus = true;
			text = text.substr(1);
		}
		if (text.empty() || text[0] == '-' || !IsValidWord(text))
		{
			throw invalid_argument("Error while parsing query");
		}
		return { text, is_minus, IsStopWord(text) };
	}

	[[nodiscard]] Query ParseQuery(const string& text) const
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

	// Existence required
	[[nodiscard]] double ComputeWordInverseDocumentFreq(const string& word) const
	{
		return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
	}

	template <typename DocumentPredicate>
	[[nodiscard]]
	vector<Document> FindAllDocuments(const Query& query,
		DocumentPredicate document_predicate) const
	{
		map<int, double> document_to_relevance;
		for (const string& word : query.plus_words)
		{
			if (word_to_document_freqs_.count(word) == 0)
			{
				continue;
			}
			const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
			for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word))
			{
				const auto& document_data = documents_.at(document_id);
				if (document_predicate(document_id, document_data.status, document_data.rating))
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
			for (const auto [document_id, _] : word_to_document_freqs_.at(word))
			{
				document_to_relevance.erase(document_id);
			}
		}

		vector<Document> matched_documents;
		for (const auto [document_id, relevance] : document_to_relevance)
		{
			matched_documents.push_back
			(
				{ document_id, relevance, documents_.at(document_id).rating }
			);
		}
		return matched_documents;
	}
};

// ==================== для примера =========================

void PrintDocument(const Document& document)
{
	cout << "{ "s
		<< "document_id = "s << document.id << ", "s
		<< "relevance = "s << document.relevance << ", "s
		<< "rating = "s << document.rating << " }"s << endl;
}

int main()
{
	try
	{
		SearchServer search_server("и \10 на"s);
	}
	catch (const invalid_argument& ex)
	{
		cout << ex.what();
	}

	SearchServer search_server("и в на"s);
	try
	{
		search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
		search_server.AddDocument(1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
		search_server.AddDocument(-1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
		search_server.AddDocument(3, "большой пёс скво\x12рец"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
	}
	catch (const invalid_argument& ex)
	{
		cout << ex.what();
	}

	try
	{
		for (const Document& document : search_server.FindTopDocuments("--пушистый"s))
		{
			PrintDocument(document);
		}
	}
	catch (const invalid_argument& ex)
	{
		cout << ex.what();
	}
}
