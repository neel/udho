struct TimerActivity: udho::activity<TimerActivity, success_t, failure_t>{
    boost::asio::deadline_timer _timer;
    
    template <typename CollectorT>
    A1(CollectorT c, boost::asio::io_context& io): activity(c), _timer(io){}
    
    void operator()(){
        _timer.expires_from_now(boost::posix_time::seconds(1));
        _timer.async_wait(boost::bind(&TimerActivity::finished, self(), boost::asio::placeholders::error));
    }
    
    void finished(const boost::system::error_code& e){
        if(!e){
            success(success_t{42});
        }else{
            failure(failure_t(24));
        }
    }
};