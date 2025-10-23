#include "ConverterJSON.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

using nlohmann::json;
namespace fs = std::filesystem;

static constexpr const char* APP_VERSION = "0.1";

static std::string read_file(const fs::path& p) {
    std::ifstream ifs(p, std::ios::binary);
    if (!ifs) throw std::runtime_error("Cannot open file: " + p.string());
    std::ostringstream ss; ss << ifs.rdbuf();
    return ss.str();
}

void ConverterJSON::setResourcesDir(const std::string& dir) { resources_dir_ = dir; }

// ---- внутренний разбор config.json с валидацией по ТЗ ----
static json parse_config_or_throw(const fs::path& cfg_path) {
    if (!fs::exists(cfg_path)) throw std::runtime_error("config file is missing");
    json j = json::parse(read_file(cfg_path));
    if (!j.contains("config")) throw std::runtime_error("config file is empty");

    const auto& cfg = j["config"];
    if (cfg.contains("version")) {
        const std::string ver = cfg["version"].get<std::string>();
        if (ver != APP_VERSION) throw std::runtime_error("config.json has incorrect file version");
    }
    return j;
}

std::vector<std::string> ConverterJSON::GetTextDocuments() {
    const fs::path cfg = fs::path(resources_dir_) / "config.json";
    json j = parse_config_or_throw(cfg);

    std::vector<std::string> docs;
    if (j.contains("files") && j["files"].is_array()) {
        for (const auto& item : j["files"]) {
            // допускаем относительные пути из ТЗ; читаем фактический файл как есть
            fs::path p = item.get<std::string>();
            // если путь относительный — считаем, что он от папки resources
            if (p.is_relative()) p = fs::path(resources_dir_) / p;
            // некоторые студенты кладут рядом, без ../ — подстрахуем filename() в resources
            if (!fs::exists(p)) {
                fs::path fallback = fs::path(resources_dir_) / p.filename();
                if (fs::exists(fallback)) p = fallback;
            }
            if (fs::exists(p)) {
                try { docs.push_back(read_file(p)); }
                catch (...) { std::cerr << "Cannot read file: " << p.string() << "\n"; }
            } else {
                // по ТЗ: «на экран выводится ошибка, но программа не прекращается»
                std::cerr << "File not found: " << p.string() << "\n";
            }
        }
    }
    return docs;
}

int ConverterJSON::GetResponsesLimit() {
    const fs::path cfg = fs::path(resources_dir_) / "config.json";
    json j = parse_config_or_throw(cfg);
    int limit = 5;
    if (j["config"].contains("max_responses")) {
        try { limit = j["config"]["max_responses"].get<int>(); }
        catch (...) { limit = 5; }
    }
    if (limit <= 0) limit = 5;
    return limit;
}

std::vector<std::string> ConverterJSON::GetRequests() {
    const fs::path rq = fs::path(resources_dir_) / "requests.json";
    std::vector<std::string> reqs;
    if (!fs::exists(rq)) return reqs;
    json j = json::parse(read_file(rq));
    if (j.contains("requests") && j["requests"].is_array()) {
        for (auto& it : j["requests"]) reqs.push_back(it.get<std::string>());
    }
    return reqs;
}

void ConverterJSON::putAnswers(const std::vector<std::vector<std::pair<int, float>>>& answers) {
    json out; json answers_obj;

    for (size_t i = 0; i < answers.size(); ++i) {
        char idbuf[16]; std::snprintf(idbuf, sizeof(idbuf), "request%03zu", i + 1);
        if (answers[i].empty()) {
            answers_obj[idbuf] = { {"result", "false"} };
        } else {
            json rel = json::array();
            for (auto& [doc, rank] : answers[i]) rel.push_back({ {"docid", doc}, {"rank", rank} });
            answers_obj[idbuf] = { {"result", "true"}, {"relevance", rel} };
        }
    }
    out["answers"] = answers_obj;

    const fs::path ans = fs::path(resources_dir_) / "answers.json";
    std::ofstream ofs(ans, std::ios::trunc);
    ofs << out.dump(2);
}
