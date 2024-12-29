#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <udho/view/data.h>
#include <udho/view/meta.h>
#include <udho/view/bridges/lua.h>
#include <udho/view/resources/resource.h>
#include <udho/view/resources/store.h>
#include <boost/variant.hpp>

#include "data.h"

struct exec_result{
    std::uint32_t job_id;
    udho::view::data::bridges::results results;
    std::string output;
    std::size_t waiting = 0;
};

TEST_CASE("Lua Concurrent bridge", "[view][lua][mt]") {
    static char buffer[] = R"TEMPLATE(
<?! vars('d', 'ctx') ?>
<?= udho.utils.thread_id() ?>
<? udho.utils.sleep(1000) ?>
)TEMPLATE";

    student p;

    constexpr const std::size_t nstates = 4;

    udho::view::data::bridges::lua lua{nstates};
    REQUIRE(lua.policy() == udho::view::data::bridges::policy::state_pool);
    lua.init();

    lua.compile(udho::view::resources::tmpl::resource("view", buffer, buffer+sizeof(buffer)), "");

    SECTION("N+1 the call to exec waits") {
        std::vector<exec_result> view_results;
        std::mutex mutex;
        constexpr const std::size_t nthreads = 20;
        std::size_t waiting = nthreads;         //  number of threads waiting at CS

        auto exec = [&lua, &view_results, &mutex, &waiting](std::uint32_t id){ // This id does not imply order
            std::string output;
            udho::view::data::bridges::results results = lua.exec("view", "", nullptr, nullptr, output);
            exec_result er;
            er.job_id = id;
            er.results = results;
            er.output = output;
            std::scoped_lock lock(mutex);
            er.waiting = --waiting;             // one job finished
                                                // hence number of threads waiting for CS should be reduced by one
                                                // er.waiting implies number of threads that were waiting
                                                //            before it exited from the CS
            view_results.push_back(er);
        };

        std::vector<std::thread> threads;
        for (int i = 0; i < nthreads; ++i) {
            threads.emplace_back(std::bind(exec, i));
        }

        for (auto& thread : threads) {
            thread.join();
        }

        CHECK(view_results.size() == nthreads);                         // TEST total number of threads are as expected
        std::set<std::size_t> state_nums;
        std::set<std::string> thread_ids;

        for(const auto& vres: view_results){
            state_nums.insert(vres.results.index());
            thread_ids.insert(vres.output);
        }

        CHECK(state_nums.size() == std::min(nstates, nthreads));        // TEST All states have been used, unless number of jobs are less tthan number of states.
        CHECK(thread_ids.size() == nthreads);                           // TEST All thread ids are different

        std::vector<exec_result> view_results_sorted_by_queue = view_results;
        std::sort(view_results_sorted_by_queue.begin(), view_results_sorted_by_queue.end(), [](const exec_result& a, const exec_result& b) {
            return a.waiting > b.waiting; // All other tasks were waiting when the first job was executed hence the 'waiting' value is the most for the first job
        });

        // Given: nthreads > nstates
        // Given: each view acquires the state for at least 1 second.
        // Therefore: some views will be waiting to get an available state
        // Expectation: waiting time should be proportional to the queue length         ----------------------------- #Ex1
        // Input: view_results -> a list of execution results
        //        Assuming r is the result of job j
        //        ∀ r ∈ view_results, r.waiting implies number of jobs waiting before this job was finished
        //                  if M = pending(j) jobs were observed to be waiting while r finished, then r must not have waited for those M jobs.
        //                  Because r has finished at the time of observation.
        //                  So, if M1 = pending(j1), M2 = pending(j2) and M1 > M2 assuming r1, r2 are results of jobs j1, j2
        //                  then more jobs were observed to be waiting for a free state when j1 finished
        //                  implying r.waiting is inversely proportional to number of jobs acquiring on the same state making j wait
        //        ∀ r ∈ view_results, r.waiting_time() implies duration of waiting before this job was started
        // Expectation:
        //      Given two jobs, j1 and j2 were allocated on the same state (index)
        //      according to @Ex1 we expect
        //          r1.waiting_time() <= r2.waiting_time() if r1.waiting >= r2.waiting

        std::vector<exec_result> view_results_sorted_by_index_asc_waiting_desc = view_results;
        std::sort(view_results_sorted_by_index_asc_waiting_desc.begin(), view_results_sorted_by_index_asc_waiting_desc.end(), [](const exec_result& a, const exec_result& b) {
            if (a.results.index() == b.results.index()) {
                return a.waiting > b.waiting;
            }
            return a.results.index() < b.results.index();
        });
        for (size_t i = 1; i < view_results_sorted_by_index_asc_waiting_desc.size(); ++i) {
            if (view_results_sorted_by_index_asc_waiting_desc[i].results.index() == view_results_sorted_by_index_asc_waiting_desc[i - 1].results.index()) {
                CAPTURE(view_results_sorted_by_index_asc_waiting_desc[i - 1].waiting, view_results_sorted_by_index_asc_waiting_desc[i].waiting, view_results_sorted_by_index_asc_waiting_desc[i - 1].results.wait_time(), view_results_sorted_by_index_asc_waiting_desc[i].results.wait_time());
                REQUIRE(view_results_sorted_by_index_asc_waiting_desc[i - 1].waiting >= view_results_sorted_by_index_asc_waiting_desc[i].waiting);
                REQUIRE(view_results_sorted_by_index_asc_waiting_desc[i - 1].results.wait_time() <= view_results_sorted_by_index_asc_waiting_desc[i].results.wait_time());
            }
        }

        // for(const auto& r: view_results){
        //     std::cout << r.waiting << "," << r.results.index() << "," << r.results.wait_time().count() << std::endl;
        // }

        // std::chrono::nanoseconds max_waiting_time = *(std::max_element(wait_times.begin(), wait_times.end()));
        // for(auto i = 0; i != nthreads; ++i){
        //     using namespace std::chrono_literals;
        //
        //     int batch = i / nstates;
        //     CHECK(wait_times[i] < (((batch+1) * std::chrono::nanoseconds{1s}) + std::chrono::milliseconds{4}) );
        //     int max_batch = (nthreads-1) / nstates;
        //     if(batch < max_batch){
        //         CHECK(wait_times[i] < max_waiting_time);
        //     }
        // }

        // for(const auto& vres: view_results){
        //     std::cout << "job_id: " << vres.job_id << std::endl
        //               << "state:  " << vres.results.index() << std::endl
        //               << "wait:   " << vres.results.wait_time().count() << "ns" << std::endl
        //               << "exec:   " << vres.results.exec_time().count() << "ns" << std::endl
        //               << "queue:  " << vres.waiting << std::endl;
        //     std::cout << "thread: " << vres.output << std::endl;
        //     std::cout << std::endl;
        // }
    }

    SECTION("call to bind waits for all states to be free") {
        std::vector<exec_result> view_results;
        std::mutex mutex;
        constexpr const std::size_t nthreads = nstates;
        std::size_t waiting = nthreads;
        boost::interprocess::interprocess_semaphore semaphore{0};

        auto exec = [&lua, &view_results, &mutex, &waiting, &semaphore](std::uint32_t id){ // This id does not imply order
            semaphore.post();
            std::string output;
            udho::view::data::bridges::results results = lua.exec("view", "", nullptr, nullptr, output);
            exec_result er;
            er.job_id = id;
            er.results = results;
            er.output = output;
            std::scoped_lock lock(mutex);
            er.waiting = --waiting;
            view_results.push_back(er);
        };

        auto exec2 = [&lua, &view_results, &mutex, &waiting, &semaphore, &p](std::uint32_t id){ // This id does not imply order
            for (int i = 0; i < nstates-1; ++i) {
                semaphore.wait();
            }
            std::string output;
            udho::view::data::bridges::results results = lua.exec("view", "", p, p, output);
            exec_result er;
            er.job_id = id;
            er.results = results;
            er.output = output;
            std::scoped_lock lock(mutex);
            er.waiting = --waiting;
            view_results.push_back(er);
        };

        auto exec3 = [&lua, &view_results, &mutex, &waiting, &semaphore, &p](std::uint32_t id){ // This id does not imply order
            for (int i = 0; i < nstates-1; ++i) {
                semaphore.wait();
            }
            std::string output;
            udho::view::data::bridges::results results = lua.exec("view", "", nullptr, nullptr, output);
            exec_result er;
            er.job_id = id;
            er.results = results;
            er.output = output;
            std::scoped_lock lock(mutex);
            er.waiting = --waiting;
            view_results.push_back(er);
        };

        {
            std::vector<std::thread> threads;
            threads.emplace_back(std::bind(exec2, 0));
            for (int i = 1; i < nthreads; ++i) {
                threads.emplace_back(std::bind(exec, i));
            }

            for (auto& thread : threads) {
                thread.join();
            }

            std::vector<exec_result> view_results_sorted_by_queue = view_results;
            std::sort(view_results_sorted_by_queue.begin(), view_results_sorted_by_queue.end(), [](const exec_result& a, const exec_result& b) {
                return a.waiting < b.waiting; // All other tasks were waiting when the first job was executed hence the 'waiting' value is the most for the first job
            });
            std::vector<exec_result> view_results_sorted_by_wait_time = view_results;
            std::sort(view_results_sorted_by_wait_time.begin(), view_results_sorted_by_wait_time.end(), [](const exec_result& a, const exec_result& b) {
                return a.results.wait_time().count() > b.results.wait_time().count(); // All other tasks were waiting when the first job was executed hence the 'waiting' value is the most for the first job
            });

            // The job id 0 is exec2
            // exec2 binds type
            // binding type require all states to be free
            // so it will wait untill all running jobs finish
            // as all other jobs are running parallelly the waiting time of exec2 will not be sum of exec time of the other jobs, it will be greater than the max of other jobs exec times.
            // there will be no job waiting on the queue while exec2 because all other will finish before it.
            // however if we put exec3 in place of exec2 then job 0 may have waiting count > 0 sometimes, because the semaphore only guards the entry section of the lambda.

            std::uint32_t most_delayed_job = view_results_sorted_by_wait_time.front().job_id;
            std::uint32_t least_waiting_list_job = view_results_sorted_by_queue.front().job_id;

            using namespace std::chrono_literals;

            CHECK(most_delayed_job == 0);                         // TEST Job 0 should be delayed most
            CHECK(least_waiting_list_job == 0);                   // TEST Job 0 should not have other jobs pending for it
            CHECK(most_delayed_job == least_waiting_list_job);    // TEST The job that is most delayed should be the job executed last, hence no pending.
            CHECK(view_results_sorted_by_wait_time.front().results.wait_time() > 1s);

            // for(const auto& vres: view_results){
            //     std::cout << "job_id: " << vres.job_id << std::endl
            //               << "state:  " << vres.results.index() << std::endl
            //               << "wait:   " << vres.results.wait_time().count() << "ns" << std::endl
            //               << "exec:   " << vres.results.exec_time().count() << "ns" << std::endl
            //               << "queue:  " << vres.waiting << std::endl;
            //     std::cout << "thread: " << vres.output << std::endl;
            //     std::cout << std::endl;
            // }

        }

        view_results.clear();

        {
            std::vector<std::thread> threads;
            threads.emplace_back(std::bind(exec3, 0));
            for (int i = 1; i < nthreads; ++i) {
                threads.emplace_back(std::bind(exec, i));
            }

            for (auto& thread : threads) {
                thread.join();
            }

            // std::vector<exec_result> view_results_sorted_by_queue = view_results;
            // std::sort(view_results_sorted_by_queue.begin(), view_results_sorted_by_queue.end(), [](const exec_result& a, const exec_result& b) {
            //     return a.waiting < b.waiting; // All other tasks were waiting when the first job was executed hence the 'waiting' value is the most for the first job
            // });
            std::vector<exec_result> view_results_sorted_by_wait_time = view_results;
            std::sort(view_results_sorted_by_wait_time.begin(), view_results_sorted_by_wait_time.end(), [](const exec_result& a, const exec_result& b) {
                return a.results.wait_time().count() > b.results.wait_time().count(); // All other tasks were waiting when the first job was executed hence the 'waiting' value is the most for the first job
            });

            // The job id 0 is exec2
            // exec2 binds type
            // binding type require all states to be free
            // so it will wait untill all running jobs finish
            // as all other jobs are running parallelly the waiting time of exec2 will not be sum of exec time of the other jobs, it will be greater than the max of other jobs exec times.
            // there will be no job waiting on the queue while exec2 because all other will finish before it.
            // however if we put exec3 in place of exec2 then job 0 may have waiting count > 0 sometimes, because the semaphore only guards the entry section of the lambda.

            using namespace std::chrono_literals;

            CHECK(view_results_sorted_by_wait_time.front().results.wait_time() < 1s);

            // for(const auto& vres: view_results){
            //     std::cout << "job_id: " << vres.job_id << std::endl
            //               << "state:  " << vres.results.index() << std::endl
            //               << "wait:   " << vres.results.wait_time().count() << "ns" << std::endl
            //               << "exec:   " << vres.results.exec_time().count() << "ns" << std::endl
            //               << "queue:  " << vres.waiting << std::endl;
            //     std::cout << "thread: " << vres.output << std::endl;
            //     std::cout << std::endl;
            // }

        }


    }

}

