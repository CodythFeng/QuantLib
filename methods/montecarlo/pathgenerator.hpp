/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2002, 2003 Ferdinando Ametrano
 Copyright (C) 2000, 2001, 2002, 2003 RiskMap srl
 Copyright (C) 2003, 2004, 2005, 2006 StatPro Italia srl

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it
 under the terms of the QuantLib license.  You should have received a
 copy of the license along with this program; if not, please email
 <quantlib-dev@lists.sf.net>. The license is also available online at
 <http://quantlib.org/license.shtml>.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

/*! \file pathgenerator.hpp
    \brief Generates random paths using a sequence generator
*/

#ifndef quantlib_montecarlo_path_generator_hpp
#define quantlib_montecarlo_path_generator_hpp

#include <ql/methods/montecarlo/brownianbridge.hpp>
#include <ql/stochasticprocess.hpp>
#include <utility>
#include <fstream>
#include <io.h>

namespace QuantLib {
    class StochasticProcess;
    class StochasticProcess1D;
    //! Generates random paths using a sequence generator
    /*! Generates random paths with drift(S,t) and variance(S,t)
        using a gaussian sequence generator

        \ingroup mcarlo

        \test the generated paths are checked against cached results
    */
    template <class GSG>
    class PathGenerator {
      public:
        typedef Sample<Path> sample_type;
        // constructors
        PathGenerator(const ext::shared_ptr<StochasticProcess>&,
                      Time length,
                      Size timeSteps,
                      GSG generator,
                      bool brownianBridge);
        PathGenerator(const ext::shared_ptr<StochasticProcess>&,
                      TimeGrid timeGrid,
                      GSG generator,
                      bool brownianBridge);
        //! \name inspectors
        //@{
        const sample_type& next() const;
        const sample_type& antithetic() const;
        const std::vector<Array> storage(Size samples) const;
        Size size() const { return dimension_; }
        const TimeGrid& timeGrid() const { return timeGrid_; }
        //@}
      private:
        const sample_type& next(bool antithetic) const;
        void writeStorage(std::string address, Size samples) const;
        std::vector<Array> readStorage(std::string address) const;
        bool brownianBridge_;
        GSG generator_;
        Size dimension_;
        TimeGrid timeGrid_;
        ext::shared_ptr<StochasticProcess1D> process_;
        mutable sample_type next_;
        mutable std::vector<Real> temp_;
        BrownianBridge bb_;
    };


    // template definitions

    template <class GSG>
    PathGenerator<GSG>::PathGenerator(const ext::shared_ptr<StochasticProcess>& process,
                                      Time length,
                                      Size timeSteps,
                                      GSG generator,
                                      bool brownianBridge)
    : brownianBridge_(brownianBridge), generator_(std::move(generator)),
      dimension_(generator_.dimension()), timeGrid_(length, timeSteps),
      process_(ext::dynamic_pointer_cast<StochasticProcess1D>(process)),
      next_(Path(timeGrid_), 1.0), temp_(dimension_), bb_(timeGrid_) {
        QL_REQUIRE(dimension_==timeSteps,
                   "sequence generator dimensionality (" << dimension_
                   << ") != timeSteps (" << timeSteps << ")");
    }

    template <class GSG>
    PathGenerator<GSG>::PathGenerator(const ext::shared_ptr<StochasticProcess>& process,
                                      TimeGrid timeGrid,
                                      GSG generator,
                                      bool brownianBridge)
    : brownianBridge_(brownianBridge), generator_(std::move(generator)),
      dimension_(generator_.dimension()), timeGrid_(std::move(timeGrid)),
      process_(ext::dynamic_pointer_cast<StochasticProcess1D>(process)),
      next_(Path(timeGrid_), 1.0), temp_(dimension_), bb_(timeGrid_) {
        QL_REQUIRE(dimension_==timeGrid_.size()-1,
                   "sequence generator dimensionality (" << dimension_
                   << ") != timeSteps (" << timeGrid_.size()-1 << ")");
    }

    template <class GSG>
    const typename PathGenerator<GSG>::sample_type&
    PathGenerator<GSG>::next() const {
        return next(false);
    }

    template <class GSG>
    const typename PathGenerator<GSG>::sample_type&
    PathGenerator<GSG>::antithetic() const {
        return next(true);
    }

    template <class GSG>
    const typename PathGenerator<GSG>::sample_type&
    PathGenerator<GSG>::next(bool antithetic) const {

        typedef typename GSG::sample_type sequence_type;
        const sequence_type& sequence_ =
            antithetic ? generator_.lastSequence()
                       : generator_.nextSequence();

        if (brownianBridge_) {
            bb_.transform(sequence_.value.begin(),
                          sequence_.value.end(),
                          temp_.begin());
        } else {
            std::copy(sequence_.value.begin(),
                      sequence_.value.end(),
                      temp_.begin());
        }

        next_.weight = sequence_.weight;

        Path& path = next_.value;
        path.front() = process_->x0();

        for (Size i=1; i<path.length(); i++) {
            Time t = timeGrid_[i-1];
            Time dt = timeGrid_.dt(i-1);
            path[i] = process_->evolve(t, path[i-1], dt,
                                       antithetic ? -temp_[i-1] :
                                                     temp_[i-1]);
        }

        return next_;
    }


    template <class GSG>
    const std::vector<Array>
        PathGenerator<GSG>::storage(Size samples) const {
        std::string rsgname = generator_.usgName_;
        std::vector<Array> rsgData;
        if (rsgname == "class QuantLib::SobolRsg") {
            std::string storageAddress = "D:/Desktop/C++/quantlib/QuantLib/storage/storagesobol.csv";
            std::ifstream infile;
            infile.open(storageAddress, std::ios::in);
            if (!infile) {
                writeStorage(storageAddress, samples);
                rsgData = readStorage(storageAddress);
            }
            else {
                // read data and save as rsgData
                int iDims = 0; // the counter of dimensions
                int iSamples = 0; // the counter of samples
                rsgData = readStorage(storageAddress);
                iSamples = rsgData.size();
                iDims = rsgData[0].size();
                if (iSamples < samples || iDims < dimension_) {
                    writeStorage(storageAddress, samples);
                    rsgData = readStorage(storageAddress);
                }
            }
        }
        else {
            QL_FAIL("no predestined case for this random sequence generator type");
        }

        if (brownianBridge_) {
            QL_FAIL("to be coded");
        }
        else {
            ;
        }
        
        for (Size i = 0; i < samples; i++) {
            rsgData[i][0] = process_->x0();
            for (Size j = 1; j < timeGrid_.size(); j++) {
                Time t = timeGrid_[j - 1];
                Time dt = timeGrid_.dt(j - 1);
                rsgData[i][j] = process_->evolve(t, rsgData[i][j-1], dt, rsgData[i][j]);
            }
        }

        return rsgData;
    }

    template <class GSG>
    typename void PathGenerator<GSG>::writeStorage(std::string address, Size samples) const{
        // write random sequences
        typedef typename GSG::sample_type sequence_type;
        std::ofstream outfile;
        outfile.open(address, std::ios::out | std::ios::trunc);
        for (Size i = 1; i <= samples; i++) {
            const sequence_type& sequence_ = generator_.nextSequence();
            for (Size j = 0; j < dimension_; j++) {
                outfile << sequence_.value[j] << ',';
            }
            outfile << std::endl;
        }
    }

    template <class GSG>
    typename std::vector<Array> PathGenerator<GSG>::readStorage(std::string address) const{
        std::ifstream infile;
        infile.open(address, std::ios::in);
        std::string strTemp;
        std::vector<Array> rsgData;
        while (std::getline(infile, strTemp)) {
            std::stringstream ssTemp(strTemp);
            std::string strTemp;
            char delim = ',';
            std::vector<Real> vectorTemp;
            vectorTemp.push_back(0.);
            while (getline(ssTemp, strTemp, delim)) {
                vectorTemp.push_back(std::stod(strTemp));
            }
            Array arrayTemp(vectorTemp.size());
            for (int i = 0; i < vectorTemp.size(); i++) {
                arrayTemp[i] = vectorTemp[i];
            }
            rsgData.push_back(arrayTemp);
        }
        return rsgData;
    }

}

#endif