/*
 * KernelConverter.h
 *
 *  Created on: Jun 28, 2018
 *      Author: florian
 */

#ifndef BASIC_KERNELCONVERTER_H_
#define BASIC_KERNELCONVERTER_H_

#include <CGAL/Cartesian_converter.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel_with_sqrt.h>

namespace laby {

struct LazyGmpqToExprConverter {
    template <typename T> auto operator()(const T& a) const -> CORE::Expr {
        return CORE::Expr(CGAL::to_double(a));
    }
};

struct ExprToLazyGmpqConverter {
    template <typename T> auto operator()(const T& a) const {
        return typename CGAL::Epeck::FT(CGAL::to_double(a));
    }
};

using To_sqrt_kernel =
    CGAL::Cartesian_converter<CGAL::Epeck,
                              CGAL::Exact_predicates_exact_constructions_kernel_with_sqrt,
                              LazyGmpqToExprConverter>;
using From_sqrt_kernel =
    CGAL::Cartesian_converter<CGAL::Exact_predicates_exact_constructions_kernel_with_sqrt,
                              CGAL::Epeck, ExprToLazyGmpqConverter>;
} // end namespace laby
#endif /* BASIC_KERNELCONVERTER_H_ */
