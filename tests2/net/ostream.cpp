#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif
#include <udho/net/ostream.h>
#include <boost/beast/_experimental/test/stream.hpp>
#include <udho/utils/encoding.h>
#include <iostream>
#include <boost/thread.hpp>

namespace cdigits{

template <typename TupleT, typename X>
struct tuple_append;

template <typename... Elements, typename X>
struct tuple_append<std::tuple<Elements...>, X>{
    using type = std::tuple<Elements..., X>;
};

template <typename X, typename TupleT>
struct tuple_prepend;

template <typename X, typename... Elements>
struct tuple_prepend<X, std::tuple<Elements...>>{
    using type = std::tuple<X, Elements...>;
};

template <std::size_t N>
struct digits_helper{
    using rest_type = digits_helper<N / 10>;

    static constexpr const std::size_t  count  = 1 + rest_type::count;
    static constexpr const std::uint8_t value  = N % 10;

    using tuple_type = typename tuple_prepend<std::uint8_t, typename rest_type::tuple_type>::type;

    template <std::size_t Idx, std::enable_if_t<Idx == std::size_t(0), bool> = true>
    static constexpr std::uint8_t at() {
        return value;
    }

    template <std::size_t Idx, std::enable_if_t<(Idx > std::size_t(0)), bool> = true>
    static constexpr std::uint8_t at() {
        return rest_type::template at<Idx-1>();
    }

    static constexpr tuple_type tuple() {
        return std::tuple_cat(rest_type::tuple(), std::tuple<std::uint8_t>(value+48));
    }

    static constexpr std::size_t fill(std::uint8_t* ptr) {
        std::size_t c = std::apply([ptr](std::uint8_t v) mutable -> std::size_t {
            *ptr = v;
            return 1+ rest_type::fill(ptr+1);
        }, tuple());
        return c;
    }
};

template <>
struct digits_helper<0> {
    static constexpr const std::size_t count   = 1;
    static constexpr const std::uint8_t value  = 0;

    using tuple_type = std::tuple<>;

    template <std::size_t Idx, std::enable_if_t<Idx == std::size_t(0), bool> = true>
    static constexpr std::uint8_t at() {
        return value;
    }

    static constexpr tuple_type tuple() {
        return tuple_type{};
    }

    static constexpr std::size_t fill(std::uint8_t* ptr) {
        return 0;
    }
};

template <std::size_t N>
struct czp{
    using helper_type = digits_helper<N>;
    static constexpr const std::size_t length = helper_type::count - 1;
    static constexpr const std::size_t max_index = length -1;

    using tuple_type = typename helper_type::tuple_type;

    template <std::size_t Idx>
    static constexpr std::uint8_t at() {
        return helper_type::template at<max_index - Idx>();
    }

    static constexpr tuple_type tuple() { return helper_type::tuple(); }

    static constexpr std::array<char, helper_type::count> data = std::apply([](auto... ds){
        return std::array<char, helper_type::count>{char(ds)..., ' '};
    }, tuple());

    static constexpr std::string_view view{data.data(), helper_type::count};
};

template <typename IntSeq, std::size_t Initial = 0>
struct czp_sequence;

template <std::size_t... Ns, std::size_t Initial>
struct czp_sequence<std::integer_sequence<std::size_t, Ns...>, Initial>{
    using tuple_type = std::tuple<czp<Initial+Ns>...>;
};

}

using stream_type     = boost::beast::test::stream;
using executor_type   = typename stream_type::executor_type;
using strand_type     = boost::asio::strand<executor_type>;
using buffered_stream = udho::net::detail::basic_buffered_ostream<stream_type>;
using queued_stream   = udho::net::detail::basic_queued_ostream<stream_type>;

struct TestType {
    int value;
    friend std::ostream& operator<<(std::ostream& os, const TestType& t) {
        return os << "TestType:" << t.value;
    }
};

template <std::size_t N>
struct multithreaded_io{
    using executor_type = boost::asio::io_context::executor_type;
    using guard_type    = boost::asio::executor_work_guard<executor_type>;

    multithreaded_io(boost::asio::io_context& io): _io(io), _guard(boost::asio::make_work_guard(io)) {
        run();
    }

    void restart() {
        _io.restart();
    }

    void run() {
        for(std::size_t i = 0; i < N; ++i) {
            _threads.create_thread([this]{
                _io.run();
            });
        }
    }

    void reset() {
        _guard.reset();
    }

    void join() {
        reset();
        _threads.join_all();
    }

private:
    boost::asio::io_context& _io;
    boost::thread_group      _threads;
    guard_type               _guard;
};


TEST_CASE("udho manifold basic_buffered_ostream", "[manifold][stream][buffered]") {
    boost::asio::io_context io;
    stream_type stream_in(io);
    stream_type stream_out(io);

    strand_type strand(stream_in.get_executor());

    stream_in.connect(stream_out);

    SECTION("buffered stream") {
        bool is_completed = false;
        udho::net::types::transfer_encoding enc{udho::net::types::transfer::encoding::plain};
        std::size_t finish_callback_called = 0;
        buffered_stream buffered_stream(stream_in, strand, enc,
            [&](boost::system::error_code ec, std::size_t bytes_written){
                INFO("error: " << ec.message());
                CHECK_FALSE(ec);
                CHECK(bytes_written == 6);
            },
            [&](boost::system::error_code ec, std::size_t bytes_written) {
                is_completed = true;
                INFO("error: " << ec.message());
                CHECK_FALSE(ec);
                CHECK(bytes_written == 6);
                ++finish_callback_called;
            }
        );

        buffered_stream.write(std::string("ABC"));
        buffered_stream.write(std::string("DEF"));
        buffered_stream.async_flush();
        io.restart();
        boost::thread_group threads;
        for(std::size_t i = 0; i < 4; ++i) {
            threads.create_thread([&io]{
                io.run();
            });
        }
        threads.join_all();

        CHECK(!is_completed);
        buffered_stream.finish();
        io.restart();
        io.run();
        CHECK(is_completed);
        CHECK(finish_callback_called == 1);
        // std::cout << "output: " << client.str() << std::endl;
    }
}

TEST_CASE("udho manifold basic_queued_ostream", "[manifold][stream][queued]") {
    boost::asio::io_context io;
    stream_type server(io);
    stream_type client(io);

    strand_type strand(server.get_executor());

    server.connect(client);

    SECTION("queued stream plain - write while pumping") {
        bool is_completed = false;
        udho::net::types::transfer_encoding enc{udho::net::types::transfer::encoding::plain};
        queued_stream queued_stream(server, strand, enc,
            [&](boost::system::error_code ec, std::size_t bytes_written) {
                is_completed = true;
                INFO("error: " << ec.message());
                CHECK_FALSE(ec);
                CHECK(bytes_written == 17);
            }
        );


        const static std::string static_str = "0123456"; // static embedded assets such as js or images etc..

        queued_stream.write(static_str);                 // pumping started
        queued_stream.write(std::string("ABC"));         // write while pumping
        queued_stream.write(std::string("ABC"));         // write while pumping
        queued_stream.write(std::string("DEFG"));        // write while pumping
        io.restart();
        boost::thread_group threads;
        for(std::size_t i = 0; i < 4; ++i) {
            threads.create_thread([&io]{
                io.run();
            });
        }
        threads.join_all();

        CHECK(!is_completed);
        CHECK(udho::utils::encode::base16(client.str()) == "3031323334353641424341424344454647");

        io.restart();
        queued_stream.finish();
        boost::thread_group threads2;
        for(std::size_t i = 0; i < 4; ++i) {
            threads2.create_thread([&io]{
                io.run();
            });
        }
        threads2.join_all();

        CHECK(is_completed);
        // std::cout << "output: " << client.str() << std::endl;
    }

    SECTION("queued stream - write while pumping") {
        // std::cout << std::endl;
        bool is_completed = false;
        udho::net::types::transfer_encoding enc{udho::net::types::transfer::encoding::chunked};
        queued_stream queued_stream(server, strand, enc,
            [&](boost::system::error_code ec, std::size_t bytes_written) {
                is_completed = true;
                INFO("error: " << ec.message());
                CHECK_FALSE(ec);
                CHECK(bytes_written == /*size ascii 3+crlf*/3+ /*ABC*/3 + /*crlf*/2 + /*size ascii 4+crlf*/3+ /*DEFG*/4 + /*crlf*/2 + /*crlf+0+crlf*/5);
            }
        );

        queued_stream.write(std::string("ABC"));        // pumping started
        queued_stream.write(std::string("DEFG"));       // write while pumping
        io.restart();
        boost::thread_group threads;
        for(std::size_t i = 0; i < 4; ++i) {
            threads.create_thread([&io]{
                io.run();
            });
        }
        threads.join_all();

        CHECK(!is_completed);
        CHECK(udho::utils::encode::base16(client.str()) == "330d0a4142430d0a340d0a444546470d0a");

        io.restart();
        queued_stream.finish();
        boost::thread_group threads2;
        for(std::size_t i = 0; i < 4; ++i) {
            threads2.create_thread([&io]{
                io.run();
            });
        }
        threads2.join_all();

        CHECK(is_completed);
        CHECK(udho::utils::encode::base16(client.str()) == "330d0a4142430d0a340d0a444546470d0a300d0a0d0a");
    }

    SECTION("queued stream - strict ordering - plain") {
        // std::cout << std::endl;
        io.restart();
        bool is_completed = false;
        udho::net::types::transfer_encoding enc{udho::net::types::transfer::encoding::plain};

        std::string expected_output;
        for(std::size_t i = 0; i < 1100; ++i) {
            std::string num = std::to_string(i) + " ";
            expected_output += num;
        }
        std::size_t expected_bytes = expected_output.size();

        queued_stream queued_stream(server, strand, enc,
            [&](boost::system::error_code ec, std::size_t bytes_written) {
                is_completed = true;
                INFO("error: " << ec.message());
                CHECK_FALSE(ec);
                CHECK(bytes_written == expected_bytes);
            }
        );

        for(std::size_t i = 0; i != 1000; ++i) {
            queued_stream.write(std::to_string(i)+" ");
        }
        cdigits::czp_sequence<std::make_index_sequence<100>, 1000>::tuple_type seqtup;
        std::apply([&queued_stream](auto... czps)  {
            (queued_stream.write(czps.view.data(), czps.view.size()), ...);
        }, seqtup);
        queued_stream.finish();

        boost::thread_group threads;
        for(std::size_t i = 0; i < 4; ++i) {
            threads.create_thread([&io]{
                io.run();
            });
        }
        threads.join_all();

        std::string output = client.str();
        // expected output
        // 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 91 92 93 94 95 96 97 98 99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122 123 124 125 126 127 128 129 130 131 132 133 134 135 136 137 138 139 140 141 142 143 144 145 146 147 148 149 150 151 152 153 154 155 156 157 158 159 160 161 162 163 164 165 166 167 168 169 170 171 172 173 174 175 176 177 178 179 180 181 182 183 184 185 186 187 188 189 190 191 192 193 194 195 196 197 198 199 200 201 202 203 204 205 206 207 208 209 210 211 212 213 214 215 216 217 218 219 220 221 222 223 224 225 226 227 228 229 230 231 232 233 234 235 236 237 238 239 240 241 242 243 244 245 246 247 248 249 250 251 252 253 254 255 256 257 258 259 260 261 262 263 264 265 266 267 268 269 270 271 272 273 274 275 276 277 278 279 280 281 282 283 284 285 286 287 288 289 290 291 292 293 294 295 296 297 298 299 300 301 302 303 304 305 306 307 308 309 310 311 312 313 314 315 316 317 318 319 320 321 322 323 324 325 326 327 328 329 330 331 332 333 334 335 336 337 338 339 340 341 342 343 344 345 346 347 348 349 350 351 352 353 354 355 356 357 358 359 360 361 362 363 364 365 366 367 368 369 370 371 372 373 374 375 376 377 378 379 380 381 382 383 384 385 386 387 388 389 390 391 392 393 394 395 396 397 398 399 400 401 402 403 404 405 406 407 408 409 410 411 412 413 414 415 416 417 418 419 420 421 422 423 424 425 426 427 428 429 430 431 432 433 434 435 436 437 438 439 440 441 442 443 444 445 446 447 448 449 450 451 452 453 454 455 456 457 458 459 460 461 462 463 464 465 466 467 468 469 470 471 472 473 474 475 476 477 478 479 480 481 482 483 484 485 486 487 488 489 490 491 492 493 494 495 496 497 498 499 500 501 502 503 504 505 506 507 508 509 510 511 512 513 514 515 516 517 518 519 520 521 522 523 524 525 526 527 528 529 530 531 532 533 534 535 536 537 538 539 540 541 542 543 544 545 546 547 548 549 550 551 552 553 554 555 556 557 558 559 560 561 562 563 564 565 566 567 568 569 570 571 572 573 574 575 576 577 578 579 580 581 582 583 584 585 586 587 588 589 590 591 592 593 594 595 596 597 598 599 600 601 602 603 604 605 606 607 608 609 610 611 612 613 614 615 616 617 618 619 620 621 622 623 624 625 626 627 628 629 630 631 632 633 634 635 636 637 638 639 640 641 642 643 644 645 646 647 648 649 650 651 652 653 654 655 656 657 658 659 660 661 662 663 664 665 666 667 668 669 670 671 672 673 674 675 676 677 678 679 680 681 682 683 684 685 686 687 688 689 690 691 692 693 694 695 696 697 698 699 700 701 702 703 704 705 706 707 708 709 710 711 712 713 714 715 716 717 718 719 720 721 722 723 724 725 726 727 728 729 730 731 732 733 734 735 736 737 738 739 740 741 742 743 744 745 746 747 748 749 750 751 752 753 754 755 756 757 758 759 760 761 762 763 764 765 766 767 768 769 770 771 772 773 774 775 776 777 778 779 780 781 782 783 784 785 786 787 788 789 790 791 792 793 794 795 796 797 798 799 800 801 802 803 804 805 806 807 808 809 810 811 812 813 814 815 816 817 818 819 820 821 822 823 824 825 826 827 828 829 830 831 832 833 834 835 836 837 838 839 840 841 842 843 844 845 846 847 848 849 850 851 852 853 854 855 856 857 858 859 860 861 862 863 864 865 866 867 868 869 870 871 872 873 874 875 876 877 878 879 880 881 882 883 884 885 886 887 888 889 890 891 892 893 894 895 896 897 898 899 900 901 902 903 904 905 906 907 908 909 910 911 912 913 914 915 916 917 918 919 920 921 922 923 924 925 926 927 928 929 930 931 932 933 934 935 936 937 938 939 940 941 942 943 944 945 946 947 948 949 950 951 952 953 954 955 956 957 958 959 960 961 962 963 964 965 966 967 968 969 970 971 972 973 974 975 976 977 978 979 980 981 982 983 984 985 986 987 988 989 990 991 992 993 994 995 996 997 998 999 1000 1001 1002 1003 1004 1005 1006 1007 1008 1009 1010 1011 1012 1013 1014 1015 1016 1017 1018 1019 1020 1021 1022 1023 1024 1025 1026 1027 1028 1029 1030 1031 1032 1033 1034 1035 1036 1037 1038 1039 1040 1041 1042 1043 1044 1045 1046 1047 1048 1049 1050 1051 1052 1053 1054 1055 1056 1057 1058 1059 1060 1061 1062 1063 1064 1065 1066 1067 1068 1069 1070 1071 1072 1073 1074 1075 1076 1077 1078 1079 1080 1081 1082 1083 1084 1085 1086 1087 1088 1089 1090 1091 1092 1093 1094 1095 1096 1097 1098 1099
        // std::cout << output << std::endl;
        CHECK(output == expected_output);

        CHECK(is_completed);
    }

    SECTION("queued stream - strict ordering - chunked") {
        // std::cout << std::endl;
        io.restart();
        bool is_completed = false;
        udho::net::types::transfer_encoding enc{udho::net::types::transfer::encoding::chunked};

        std::string expected_output;
        for(std::size_t i = 0; i < 1100; ++i) {
            std::string num = std::to_string(i) + " ";
            std::array<char, 20> hex_buf{};
            auto result = std::to_chars(hex_buf.data(), hex_buf.data() + hex_buf.size(), num.size(), 16);
            std::string hex_size(hex_buf.data(), result.ptr);
            expected_output += hex_size + "\r\n" + num + "\r\n";
        }
        expected_output += "0\r\n\r\n";
        std::size_t expected_bytes = expected_output.size();

        queued_stream queued_stream(server, strand, enc,
            [&](boost::system::error_code ec, std::size_t bytes_written) {
                is_completed = true;
                INFO("error: " << ec.message());
                CHECK_FALSE(ec);
                CHECK(bytes_written == expected_bytes);
            }
        );

        for(std::size_t i = 0; i != 1000; ++i) {
            queued_stream.write(std::to_string(i)+" ");
        }
        cdigits::czp_sequence<std::make_index_sequence<100>, 1000>::tuple_type seqtup;
        std::apply([&queued_stream](auto... czps)  {
            (queued_stream.write(czps.view.data(), czps.view.size()), ...);
        }, seqtup);
        queued_stream.finish();

        boost::thread_group threads;
        for(std::size_t i = 0; i < 4; ++i) {
            threads.create_thread([&io]{
                io.run();
            });
        }
        threads.join_all();

        std::string output = client.str();
        // expected output
        // 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 91 92 93 94 95 96 97 98 99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122 123 124 125 126 127 128 129 130 131 132 133 134 135 136 137 138 139 140 141 142 143 144 145 146 147 148 149 150 151 152 153 154 155 156 157 158 159 160 161 162 163 164 165 166 167 168 169 170 171 172 173 174 175 176 177 178 179 180 181 182 183 184 185 186 187 188 189 190 191 192 193 194 195 196 197 198 199 200 201 202 203 204 205 206 207 208 209 210 211 212 213 214 215 216 217 218 219 220 221 222 223 224 225 226 227 228 229 230 231 232 233 234 235 236 237 238 239 240 241 242 243 244 245 246 247 248 249 250 251 252 253 254 255 256 257 258 259 260 261 262 263 264 265 266 267 268 269 270 271 272 273 274 275 276 277 278 279 280 281 282 283 284 285 286 287 288 289 290 291 292 293 294 295 296 297 298 299 300 301 302 303 304 305 306 307 308 309 310 311 312 313 314 315 316 317 318 319 320 321 322 323 324 325 326 327 328 329 330 331 332 333 334 335 336 337 338 339 340 341 342 343 344 345 346 347 348 349 350 351 352 353 354 355 356 357 358 359 360 361 362 363 364 365 366 367 368 369 370 371 372 373 374 375 376 377 378 379 380 381 382 383 384 385 386 387 388 389 390 391 392 393 394 395 396 397 398 399 400 401 402 403 404 405 406 407 408 409 410 411 412 413 414 415 416 417 418 419 420 421 422 423 424 425 426 427 428 429 430 431 432 433 434 435 436 437 438 439 440 441 442 443 444 445 446 447 448 449 450 451 452 453 454 455 456 457 458 459 460 461 462 463 464 465 466 467 468 469 470 471 472 473 474 475 476 477 478 479 480 481 482 483 484 485 486 487 488 489 490 491 492 493 494 495 496 497 498 499 500 501 502 503 504 505 506 507 508 509 510 511 512 513 514 515 516 517 518 519 520 521 522 523 524 525 526 527 528 529 530 531 532 533 534 535 536 537 538 539 540 541 542 543 544 545 546 547 548 549 550 551 552 553 554 555 556 557 558 559 560 561 562 563 564 565 566 567 568 569 570 571 572 573 574 575 576 577 578 579 580 581 582 583 584 585 586 587 588 589 590 591 592 593 594 595 596 597 598 599 600 601 602 603 604 605 606 607 608 609 610 611 612 613 614 615 616 617 618 619 620 621 622 623 624 625 626 627 628 629 630 631 632 633 634 635 636 637 638 639 640 641 642 643 644 645 646 647 648 649 650 651 652 653 654 655 656 657 658 659 660 661 662 663 664 665 666 667 668 669 670 671 672 673 674 675 676 677 678 679 680 681 682 683 684 685 686 687 688 689 690 691 692 693 694 695 696 697 698 699 700 701 702 703 704 705 706 707 708 709 710 711 712 713 714 715 716 717 718 719 720 721 722 723 724 725 726 727 728 729 730 731 732 733 734 735 736 737 738 739 740 741 742 743 744 745 746 747 748 749 750 751 752 753 754 755 756 757 758 759 760 761 762 763 764 765 766 767 768 769 770 771 772 773 774 775 776 777 778 779 780 781 782 783 784 785 786 787 788 789 790 791 792 793 794 795 796 797 798 799 800 801 802 803 804 805 806 807 808 809 810 811 812 813 814 815 816 817 818 819 820 821 822 823 824 825 826 827 828 829 830 831 832 833 834 835 836 837 838 839 840 841 842 843 844 845 846 847 848 849 850 851 852 853 854 855 856 857 858 859 860 861 862 863 864 865 866 867 868 869 870 871 872 873 874 875 876 877 878 879 880 881 882 883 884 885 886 887 888 889 890 891 892 893 894 895 896 897 898 899 900 901 902 903 904 905 906 907 908 909 910 911 912 913 914 915 916 917 918 919 920 921 922 923 924 925 926 927 928 929 930 931 932 933 934 935 936 937 938 939 940 941 942 943 944 945 946 947 948 949 950 951 952 953 954 955 956 957 958 959 960 961 962 963 964 965 966 967 968 969 970 971 972 973 974 975 976 977 978 979 980 981 982 983 984 985 986 987 988 989 990 991 992 993 994 995 996 997 998 999 1000 1001 1002 1003 1004 1005 1006 1007 1008 1009 1010 1011 1012 1013 1014 1015 1016 1017 1018 1019 1020 1021 1022 1023 1024 1025 1026 1027 1028 1029 1030 1031 1032 1033 1034 1035 1036 1037 1038 1039 1040 1041 1042 1043 1044 1045 1046 1047 1048 1049 1050 1051 1052 1053 1054 1055 1056 1057 1058 1059 1060 1061 1062 1063 1064 1065 1066 1067 1068 1069 1070 1071 1072 1073 1074 1075 1076 1077 1078 1079 1080 1081 1082 1083 1084 1085 1086 1087 1088 1089 1090 1091 1092 1093 1094 1095 1096 1097 1098 1099
        // std::cout << output << std::endl;
        CHECK(output == expected_output);

        CHECK(is_completed);
    }
}

TEST_CASE("udho manifold composite stream no switching", "[manifold][stream][buffered]") {
    boost::asio::io_context io;
    stream_type server(io);
    stream_type client(io);

    SECTION("composite stream - write after finish should not crash") {
        multithreaded_io<4> mio(io);

        server.connect(client);

        int callback_count = 0;
        udho::net::basic_ostream<stream_type> ostream(server,
            [&](boost::system::error_code ec, std::size_t) {
                callback_count++;
                CHECK_FALSE(ec);
            }
        );
        ostream.encoding(udho::net::types::transfer::encoding::plain);

        ostream.write(std::string("ABC"));
        ostream.finish();

        ostream.write(std::string("SHOULD_NOT_APPEAR"));

        mio.join();

        CHECK(callback_count == 1);
        CHECK(client.str().find("SHOULD_NOT_APPEAR") == std::string::npos);
    }

    SECTION("composite stream - multiple finish calls are idempotent") {
        multithreaded_io<4> mio(io);

        server.connect(client);

        int callback_count = 0;
        udho::net::basic_ostream<stream_type> ostream(server,
            [&](boost::system::error_code ec, std::size_t) {
                callback_count++;
                CHECK_FALSE(ec);
            }
        );
        ostream.encoding(udho::net::types::transfer::encoding::plain);

        ostream.write(std::string("ABC"));
        ostream.finish();
        ostream.finish(); // Second finish should not cause issues
        ostream.finish(); // Third finish

        mio.join();

        CHECK(callback_count == 1); // Only one completion callback
    }

    SECTION("composite stream with chunked encoding - only buffered") {
        multithreaded_io<4> mio(io);

        server.connect(client);

        bool completed = false;
        udho::net::basic_ostream<stream_type> ostream(server,
            [&](boost::system::error_code ec, std::size_t) {
                completed = true;
                CHECK_FALSE(ec);
            }
        );
        ostream.encoding(udho::net::types::transfer::encoding::chunked);

        ostream.write(std::string("All buffered"));
        ostream.finish(); // Finish without disabling buffering

        mio.join();

        CHECK(completed);
        std::string output = client.str();
        CHECK(output.find("c\r\nAll buffered\r\n") != std::string::npos); // "All buffered" is 12 chars = C in hex
        CHECK(output.find("0\r\n\r\n") != std::string::npos);
    }
}

TEST_CASE("udho manifold composite stream switching", "[manifold][stream][buffered][queued]") {
    boost::asio::io_context io;
    stream_type stream_in(io);
    stream_type stream_out(io);

    SECTION("happy path") {
        io.restart();
        auto guard = boost::asio::make_work_guard(io);
        boost::thread_group threads;
        for(std::size_t i = 0; i < 4; ++i) {
            threads.create_thread([&io]{
                io.run();
            });
        }

        stream_in.connect(stream_out);
        bool is_completed = false;
        udho::net::basic_ostream<stream_type> ostream(stream_in,
           [&](boost::system::error_code ec, std::size_t bytes_written) {
               is_completed = true;
               // std::cout << "bytes_written " << bytes_written << std::endl;
               INFO("error: " << ec.message());
               CHECK_FALSE(ec);
           }
        );
        ostream.encoding(udho::net::types::transfer::encoding::plain);

        const static std::string static_str = "0123456"; // static embedded assets such as js or images etc..
        ostream.write(std::string("ABC"));         // goes to buffered stream
        ostream.disable_buffering();
        ostream.write(static_str);                 // pumping started
        ostream.write(std::string("ABC"));         // write while pumping
        ostream.write(std::string("DEFG"));        // write while pumping

        guard.reset();
        threads.join_all();

        CHECK(!is_completed);
        // std::cout << "output: " << client.str() << std::endl;

        io.restart();
        ostream.finish();
        for(std::size_t i = 0; i < 4; ++i) {
            threads.create_thread([&io]{
                io.run();
            });
        }
        threads.join_all();

        CHECK(is_completed);
        // std::cout << "output: " << client.str() << std::endl;
    }

    SECTION("composite stream - empty writes") {
        multithreaded_io<1> mio(io);

        stream_in.connect(stream_out);

        bool completed = false;
        udho::net::basic_ostream<stream_type> ostream(stream_in,
           [&](boost::system::error_code ec, std::size_t bytes_written) {
                completed = true;
                CHECK_FALSE(ec);
                CHECK(bytes_written == 52);
           }
        );
        ostream.encoding(udho::net::types::transfer::encoding::plain);

        // Empty string write
        ostream.write(std::string(""));
        ostream.write(udho::utils::string_view(""));
        ostream.disable_buffering();
        ostream.write(std::string("")); // Empty in queued mode
        ostream.finish();

        mio.join();

        CHECK(completed);
        // Should still have headers
        std::string output = stream_out.str();
        std::string expected_output = "HTTP/1.1 200 OK\r\n"
                                      "Transfer-Encoding: chunked\r\n"   // disable_buffering
                                      "\r\n"
                                      "0"
                                      "\r\n"
                                      "\r\n"
        ;
        CHECK(output == expected_output);
    }

    SECTION("composite stream - large writes that exceed typical buffer") {
        multithreaded_io<4> mio(io);

        stream_in.connect(stream_out);

        const std::size_t large_size = 1024 * 1024; // 1MB
        std::string large_data(large_size, 'X');

        bool completed = false;
        udho::net::basic_ostream<stream_type> ostream(stream_in,
            [&](boost::system::error_code ec, std::size_t bytes) {
                completed = true;
                CHECK_FALSE(ec);
                CHECK(bytes > large_size);
            }
        );
        ostream.encoding(udho::net::types::transfer::encoding::plain);

        ostream.disable_buffering();
        ostream.write(large_data);
        ostream.finish();

        mio.join();

        CHECK(completed);
        CHECK(stream_out.str().size() > large_size);
    }

    SECTION("composite stream - concurrent writes from multiple threads") {
        multithreaded_io<4> mio(io);

        stream_in.connect(stream_out);

        std::atomic<int> write_count{0};
        std::atomic<int> callback_count{0};
        const int total_writes = 100;

        udho::net::basic_ostream<stream_type> ostream(stream_in,
            [&](boost::system::error_code ec, std::size_t) {
                callback_count++;
                CHECK_FALSE(ec);
            }
        );
        ostream.encoding(udho::net::types::transfer::encoding::plain);

        ostream.disable_buffering();

        // Multiple threads writing concurrently
        boost::thread_group writers;
        for (int i = 0; i < 4; ++i) {
            writers.create_thread([&]() {
                for (int j = 0; j < total_writes/4; ++j) {
                    ostream.write(std::string("ThreadWrite"));
                    write_count++;
                }
            });
        }

        writers.join_all();
        ostream.finish();

        mio.join();

        CHECK(write_count == total_writes);
        CHECK(callback_count == 1);
    }

    SECTION("composite stream - rapid disable_buffering calls") {
        multithreaded_io<4> mio(io);

        stream_in.connect(stream_out);

        bool completed = false;
        udho::net::basic_ostream<stream_type> ostream(stream_in,
            [&](boost::system::error_code ec, std::size_t) {
                completed = true;
                CHECK_FALSE(ec);
            }
        );

        ostream.encoding(udho::net::types::transfer::encoding::plain);

        // Call disable_buffering multiple times (should be idempotent after first)
        ostream.disable_buffering();
        ostream.disable_buffering(); // Second call should be no-op
        ostream.disable_buffering(); // Third call

        ostream.write(std::string("Test"));
        ostream.finish();

        mio.join();

        CHECK(completed);
    }

    SECTION("composite stream with chunked encoding - buffered then queued") {
        multithreaded_io<4> mio(io);

        stream_in.connect(stream_out);

        bool completed = false;
        udho::net::basic_ostream<stream_type> ostream(stream_in,
        [&](boost::system::error_code ec, std::size_t bytes) {
                completed = true;
                CHECK_FALSE(ec);
                // Calculate expected bytes: headers + chunked data + terminal chunk
                CHECK(bytes > 0);
            }
        );
        ostream.encoding(udho::net::types::transfer::encoding::chunked);

        // Write in buffered mode
        ostream.write(std::string("Buffered"));
        ostream.disable_buffering();

        // Write in queued mode
        ostream.write(std::string("Queued"));
        ostream.write(std::string("Data"));

        ostream.finish();

        mio.join();

        CHECK(completed);
        std::string output = stream_out.str();
        CHECK(output.find("HTTP/1.1 200 OK") != std::string::npos);
        CHECK(output.find("0\r\n\r\n") != std::string::npos); // Terminal chunk
    }

    SECTION("composite stream - mixed write types (string, string_view, char*)") {
        multithreaded_io<4> mio(io);

        stream_in.connect(stream_out);

        bool completed = false;
        udho::net::basic_ostream<stream_type> ostream(stream_in,
            [&](boost::system::error_code ec, std::size_t) {
                completed = true;
                CHECK_FALSE(ec);
            }
        );
        ostream.encoding(udho::net::types::transfer::encoding::plain);

        // Test all write overloads
        ostream.write(std::string("String"));                    // std::string&&

        std::string temp = "Temporary";
        ostream.write(temp);                                     // udho::utils::string_view

        const char* cstr = "CString";
        ostream.write(cstr, strlen(cstr), false);                // const char*, size_t

        ostream.write(TestType{42});

        ostream.disable_buffering();
        ostream.finish();

        mio.join();

        CHECK(completed);
        std::string output = stream_out.str();
        CHECK(output.find("String") != std::string::npos);
        CHECK(output.find("Temporary") != std::string::npos);
        CHECK(output.find("CString") != std::string::npos);
        CHECK(output.find("TestType:42") != std::string::npos);
    }

    SECTION("composite stream - sequence of many small writes") {
        multithreaded_io<4> mio(io);

        stream_in.connect(stream_out);

        const int num_writes = 1000;
        std::atomic<int> writes_done{0};

        bool completed = false;
        udho::net::basic_ostream<stream_type> ostream(stream_in,
           [&](boost::system::error_code ec, std::size_t) {
               completed = true;
               CHECK_FALSE(ec);
           }
        );
        ostream.encoding(udho::net::types::transfer::encoding::plain);

        ostream.disable_buffering();

        // Many small writes
        for (int i = 0; i < num_writes; ++i) {
            ostream.write(std::string("W")); // Single character
            writes_done++;
        }

        ostream.finish();

        mio.join();

        CHECK(writes_done == num_writes);
        CHECK(completed);
        std::string output = stream_out.str();
        CHECK(output.size() > num_writes);
    }

    SECTION("composite stream - stress test with mixed operations") {
        multithreaded_io<4> mio(io);

        stream_in.connect(stream_out);

        std::atomic<int> operations_completed{0};
        const int total_operations = 50;

        udho::net::basic_ostream<stream_type> ostream(stream_in,
            [&](boost::system::error_code ec, std::size_t) {
                operations_completed++;
                CHECK_FALSE(ec);
            }
        );

        ostream.encoding(udho::net::types::transfer::encoding::chunked);

        // Mix of operations
        for (int i = 0; i < total_operations; ++i) {
            if (i == 10) ostream.disable_buffering();
            if (i == 25) {
                // Write a larger chunk
                ostream.write(std::string(1000, 'X'));
            } else {
                ostream.write(std::to_string(i));
            }

            // Occasionally call finish and check it's handled
            if (i == total_operations - 1) {
                ostream.finish();
            }
        }

        mio.join();

        // Should complete without crashing
        CHECK(operations_completed == 1); // Only one completion callback
    }
}
