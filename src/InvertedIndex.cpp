#include "InvertedIndex.h"
#include <sstream>
#include <unordered_map>
#include <algorithm>

static std::vector<std::string> split_words(const std::string& text) {
    std::vector<std::string> words;
    std::istringstream iss(text);
    std::string w;
    while (iss >> w) words.push_back(w);
    return words;
}

void InvertedIndex::UpdateDocumentBase(const std::vector<std::string>& input_docs) {
    docs_ = input_docs;
    freq_dictionary_.clear();

    for (size_t id = 0; id < docs_.size(); ++id) {
        std::unordered_map<std::string, size_t> local;
        for (auto& w : split_words(docs_[id])) ++local[w]; // считаем вхождения слова
        for (auto& [w, cnt] : local)
            freq_dictionary_[w].push_back({ id, cnt });
    }

    for (auto& [word, entries] : freq_dictionary_) {
        std::sort(entries.begin(), entries.end(),
                  [](const Entry& a, const Entry& b) { return a.doc_id < b.doc_id; });
    }
}

std::vector<Entry> InvertedIndex::GetWordCount(const std::string& word) {
    auto it = freq_dictionary_.find(word);
    if (it == freq_dictionary_.end()) return {};
    return it->second;
}
