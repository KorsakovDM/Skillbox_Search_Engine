#pragma once
#include "InvertedIndex.h"
#include <string>
#include <vector>

struct RelativeIndex {
    size_t doc_id{};
    float rank{};     // относительная релевантность (0.0–1.0)

    bool operator==(const RelativeIndex& other) const {
        return doc_id == other.doc_id && rank == other.rank;
    }
};

class SearchServer {
public:
    explicit SearchServer(InvertedIndex& idx, int responses_limit = 5)
        : index_(idx), responses_limit_(responses_limit) {}

    void setResponsesLimit(int limit) { responses_limit_ = (limit > 0 ? limit : 5); }

    std::vector<std::vector<RelativeIndex>>
    search(const std::vector<std::string>& queries_input);

private:
    InvertedIndex& index_; // ссылка на индекс
    int responses_limit_ = 5;
};