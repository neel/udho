#ifndef UDHO_DB_PG_SCHEMA_CONSTRAINTS_H
#define UDHO_DB_PG_SCHEMA_CONSTRAINTS_H

#include <type_traits>
#include <udho/db/pg/schema/fwd.h>
#include <udho/db/pg/constructs/strings.h>
// #include <udho/db/pg/schema/column.h>

namespace udho{
namespace db{
namespace pg{

namespace constraints{

    namespace labels{
        struct primary{};
        struct not_null{};
        struct default_value{};
        struct unique{};
        struct references{};
    }

    namespace ref_policies{
        struct cascade{
            using str = pg::constants::cascade;
        };
        struct restrict{
            using str = pg::constants::restrict;
        };
        struct set_null{
            using str = pg::constants::set_null;
        };
    };

    template <typename FieldT, typename ValueT>
    struct primary: labels::primary{};

    template <typename FieldT, typename ValueT>
    struct not_null: labels::not_null{};

    template <typename FieldT, typename ValueT>
    struct unique: labels::unique{};

    template <typename Constant>
    struct default_{
        template <typename FieldT, typename ValueT>
        struct value: labels::default_value{
            using default_value = Constant;
        };
    };



    template <typename FieldT>
    struct references;

    template <typename ColumnT, typename PolicyT>
    struct references_with_policy{
        template <typename FieldT, typename ValueT>
        struct referenced: labels::references{
            struct foreign_ref{
                using target = ColumnT;
                using policy = PolicyT;

                static constexpr auto target_str(){
                    using namespace ozo::literals;
                    return ColumnT::relation_type::name() + "("_SQL + ColumnT::field_type::unqualified_name() + ")"_SQL;
                }
                static constexpr auto policy_str(){
                    typename PolicyT::str pstr;

                    using namespace ozo::literals;
                    return "on delete "_SQL + ozo::make_query_builder(boost::hana::make_tuple(ozo::make_query_text((pstr))));
                }
                static constexpr auto str(){
                    using namespace ozo::literals;
                    return target_str() + " "_SQL + policy_str();
                }
            };
        };
    };

    template <typename TargetFieldT, typename RelationT>
    struct references<pg::column<TargetFieldT, RelationT>>{
        using column   = pg::column<TargetFieldT, RelationT>;

        template <typename FieldT, typename ValueT>
        using cascade  = typename references_with_policy<column, ref_policies::cascade> ::template referenced<FieldT, ValueT>;
        template <typename FieldT, typename ValueT>
        using restrict = typename references_with_policy<column, ref_policies::restrict>::template referenced<FieldT, ValueT>;
        template <typename FieldT, typename ValueT>
        using set_null = typename references_with_policy<column, ref_policies::set_null>::template referenced<FieldT, ValueT>;
    };

    namespace has{
        template<typename FieldT>
        using primary       = std::is_base_of<pg::constraints::labels::primary,       FieldT>;
        template<typename FieldT>
        using not_null      = std::is_base_of<pg::constraints::labels::not_null,      FieldT>;
        template<typename FieldT>
        using unique        = std::is_base_of<pg::constraints::labels::unique,        FieldT>;
        template<typename FieldT>
        using default_value = std::is_base_of<pg::constraints::labels::default_value, FieldT>;
        template<typename FieldT>
        using references    = std::is_base_of<pg::constraints::labels::references,    FieldT>;
    }

    template <typename FieldT, bool HasReference = pg::constraints::has::references<FieldT>::value>
    struct reference_target;

    template <typename FieldT>
    struct reference_target<FieldT, true>{
        using relation = typename FieldT::referenced::foreign_ref::target::relation_type;
        using field    = typename FieldT::referenced::foreign_ref::target::field_type;
    };

    template <typename FieldT>
    struct reference_target<FieldT, false>{
        using relation = void;
        using field    = void;
    };

    template <typename FieldT, typename ReferencedByT=void, typename... Fields>
    struct referenced_by_helper{
        using target   = typename referenced_by_helper<FieldT, Fields...>::target;
    };

    template <typename ReferencedByT, typename... Fields>
    struct referenced_by_helper<typename ReferencedByT::field_type, ReferencedByT, Fields...>{
        using target   = ReferencedByT;
    };

    template <typename FieldT, typename... Fields>
    struct referenced_by_helper<FieldT, void, Fields...>{
        using target   = void;
    };

    template <typename FieldT, typename... Fields>
    struct referenced_by{
        using target   = typename referenced_by_helper<typename reference_target<FieldT>::field, Fields...>::target;
        using relation = typename target::relation_type;
        using field    = typename target::field_type;
    };

}


}
}
}

#endif // UDHO_DB_PG_SCHEMA_CONSTRAINTS_H
