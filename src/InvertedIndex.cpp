#include "InvertedIndex.h"
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <thread>
#include <mutex>

// --- нормализация: только строчные латиница и пробелы ---
static std::string normalize(std::string s) {
    for (char& ch : s) {
        if (ch >= 'A' && ch <= 'Z') ch = char(ch - 'A' + 'a');
        else if (!((ch >= 'a' && ch <= 'z') || ch == ' ')) ch = ' ';
    }
    return s;
}

static std::vector<std::string> split_words(const std::string& text) {
    std::vector<std::string> words;
    std::istringstream iss(normalize(text));
    std::string w;
    while (iss >> w) words.push_back(w);
    return words;
}

void InvertedIndex::UpdateDocumentBase(const std::vector<std::string>& input_docs) {
    docs_ = input_docs;
    freq_dictionary_.clear();

    const size_t n = docs_.size();
    if (n == 0) return;

    // планируем потоки
    const size_t hw = std::max<size_t>(1, std::thread::hardware_concurrency());
    const size_t threads = std::min(hw, n);
    const size_t step = (n + threads - 1) / threads;

    std::mutex mtx;

    auto worker = [&](size_t L, size_t R) {
        // локальный словарь: слово -> список {doc_id,count}
        std::unordered_map<std::string, std::vector<Entry>> local;
        for (size_t id = L; id < R; ++id) {
            std::unordered_map<std::string, size_t> counts;
            for (auto& w : split_words(docs_[id])) ++counts[w];
            for (auto& [w, c] : counts) local[w].push_back(Entry{ id, c });
        }
        // слияние в общий словарь
        std::lock_guard<std::mutex> lk(mtx);
        for (auto& [w, vec] : local) {
            auto& dst = freq_dictionary_[w];
            dst.insert(dst.end(), vec.begin(), vec.end());
        }
    };

    std::vector<std::thread> ts;
    ts.reserve(threads);
    for (size_t i = 0; i < threads; ++i) {
        size_t L = i * step;
        size_t R = std::min(n, L + step);
        if (L < R) ts.emplace_back(worker, L, R);
    }
    for (auto& t : ts) t.join();

    // стабильный порядок для тестов
    for (auto& [w, vec] : freq_dictionary_) {
        std::sort(vec.begin(), vec.end(),
                  [](const Entry& a, const Entry& b){ return a.doc_id < b.doc_id; });
    }
}

std::vector<Entry> InvertedIndex::GetWordCount(const std::string& word) {
    auto it = freq_dictionary_.find(word);
    if (it == freq_dictionary_.end()) return {};
    return it->second; // уже отсортировано по doc_id в UpdateDocumentBase
}
