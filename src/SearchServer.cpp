#include "SearchServer.h"
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <iostream>

static constexpr size_t MAX_WORD_LEN    = 100;
static constexpr size_t MAX_QUERY_WORDS = 10;
static constexpr size_t MAX_REQUESTS    = 1000;

// только латиница и пробелы, нижний регистр
static std::string normalize_q(std::string s) {
    for (char& ch : s) {
        if (ch >= 'A' && ch <= 'Z') ch = char(ch - 'A' + 'a');
        else if (!((ch >= 'a' && ch <= 'z') || ch == ' ')) ch = ' ';
    }
    return s;
}

// Разбивка запроса + фильтрация длины слова
static std::vector<std::string> tokenize_query(const std::string& raw) {
    std::vector<std::string> words;
    std::istringstream iss(normalize_q(raw));
    std::string w;
    while (iss >> w) {
        if (w.size() <= MAX_WORD_LEN) words.push_back(w);
    }
    // Ограничиваем число слов в запросе
    if (words.size() > MAX_QUERY_WORDS) {
        std::cerr << "[Query] Truncated to " << MAX_QUERY_WORDS
                  << " tokens (had " << words.size() << ")\n";
        words.resize(MAX_QUERY_WORDS);
    }
    return words;
}

std::vector<std::vector<RelativeIndex>>
SearchServer::search(const std::vector<std::string>& queries_input) {
    std::vector<std::vector<RelativeIndex>> all;

    // ограничение на количество запросов
    size_t limit_requests = queries_input.size();
    if (limit_requests > MAX_REQUESTS) {
        std::cerr << "[Search] Requests truncated to " << MAX_REQUESTS
                  << " (had " << limit_requests << ")\n";
        limit_requests = MAX_REQUESTS;
    }
    all.reserve(limit_requests);

    for (size_t qi = 0; qi < limit_requests; ++qi) {
        // 1) токенизация + ограничения
        auto words_raw = tokenize_query(queries_input[qi]);
        if (words_raw.empty()) { all.push_back({}); continue; }

        // 2) уникалльность слов
        std::vector<std::string> words;
        words.reserve(words_raw.size());
        {
            std::unordered_set<std::string> seen;
            for (auto& w : words_raw) {
                if (seen.insert(w).second) words.push_back(std::move(w));
            }
        }

        // 3) получаем posting-листы и считаем "редкость"
        struct WP { std::string word; std::vector<Entry> postings; size_t rare = 0; };
        std::vector<WP> wps;
        wps.reserve(words.size());
        for (const auto& w : words) {
            auto postings = index_.GetWordCount(w);
            if (!postings.empty()) {
                size_t rare = postings.size(); // чем меньше, тем реже слово
                wps.push_back(WP{ w, std::move(postings), rare });
            }
        }
        // если хоть одно из оригинальных слов отсутствует — AND-пересечение даст пусто
        if (wps.size() != words.size()) { all.push_back({}); continue; }

        // 4) сортируем слова по редкости (самые редкие первыми)
        std::sort(wps.begin(), wps.end(),
                  [](const WP& a, const WP& b){ return a.rare < b.rare; });

        // 5) явное пересечение doc-списков (AND)
        // инициализируем множеством doc_id из самого редкого слова
        std::vector<size_t> alive;
        alive.reserve(wps.front().postings.size());
        for (const auto& e : wps.front().postings) alive.push_back(e.doc_id);

        // пересекаем с остальными словами
        for (size_t i = 1; i < wps.size() && !alive.empty(); ++i) {
            std::vector<size_t> next;
            next.reserve(std::min(alive.size(), wps[i].postings.size()));
            // два указателя по отсортированным по doc_id векторам
            size_t p = 0, q = 0;
            while (p < alive.size() && q < wps[i].postings.size()) {
                if (alive[p] == wps[i].postings[q].doc_id) {
                    next.push_back(alive[p]); ++p; ++q;
                } else if (alive[p] < wps[i].postings[q].doc_id) {
                    ++p;
                } else {
                    ++q;
                }
            }
            alive.swap(next);
        }

        if (alive.empty()) { all.push_back({}); continue; }

        // 6) считаем абсолютную релевантность ТОЛЬКО по пересечению alive
        std::unordered_map<size_t, float> abs;
        abs.reserve(alive.size());
        for (const auto& wp : wps) {
            // postings отсортированы по doc_id; аккуратно суммируем только пересекающиеся id
            size_t q = 0;
            for (size_t p = 0; p < alive.size() && q < wp.postings.size();) {
                const size_t aid = alive[p];
                const size_t pid = wp.postings[q].doc_id;
                if (aid == pid) {
                    abs[aid] += static_cast<float>(wp.postings[q].count);
                    ++p; ++q;
                } else if (aid < pid) {
                    ++p;
                } else {
                    ++q;
                }
            }
        }
        if (abs.empty()) { all.push_back({}); continue; }

        // 7) нормализация: rank = abs / max_abs
        float mx = 0.f;
        for (const auto& kv : abs) mx = std::max(mx, kv.second);

        std::vector<RelativeIndex> rel;
        rel.reserve(abs.size());
        for (const auto& kv : abs) rel.push_back({ kv.first, kv.second / mx });

        // 8) сортировка: rank ↓, при равенстве doc_id ↑
        std::sort(rel.begin(), rel.end(), [](const RelativeIndex& a, const RelativeIndex& b){
            if (a.rank == b.rank) return a.doc_id < b.doc_id;
            return a.rank > b.rank;
        });

        // 9) Top-N после сортировки
        if (static_cast<int>(rel.size()) > responses_limit_) rel.resize(responses_limit_);

        all.push_back(std::move(rel));
    }

    return all;
}
