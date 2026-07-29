// SPDX-FileCopyrightText: 2024 Neel Basu <email>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_VIEW_RESULTS_H
#define UDHO_VIEW_RESULTS_H

#include <chrono>
#include <cstdint>

namespace udho{
namespace view{
namespace data{
namespace bridges{

/**
 * @brief results of view execution
 * @ingroup DoxyG_view
 * @details Intended to be constructed by a view bridge::exec method.
 */
struct results{
    using time_type = std::chrono::time_point<std::chrono::system_clock>;

    /**
     * @brief constructs a result object
     * start time is initialized at construction. The caller needs to update the end time, otherwise the results will remain invalid.
     */
    inline results(): _size(0), _queued(std::chrono::system_clock::now()), _index(-1) {}
    inline bool valid() const { return _end > _start && _start > _queued; }
    /**
     * @brief sets the execution start time along with the index of the state
     * @note The bridge first creates an empty results.
     *       Bridge calls the start method just before the view enters the execution critical section.
     * @pre waiting for type binding critical section
     * @pre waiting for a state to be free
     * @pre a free state has been found for executing the view
     * @post the execution critical section begins
     * @param index index of the state on which the view was/will be invoked.
     */
    inline void start(std::int32_t index){
        assert(index >= 0);
        _index = index;
        _start = std::chrono::system_clock::now();

    }
    /**
     * @brief finalizes the result object by setting the size of the output.
     * Current time is set as the end time.
     * @param size size of output
     */
    inline void finish(std::size_t size){
        _size = size;
        _end  = std::chrono::system_clock::now();
    }
    /**
     * @brief time duration of waiting before a state was allocated.
     */
    inline std::chrono::nanoseconds wait_time() const { return _start - _queued; }
    inline std::chrono::nanoseconds exec_time() const { return _end   - _start;  }
    inline std::size_t size()  const { return _size;  }
    inline std::size_t index() const { return _index; }

    private:
        std::size_t _size;
        time_type   _queued;
        time_type   _start;
        time_type   _end;
        std::int32_t _index;
};

}
}
}
}

#endif // UDHO_VIEW_RESULTS_H
