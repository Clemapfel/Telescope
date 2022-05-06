//
// Copyright (c) Joshua Higginbotham, 2022
//

#include <test/test.hpp>
#include <telescope.h>
#include <telescope.cpp> // ew but necessary

int main()
{
    Test::initialize();

    Test::testset("TS_VmaCreateBuffer", [](){
    });

    Test::testset("TS_VmaCreateImage", [](){
    });

    Test::testset("TS_VmaCreateImage", [](){
    });

    Test::testset("TS_VmaDestroyBuffers", [](){
    });

    Test::testset("TS_VmaDestroyAllocator", [](){
    });

    return Test::conclude();
}

