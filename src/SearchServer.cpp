#include "SearchServer.h"
#include <sstream>
#include <unordered_map>
#include <algorithm>

// Разделение запроса на слова
static std::vector<std::string> split_words_q(const std::string& text) {
    std::vector<std::string> words;
    std::istringstream iss(text);
    std::string w;
    while (iss >> w) words.push_back(w);
    return words;
}

// Поиск документов по запросам
std::vector<std::vector<RelativeIndex>>
SearchServer::search(const std::vector<std::string>& queries_input) {
    std::vector<std::vector<RelativeIndex>> all;
    all.reserve(queries_input.size());

    for (const auto& q : queries_input) {
        auto words = split_words_q(q);
        if (words.empty()) { all.push_back({}); continue; }

        // Абсолютная релевантность (сумма count по словам запроса)
        std::unordered_map<size_t, float> abs;
        for (const auto& w : words) {
            for (const auto& e : index_.GetWordCount(w))
                abs[e.doc_id] += static_cast<float>(e.count);
        }
        if (abs.empty()) { all.push_back({}); continue; }

        // Относительная релевантность rank = abs / max(abs)
        float mx = 0.f;
        for (const auto& kv : abs) mx = std::max(mx, kv.second);

        std::vector<RelativeIndex> rel;
        rel.reserve(abs.size());
        for (const auto& kv : abs)
            rel.push_back({ kv.first, kv.second / mx });

        // Сортировка: по rank ↓, doc_id ↑
        std::sort(rel.begin(), rel.end(), [](const RelativeIndex& a, const RelativeIndex& b) {
            if (a.rank == b.rank) return a.doc_id < b.doc_id;
            return a.rank > b.rank;
        });

        // Ограничение по max_responses
        if (static_cast<int>(rel.size()) > responses_limit_)
            rel.resize(responses_limit_);

        all.push_back(std::move(rel));
    }

    return all;
}
