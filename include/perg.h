#ifndef PERG_H
#define PERG_H

#include <queue>
#include <regex>
#include <string>
#include <filesystem>

struct Flags {

    bool recursive;
    bool invert;
    bool verbose;
    bool isFile;
    bool fileWise;
    bool checkHidden;
    bool extra;
    int numExtra;
    std::filesystem::path file;
    std::string pattern;

    Flags(): recursive(),
               invert(),
               verbose(),
               isFile(),
               fileWise(),
               checkHidden(),
               extra(),
               numExtra(),
               file(),
               pattern() {}
};



class Perg {
private:
    std::vector<std::string> args;
    int argc;
    [[nodiscard]] bool needHelp() const;
    [[nodiscard]] std::queue<std::string> serializeSettings() const;
    void handleFileFlag(std::queue<std::string>& settings);
    void handleAfterContextFlag(std::queue<std::string>& settings);
    void handlePattern(const std::string& arg, std::queue<std::string>& settings);
    void validatePattern();
    bool isNextArgOption(const std::string& arg) ;
    void reportError(const std::string& message);
    void findPatternMatches(std::string& line, std::regex& rgx , std::string& output , std::string& fileName , std::ifstream& file);
    std::string findPatternMatches(std::string &line, std::regex& rgx, std::ifstream& file,
                                   unsigned long long& lineNum, unsigned long long maxLines);
    unsigned long long getTotalLines(std::ifstream& file);
    void getExtraContext(std::ifstream& file , std::string line , std::string& output ,
    unsigned long long lineNum , unsigned long long maxLines , std::regex& rgx);
    bool needsParameter(const std::string& flag);
    bool isFlag(const std::string& arg);

public:
    Flags flags;

    std::queue<std::filesystem::path> filePaths;
    Perg( std::vector<std::string>& args);
    Perg();
    bool helpCheck() const;
    void parseCommandLine();
    void printMultiple();
    void printSingle();
    void findAll(std::filesystem::path& path);
};




#endif