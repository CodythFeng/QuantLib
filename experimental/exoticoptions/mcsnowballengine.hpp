#ifndef quantlib_mc_snowball_engines_hpp
#define quantlib_mc_snowball_engines_hpp

#include<ql/quantlib.hpp>

namespace QuantLib {
    template <class RNG = PseudoRandom, class S = Statistics>
    class MCContinuousSnowballEngine : public ContinuousSnowballOption::engine,
        public McSimulation<SingleVariate, RNG, S> {
    public:
        typedef typename McSimulation<SingleVariate, RNG, S>::path_generator_type
            path_generator_type;
        typedef typename McSimulation<SingleVariate, RNG, S>::path_pricer_type
            path_pricer_type;
        typedef typename McSimulation<SingleVariate, RNG, S>::stats_type
            stats_type;
        // constructor
     MCContinuousSnowballEngine(ext::shared_ptr<GeneralizedBlackScholesProcess> process,
        Size timeSteps,
        Size timeStepsPerYear,
        bool brownianBridge,
        bool antitheticVariate,
        Size requiredSamples,
        Real requiredTolerance,
        Size maxSamples,
        bool isBiased,
        BigNatural seed);
    void calculate() const override {
        Real spot = process_->x0();
        QL_REQUIRE(spot > 0.0, "negative or null underlying given");
        McSimulation<SingleVariate, RNG, S>::calculate(requiredTolerance_,
            requiredSamples_,
            maxSamples_);
        results_.value = this->mcModel_->sampleAccumulator().mean();
        if (RNG::allowsErrorEstimate)
            results_.errorEstimate =
            this->mcModel_->sampleAccumulator().errorEstimate();
    }

    protected:
        // McSimulation implementation
        TimeGrid timeGrid() const override;
        ext::shared_ptr<path_generator_type> pathGenerator() const override {
            TimeGrid grid = timeGrid();
            typename RNG::rsg_type gen =
                RNG::make_sequence_generator(grid.size() - 1, seed_);
            return ext::shared_ptr<path_generator_type>(
                new path_generator_type(process_,
                    grid, gen, brownianBridge_));
        }
        ext::shared_ptr<path_pricer_type> pathPricer() const override;
        // data members
        ext::shared_ptr<GeneralizedBlackScholesProcess> process_;
        Size timeSteps_, timeStepsPerYear_;
        Size requiredSamples_, maxSamples_;
        Real requiredTolerance_;
        bool isBiased_;
        bool brownianBridge_;
        BigNatural seed_;
    };


    //! Monte Carlo barrier-option engine factory
    template <class RNG = PseudoRandom, class S = Statistics>
    class MakeMCContinuousSnowballEngine {
    public:
        MakeMCContinuousSnowballEngine(ext::shared_ptr<GeneralizedBlackScholesProcess>);
        // named parameters
        MakeMCContinuousSnowballEngine& withSteps(Size steps);
        MakeMCContinuousSnowballEngine& withStepsPerYear(Size steps);
        MakeMCContinuousSnowballEngine& withBrownianBridge(bool b = true);
        MakeMCContinuousSnowballEngine& withAntitheticVariate(bool b = true);
        MakeMCContinuousSnowballEngine& withSamples(Size samples);
        MakeMCContinuousSnowballEngine& withAbsoluteTolerance(Real tolerance);
        MakeMCContinuousSnowballEngine& withMaxSamples(Size samples);
        MakeMCContinuousSnowballEngine& withBias(bool b = true);
        MakeMCContinuousSnowballEngine& withSeed(BigNatural seed);
        // conversion to pricing engine
        operator ext::shared_ptr<PricingEngine>() const;
    private:
        ext::shared_ptr<GeneralizedBlackScholesProcess> process_;
        bool brownianBridge_ = false, antithetic_ = false, biased_ = false;
        Size steps_, stepsPerYear_, samples_, maxSamples_;
        Real tolerance_;
        BigNatural seed_ = 0;
    };


    class ContinuousSnowballPathPricer : public PathPricer<Path> {
    public:
        ContinuousSnowballPathPricer(
            Real knockOutLevel,
            Real knockInLevel,
            Real couponRate,
            Real principal,
            Real strike,
            bool payAtMaturity,
            Option::Type type,
            std::vector<DiscountFactor> discounts,
            ext::shared_ptr<StochasticProcess1D> diffProcess,
            PseudoRandom::ursg_type sequenceGen);
    private:
        Real operator()(const Path& path) const override;
        Real knockOutLevel_, knockInLevel_;
        Real couponRate_, principal_, strike_;
        bool payAtMaturity_;
        ext::shared_ptr<StochasticProcess1D> diffProcess_;
        PseudoRandom::ursg_type sequenceGen_;
        PlainVanillaPayoff payoff_;
        std::vector<DiscountFactor> discounts_;
    };

    // template definitions

    template <class RNG, class S>
    inline MCContinuousSnowballEngine<RNG, S>::MCContinuousSnowballEngine(
        ext::shared_ptr<GeneralizedBlackScholesProcess> process,
        Size timeSteps,
        Size timeStepsPerYear,
        bool brownianBridge,
        bool antitheticVariate,
        Size requiredSamples,
        Real requiredTolerance,
        Size maxSamples,
        bool isBiased,
        BigNatural seed)
        : McSimulation<SingleVariate, RNG, S>(antitheticVariate, false), process_(std::move(process)),
        timeSteps_(timeSteps), timeStepsPerYear_(timeStepsPerYear), requiredSamples_(requiredSamples),
        maxSamples_(maxSamples), requiredTolerance_(requiredTolerance), isBiased_(isBiased),
        brownianBridge_(brownianBridge), seed_(seed) {
        QL_REQUIRE(timeSteps != Null<Size>() ||
            timeStepsPerYear != Null<Size>(),
            "no time steps provided");
        QL_REQUIRE(timeSteps == Null<Size>() ||
            timeStepsPerYear == Null<Size>(),
            "both time steps and time steps per year were provided");
        QL_REQUIRE(timeSteps != 0,
            "timeSteps must be positive, " << timeSteps <<
            " not allowed");
        QL_REQUIRE(timeStepsPerYear != 0,
            "timeStepsPerYear must be positive, " << timeStepsPerYear <<
            " not allowed");
        registerWith(process_);
    }

    template <class RNG, class S>
    inline TimeGrid MCContinuousSnowballEngine<RNG, S>::timeGrid() const {

        Time residualTime = process_->time(arguments_.exercise->lastDate());
        if (timeSteps_ != Null<Size>()) {
            return TimeGrid(residualTime, timeSteps_);
        }
        else if (timeStepsPerYear_ != Null<Size>()) {
            Size steps = static_cast<Size>(timeStepsPerYear_ * residualTime);
            return TimeGrid(residualTime, std::max<Size>(steps, 1));
        }
        else {
            QL_FAIL("time steps not specified");
        }
    }


    template <class RNG, class S>
    inline
        ext::shared_ptr<typename MCContinuousSnowballEngine<RNG, S>::path_pricer_type>
        MCContinuousSnowballEngine<RNG, S>::pathPricer() const {
        ext::shared_ptr<PlainVanillaPayoff> payoff =
            ext::dynamic_pointer_cast<PlainVanillaPayoff>(arguments_.payoff);
        QL_REQUIRE(payoff, "non-plain payoff given");

        TimeGrid grid = timeGrid();
        std::vector<DiscountFactor> discounts(grid.size());
        for (Size i = 0; i < grid.size(); i++)
            discounts[i] = process_->riskFreeRate()->discount(grid[i]);

        // do this with template parameters?
        if (isBiased_) {
        }
        else {
            PseudoRandom::ursg_type sequenceGen(grid.size() - 1,
                PseudoRandom::urng_type(5));
            return ext::shared_ptr<
                typename MCContinuousSnowballEngine<RNG, S>::path_pricer_type>(
                    new ContinuousSnowballPathPricer(
                        arguments_.knockOutLevel,
                        arguments_.knockInLevel,
                        arguments_.couponRate,
                        arguments_.principal,
                        arguments_.strike,
                        arguments_.payAtMaturity,
                        payoff->optionType(),
                        discounts,
                        process_,
                        sequenceGen));
        }
    }

    template <class RNG, class S>
    inline MakeMCContinuousSnowballEngine<RNG, S>::MakeMCContinuousSnowballEngine(
        ext::shared_ptr<GeneralizedBlackScholesProcess> process)
        : process_(std::move(process)), steps_(Null<Size>()), stepsPerYear_(Null<Size>()),
        samples_(Null<Size>()), maxSamples_(Null<Size>()), tolerance_(Null<Real>()) {}

    template <class RNG, class S>
    inline MakeMCContinuousSnowballEngine<RNG, S>&
        MakeMCContinuousSnowballEngine<RNG, S>::withSteps(Size steps) {
        steps_ = steps;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCContinuousSnowballEngine<RNG, S>&
        MakeMCContinuousSnowballEngine<RNG, S>::withStepsPerYear(Size steps) {
        stepsPerYear_ = steps;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCContinuousSnowballEngine<RNG, S>&
        MakeMCContinuousSnowballEngine<RNG, S>::withBrownianBridge(bool brownianBridge) {
        brownianBridge_ = brownianBridge;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCContinuousSnowballEngine<RNG, S>&
        MakeMCContinuousSnowballEngine<RNG, S>::withAntitheticVariate(bool b) {
        antithetic_ = b;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCContinuousSnowballEngine<RNG, S>&
        MakeMCContinuousSnowballEngine<RNG, S>::withSamples(Size samples) {
        QL_REQUIRE(tolerance_ == Null<Real>(),
            "tolerance already set");
        samples_ = samples;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCContinuousSnowballEngine<RNG, S>&
        MakeMCContinuousSnowballEngine<RNG, S>::withAbsoluteTolerance(Real tolerance) {
        QL_REQUIRE(samples_ == Null<Size>(),
            "number of samples already set");
        QL_REQUIRE(RNG::allowsErrorEstimate,
            "chosen random generator policy "
            "does not allow an error estimate");
        tolerance_ = tolerance;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCContinuousSnowballEngine<RNG, S>&
        MakeMCContinuousSnowballEngine<RNG, S>::withMaxSamples(Size samples) {
        maxSamples_ = samples;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCContinuousSnowballEngine<RNG, S>&
        MakeMCContinuousSnowballEngine<RNG, S>::withBias(bool biased) {
        biased_ = biased;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCContinuousSnowballEngine<RNG, S>&
        MakeMCContinuousSnowballEngine<RNG, S>::withSeed(BigNatural seed) {
        seed_ = seed;
        return *this;
    }

    template <class RNG, class S>
    inline
        MakeMCContinuousSnowballEngine<RNG, S>::operator ext::shared_ptr<PricingEngine>()
        const {
        QL_REQUIRE(steps_ != Null<Size>() || stepsPerYear_ != Null<Size>(),
            "number of steps not given");
        QL_REQUIRE(steps_ == Null<Size>() || stepsPerYear_ == Null<Size>(),
            "number of steps overspecified");
        return ext::shared_ptr<PricingEngine>(new
            MCContinuousSnowballEngine<RNG, S>(process_,
                steps_,
                stepsPerYear_,
                brownianBridge_,
                antithetic_,
                samples_, tolerance_,
                maxSamples_,
                biased_,
                seed_));
    }

}

#endif
