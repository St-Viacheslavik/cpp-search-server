#include <cmath>
#include <numeric>

#include "document.h"
#include "string_processing.h"
#include "search_server.h"

//must be global variable
const std::map<std::string, double> empty_map = {};

SearchServer::SearchServer(const std::string& stop_words_text)
	:SearchServer(SplitIntoWords(stop_words_text))
{ }

void SearchServer::AddDocument(int document_id, const std::string& document,
	const DocumentStatus status, const std::vector<int>& ratings)
{
	if (document_id < 0 || documents_.count(document_id) > 0 || !IsValidWord(document))
	{
		throw std::invalid_argument("Error while adding document");
	}

	std::map<std::string, double> document_to_word_freqs;

	const std::vector<std::string> words = SplitIntoWordsNoStop(document);
	const double inv_word_count = 1.0 / static_cast<double>(words.size());
	for (const std::string& word : words)
	{
		word_to_document_freqs_[word][document_id] += inv_word_count;
		document_to_word_freqs[word] += inv_word_count;
	}
	documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status, document_to_word_freqs });
	document_ids_.insert(document_id);
}

[[nodiscard]] std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, const DocumentStatus status) const
{
	return FindTopDocuments(
		raw_query, [status](int document_id, const DocumentStatus document_status, int rating)
		{
			return document_status == status;
		});
}

[[nodiscard]] std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const
{
	return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

[[nodiscard]] int SearchServer::GetDocumentCount() const
{
	return static_cast<int>(documents_.size());
}

[[nodiscard]] std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, const int document_id) const
{
	if (document_id < 0)
	{
		throw std::invalid_argument("Invalid doc_id for matching document");
	}
	const Query query = ParseQuery(raw_query);

	std::vector<std::string> matched_words;
	for (const std::string& word : query.plus_words)
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
	for (const std::string& word : query.minus_words)
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

	return { matched_words, documents_.at(document_id).status };
}

[[nodiscard]] std::set<int>::const_iterator SearchServer::begin() const
{
	return document_ids_.begin();
}

[[nodiscard]] std::set<int>::const_iterator SearchServer::end() const
{
	return document_ids_.end();
}

[[nodiscard]] const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const
{
	
	if (documents_.count(document_id) == 0)
	{
		return empty_map;
	}
	return documents_.at(document_id).document_to_word_freqs;
}

void SearchServer::RemoveDocument(int document_id)
{
	const auto it = documents_.find(document_id);
	if (it == documents_.end()) return;

	for (const auto& [word, freq] : it->second.document_to_word_freqs)
	{
		word_to_document_freqs_.at(word).erase(document_id);
	}
	documents_.erase(document_id);
	document_ids_.erase(document_id);
}


[[nodiscard]] bool SearchServer::IsStopWord(const std::string& word) const
{
	return stop_words_.count(word) > 0;
}

[[nodiscard]] std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const
{
	std::vector<std::string> words;
	for (const std::string& word : SplitIntoWords(text))
	{
		if (!IsStopWord(word))
		{
			words.push_back(word);
		}
	}
	return words;
}

[[nodiscard]] bool SearchServer::IsValidWord(const std::string& word)
{
	// A valid word must not contain special characters
	return std::none_of(word.begin(), word.end(), [](char c)
		{
			// "\37" - octal = 31 decimal
			return c >= '\0' && c <= '\37';
		});
}

[[nodiscard]] int SearchServer::ComputeAverageRating(const std::vector<int>& ratings)
{
	if (ratings.empty())
	{
		return 0;
	}
	const int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);

	return rating_sum / static_cast<int>(ratings.size());
}

[[nodiscard]] SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const
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
		throw std::invalid_argument("Error while parsing query");
	}
	return { text, is_minus, IsStopWord(text) };
}

[[nodiscard]] SearchServer::Query SearchServer::ParseQuery(const std::string& text) const
{
	Query query;
	for (const std::string& word : SplitIntoWords(text))
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
[[nodiscard]] double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const
{
	return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}