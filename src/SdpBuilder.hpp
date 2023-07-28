#include <vector>
#include <string>
#include "utils.hpp"

class SdpBuilder{
private:
    std::vector<std::string> lines;
    std::vector<std::string> newLine;

    void add(const std::string& line);
    void push(const std::string& word);
    void addJoined(const std::string& separator = "");
    void addCandidate(const Candidate& c);
    void addHeader(int64_t session_id);
    void addTransport(const Transport& transport);
    void addSsrcEntry(const Transport& transport);
    std::string join();
    std::string finalize();
    void addConference(const Conference& conference);

public:
    static std::string fromConference(const Conference& conference);
};
