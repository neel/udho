struct success_t{
    int _value;
    inline success_t(): _value(0){}
    inline explicit success_t(int v): _value(v){}
};
struct failure_t{
    int _value;
    inline failure_t(): _value(0){}
    inline explicit failure_t(int v): _value(v){}
};