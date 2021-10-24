auto collector = activities::collect<A1, A2>(ctx);
auto a1 = std::make_shared<A1>(collector);
auto a2 = std::make_shared<A2>(collector);

auto combinator = std::make_shared<activities::combinator<A2, A1>>(a2);
a1->done(combinator);