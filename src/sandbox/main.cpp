/*****************************************************************/ /**
 * \file   main.cpp
 * \brief  Entry Point to our Application [Temporary]
 * 
 * \author Dhaval
 * \date   May 2022
 *********************************************************************/

#include <iostream>
#include <cassert>

#include "VulkanApplication.hpp"
#include "VulkanRenderer.hpp"
#include "VulkanRendererContext.hpp"
#include "VulkanRenderData.hpp"
#include "VulkanUtils.hpp"

#include <GLFW/glfw3.h>

int main(void)
{
    if (!glfwInit())
    {
        return EXIT_FAILURE;
    }

    try
    {
        application sandbox;
        sandbox.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;

        glfwTerminate();

        return EXIT_FAILURE;
    }

    glfwTerminate();

    return EXIT_SUCCESS;
}
