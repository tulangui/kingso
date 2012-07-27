#include "MergerServer.h"
#include "framework/Command.h"
#include "framework/namespace.h"

int main(int argc, char * argv[]) {
	FRAMEWORK::CommandLine::registerSig();
	return MERGER::MergerCommand().run(argc, argv);
}
