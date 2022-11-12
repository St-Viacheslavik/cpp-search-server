#pragma once
const int MAX_RESULT_DOCUMENT_COUNT = 5;

struct Document
{
	Document();
	Document(const int id, const double relevance, const int rating);

	int id = 0;
	double relevance = 0.0;
	int rating = 0;
};

enum class DocumentStatus
{
	ACTUAL,
	IRRELEVANT,
	BANNED,
	REMOVED,
};