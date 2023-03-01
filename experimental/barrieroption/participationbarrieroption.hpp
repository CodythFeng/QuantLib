#ifndef quantlib_participation_barrier_option_hpp
#define quantlib_participation_barrier_option_hpp

#include <ql/instruments/oneassetoption.hpp>
#include <ql/instruments/barriertype.hpp>
#include <ql/instruments/payoffs.hpp>

namespace QuantLib {

    class GeneralizedBlackScholesProcess;

    //! %Barrier option on a single asset.
    /*! The analytic pricing engine will be used if none if passed.

        \ingroup instruments
    */
    class ParticipationBarrierOption : public OneAssetOption {
    public:
        class arguments;
        class engine;
        ParticipationBarrierOption(Barrier::Type barrierType,
            Real barrier,
            Real rebate,
            Real participation,
            const ext::shared_ptr<StrikedTypePayoff>& payoff,
            const ext::shared_ptr<Exercise>& exercise);
        void setupArguments(PricingEngine::arguments*) const override;
        /*! \warning see VanillaOption for notes on implied-volatility
                     calculation.
        */
    protected:
        // arguments
        Barrier::Type barrierType_;
        Real barrier_;
        Real rebate_;
        Real participation_;
    };

    //! %Arguments for barrier option calculation
    class ParticipationBarrierOption::arguments : public OneAssetOption::arguments {
    public:
        arguments();
        Barrier::Type barrierType;
        Real barrier;
        Real rebate;
        Real participation;
        void validate() const override;
    };

    //! %Barrier-option %engine base class
    class ParticipationBarrierOption::engine
        : public GenericEngine<ParticipationBarrierOption::arguments,
        ParticipationBarrierOption::results> {
    protected:
        bool triggered(Real underlying) const;
    };

}

#endif
