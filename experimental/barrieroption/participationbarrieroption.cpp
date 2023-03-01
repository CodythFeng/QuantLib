#include <ql/exercise.hpp>
#include <ql/experimental/barrieroption/participationbarrieroption.hpp>
#include <ql/instruments/impliedvolatility.hpp>
#include <ql/pricingengines/barrier/analyticbarrierengine.hpp>
#include <memory>

namespace QuantLib {

    ParticipationBarrierOption::ParticipationBarrierOption(
        Barrier::Type barrierType,
        Real barrier,
        Real rebate,
        Real participation,
        const ext::shared_ptr<StrikedTypePayoff>& payoff,
        const ext::shared_ptr<Exercise>& exercise)
        : OneAssetOption(payoff, exercise),
        barrierType_(barrierType), barrier_(barrier), rebate_(rebate), participation_(participation) {}

    void ParticipationBarrierOption::setupArguments(PricingEngine::arguments* args) const {

        OneAssetOption::setupArguments(args);

        auto* moreArgs = dynamic_cast<ParticipationBarrierOption::arguments*>(args);
        QL_REQUIRE(moreArgs != nullptr, "wrong argument type");
        moreArgs->barrierType = barrierType_;
        moreArgs->barrier = barrier_;
        moreArgs->rebate = rebate_;
        moreArgs->participation = participation_;
    }


    ParticipationBarrierOption::arguments::arguments()
        : barrierType(Barrier::Type(-1)), barrier(Null<Real>()),
        rebate(Null<Real>()), participation(Null<Real>()) {}

    void ParticipationBarrierOption::arguments::validate() const {
        OneAssetOption::arguments::validate();

        switch (barrierType) {
        case Barrier::DownIn:
        case Barrier::UpIn:
        case Barrier::DownOut:
        case Barrier::UpOut:
            break;
        default:
            QL_FAIL("unknown type");
        }

        QL_REQUIRE(barrier != Null<Real>(), "no barrier given");
        QL_REQUIRE(rebate != Null<Real>(), "no rebate given");
        QL_REQUIRE(participation != Null<Real>(), "no participation given");
    }

    bool ParticipationBarrierOption::engine::triggered(Real underlying) const {
        switch (arguments_.barrierType) {
        case Barrier::DownIn:
        case Barrier::DownOut:
            return underlying < arguments_.barrier;
        case Barrier::UpIn:
        case Barrier::UpOut:
            return underlying > arguments_.barrier;
        default:
            QL_FAIL("unknown type");
        }
    }

}

