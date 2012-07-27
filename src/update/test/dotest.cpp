#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/ui/text/TextTestRunner.h>
#include <cppunit/CompilerOutputter.h>
#include "util/Log.h"

int main( int argc, char **argv) {
  alog::Configurator::configureRootLogger();
  
  CppUnit::TextUi::TestRunner runner;
  runner.setOutputter(new CppUnit::CompilerOutputter(&runner.result(), std::cerr));
  CppUnit::TestFactoryRegistry &registry = CppUnit::TestFactoryRegistry::getRegistry();
  runner.addTest( registry.makeTest() );
  bool ok = runner.run("", false);
  return ok ? 0 : 1;
}

