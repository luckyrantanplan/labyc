/*
 * KernelConverter.h
 *
 *  Created on: Jun 28, 2018
 *      Author: florian
 */

#ifndef BASIC_KERNELCONVERTER_H_
#define BASIC_KERNELCONVERTER_H_

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel_with_sqrt.h>
#include <CGAL/Cartesian_converter.h>

namespace laby {

struct Lazy_gmpq_to_Expr_converter {
    template<typename T>
    CORE::Expr operator()(const T &a) const {
        return CORE::Expr(CGAL::to_double(a));
    }
};

struct Expr_to_Lazy_gmpq_converter {
    template<typename T>
    auto operator()(const T &a) const {
        return typename CGAL::Epeck::FT(CGAL::to_double(a));
    }
};

typedef CGAL::Cartesian_converter<CGAL::Epeck, CGAL::Exact_predicates_exact_constructions_kernel_with_sqrt, Lazy_gmpq_to_Expr_converter> To_sqrt_kernel;
typedef CGAL::Cartesian_converter<CGAL::Exact_predicates_exact_constructions_kernel_with_sqrt, CGAL::Epeck, Expr_to_Lazy_gmpq_converter> From_sqrt_kernel;
} // end namespace laby
#endif /* BASIC_KERNELCONVERTER_H_ */
