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
}


}
}
}

#endif // UDHO_DB_PG_SCHEMA_CONSTRAINTS_H
