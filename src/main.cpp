#include "ConverterJSON.h"
#include "InvertedIndex.h"
#include "SearchServer.h"
#include <iostream>
#include <utility>

int main() {
    try {
        std::cout << "Starting SkillboxSearchEngine v0.1\n";

        ConverterJSON cj;
        cj.setResourcesDir("resources");

        auto docs = cj.GetTextDocuments();

        InvertedIndex idx;
        idx.UpdateDocumentBase(docs);

        SearchServer srv(idx);
        srv.setResponsesLimit(cj.GetResponsesLimit());

        auto queries = cj.GetRequests();
        auto results = srv.search(queries);

        std::vector<std::vector<std::pair<int, float>>> out;
        out.reserve(results.size());
        for (auto& vec : results) {
            std::vector<std::pair<int, float>> row;
            row.reserve(vec.size());
            for (auto& r : vec) row.emplace_back(static_cast<int>(r.doc_id), r.rank);
            out.push_back(std::move(row));
        }

        cj.putAnswers(out);
        std::cout << "Done. See resources/answers.json\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
