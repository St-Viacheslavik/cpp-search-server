#include "search_server.h"

using namespace std;

SearchServer::SearchServer()
= default;

SearchServer::SearchServer(const string &stop_words_text)
	: SearchServer(SplitIntoWords(stop_words_text))
{
}

SearchServer::SearchServer(const string_view stop_words_text)
	: SearchServer(SplitIntoWords(stop_words_text))
{
}

std::string_view SearchServer::AddUniqueWord(const std::string &word)
{
	return std::string_view(*(unique_words.insert(word).first));
}

void SearchServer::AddDocument(int document_id, const string_view document, DocumentStatus status, const vector<int> &ratings)
{
	if ((document_id < 0) || (documents_.count(document_id) > 0))
	{
		throw invalid_argument("Invalid document_id"s);
	}

	const auto words = SplitIntoWordsNoStop(document);

	//unordered_set<std::string_view> document_words;
	set<std::string_view> document_words;

	const double inv_word_count = 1.0 / static_cast<double>(words.size());

	for (const auto &word : words)
	{
		const string_view current_word = AddUniqueWord(string(word));

		document_to_word_freqs_[document_id][word] += inv_word_count;
		word_to_document_freqs_[current_word][document_id] += inv_word_count;

		document_words.insert(current_word);
	}

	documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status, document_words});
	document_ids_.emplace(document_id);
}

vector<Document> SearchServer::FindTopDocuments(const string_view raw_query, DocumentStatus status) const
{
	return FindTopDocuments(std::execution::seq, raw_query, [status](int document_id, DocumentStatus document_status, int rating)
	                        { return document_status == status; });
}

vector<Document> SearchServer::FindTopDocuments(std::execution::sequenced_policy policy, const string_view raw_query, DocumentStatus status) const
{
	return FindTopDocuments(std::execution::seq, raw_query, [status](int document_id, DocumentStatus document_status, int rating)
	                        { return document_status == status; });
}

vector<Document> SearchServer::FindTopDocuments(std::execution::parallel_policy policy, const string_view raw_query, DocumentStatus status) const
{
	return FindTopDocuments(std::execution::par, raw_query, [status](int document_id, DocumentStatus document_status, int rating)
	                        { return document_status == status; });
}

vector<Document> SearchServer::FindTopDocuments(const string_view raw_query) const
{
	return FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}

vector<Document> SearchServer::FindTopDocuments(std::execution::sequenced_policy policy, const string_view raw_query) const
{
	return FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}

vector<Document> SearchServer::FindTopDocuments(std::execution::parallel_policy policy, const string_view raw_query) const
{
	return FindTopDocuments(std::execution::par, raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const
{
	return static_cast<int>(documents_.size());
}

std::set<int>::const_iterator SearchServer::begin() const
{
	return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::end() const
{
	return document_ids_.end();
}

const std::map<std::string_view, double> &SearchServer::GetWordFrequencies(int document_id) const
{
	static const std::map<std::string_view, double> EMPTY_MAP;
	if (documents_.count(document_id) > 0)
	{
		return document_to_word_freqs_.at(document_id);
	}
	return EMPTY_MAP;
}

void SearchServer::RemoveDocument(execution::parallel_policy policy, int document_id)
{
	{
		if (documents_.count(document_id) == 0)
		{
			return;
		}

		map<string_view, double> word_freq = document_to_word_freqs_[document_id];
		vector<string_view> varStr(word_freq.size());

		transform(
			execution::par,
			word_freq.begin(), word_freq.end(),
			varStr.begin(),
			[&](auto word)
			{ return word.first; });

		for_each(
			execution::par,
			varStr.begin(), varStr.end(),
			[&document_id, this](string_view word)
			{
				word_to_document_freqs_.find(word)->second.erase(document_id);
			});

		documents_.erase(document_id);
		document_ids_.erase(document_id);
	}
}

void SearchServer::RemoveDocument(int document_id)
{
	RemoveDocument(execution::seq, document_id);
}

void SearchServer::RemoveDocument(execution::sequenced_policy policy, int document_id)
{
	if (documents_.count(document_id) == 0)
	{
		return;
	}

	for_each(
		document_to_word_freqs_.at(document_id).begin(),
		document_to_word_freqs_.at(document_id).end(),
		[this, document_id](const auto &word)
		{
			word_to_document_freqs_[word.first].erase(document_id);
		});

	documents_.erase(document_id);
	document_ids_.erase(document_id);
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const string_view raw_query, int document_id) const
{
	return MatchDocument(execution::seq, raw_query, document_id);
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(execution::parallel_policy policy, const string_view raw_query, int document_id) const
{
	const auto query = ParseQuery(raw_query);

	std::vector<std::string> plus_words{query.plus_words.begin(), query.plus_words.end()};
	std::vector<std::string> minus_words{query.minus_words.begin(), query.minus_words.end()};

	vector<string_view> matched_words;

	for (const string_view word : documents_.at(document_id).words)
	{
		if (find(minus_words.begin(), minus_words.end(), word) != minus_words.end())
		{
			return {matched_words, documents_.at(document_id).status};
		}
	}

	for (const string_view word : documents_.at(document_id).words)
	{
		if (find(plus_words.begin(), plus_words.end(), word) != plus_words.end())
		{
			matched_words.push_back(word);
		}
	}

	return {matched_words, documents_.at(document_id).status};
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(execution::sequenced_policy policy, const string_view raw_query, int document_id) const
{
	auto query = ParseQuery(raw_query);

	set<string_view> matched_words;

	/*std::sort(query.plus_words.begin(), query.plus_words.end()); //�������� ��������, ��������� ����� �������� ������
	auto last_plus = std::unique(query.plus_words.begin(), query.plus_words.end());
	query.plus_words.erase(last_plus, query.plus_words.end());*/

	for (const string_view word : query.plus_words)
	{
		if (documents_.at(document_id).words.count(word) == 0)
		{
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id))
		{
			matched_words.insert(word);
		}
	}
	for (const string_view word : query.minus_words)
	{
		if (documents_.at(document_id).words.count(word) == 0)
		{
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id))
		{
			matched_words.clear();
			break;
		}
	}

	vector<string_view> matched_words_vector(matched_words.size());

	transform(
		matched_words.begin(), matched_words.end(),
		matched_words_vector.begin(),
		[](auto word)
		{ return word; });

	return {matched_words_vector, documents_.at(document_id).status};
}

bool SearchServer::IsStopWord(const string &word) const
{
	return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const string &word)
{
	return none_of(word.begin(), word.end(), [](char c)
				   { return c >= '\0' && c < ' '; });
}

deque<string_view> SearchServer::SplitIntoWordsNoStop(const string_view text) const
{
	deque<string_view> words;

	auto vec = SplitIntoWords(text);

	for (const string_view word : vec)
	{
		if (!IsValidWord(string(word)))
		{
			throw invalid_argument("Word "s + string(word) + " is invalid"s);
		}
		if (!IsStopWord(string(word)))
		{
			words.push_back(word);
		}
	}
	return words;
}

int SearchServer::ComputeAverageRating(const vector<int> &ratings)
{
	if (ratings.empty())
	{
		return 0;
	}
	const int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
	return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const string_view text) const
{
	if (text.empty())
	{
		throw invalid_argument("Query word is empty"s);
	}

	auto word = text;

	bool is_minus = false;

	if (word[0] == '-')
	{
		is_minus = true;
		word = word.substr(1);
	}

	if (word.empty() || word[0] == '-' || !IsValidWord(string(word)))
	{
		throw invalid_argument("Query word "s + string(word) + " is invalid");
	}

	return {word, is_minus, IsStopWord(string(word))};
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text) const
{
	SearchServer::Query result;
	for (const auto word : SplitIntoWords(text))
	{
		const auto query_word = ParseQueryWord(word);
		if (!query_word.is_stop)
		{
			if (query_word.is_minus)
			{
				result.minus_words.push_back(query_word.data);
			}
			else
			{
				result.plus_words.push_back(query_word.data);
			}
		}
	}
	return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const string_view word) const
{
	return log(GetDocumentCount() * 1.0 / static_cast<double>(word_to_document_freqs_.at(word).size()));
}