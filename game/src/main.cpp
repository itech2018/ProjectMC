#include <iostream>
#include "projectmc/Version.hpp"
int main() {
  std::cout << "ProjectMC Game " << projectmc::Version::string() << '\n';
  std::cout << "v0.0.1 bootstrap initialized.\n";
  return 0;
}
