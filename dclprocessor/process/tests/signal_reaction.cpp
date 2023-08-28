#include "gtest/gtest.h"

extern "C" {
#include "process/signal_reaction.h"
extern pthread_t *signal_thread_id_ptr;
}

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

TEST(signal_reaction, signal_thread_init_destroy) {
    int exit_flag = 0;
    signal_thread_init(&exit_flag);
    set_signal_reactions();
    signal_thread_destroy();
}

namespace {
    void reactionTest(int signal, int expectedFlag) {
        int flag = 0;
        signal_thread_init(&flag);
        set_signal_reactions();
        pthread_kill(*signal_thread_id_ptr, signal);
        std::this_thread::sleep_for(1s);
        signal_thread_destroy();
        ASSERT_EQ(flag, expectedFlag);
    }
}

TEST(signal_reaction, reaction_SIGINT) {
    reactionTest(SIGINT, 1);
}

TEST(signal_reaction, reaction_SIGTERM) {
    reactionTest(SIGTERM, 1);
}

TEST(signal_reaction, reaction_SIGCHLD) {
    reactionTest(SIGCHLD, 0);
}

TEST(signal_reaction, reaction_SIGABRT) {
    ASSERT_DEATH({
                     reactionTest(SIGABRT, 1);
                 }, "signal 6 received");
}

TEST(signal_reaction, reaction_SIGFPE) {
    ASSERT_DEATH({
                     reactionTest(SIGFPE, 1);
                 }, "signal 8 received");
}

TEST(signal_reaction, reaction_SIGILL) {
    ASSERT_DEATH({
                     reactionTest(SIGILL, 1);
                 }, "signal 4 received");
}
