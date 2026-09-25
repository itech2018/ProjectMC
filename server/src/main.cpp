#include <iostream>
#include "projectmc/Version.hpp"
int main() {
  std::cout << "ProjectMC Dedicated Server " << projectmc::Version::string() << '\n';
  std::cout << "Authoritative server bootstrap initialized.\n";
  return 0;
}
