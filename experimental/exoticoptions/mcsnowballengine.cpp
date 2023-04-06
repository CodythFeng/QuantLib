#include <ql/quantlib.hpp>

namespace QuantLib {
    ContinuousSnowballPathPricer::ContinuousSnowballPathPricer(
        Real knockOutLevel,
        Real knockInLevel,
        Real couponRate,
        Real principal,
        Real strike,
        bool payAtMaturity,
        Option::Type type,
        std::vector<DiscountFactor> discounts,
        ext::shared_ptr<StochasticProcess1D> diffProcess,
        PseudoRandom::ursg_type sequenceGen)
        : knockOutLevel_(knockOutLevel), knockInLevel_(knockInLevel),
        couponRate_(couponRate), principal_(principal), strike_(strike), payAtMaturity_(payAtMaturity),
        diffProcess_(std::move(diffProcess)), sequenceGen_(std::move(sequenceGen)),
        payoff_(type, strike), discounts_(std::move(discounts)) {}

    Real ContinuousSnowballPathPricer::operator()(const Path& path) const {

        static Size null = Null<Size>();
        Size n = path.length();
        QL_REQUIRE(n > 1, "the path cannot be empty");
        bool isKnockedIn = false;
        Size knockNode = null;
        Real new_asset_price;
        const TimeGrid& timeGrid = path.timeGrid();
        std::vector<Real> u = sequenceGen_.nextSequence().value;
        Size i;

        for (i = 0; i < n; i++) {
            new_asset_price = path[i];
            if (new_asset_price >= knockOutLevel_) {
                if (payAtMaturity_) {
                    return principal_ * (1 + couponRate_ * timeGrid[i]) * discounts_.back();
                }
                else {
                    return principal_ * (1 + couponRate_ * timeGrid[i]) * discounts_[i];
                }
            }
            if (isKnockedIn == false && new_asset_price <= knockInLevel_) {
                isKnockedIn = true;
            }
        }
        if (!isKnockedIn) {
            return principal_ * (1 + couponRate_ * timeGrid.back()) * discounts_.back();
        }
        else {
            if (new_asset_price > strike_) {
                return principal_ * discounts_.back();
            }
            else {
                return principal_ * new_asset_price / strike_ * discounts_.back();
            }
        }
    }

}

