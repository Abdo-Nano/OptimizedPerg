#ifndef PERG_H
#define PERG_H

#include <queue>
#include <regex>
#include <string>
#include <filesystem>
#include <optional>

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
    // helper functions for checkHelpPage public function
    [[nodiscard]] bool checkHelpArgs() const;
    static void printHelpPage();

    // helper functions for parseCommandLine public function
    [[nodiscard]] std::queue<std::string> serializeSettings() const;
    void handleFileFlag(std::queue<std::string>& settings);
    void handleAfterContextFlag(std::queue<std::string>& settings);
    void handlePattern(const std::string& arg, const std::queue<std::string>& settings);
    void validatePattern() const;
    static bool isFlag(const std::string& arg);
    static bool isNextArgOption(const std::string& arg) ;
    static void reportError(const std::string& message);

    // helper functions for printMultiple public function

    // to reduce the parameters number
    struct Info {
        std::string line;
        std::regex pattern;
        std::string fileName;
        std::ifstream file;

        Info(const std::string& line , const std::regex& pattern , const std::string fileName , std::ifstream file):
        line(line) , pattern(pattern) , fileName(fileName) , file(std::move(file)){}
    };

    struct FileInfo {

    };

    [[nodiscard]] bool shouldOutputLine(Info& info) const;
    [[nodiscard]] std::string formatOutput(Info& info) const;
    [[nodiscard]] std::string getExtraLinesIfNeeded(Info& info) const;
    [[nodiscard]] std::string getFilePathsFront();
    [[nodiscard]] std::optional<std::ifstream> openFile(const std::string& fileName);
    [[nodiscard]] std::vector<std::string> getFiles();
    void printFormatedOutput(const std::string& output) const;
    void findPatternMatches(Info& info , std::string& output) const;




    [[nodiscard]] std::string getFront();

public:
    Flags flags;

    std::queue<std::filesystem::path> filePaths;
    Perg( std::vector<std::string>& args);
    Perg();

    void checkHelpPage() const;

    void parseCommandLine();

    void printMultiple();

    void printSingle();
    void findAll(std::filesystem::path& path);
};




#endif