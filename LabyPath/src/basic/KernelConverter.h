/*
 * KernelConverer.h
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
struct Lazy_gmpq_to_Expr_converter: //
public std::unary_function<CGAL::Lazy_exact_nt<CGAL::Gmpq>, CORE::Expr> {
    CORE::Expr operator()(const CGAL::Lazy_exact_nt<CGAL::Gmpq> &a) const {
        return ::CORE::BigRat(exact(a).mpq());
    }
};

struct Expr_to_Lazy_gmpq_converter: //
public std::unary_function<CORE::Expr, CGAL::Lazy_exact_nt<CGAL::Gmpq> > {
    CGAL::Lazy_exact_nt<CGAL::Gmpq> operator()(const CORE::Expr &a) const {
        a.approx(120, 2048);
        return CGAL::Lazy_exact_nt<CGAL::Gmpq>(a.BigRatValue().get_mp());

    }
};

typedef CGAL::Cartesian_converter<CGAL::Epeck, CGAL::Exact_predicates_exact_constructions_kernel_with_sqrt, Lazy_gmpq_to_Expr_converter> To_sqrt_kernel;
typedef CGAL::Cartesian_converter<CGAL::Exact_predicates_exact_constructions_kernel_with_sqrt, CGAL::Epeck, Expr_to_Lazy_gmpq_converter> From_sqrt_kernel;
} // end namespace laby
#endif /* BASIC_KERNELCONVERTER_H_ */
