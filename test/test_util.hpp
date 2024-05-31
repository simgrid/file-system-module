/* Copyright (c) 2024. The FSMOD Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MODULE_FS_TEST_UTIL_H_
#define SIMGRID_MODULE_FS_TEST_UTIL_H_
#include <functional>

static void DO_TEST_WITH_FORK(const std::function<void()> &lambda) {
    pid_t pid = fork();
    if (pid) {
        int exit_code;
        waitpid(pid, &exit_code, 0);
        ASSERT_EQ(exit_code, 0);
    } else {
        lambda();
        exit(::testing::Test::HasFailure() ? 255 : 0);
    }
}

#endif