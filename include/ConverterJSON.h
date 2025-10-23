#pragma once
#include <string>
#include <vector>

class ConverterJSON {
public:
    ConverterJSON() = default;

    // где лежат config.json / requests.json / answers.json (по умолчанию "resources")
    void setResourcesDir(const std::string& dir);

    // документы из config.json["files"]
    std::vector<std::string> GetTextDocuments();

    // лимит ответов (если нет — 5)
    int GetResponsesLimit();

    // список запросов из requests.json
    std::vector<std::string> GetRequests();

    // ответы в answers.json
    void putAnswers(const std::vector<std::vector<std::pair<int, float>>>& answers);

private:
    std::string resources_dir_ = "resources";
};
