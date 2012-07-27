#include "DetailServer.h"
#include "framework/Command.h"

int main(int argc, char *argv[]) {
    FRAMEWORK::CommandLine::registerSig();
    return DETAIL::DetailCommand().run(argc, argv);
}
