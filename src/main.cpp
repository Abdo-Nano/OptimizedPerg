#include "../include/perg.h"



int main(int argc, char *argv[]) {
    std::vector<std::string> args;

    // Convert command line arguments
    for (int i = 0; i < argc; i++) {
        args.push_back(std::string(argv[i]));
    }

    // Initialize and parse
    Perg perg(args);
    perg.parseCommandLine();

    // Execute search based on flags
    if (perg.flags.isFile) {
        // Single file mode (-f flag)
        perg.filePaths.push(perg.flags.file);
        if (perg.flags.fileWise) {
            perg.printMultiple();
        } else {
            perg.printSingle();
        }
    } else {
        // Directory search mode
        std::filesystem::path startPath = ".";
        perg.findAll(startPath);
    }

    return 0;
}