#pragma once
#include <algorithm>
#include <map>
#include <ostream>
#include <stdexcept>

#include "document.h"
#include "string_processing.h"

class SearchServer
{
public:
	template <typename StringContainer>
	explicit SearchServer(const StringContainer& stop_words);

	explicit SearchServer(const std::string& stop_words_text);

	void AddDocument(int document_id, const std::string& document,
		const DocumentStatus status, const std::vector<int>& ratings);

	template <typename DocumentPredicate>
	[[nodiscard]] std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const;
	[[nodiscard]] std::vector<Document> FindTopDocuments(const std::string& raw_query, const DocumentStatus status) const;
	[[nodiscard]] std::vector<Document> FindTopDocuments(const std::string& raw_query) const;

	[[nodiscard]] int GetDocumentCount() const;

	[[nodiscard]] std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, const int document_id) const;

	[[nodiscard]] std::set<int>::const_iterator begin() const;

	[[nodiscard]] std::set<int>::const_iterator end() const;

	[[nodiscard]] const std::map<std::string, double>& GetWordFrequencies(int document_id) const;

	void RemoveDocument(int document_id);

private:
	const double EPSILON = 1e-6;

	struct DocumentData
	{
		int rating;
		DocumentStatus status;
		std::map<std::string, double> document_to_word_freqs;
	};

	struct QueryWord
	{
		std::string data;
		bool is_minus;
		bool is_stop;
	};

	struct Query
	{
		std::set<std::string> plus_words;
		std::set<std::string> minus_words;
	};

	const std::set<std::string> stop_words_;
	std::map<std::string, std::map<int, double>> word_to_document_freqs_;
	std::map<int, DocumentData> documents_;
	std::set<int> document_ids_;

	[[nodiscard]] bool IsStopWord(const std::string& word) const;

	[[nodiscard]] std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

	[[nodiscard]] static bool IsValidWord(const std::string& word);

	[[nodiscard]] static int ComputeAverageRating(const std::vector<int>& ratings);

	[[nodiscard]] QueryWord ParseQueryWord(std::string text) const;

	[[nodiscard]] Query ParseQuery(const std::string& text) const;

	// Existence required
	[[nodiscard]] double ComputeWordInverseDocumentFreq(const std::string& word) const;

	template <typename DocumentPredicate>
	[[nodiscard]]
	std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;
};

inline std::ostream& operator<<(std::ostream& out, const Document& document)
{
	out << "{ document_id = " << document.id << ", relevance = " << document.relevance << ", rating = " << document.rating << " }";
	return out;
}

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
	:stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
	if (!std::all_of(stop_words_.begin(), stop_words_.end(), [](const std::string& word) {return IsValidWord(word); }))
	{
		throw std::invalid_argument("Error while parsing stop words");
	}
}

template <typename DocumentPredicate>
[[nodiscard]] std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const
{
	const Query query = ParseQuery(raw_query);

	std::vector<Document> topDocuments = FindAllDocuments(query, document_predicate);
	std::sort(topDocuments.begin(), topDocuments.end(),
		[this](const Document& lhs, const Document& rhs)
		{
			if (std::abs(lhs.relevance - rhs.relevance) < EPSILON)
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

template <typename DocumentPredicate>
[[nodiscard]]
std::vector<Document> SearchServer::FindAllDocuments(const Query& query,
	DocumentPredicate document_predicate) const
{
	std::map<int, double> document_to_relevance;
	for (const auto& word : query.plus_words)
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

	for (const auto& word : query.minus_words)
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

	std::vector<Document> matched_documents;
	for (const auto [document_id, relevance] : document_to_relevance)
	{
		matched_documents.push_back
		(
			{ document_id, relevance, documents_.at(document_id).rating }
		);
	}
	return matched_documents;
}