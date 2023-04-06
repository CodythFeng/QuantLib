#ifndef quantlib_snowball_option_hpp
#define quantlib_snowball_option_hpp

#include <ql/instruments/oneassetoption.hpp>
#include <ql/instruments/payoffs.hpp>

namespace QuantLib {

    class ContinuousSnowballOption : public OneAssetOption {
    public:
        class arguments;
        class engine;
        ContinuousSnowballOption(
            const ext::shared_ptr<StrikedTypePayoff>& payoff,
            const ext::shared_ptr<Exercise>& exercise,
            Real knockOutLevel, Real knockInLevel, Real couponRate,
            Real principal, Real strike, bool payAtMaturity);

        void setupArguments(PricingEngine::arguments*) const override;

    protected:
        // arguments
        Real knockOutLevel_;
        Real knockInLevel_;
        Real couponRate_;
        Real principal_;
        Real strike_;
        bool payAtMaturity_;
    };

    class ContinuousSnowballOption::arguments : public OneAssetOption::arguments {
    public:
        arguments();
        Real knockOutLevel;
        Real knockInLevel;
        Real couponRate;
        Real principal;
        Real strike;
        bool payAtMaturity;
        void validate() const override;
    };

    class ContinuousSnowballOption::engine
        : public GenericEngine<ContinuousSnowballOption::arguments,
        ContinuousSnowballOption::results> {
    };
}


#endif