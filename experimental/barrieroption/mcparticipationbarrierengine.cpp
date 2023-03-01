#include <ql/experimental/barrieroption/mcparticipationbarrierengine.hpp>
#include <utility>

namespace QuantLib {

    ParticipationBarrierPathPricer::ParticipationBarrierPathPricer(Barrier::Type barrierType,
        Real barrier,
        Real rebate,
        Real participation,
        Option::Type type,
        Real strike,
        std::vector<DiscountFactor> discounts,
        ext::shared_ptr<StochasticProcess1D> diffProcess,
        PseudoRandom::ursg_type sequenceGen)
        : barrierType_(barrierType), barrier_(barrier), rebate_(rebate), participation_(participation),
        diffProcess_(std::move(diffProcess)), sequenceGen_(std::move(sequenceGen)),
        payoff_(type, strike), discounts_(std::move(discounts)) {
        QL_REQUIRE(strike >= 0.0,
            "strike less than zero not allowed");
        QL_REQUIRE(barrier > 0.0,
            "barrier less/equal zero not allowed");
    }


    Real ParticipationBarrierPathPricer::operator()(const Path& path) const {
        static Size null = Null<Size>();
        Size n = path.length();
        QL_REQUIRE(n > 1, "the path cannot be empty");

        bool isOptionActive = false;
        Size knockNode = null;
        Real asset_price = path.front();
        Real new_asset_price;
        Real x, y;
        Volatility vol;
        const TimeGrid& timeGrid = path.timeGrid();
        Time dt;
        std::vector<Real> u = sequenceGen_.nextSequence().value;
        Size i;

        switch (barrierType_) {
        case Barrier::DownIn:
            isOptionActive = false;
            for (i = 0; i < n - 1; i++) {
                new_asset_price = path[i + 1];
                // terminal or initial vol?
                vol = diffProcess_->diffusion(timeGrid[i], asset_price);
                dt = timeGrid.dt(i);

                x = std::log(new_asset_price / asset_price);
                y = 0.5 * (x - std::sqrt(x * x - 2 * vol * vol * dt * std::log(u[i])));
                y = asset_price * std::exp(y);
                if (y <= barrier_) {
                    isOptionActive = true;
                    if (knockNode == null)
                        knockNode = i + 1;
                }
                asset_price = new_asset_price;
            }
            break;
        case Barrier::UpIn:
            isOptionActive = false;
            for (i = 0; i < n - 1; i++) {
                new_asset_price = path[i + 1];
                // terminal or initial vol?
                vol = diffProcess_->diffusion(timeGrid[i], asset_price);
                dt = timeGrid.dt(i);

                x = std::log(new_asset_price / asset_price);
                y = 0.5 * (x + std::sqrt(x * x - 2 * vol * vol * dt * std::log((1 - u[i]))));
                y = asset_price * std::exp(y);
                if (y >= barrier_) {
                    isOptionActive = true;
                    if (knockNode == null)
                        knockNode = i + 1;
                }
                asset_price = new_asset_price;
            }
            break;
        case Barrier::DownOut:
            isOptionActive = true;
            for (i = 0; i < n - 1; i++) {
                new_asset_price = path[i + 1];
                // terminal or initial vol?
                vol = diffProcess_->diffusion(timeGrid[i], asset_price);
                dt = timeGrid.dt(i);

                x = std::log(new_asset_price / asset_price);
                y = 0.5 * (x - std::sqrt(x * x - 2 * vol * vol * dt * std::log(u[i])));
                y = asset_price * std::exp(y);
                if (y <= barrier_) {
                    isOptionActive = false;
                    if (knockNode == null)
                        knockNode = i + 1;
                }
                asset_price = new_asset_price;
            }
            break;
        case Barrier::UpOut:
            isOptionActive = true;
            for (i = 0; i < n - 1; i++) {
                new_asset_price = path[i + 1];
                // terminal or initial vol?
                vol = diffProcess_->diffusion(timeGrid[i], asset_price);
                dt = timeGrid.dt(i);

                x = std::log(new_asset_price / asset_price);
                y = 0.5 * (x + std::sqrt(x * x - 2 * vol * vol * dt * std::log((1 - u[i]))));
                y = asset_price * std::exp(y);
                if (y >= barrier_) {
                    isOptionActive = false;
                    if (knockNode == null)
                        knockNode = i + 1;
                }
                asset_price = new_asset_price;
            }
            break;
        default:
            QL_FAIL("unknown barrier type");
        }

        if (isOptionActive) {
            return payoff_(asset_price) * participation_ * discounts_.back();
        }
        else {
            switch (barrierType_) {
            case Barrier::UpIn:
            case Barrier::DownIn:
                return rebate_ * participation_ * discounts_.back();
            case Barrier::UpOut:
            case Barrier::DownOut:
                return rebate_ * participation_ * discounts_[knockNode];
            default:
                QL_FAIL("unknown barrier type");
            }
        }
    }
}
