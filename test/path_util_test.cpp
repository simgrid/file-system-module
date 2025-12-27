/* Copyright (c) 2024-2026. The FSMOD Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <gtest/gtest.h>
#include <iostream>
#include "fsmod/PathUtil.hpp"

namespace sgfs=simgrid::fsmod;

// Ugly macro, but useful for these specific tests for knowing what's happening
#define MY_ASSERT_EQ(lhs, rhs, message) if ((lhs) != (rhs)) ASSERT_EQ((lhs), (rhs)) \
    << "Test failed on input string: " << (message)

class PathUtilTest : public ::testing::Test {
};

TEST_F(PathUtilTest, PathSimplification)  {
    std::vector<std::pair<std::string, std::string>> input_output = {
            {"////", "/"},
            {"////foo", "/foo"},
            {"/////foo", "/foo"},
            {"../../../././././/////foo", "/foo"},
            {"../../", "/"},
            {"/../../","/"},
            {"foo/bar/../", "/foo"},
            {"/////foo/////././././bar/../bar/..///../../", "/"},
            {"./foo/.../........", "/foo/.../........"},
            {"./foo/", "/foo"},
            {"./foo", "/foo"}
    };

    for (const auto &[input, output] : input_output) {
        MY_ASSERT_EQ(sgfs::PathUtil::simplify_path_string(input), output, input);
    }
}


TEST_F(PathUtilTest, SplitPath) {
    std::vector<std::pair<std::string, std::pair<std::string, std::string>>> input_output = {
            {"/a/b/c/d",                     {"/a/b/c", "d"}},
            {"/a/b////c/d",                  {"/a/b/c", "d"}},
            {"/",                            {"/", ""}},
            {"a",                            {"/", "a"}},
    };
    for (const auto& [input, output]: input_output) {
        auto simplified_path = sgfs::PathUtil::simplify_path_string(input);
        auto [dir, file] = sgfs::PathUtil::split_path(simplified_path);
        MY_ASSERT_EQ(dir, output.first, input + " (wrong directory)");
        MY_ASSERT_EQ(file, output.second, input + " (wrong file)");
    }
    // test this case without simplifuing the path first
    std::string path = "a";
    const auto [dir, file] = sgfs::PathUtil::split_path(path);
    MY_ASSERT_EQ(dir, "", "a (wrong directory)");
    MY_ASSERT_EQ(file, "a", "a (wrong file)");
}

TEST_F(PathUtilTest, PathAtMountPoint) {
    std::vector<std::tuple<std::string, std::string, std::string>> input_output = {
            {"////../",                 "/dev/a",    "Exception"},
            {"/dev/a/foo/bar/",         "/dev/a",    "/foo/bar"},
            {"/dev/a/foo/../bar/",      "/dev/a/",   "/bar"},
            {"/dev/a/foo/../bar////",   "/dev/a///", "/bar"},
            {"/dev/a////foo/../../",    "dev/a",     "Exception"},
            {"/dev/b/foo/../bar/bar2/", "/bar/bar2", "Exception"},
            {"/dev/b/foo/../bar/bar2",  "/dev/a",    "Exception"},
            {"/foo/dev/b/foo/bar/../",  "/dev/a",    "Exception"},
            {"/dev/a/../a/fioo",        "/dev/a",    "/fioo"}
    };

    for (const auto &test_item: input_output) {
        const auto& [path, mp, output] = test_item;
        auto simplified_path = sgfs::PathUtil::simplify_path_string(path);
        try {
            auto path_at_mp = sgfs::PathUtil::path_at_mount_point(simplified_path, "/dev/a");
            MY_ASSERT_EQ(output, path_at_mp,  "{" + path + ", " + mp + "}");
        } catch (std::logic_error&) {
            MY_ASSERT_EQ(output, "Exception", "{" + path + ", " + mp + "}");
        }
    }
}