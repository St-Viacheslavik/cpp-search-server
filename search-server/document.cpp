#include "document.h"

Document::Document() = default;

Document::Document(const int id, const double relevance, const int rating)
	: id(id)
	, relevance(relevance)
	, rating(rating) {
}