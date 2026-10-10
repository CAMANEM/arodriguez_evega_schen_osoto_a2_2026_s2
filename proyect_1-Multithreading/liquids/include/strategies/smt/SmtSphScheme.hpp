#ifndef SMT_SPH_SCHEME_HPP
#define SMT_SPH_SCHEME_HPP

#include "strategies/threaded/ThreadedSphScheme.hpp"

class SmtSphScheme : public ThreadedSphScheme {
public:
    explicit SmtSphScheme(unsigned int oversubscriptionFactor = 2);

    unsigned int getOversubscriptionFactor() const;
    static unsigned int logicalProcessorCount();
    unsigned int requestedThreadCountForBenchmark() const;
    std::string getSchemeName() const override;
    execution_model getExecutionModel() const override;

protected:
    unsigned int requestedThreadCount() const override;

private:
    unsigned int oversubscriptionFactor_;
};

#endif
