
#ifndef BOOST_MPL_SET_AUX_SET0_HPP_INCLUDED
#define BOOST_MPL_SET_AUX_SET0_HPP_INCLUDED

// Copyright Aleksey Gurtovoy 2003-2004
// Copyright David Abrahams 2003-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: set0.hpp 49239 2008-10-10 09:10:26Z agurtovoy $
// $Date: 2008-10-10 14:40:26 +0530 (Fri, 10 Oct 2008) $
// $Revision: 49239 $

#include <boost/mpl/long.hpp>
#include <boost/mpl/void.hpp>
#include <boost/mpl/aux_/na.hpp>
#include <boost/mpl/set/aux_/tag.hpp>
#include <boost/mpl/aux_/yes_no.hpp>
#include <boost/mpl/aux_/overload_names.hpp>
#include <boost/mpl/aux_/config/operators.hpp>

#include <boost/preprocessor/cat.hpp>

namespace boost { namespace mpl {

#if defined(BOOST_MPL_CFG_USE_OPERATORS_OVERLOADING)

#   define BOOST_MPL_AUX_SET0_OVERLOAD(R, f, X, T) \
    friend R BOOST_PP_CAT(BOOST_MPL_AUX_OVERLOAD_,f)(X const&, T) \
/**/

#   define BOOST_MPL_AUX_SET_OVERLOAD(R, f, X, T) \
    BOOST_MPL_AUX_SET0_OVERLOAD(R, f, X, T) \
/**/

#else

#   define BOOST_MPL_AUX_SET0_OVERLOAD(R, f, X, T) \
    static R BOOST_PP_CAT(BOOST_MPL_AUX_OVERLOAD_,f)(X const&, T) \
/**/

#   define BOOST_MPL_AUX_SET_OVERLOAD(R, f, X, T) \
    BOOST_MPL_AUX_SET0_OVERLOAD(R, f, X, T); \
    using Base::BOOST_PP_CAT(BOOST_MPL_AUX_OVERLOAD_,f) \
/**/

#endif

template< typename Dummy = na > struct set0
{
    typedef set0<>          item_;
    typedef item_           type;
    typedef aux::set_tag    tag;
    typedef void_           last_masked_;
    typedef void_           item_type_;
    typedef long_<0>        size;
    typedef long_<1>        order;

    BOOST_MPL_AUX_SET0_OVERLOAD( aux::no_tag, ORDER_BY_KEY, set0<>, void const volatile* );
    BOOST_MPL_AUX_SET0_OVERLOAD( aux::yes_tag, IS_MASKED, set0<>, void const volatile* );
};

}}

#endif // BOOST_MPL_SET_AUX_SET0_HPP_INCLUDED
