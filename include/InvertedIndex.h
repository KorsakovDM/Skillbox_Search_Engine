#pragma once
#include <map>
#include <string>
#include <vector>


struct Entry {
    size_t doc_id{};
    size_t count{};    // количество вхождений слова в документ

    bool operator==(const Entry& other) const {
        return doc_id == other.doc_id && count == other.count;
    }
};

class InvertedIndex {
public:
    InvertedIndex() = default;

    void UpdateDocumentBase(const std::vector<std::string>& input_docs);

    std::vector<Entry> GetWordCount(const std::string& word);

private:
    std::vector<std::string> docs_;
    std::map<std::string, std::vector<Entry>> freq_dictionary_;
};
