/* Copyright (c) 2024. The FSMOD Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <gtest/gtest.h>
#include <xbt.h>

int main(int argc, char **argv) {
    // disable log
    xbt_log_control_set("root.thresh:critical");

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
