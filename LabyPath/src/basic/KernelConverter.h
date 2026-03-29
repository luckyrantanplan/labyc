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

struct Lazy_gmpq_to_Expr_converter {
    template <typename T> auto operator()(const T& a) const -> CORE::Expr {
        return CORE::Expr(CGAL::to_double(a));
    }
};

struct Expr_to_Lazy_gmpq_converter {
    template <typename T> auto operator()(const T& a) const {
        return typename CGAL::Epeck::FT(CGAL::to_double(a));
    }
};

using To_sqrt_kernel =
    CGAL::Cartesian_converter<CGAL::Epeck,
                              CGAL::Exact_predicates_exact_constructions_kernel_with_sqrt,
                              Lazy_gmpq_to_Expr_converter>;
using From_sqrt_kernel =
    CGAL::Cartesian_converter<CGAL::Exact_predicates_exact_constructions_kernel_with_sqrt,
                              CGAL::Epeck, Expr_to_Lazy_gmpq_converter>;
} // end namespace laby
#endif /* BASIC_KERNELCONVERTER_H_ */
