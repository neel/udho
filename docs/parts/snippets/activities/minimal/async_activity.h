struct A: activities::activity<A, success_t, failure_t>{
    using activity::activity;
    void operator()() {
        SomeAsyncOperation op;
        op.on_finish(std::bind(&X::finish, self(), _1));
        op.run();
    }
    void finish(int answer){
        success(success_t{answer});
    }
};
