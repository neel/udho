#ifndef UDHO_MANIFOLD_EVALUATION_RESULT_H
#define UDHO_MANIFOLD_EVALUATION_RESULT_H

#include <udho/exceptions/exceptions.h>

namespace udho {
namespace manifold {

/**
 * @ingroup manifold
 * @{
 */


/**
 * @brief A wrapper for storing either a successful result or an exception
 *
 * This class encapsulates the result of an asynchronous operation that can
 * either succeed (returning true) or fail (storing an exception). It provides
 * a unified interface for checking success and rethrowing exceptions.
 *
 * @note Designed to be used in pipeline evaluation where exceptions need to
 *       be propagated across asynchronous boundaries.
 */
class evaluation_result{
    udho::exceptions::captured _capex;
    bool _success;
public:
    evaluation_result(): _success(false) {}                      ///< Default Constructor
    evaluation_result(const evaluation_result&) = default;        ///< Copy constructor
    evaluation_result(evaluation_result&&) = default;             ///< Move constructor
    evaluation_result& operator=(const evaluation_result&) = default;    ///< Copy assignment operator

    /// @brief Construct with an exception
    /// @param capex Captured exception to store
    evaluation_result(udho::exceptions::captured&& capex): _capex(std::move(capex)), _success(false) {}

    evaluation_result(bool success): _success(success) {}

public:
    /// @brief Assign an exception
    /// @param capex Captured exception to store
    /// @return Reference to this object
    evaluation_result& operator=(udho::exceptions::captured&& capex) {
        _capex     = std::move(capex);
        _success   = false;
        return *this;
    }

    /// @brief Assign an exception
    /// @param success Success state to assign
    /// @return Reference to this object
    evaluation_result& operator=(bool success) {
        _success = success;
        if(_success) {
            _capex.reset();
        }
        return *this;
    }

    const udho::exceptions::captured& capex() const {
        return _capex;
    }
public:
    /// @brief Check and propagate exception
    /// @return Always returns true if no exception stored
    /// @throws The stored exception if one exists
    bool operator()() const {
        if(_capex) {
            rethrow();
        }
        return true;
    }
public:
    bool value() const { return _success; }
    bool has_exception() const { return !_capex.empty(); }
public:
    /// @brief Check if operation was successful
    /// @return true if no exception stored
    bool success() const { return !_capex && _success; }
    /// @brief Check if operation failed
    /// @return true if an exception is stored
    bool error() const { return !success(); }
public:
    /// @brief Dereference operator for checking success
    /// @return true if no exception stored
    bool operator*() const { return success(); }
    /// @brief Rethrow the stored exception
    /// @pre error() must be true
    void rethrow() const {
        assert(_capex);
        if(has_exception()) {
            _capex.rethrow();
        }
    }
public:
    /// @brief Boolean conversion for checking success
    /// @return true if no exception stored
    operator bool() const { return success(); }
    /// @brief Negation operator for checking failure
    /// @return true if an exception is stored
    bool operator!() const { return error(); }
};

/**
 * @}
 */


}
}




#endif // UDHO_MANIFOLD_EVALUATION_RESULT_H
