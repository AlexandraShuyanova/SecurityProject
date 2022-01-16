#include <vector>

#ifdef _WIN32
bool getDMI(std::vector<uint8_t> &buffer);
#else
bool getDMI(const std::string &path, std::vector<uint8_t> &buffer);
#endif
bool printSMBIOS(smbios::Parser &parser, QTextStream &stream);

