#include <iostream>
#include <map>
#include <set>

#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server)
{
	using namespace std::literals;
	std::map<std::vector<std::string_view>, std::set<int>> words_to_document_ids;
	for (const int document_id : search_server) 
	{
		const auto& word_to_freq = search_server.GetWordFrequencies(document_id);
		std::vector<std::string_view> words;
		words.reserve(word_to_freq.size());
		for (const auto& [word, freq] : word_to_freq) 
		{
			words.push_back(word);
		}
		words_to_document_ids[words].insert(document_id);
	}
	for (auto& [words, document_ids] : words_to_document_ids) 
	{
		if (document_ids.size() > 1) {
			for (auto it = std::next(document_ids.begin()); it != document_ids.end(); ++it) 
			{
				search_server.RemoveDocument(*it);
				std::cout << "Found duplicate document id "s << *it << std::endl;
			}
		}
	}
}