#include <gtest/gtest.h>
#include <xbt.h>

int main(int argc, char **argv) {

    // disable log
    xbt_log_control_set("root.thresh:critical");

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
