#include <functional>

void DO_TEST_WITH_FORK(const std::function<void()> &lambda) {
    pid_t pid = fork();
    if (pid) {
        int exit_code;
        waitpid(pid, &exit_code, 0);
        ASSERT_EQ(exit_code, 0);
    } else {
        lambda();
        exit((::testing::Test::HasFailure() ? 255 : 0));
    }
}