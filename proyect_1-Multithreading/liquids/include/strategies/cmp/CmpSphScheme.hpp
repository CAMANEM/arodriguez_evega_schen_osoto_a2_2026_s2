#ifndef CMP_SPH_SCHEME_HPP
#define CMP_SPH_SCHEME_HPP

#include "strategies/threaded/ThreadedSphScheme.hpp"

class CmpSphScheme : public ThreadedSphScheme {
public:
    static unsigned int logicalProcessorCount();
    std::string getSchemeName() const override;
    execution_model getExecutionModel() const override;

protected:
    unsigned int requestedThreadCount() const override;
};

#endif
