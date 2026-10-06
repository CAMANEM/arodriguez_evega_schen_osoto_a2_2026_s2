#include "core/CoarseWorkerCheckpoint.hpp"

#include <sstream>

CoarseWorkerCheckpoint CoarseWorkerCheckpoint::snapshot() const {
    return *this;
}

void CoarseWorkerCheckpoint::restoreInto(CoarseWorkerCheckpoint& target) const {
    target = *this;
}

std::string CoarseWorkerCheckpoint::toString() const {
    std::ostringstream out;
    out << "worker=" << workerId
        << " block=[" << startIndex << "," << endIndex << ")"
        << " next=" << nextBoidIndex
        << " done=" << boidsCompleted
        << " stalls=" << stallCount;
    return out.str();
}
