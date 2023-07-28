#include <string>
#include "utils.hpp"

class MediaDescription {
public:
    std::string cname;
    std::string trackId;
    std::string msid;
    std::string mid;
    uint32_t ssrc;

    explicit MediaDescription(uint8_t mid);
};
