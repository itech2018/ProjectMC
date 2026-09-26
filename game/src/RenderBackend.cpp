#include "projectmc/game/RenderBackend.hpp"

namespace projectmc::game {

// Translation unit intentionally owns the backend interface ABI. Concrete
// backends live in separate files so SDL UI rendering and future GPU world
// rendering do not become coupled again.

}
