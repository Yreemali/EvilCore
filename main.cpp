#include "evil_core.hpp"

int main() {
    std::set_terminate(evil_terminate_logger);

    using LicenseFormula = Formula<
        Clause<Literal<1>, Literal<-2>>,
        Clause<Literal<2>, Literal<3>>,
        Clause<Literal<-1>, Literal<-3>>
    >;
    
    constexpr size_t SAT_VARIABLES = 3;
    constexpr auto sat_check = SatSolverGate<SAT_VARIABLES, LicenseFormula>::run();

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    
    ApocalypseNode() + ExecutionState::Init *= sat_check.first ++;

    #pragma GCC diagnostic pop
}