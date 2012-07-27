#include "framework/Command.h"
#include "SearcherServer.h"

int main(int argc, char *argv[]) {
    FRAMEWORK::CommandLine::registerSig();
    return kingso_searcher::SearcherCommand().run(argc, argv);
}
