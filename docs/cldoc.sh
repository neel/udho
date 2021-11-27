/home/neel/.local/bin/cldoc generate -std=c++17 -ftemplate-backtrace-limit=0 -ferror-limit=0 \
-DBOOST_BEAST_USE_POSIX_FILE=ON -DBOOST_BEAST_USE_POSIX_FADVISE=ON \
-I/usr/lib/clang/13.0.0/include \
-I/usr/lib/gcc/x86_64-pc-linux-gnu/11.1.0/include \
-I/usr/lib/gcc/x86_64-pc-linux-gnu/11.1.0/include-fixed \
-I/usr/include \
-I/usr/local/include \
-I/usr/include/c++/11.1.0 \
-I/usr/include/c++/11.1.0/x86_64-pc-linux-gnu \
-I/usr/include/c++/11.1.0/backward \
-I/home/neel/Projects/udho/includes \
-I/home/neel/Projects/udho/impl \
-I/home/neel/Projects/udho/vendor/certify/include \
-- --output /home/neel/Projects/udho/build/docs/cldoc-docs \
/home/neel/Projects/udho/includes/udho/client.h
