#include <ql/exercise.hpp>
#include <ql/experimental/exoticoptions/snowballoption.hpp>
#include <ql/instruments/dividendbarrieroption.hpp>
#include <ql/instruments/impliedvolatility.hpp>
#include <memory>

namespace QuantLib {
    ContinuousSnowballOption::ContinuousSnowballOption(
        const ext::shared_ptr<StrikedTypePayoff>& payoff,
        const ext::shared_ptr<Exercise>& exercise,
        Real knockOutLevel, Real knockInLevel, Real couponRate,
        Real principal, Real strike, bool payAtMaturity)
        : OneAssetOption(payoff, exercise), knockOutLevel_(knockOutLevel),
        knockInLevel_(knockInLevel), couponRate_(couponRate), principal_(principal),
        strike_(strike), payAtMaturity_(payAtMaturity) {};

    void ContinuousSnowballOption::setupArguments(PricingEngine::arguments* args) const {

        OneAssetOption::setupArguments(args);

        auto* moreArgs = dynamic_cast<ContinuousSnowballOption::arguments*>(args);
        QL_REQUIRE(moreArgs != nullptr, "wrong argument type");
        moreArgs->knockOutLevel = knockOutLevel_;
        moreArgs->knockInLevel = knockInLevel_;
        moreArgs->couponRate = couponRate_;
        moreArgs->principal = principal_;
        moreArgs->strike = strike_;
        moreArgs->payAtMaturity = payAtMaturity_;
    }

    ContinuousSnowballOption::arguments::arguments()
        : knockOutLevel(Null<Real>()), knockInLevel(Null<Real>()),
        couponRate(Null<Real>()), principal(Null<Real>()),
        strike(Null<Real>()), payAtMaturity(Null<bool>()) {}

    void ContinuousSnowballOption::arguments::validate() const{
        OneAssetOption::arguments::validate();
        QL_REQUIRE(knockOutLevel != Null<Real>(), "no knockoutlevel given");
        QL_REQUIRE(knockInLevel != Null<Real>(), "no knockinlevel given");
        QL_REQUIRE(couponRate != Null<Real>(), "no couponrate given");
        QL_REQUIRE(principal != Null<Real>(), "no principal given");
        QL_REQUIRE(strike != Null<Real>(), "no principal given");
        QL_REQUIRE(payAtMaturity == 0 || payAtMaturity == 1, "no payatmaturity given");
    }


}