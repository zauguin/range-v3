// Range v3 library
//
//  Copyright Eric Niebler 2013-2014
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//

#ifndef RANGES_V3_VIEW_IOTA_HPP
#define RANGES_V3_VIEW_IOTA_HPP

#include <climits>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <range/v3/range_fwd.hpp>
#include <range/v3/range_concepts.hpp>
#include <range/v3/range_facade.hpp>
#include <range/v3/utility/meta.hpp>
#include <range/v3/utility/concepts.hpp>
#include <range/v3/view/take.hpp>
#include <range/v3/view/delimit.hpp>

namespace ranges
{
    inline namespace v3
    {
        namespace concepts
        {
            struct InputIota
              : refines<SemiRegular>
            {
                template<typename T>
                auto requires_(T t) -> decltype(
                    concepts::valid_expr(
                        concepts::has_type<T &>(++t)
                    ));
            };

            struct ForwardIota
              : refines<InputIota, Regular>
            {};

            struct BidirectionalIota
              : refines<ForwardIota>
            {
                template<typename T>
                auto requires_(T t) -> decltype(
                    concepts::valid_expr(
                        concepts::has_type<T &>(--t)
                    ));
            };

            struct RandomAccessIota
              : refines<BidirectionalIota>
            {
                template<typename T>
                auto requires_(T t) -> decltype(
                    concepts::valid_expr(
                        concepts::model_of<Integral>(t - t),
                        concepts::has_type<T &>(t += (t - t)),
                        concepts::has_type<T &>(t -= (t - t)),
                        concepts::convertible_to<T>(t - (t - t)),
                        concepts::convertible_to<T>(t + (t - t)),
                        concepts::convertible_to<T>((t - t) + t)
                    ));
            };
        }

        template<typename T>
        using InputIota = concepts::models<concepts::InputIota, T>;

        template<typename T>
        using ForwardIota = concepts::models<concepts::ForwardIota, T>;

        template<typename T>
        using BidirectionalIota = concepts::models<concepts::BidirectionalIota, T>;

        template<typename T>
        using RandomAccessIota = concepts::models<concepts::RandomAccessIota, T>;

        template<typename T>
        using iota_concept =
            concepts::most_refined<
                meta::list<
                    concepts::RandomAccessIota,
                    concepts::BidirectionalIota,
                    concepts::ForwardIota,
                    concepts::InputIota>, T>;

        template<typename T>
        using iota_concept_t = meta::eval<iota_concept<T>>;

        /// \cond
        namespace detail
        {
            template<typename Val, typename Iota = iota_concept_t<Val>, bool IsIntegral = std::is_integral<Val>::value>
            struct iota_difference_
            {
                using type = std::ptrdiff_t;
            };

            template<typename Val>
            struct iota_difference_<Val, concepts::RandomAccessIota, false>
            {
                using type = decltype(std::declval<Val>() - std::declval<Val>());
            };

            template<typename Val>
            struct iota_difference_<Val, concepts::RandomAccessIota, true>
            {
            private:
                using difference_t = decltype(std::declval<Val>() - std::declval<Val>());
                static constexpr std::size_t bits = sizeof(difference_t) * CHAR_BIT;
            public:
                using type =
                    meta::if_<
                        meta::not_<std::is_same<Val, difference_t>>,
                        meta::eval<std::make_signed<difference_t>>,
                        meta::if_c<
                            (bits < 8),
                            std::int_fast8_t,
                            meta::if_c<
                                (bits < 16),
                                std::int_fast16_t,
                                meta::if_c<
                                    (bits < 32),
                                    std::int_fast32_t,
                                    std::int_fast64_t> > > >;
            };

            template<typename Val>
            struct iota_difference
              : iota_difference_<Val>
            {};

            template<typename Val>
            using iota_difference_t = meta::eval<iota_difference<Val>>;

            template<typename Val, CONCEPT_REQUIRES_(!Integral<Val>())>
            iota_difference_t<Val> iota_minus(Val const &v0, Val const &v1)
            {
                return v0 - v1;
            }

            template<typename Val, CONCEPT_REQUIRES_(SignedIntegral<Val>())>
            iota_difference_t<Val> iota_minus(Val const &v0, Val const &v1)
            {
                using D = iota_difference_t<Val>;
                return (D) v0 - (D) v1;
            }

            template<typename Val, CONCEPT_REQUIRES_(UnsignedIntegral<Val>())>
            iota_difference_t<Val> iota_minus(Val const &v0, Val const &v1)
            {
                using D = iota_difference_t<Val>;
                return (D) (v0 - v1);
            }
        }
        /// \endcond

        /// \addtogroup group-views
        /// @{
        template<typename Val>
        struct iota_view
          : range_facade<iota_view<Val>, true>
        {
        private:
            using iota_concept_t = ranges::iota_concept<Val>;
            friend range_access;
            using difference_type_ = detail::iota_difference_t<Val>;

            Val value_;

            Val current() const
            {
                return value_;
            }
            void next()
            {
                ++value_;
            }
            constexpr bool done() const
            {
                return false;
            }
            CONCEPT_REQUIRES(ForwardIota<Val>())
            bool equal(iota_view const &that) const
            {
                return that.value_ == value_;
            }
            CONCEPT_REQUIRES(BidirectionalIota<Val>())
            void prev()
            {
                --value_;
            }
            CONCEPT_REQUIRES(RandomAccessIota<Val>())
            void advance(difference_type_ n)
            {
                value_ += n;
            }
            CONCEPT_REQUIRES(RandomAccessIota<Val>())
            difference_type_ distance_to(iota_view const &that) const
            {
                return detail::iota_minus(that.value_, value_);
            }
        public:
            iota_view() = default;
            constexpr explicit iota_view(Val value)
              : value_(detail::move(value))
            {}
        };

        /// An iota view in a closed range with non-random access iota value type
        template<typename Val, typename Val2 = Val>
        struct closed_iota_view
          : range_facade<closed_iota_view<Val, Val2>, true>
        {
        private:
            using iota_concept_t = ranges::iota_concept<Val>;
            friend range_access;
            using difference_type_ = detail::iota_difference_t<Val>;

            Val from_;
            Val2 to_;
            bool done_ = false;

            Val current() const
            {
                return from_;
            }
            void next()
            {
                if(from_ == to_)
                    done_ = true;
                else
                    ++from_;
            }
            bool done() const
            {
                return done_;
            }
            CONCEPT_REQUIRES(ForwardIota<Val>())
            bool equal(closed_iota_view const &that) const
            {
                return that.from_ == from_;
            }
            CONCEPT_REQUIRES(BidirectionalIota<Val>())
            void prev()
            {
                --from_;
            }
            CONCEPT_REQUIRES(RandomAccessIota<Val>())
            void advance(difference_type_ n)
            {
                RANGES_ASSERT(detail::iota_minus(to_, from_) >= n);
                from_ += n;
            }
            CONCEPT_REQUIRES(RandomAccessIota<Val>())
            difference_type_ distance_to(closed_iota_view const &that) const
            {
                return detail::iota_minus(that.from_, from_);
            }
        public:
            closed_iota_view() = default;
            closed_iota_view(Val from, Val2 to)
              : from_(std::move(from)), to_(std::move(to))
            {}
        };

        namespace view
        {
            struct iota_fn
            {
            private:
                template<typename Val>
                static take_view<iota_view<Val>>
                impl(Val from, Val to, concepts::RandomAccessIota *)
                {
                    return {iota_view<Val>{std::move(from)}, detail::iota_minus(to, from) + 1};
                }
                template<typename Val, typename Val2>
                static closed_iota_view<Val, Val2>
                impl(Val from, Val2 to, concepts::InputIota *)
                {
                    return {std::move(from), std::move(to)};
                }
            public:
                template<typename Val>
                iota_view<Val> operator()(Val value) const
                {
                    CONCEPT_ASSERT(InputIota<Val>());
                    return iota_view<Val>{std::move(value)};
                }
                template<typename Val, typename Val2>
                meta::if_<
                    meta::and_<RandomAccessIota<Val>, Same<Val, Val2>>,
                    take_view<iota_view<Val>>,
                    closed_iota_view<Val, Val2>>
                operator()(Val from, Val2 to) const
                {
                    CONCEPT_ASSERT(EqualityComparable<Val, Val2>());
                    return iota_fn::impl(std::move(from), std::move(to), iota_concept<Val>{});
                }
            };

            /// \sa `iota_fn`
            /// \ingroup group-views
            constexpr iota_fn iota{};

            struct ints_fn
              : iota_view<int>
            {
                using iota_view<int>::iota_view;

                template<typename Val, CONCEPT_REQUIRES_(Integral<Val>())>
                iota_view<Val> operator()(Val value) const
                {
                    return iota_view<Val>{value};
                }
                template<typename Val, CONCEPT_REQUIRES_(Integral<Val>())>
                take_view<iota_view<Val>> operator()(Val from, Val to) const
                {
                    return {iota_view<Val>{from}, detail::iota_minus(to, from) + 1};
                }
            };

            /// \sa `ints_fn`
            /// \ingroup group-views
            constexpr ints_fn ints{};
        }
        /// @}
    }
}

#endif
