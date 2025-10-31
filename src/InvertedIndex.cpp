#include "InvertedIndex.h"
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <thread>
#include <mutex>
#include <iostream>

// ---- ограничения ТЗ ----
static constexpr size_t MAX_WORD_LEN   = 100;   // длина слова ≤ 100
static constexpr size_t MAX_DOC_WORDS  = 1000;  // в документе ≤ 1000 слов

// оставляем только [a-z] и пробелы, приводим к нижнему регистру
static std::string normalize(std::string s) {
    for (char& ch : s) {
        if (ch >= 'A' && ch <= 'Z') ch = char(ch - 'A' + 'a');
        else if (!((ch >= 'a' && ch <= 'z') || ch == ' ')) ch = ' ';
    }
    return s;
}

// Разбиваем на слова + фильтруем длину слова
static std::vector<std::string> tokenize_doc(const std::string& raw) {
    std::vector<std::string> words;
    std::istringstream iss(normalize(raw));
    std::string w;
    while (iss >> w) {
        if (w.size() <= MAX_WORD_LEN) words.push_back(w);
    }
    // Ограничиваем документ по количеству слов
    if (words.size() > MAX_DOC_WORDS) {
        std::cerr << "[Index] Document truncated to " << MAX_DOC_WORDS
                  << " words (had " << words.size() << ")\n";
        words.resize(MAX_DOC_WORDS);
    }
    return words;
}

void InvertedIndex::UpdateDocumentBase(const std::vector<std::string>& input_docs) {
    docs_ = input_docs;
    freq_dictionary_.clear();

    const size_t n = docs_.size();
    if (n == 0) return;

    std::mutex mtx;

    // каждый файл индексируется в отдельном потоке
    std::vector<std::thread> threads;
    threads.reserve(n);

    for (size_t doc_id = 0; doc_id < n; ++doc_id) {
        threads.emplace_back([&, doc_id](){
            // локальный счётчик слов текущего документа
            std::unordered_map<std::string, size_t> local;
            for (const auto& w : tokenize_doc(docs_[doc_id])) ++local[w];

            // мержим локальные (слово -> {doc_id, count}) в общий словарь
            std::lock_guard<std::mutex> lk(mtx);
            for (const auto& [word, cnt] : local) {
                freq_dictionary_[word].push_back(Entry{ doc_id, cnt });
            }
        });
    }

    for (auto& t : threads) t.join();

    // стабилизируем порядок
    for (auto& [word, entries] : freq_dictionary_) {
        std::sort(entries.begin(), entries.end(),
                  [](const Entry& a, const Entry& b){ return a.doc_id < b.doc_id; });
    }
}

std::vector<Entry> InvertedIndex::GetWordCount(const std::string& word) {
    auto it = freq_dictionary_.find(word);
    if (it == freq_dictionary_.end()) return {};
    return it->second; // уже отсортировано по doc_id
}
