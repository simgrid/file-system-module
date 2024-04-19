#include <gtest/gtest.h>
#include <iostream>
#include "../include/PathUtil.hpp"

namespace fsmod=simgrid::module::fs;

// Ugly macro, but nec
// essary for knowing what's happening
#define MY_ASSERT_EQ(lhs, rhs, message) if (lhs != rhs) ASSERT_EQ(lhs, rhs) << "Test failed on input string: " << message

class PathUtilTest : public ::testing::Test {
};

TEST_F(PathUtilTest, PathSimplification)  {

    std::vector<std::pair<std::string, std::string>> input_output = {
            {"////", "/"},
            {"////foo", "/foo"},
            {"/////foo", "/foo"},
            {"../../../././././/////foo", "../../../foo"},
            {"../../", "../.."},
            {"/../../","/"},
            {"foo/bar/../", "foo"},
            {"/////foo/////././././bar/../bar/..///../../", "/"},
            {"./foo/.../........", "foo/.../........"},
            {"./foo/", "foo"},
            {"./foo", "foo"},
    };

    for (const auto &test_item : input_output) {
        MY_ASSERT_EQ(fsmod::PathUtil::simplify_path_string(test_item.first), test_item.second, test_item.first);
    }
}

TEST_F(PathUtilTest, PathGoesUp)  {
    std::vector<std::pair<std::string, bool>> input_output = {
            {"/../../", false},
            {"../foo/", true},
            {"/a/b/c/../../../", false},
            {"/a/b/c/../../../../", false},
            {"/aa/bb/cc/dd/../../ee/ff/", false}
    };

    for (const auto &test_item : input_output) {
        auto simplified_path = fsmod::PathUtil::simplify_path_string(test_item.first);
        MY_ASSERT_EQ(fsmod::PathUtil::goes_up(simplified_path), test_item.second, test_item.first);
    }
}

TEST_F(PathUtilTest, PathIsAbsolute) {
    std::vector<std::pair<std::string, bool>> input_output = {
            {"/../../",                   true},
            {"../foo/",                   false},
            {"/a/b/c/../../../",          true},
            {"/a/b/c/../../../../",       true},
            {"/aa/bb/cc/dd/../../ee/ff/", true}
    };
    for (const auto &test_item: input_output) {
        auto simplified_path = fsmod::PathUtil::simplify_path_string(test_item.first);
        MY_ASSERT_EQ(fsmod::PathUtil::is_absolute(simplified_path), test_item.second, test_item.first);
    }
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
        auto [path, mp, output] = test_item;
        auto simplified_path = simgrid::module::fs::PathUtil::simplify_path_string(path);
        try {
            auto path_at_mp = simgrid::module::fs::PathUtil::path_at_mount_point(simplified_path, "/dev/a");
            MY_ASSERT_EQ(output, path_at_mp,  "{" + path + ", " + mp + "}");
        } catch (std::logic_error &e) {
            MY_ASSERT_EQ(output, "Exception", "{" + path + ", " + mp + "}");
        }
    }
}