//
//  main.cpp
//  VulkanTest
//
//  Created by Ranjith on 5/19/21.
//

#include <iostream>

#include "TriangleApp.h"

int main(int argc, const char * argv[]) {
    auto application = std::make_shared<TriangleApp>();
    
    application->create("Hello Triangle");
    application->run();
    
    return 0;
}

