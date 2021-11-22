struct X: activities::activity<X, success_t, failure_t>{
    using activity::activity;
    void operator()() {
        success(success_t{42});
    }
};

struct Y: activities::activity<Y, success_t, failure_t>{
    using activity::activity;
    void operator()() {
        failure(failure_t{24});
    }
};
