//> includes
#include "vulkan_renderer.h"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <glm/gtx/transform.hpp>


#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

#include "initializers.h"
#include "types.h"

#include <chrono>
#include <thread>

//bootstrap library
#include "VkBootstrap.h"

#include "images.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "pipelines.h"
#include "loader.h"

#include "geometry.h"

VulkanRenderer* loadedEngine = nullptr;

constexpr bool bUseValidationLayers = true;

VulkanRenderer& VulkanRenderer::get()
{
	return *loadedEngine;
}

void VulkanRenderer::init(uint32_t width, uint32_t height, SDL_Window* window)
{
	// only one engine initialization is allowed with the application.
	assert(loadedEngine == nullptr);
	loadedEngine = this;

	_windowExtent.width = width;
	_windowExtent.height = height;

	_window = window;

	init_vulkan();

	init_swapchain();

	init_commands();

	init_sync_structures();

	init_descriptors();

	init_pipelines();

	init_default_data();

	init_scenes();

	init_imgui();

	//everything went fine
	_isInitialized = true;
}

void VulkanRenderer::cleanup()
{
	if (_isInitialized)
	{
		//make sure the gpu has stopped doing its things
		vkDeviceWaitIdle(_device);

		_loadedScenes.clear();

		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
			vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr);

			//destroy sync objects
			vkDestroyFence(_device, _frames[i]._renderFence, nullptr);
			vkDestroySemaphore(_device, _frames[i]._renderSemaphore, nullptr);
			vkDestroySemaphore(_device, _frames[i]._swapchainSemaphore, nullptr);

			_frames[i]._deletionQueue.flush();
		}

		for (auto& mesh : _test_meshes)
		{
			destroy_buffer(mesh->meshBuffers.indexBuffer);
			destroy_buffer(mesh->meshBuffers.vertexBuffer);
		}

		//flush the global deletion queue
		_mainDeletionQueue.flush();

		destroy_swapchain();

		vkDestroySurfaceKHR(_instance, _surface, nullptr);
		vkDestroyDevice(_device, nullptr);

		vkb::destroy_debug_utils_messenger(_instance, _debugMessenger);
		vkDestroyInstance(_instance, nullptr);
		SDL_DestroyWindow(_window);
	}

	// clear engine pointer
	loadedEngine = nullptr;
}

void VulkanRenderer::process_sdl_event(const SDL_Event* sdl_event)
{
	ImGui_ImplSDL2_ProcessEvent(sdl_event);
}

void VulkanRenderer::draw()
{
	auto start = std::chrono::system_clock::now();

	if (_resizeRequested)
	{
		resize_swapchain();
	}

	update_imgui();

	update_scene();

	draw_scene();

	//get clock again, compare with start clock
	auto end = std::chrono::system_clock::now();

	//convert to microseconds (integer), and then come back to miliseconds
	auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	_stats.frametime = elapsed.count() / 1000.f;
}

void VulkanRenderer::draw_scene()
{
	// wait on the fence

// wait until the gpu has finished rendering the last frame. Timeout of 1
// second
	VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._renderFence, true, 1000000000));

	get_current_frame()._deletionQueue.flush();
	get_current_frame()._frameDescriptors.clear_pools(_device);

	VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._renderFence));

	// acquire the swapchain

// request image from the swapchain
	uint32_t swapchain_image_index;
	VkResult acquire_result = vkAcquireNextImageKHR(_device, _swapchain, 1000000000, get_current_frame()._swapchainSemaphore, nullptr, &swapchain_image_index);
	if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		_resizeRequested = true;
		return;
	}

	// fill the command buffer

// naming it cmd for shorter writing
	VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;

	// now that we are sure that the commands finished executing, we can safely
	// reset the command buffer to begin recording again.
	VK_CHECK(vkResetCommandBuffer(cmd, 0));

	// begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
	VkCommandBufferBeginInfo cmd_begin_info = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	_drawExtent.height = static_cast<uint32_t>(std::min(_swapchainExtent.height, _drawImage.imageExtent.height) * _renderScale);
	_drawExtent.width = static_cast<uint32_t>(std::min(_swapchainExtent.width, _drawImage.imageExtent.width) * _renderScale);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));

	// transition our main draw image into general layout so we can write into it with a compute shader
	// we will overwrite it all so we dont care about what was the older layout
	vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	draw_background(cmd);

	// transition our main draw and depth images into color attachment optimal layout so we can render into it with vertex and fragment shaders
	vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	vkutil::transition_image(cmd, _depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	draw_geometry(cmd);

	//transition the draw image and the swapchain image into their correct transfer layouts
	vkutil::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	vkutil::transition_image(cmd, _swapchainImages[swapchain_image_index], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// execute a copy from the draw image into the swapchain
	vkutil::copy_image_to_image(cmd, _drawImage.image, _swapchainImages[swapchain_image_index], _drawExtent, _swapchainExtent);

	// set swapchain image layout to Attachment Optimal so we can draw it with ImGui
	vkutil::transition_image(cmd, _swapchainImages[swapchain_image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	// draw imgui into the swapchain image
	draw_imgui(cmd, _swapchainImageViews[swapchain_image_index]);

	// set swapchain image layout to Present so we can draw it
	vkutil::transition_image(cmd, _swapchainImages[swapchain_image_index], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	// finalize the command buffer (we can no longer add commands, but it can now be executed)
	VK_CHECK(vkEndCommandBuffer(cmd));


	// submit to the queue

//prepare the submission to the queue. 
//we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
//we will signal the _renderSemaphore, to signal that rendering has finished

	VkCommandBufferSubmitInfo cmd_info = vkinit::command_buffer_submit_info(cmd);

	VkSemaphoreSubmitInfo wait_info = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, get_current_frame()._swapchainSemaphore);
	VkSemaphoreSubmitInfo signal_info = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, get_current_frame()._renderSemaphore);

	VkSubmitInfo2 submit = vkinit::submit_info(&cmd_info, &signal_info, &wait_info);

	//submit command buffer to the queue and execute it.
	// _renderFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, get_current_frame()._renderFence));


	// present to the swapchain

//prepare present
// this will put the image we just rendered to into the visible window.
// we want to wait on the _renderSemaphore for that, 
// as it's necessary that drawing commands have finished before the image is displayed to the user
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.pSwapchains = &_swapchain;
	presentInfo.swapchainCount = 1;

	presentInfo.pWaitSemaphores = &get_current_frame()._renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices = &swapchain_image_index;

	VkResult presentResult = vkQueuePresentKHR(_graphicsQueue, &presentInfo);
	if (presentResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		_resizeRequested = true;
	}

	//increase the number of frames drawn
	_frameNumber++;
}

void VulkanRenderer::init_vulkan()
{
	vkb::InstanceBuilder builder;

	//make the vulkan instance, with basic debug features
	auto inst_ret = builder.set_app_name("Example Vulkan Application")
		.request_validation_layers(bUseValidationLayers)
		.use_default_debug_messenger()
		.require_api_version(1, 3, 0)
		.build();

	vkb::Instance vkb_inst = inst_ret.value();

	//grab the instance 
	_instance = vkb_inst.instance;
	_debugMessenger = vkb_inst.debug_messenger;

	SDL_Vulkan_CreateSurface(_window, _instance, &_surface);

	//vulkan 1.3 features
	VkPhysicalDeviceVulkan13Features features13{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	features13.dynamicRendering = true;
	features13.synchronization2 = true;

	//vulkan 1.2 features
	VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	features12.bufferDeviceAddress = true;
	features12.descriptorIndexing = true;

	//use vkbootstrap to select a gpu. 
	//We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
	vkb::PhysicalDeviceSelector selector{ vkb_inst };
	vkb::PhysicalDevice physicalDevice = selector
		.set_minimum_version(1, 3)
		.set_required_features_13(features13)
		.set_required_features_12(features12)
		.set_surface(_surface)
		.select()
		.value();

	//create the final vulkan device
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	vkb::Device vkbDevice = deviceBuilder.build().value();

	// Get the VkDevice handle used in the rest of a vulkan application
	_device = vkbDevice.device;
	_chosenGPU = physicalDevice.physical_device;

	// use vkbootstrap to get a Graphics queue
	_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	// initialize the memory allocator
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = _chosenGPU;
	allocatorInfo.device = _device;
	allocatorInfo.instance = _instance;
	allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	vmaCreateAllocator(&allocatorInfo, &_allocator);

	_mainDeletionQueue.push_function([&]() {
		vmaDestroyAllocator(_allocator);
		});
}

void VulkanRenderer::init_swapchain()
{
	create_swapchain(_windowExtent.width, _windowExtent.height);

	//draw image size will match the window
	VkExtent3D drawImageExtent = {
		_windowExtent.width,
		_windowExtent.height,
		1
	};

	//hardcoding the draw format to 32 bit float
	_drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	_drawImage.imageExtent = drawImageExtent;

	VkImageUsageFlags drawImageUsages{};
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	VkImageCreateInfo rimg_info = vkinit::image_create_info(_drawImage.imageFormat, drawImageUsages, drawImageExtent);

	//for the draw image, we want to allocate it from gpu local memory
	VmaAllocationCreateInfo rimg_allocinfo = {};
	rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//allocate and create the image
	vmaCreateImage(_allocator, &rimg_info, &rimg_allocinfo, &_drawImage.image, &_drawImage.allocation, nullptr);

	//build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo rview_info = vkinit::imageview_create_info(_drawImage.imageFormat, _drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

	VK_CHECK(vkCreateImageView(_device, &rview_info, nullptr, &_drawImage.imageView));

	_depthImage.imageFormat = VK_FORMAT_D32_SFLOAT;
	_depthImage.imageExtent = drawImageExtent;
	VkImageUsageFlags depthImageUsages{};
	depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	VkImageCreateInfo dimg_info = vkinit::image_create_info(_depthImage.imageFormat, depthImageUsages, drawImageExtent);

	//allocate and create the image
	vmaCreateImage(_allocator, &dimg_info, &rimg_allocinfo, &_depthImage.image, &_depthImage.allocation, nullptr);

	//build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo dview_info = vkinit::imageview_create_info(_depthImage.imageFormat, _depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

	VK_CHECK(vkCreateImageView(_device, &dview_info, nullptr, &_depthImage.imageView));

	//add to deletion queues
	_mainDeletionQueue.push_function([=]() {
		vkDestroyImageView(_device, _drawImage.imageView, nullptr);
		vmaDestroyImage(_allocator, _drawImage.image, _drawImage.allocation);

		vkDestroyImageView(_device, _depthImage.imageView, nullptr);
		vmaDestroyImage(_allocator, _depthImage.image, _depthImage.allocation);
		});
}

void VulkanRenderer::init_commands()
{
	//create a command pool for commands submitted to the graphics queue.
	//we also want the pool to allow for resetting of individual command buffers
	VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));

		// allocate the default command buffer that we will use for rendering
		VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1);

		VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer));
	}

	// immediate commands

	VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_immCommandPool));

	// allocate the command buffer for immediate submits
	VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_immCommandPool, 1);

	VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_immediate_command_buffer));

	_mainDeletionQueue.push_function([=]() {
		vkDestroyCommandPool(_device, _immCommandPool, nullptr);
		});

}

void VulkanRenderer::init_sync_structures()
{
	//create syncronization structures
	//one fence to control when the gpu has finished rendering the frame,
	//and 2 semaphores to syncronize rendering with swapchain
	//we want the fence to start signalled so we can wait on it on the first frame

	VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frames[i]._renderFence));

		VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._swapchainSemaphore));
		VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._renderSemaphore));
	}

	// immediate

	VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_immediate_fence));
	_mainDeletionQueue.push_function([=]() {
		vkDestroyFence(_device, _immediate_fence, nullptr);
		});
}

void VulkanRenderer::init_descriptors()
{
	//create a descriptor pool that will hold 10 sets with 1 image each
	std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes =
	{
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
	};

	_globalDescriptorAllocator.init(_device, 10, sizes);

	//make the descriptor set layout for our compute draw
	{
		DescriptorLayoutBuilder builder;
		builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		_drawImageDescriptorLayout = builder.build(_device, VK_SHADER_STAGE_COMPUTE_BIT);
	}

	//allocate a descriptor set for our draw image
	_drawImageDescriptors = _globalDescriptorAllocator.allocate(_device, _drawImageDescriptorLayout);

	VkDescriptorImageInfo imgInfo{};
	imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	imgInfo.imageView = _drawImage.imageView;

	VkWriteDescriptorSet drawImageWrite = {};
	drawImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	drawImageWrite.pNext = nullptr;

	drawImageWrite.dstBinding = 0;
	drawImageWrite.dstSet = _drawImageDescriptors;
	drawImageWrite.descriptorCount = 1;
	drawImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	drawImageWrite.pImageInfo = &imgInfo;

	vkUpdateDescriptorSets(_device, 1, &drawImageWrite, 0, nullptr);

	//make sure both the descriptor allocator and the new layout get cleaned up properly
	_mainDeletionQueue.push_function([&]() {
		_globalDescriptorAllocator.destroy_pools(_device);

		vkDestroyDescriptorSetLayout(_device, _drawImageDescriptorLayout, nullptr);
		});

	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		// create a descriptor pool
		std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frame_sizes = {
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
		};

		_frames[i]._frameDescriptors = DescriptorAllocatorGrowable{};
		_frames[i]._frameDescriptors.init(_device, 1000, frame_sizes);

		_mainDeletionQueue.push_function([&, i]() {
			_frames[i]._frameDescriptors.destroy_pools(_device);
			});
	}

	//make the descriptor set layout for our scene data
	{
		DescriptorLayoutBuilder builder;
		builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		_gpuSceneDataDescriptorLayout = builder.build(_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	}

	//make the descriptor set layout for our textured mesh draw
	{
		DescriptorLayoutBuilder builder;
		builder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		_singleImageDescriptorLayout = builder.build(_device, VK_SHADER_STAGE_FRAGMENT_BIT);
	}
}

void VulkanRenderer::init_pipelines()
{
	// compute pipelines
	init_background_pipelines();

	// graphics pipelins
	init_triangle_pipeline();
	init_mesh_pipeline();

	_metalRoughMaterial.build_pipelines(_device, _gpuSceneDataDescriptorLayout, _drawImage.imageFormat, _depthImage.imageFormat);
}

void VulkanRenderer::init_background_pipelines()
{
	// create the pipeline layout

	VkPipelineLayoutCreateInfo computeLayout{};
	computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	computeLayout.pNext = nullptr;
	computeLayout.pSetLayouts = &_drawImageDescriptorLayout;
	computeLayout.setLayoutCount = 1;

	VkPushConstantRange pushConstant{};
	pushConstant.offset = 0;
	pushConstant.size = sizeof(ComputePushConstants);
	pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	computeLayout.pPushConstantRanges = &pushConstant;
	computeLayout.pushConstantRangeCount = 1;

	VK_CHECK(vkCreatePipelineLayout(_device, &computeLayout, nullptr, &_computePipelineLayout));

	//create the pipeline

	VkShaderModule gradientShader;
	if (!vkutil::load_shader_module("../shaders/gradient_color.comp.spv", _device, &gradientShader))
	{
		fmt::println("Error when building the compute shader (gradient_color.comp)");
	}
	else
	{
		fmt::println("Gradient compute shader succesfully loaded (gradient_color.comp)");
	}

	VkShaderModule skyShader;
	if (!vkutil::load_shader_module("../shaders/sky.comp.spv", _device, &skyShader))
	{
		fmt::println("Error when building the sky compute shader (sky.comp)");
	}
	else
	{
		fmt::println("The sky compute shader succesfully loaded (sky.comp)");
	}

	VkPipelineShaderStageCreateInfo stageinfo{};
	stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageinfo.pNext = nullptr;
	stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stageinfo.module = gradientShader;
	stageinfo.pName = "main";

	VkComputePipelineCreateInfo computePipelineCreateInfo{};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.pNext = nullptr;
	computePipelineCreateInfo.layout = _computePipelineLayout;
	computePipelineCreateInfo.stage = stageinfo;

	ComputeEffect gradient;
	gradient.layout = _computePipelineLayout;
	gradient.name = "gradient";
	gradient.data = {};

	//default colors
	gradient.data.data1 = glm::vec4(1, 0, 0, 1);
	gradient.data.data2 = glm::vec4(0, 0, 1, 1);

	VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &gradient.pipeline));

	//change the shader module only to create the sky shader
	computePipelineCreateInfo.stage.module = skyShader;

	ComputeEffect sky;
	sky.layout = _computePipelineLayout;
	sky.name = "sky";
	sky.data = {};
	//default sky parameters
	sky.data.data1 = glm::vec4(0.1, 0.2, 0.4, 0.97);

	VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &sky.pipeline));

	//add the 2 background effects into the array
	_backgroundEffects.push_back(gradient);
	_backgroundEffects.push_back(sky);

	//destroy structures properly
	vkDestroyShaderModule(_device, gradientShader, nullptr);
	vkDestroyShaderModule(_device, skyShader, nullptr);
	_mainDeletionQueue.push_function([=]() {
		vkDestroyPipelineLayout(_device, _computePipelineLayout, nullptr);
		vkDestroyPipeline(_device, sky.pipeline, nullptr);
		vkDestroyPipeline(_device, gradient.pipeline, nullptr);
		});
}

void VulkanRenderer::init_triangle_pipeline()
{
	VkShaderModule triangleFragShader;
	if (!vkutil::load_shader_module("../shaders/colored_triangle.frag.spv", _device, &triangleFragShader))
	{
		fmt::println("Error when building the triangle fragment shader module (colored_triangle.frag)");
	}
	else
	{
		fmt::println("Triangle fragment shader succesfully loaded (colored_triangle.frag)");
	}

	VkShaderModule triangleVertexShader;
	if (!vkutil::load_shader_module("../shaders/colored_triangle.vert.spv", _device, &triangleVertexShader))
	{
		fmt::println("Error when building the triangle vertex shader module (colored_triangle.vert)");
	}
	else
	{
		fmt::println("Triangle vertex shader succesfully loaded (colored_triangle.vert)");
	}

	//build the pipeline layout that controls the inputs/outputs of the shader
	//we are not using descriptor sets or other systems yet, so no need to use anything other than empty default
	VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();
	VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_trianglePipelineLayout));

	PipelineBuilder pipelineBuilder;

	//use the triangle layout we created
	pipelineBuilder.set_pipeline_layout(_trianglePipelineLayout);
	//connecting the vertex and pixel shaders to the pipeline
	pipelineBuilder.set_shaders(triangleVertexShader, triangleFragShader);
	//it will draw triangles
	pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	//filled triangles
	pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
	//no backface culling
	pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	//no multisampling
	pipelineBuilder.set_multisampling_none();
	//no blending
	pipelineBuilder.disable_blending();

	//depth testing
	//pipelineBuilder.disable_depthtest();
	pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

	//connect the image format we will draw into, from draw image
	pipelineBuilder.set_color_attachment_format(_drawImage.imageFormat);
	pipelineBuilder.set_depth_format(_depthImage.imageFormat);

	//finally build the pipeline
	_trianglePipeline = pipelineBuilder.build_pipeline(_device);

	//clean structures
	vkDestroyShaderModule(_device, triangleFragShader, nullptr);
	vkDestroyShaderModule(_device, triangleVertexShader, nullptr);

	_mainDeletionQueue.push_function([&]() {
		vkDestroyPipelineLayout(_device, _trianglePipelineLayout, nullptr);
		vkDestroyPipeline(_device, _trianglePipeline, nullptr);
		});
}

void VulkanRenderer::init_mesh_pipeline()
{
	VkShaderModule triangleFragShader;
	if (!vkutil::load_shader_module("../shaders/tex_image.frag.spv", _device, &triangleFragShader))
	{
		fmt::println("Error when building the texture image fragment shader (tex_image.frag)");
	}
	else
	{
		fmt::println("Texture image fragment shader succesfully loaded (tex_image.frag)");
	}

	VkShaderModule triangleVertexShader;
	if (!vkutil::load_shader_module("../shaders/colored_triangle_mesh.vert.spv", _device, &triangleVertexShader))
	{
		fmt::println("Error when building the colored triangle vertex shader (colored_triangle_mesh.vert)");
	}
	else
	{
		fmt::println("Colored triangle vertex shader succesfully loaded (colored_triangle_mesh.vert)");
	}

	VkPushConstantRange bufferRange{};
	bufferRange.offset = 0;
	bufferRange.size = sizeof(GPUDrawPushConstants);
	bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();
	pipeline_layout_info.pPushConstantRanges = &bufferRange;
	pipeline_layout_info.pushConstantRangeCount = 1;
	pipeline_layout_info.pSetLayouts = &_singleImageDescriptorLayout;
	pipeline_layout_info.setLayoutCount = 1;

	VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_meshPipelineLayout));

	PipelineBuilder pipelineBuilder;

	//use the triangle layout we created
	pipelineBuilder.set_pipeline_layout(_meshPipelineLayout);
	//connecting the vertex and pixel shaders to the pipeline
	pipelineBuilder.set_shaders(triangleVertexShader, triangleFragShader);
	//it will draw triangles
	pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	//filled triangles
	pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
	//no backface culling
	pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	//no multisampling
	pipelineBuilder.set_multisampling_none();
	//no blending
	pipelineBuilder.disable_blending();

	//pipelineBuilder.disable_depthtest();
	pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

	//connect the image format we will draw into, from draw image
	pipelineBuilder.set_color_attachment_format(_drawImage.imageFormat);
	pipelineBuilder.set_depth_format(_depthImage.imageFormat);

	//pipelineBuilder.disable_blending();
	pipelineBuilder.enable_blending_additive();

	//finally build the pipeline
	_meshPipeline = pipelineBuilder.build_pipeline(_device);

	//clean structures
	vkDestroyShaderModule(_device, triangleFragShader, nullptr);
	vkDestroyShaderModule(_device, triangleVertexShader, nullptr);

	_mainDeletionQueue.push_function([&]() {
		vkDestroyPipelineLayout(_device, _meshPipelineLayout, nullptr);
		vkDestroyPipeline(_device, _meshPipeline, nullptr);
		});
}

void VulkanRenderer::init_default_data()
{
	// meshes

	std::array<Vertex, 4> rect_vertices;

	rect_vertices[0].position = { 0.5,-0.5, 0 };
	rect_vertices[1].position = { 0.5,0.5, 0 };
	rect_vertices[2].position = { -0.5,-0.5, 0 };
	rect_vertices[3].position = { -0.5,0.5, 0 };

	rect_vertices[0].color = { 0,0, 0,1 };
	rect_vertices[1].color = { 0.5,0.5,0.5 ,1 };
	rect_vertices[2].color = { 1,0, 0,1 };
	rect_vertices[3].color = { 0,1, 0,1 };

	std::array<uint32_t, 6> rect_indices;

	rect_indices[0] = 0;
	rect_indices[1] = 1;
	rect_indices[2] = 2;

	rect_indices[3] = 2;
	rect_indices[4] = 1;
	rect_indices[5] = 3;

	_rectangle = upload_mesh(rect_indices, rect_vertices);

	_test_meshes = LoadedGLTF::load_gltf_meshes("..\\assets\\basicmesh.glb",
		[this](std::span<uint32_t> indices, std::span<Vertex> vertices)
		{
			return this->upload_mesh(indices, vertices);
		}).value();

	//delete the rectangle data on engine shutdown
	_mainDeletionQueue.push_function([&]() {
		destroy_buffer(_rectangle.indexBuffer);
		destroy_buffer(_rectangle.vertexBuffer);
		});

	// textures

	//3 default textures, white, grey, black. 1 pixel each
	uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
	_whiteImage = create_image((void*)&white, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

	uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
	_greyImage = create_image((void*)&grey, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

	uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
	_blackImage = create_image((void*)&black, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

	//checkerboard image
	uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
	std::array<uint32_t, 16 * 16 > pixels; //for 16x16 checkerboard texture
	for (int x = 0; x < 16; x++)
	{
		for (int y = 0; y < 16; y++)
		{
			pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
		}
	}
	_errorCheckerboardImage = create_image(pixels.data(), VkExtent3D{ 16, 16, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT);

	VkSamplerCreateInfo sampler = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

	sampler.magFilter = VK_FILTER_NEAREST;
	sampler.minFilter = VK_FILTER_NEAREST;

	vkCreateSampler(_device, &sampler, nullptr, &_defaultSamplerNearest);

	sampler.magFilter = VK_FILTER_LINEAR;
	sampler.minFilter = VK_FILTER_LINEAR;
	vkCreateSampler(_device, &sampler, nullptr, &_defaultSamplerLinear);

	_mainDeletionQueue.push_function([&]() {
		vkDestroySampler(_device, _defaultSamplerNearest, nullptr);
		vkDestroySampler(_device, _defaultSamplerLinear, nullptr);

		destroy_image(_whiteImage);
		destroy_image(_greyImage);
		destroy_image(_blackImage);
		destroy_image(_errorCheckerboardImage);
		});

	// material

	GLTFMetallic_Roughness::MaterialResources material_resources;
	//default the material textures
	material_resources.colorImage = _whiteImage;
	material_resources.colorSampler = _defaultSamplerLinear;
	material_resources.metalRoughImage = _whiteImage;
	material_resources.metalRoughSampler = _defaultSamplerLinear;

	//set the uniform buffer for the material data
	AllocatedBuffer material_constants = create_buffer(sizeof(GLTFMetallic_Roughness::MaterialConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	//write the buffer
	GLTFMetallic_Roughness::MaterialConstants* scene_uniform_data = (GLTFMetallic_Roughness::MaterialConstants*)material_constants.allocation->GetMappedData();
	scene_uniform_data->colorFactors = glm::vec4{ 1,1,1,1 };
	scene_uniform_data->metal_rough_factors = glm::vec4{ 1,0.5,0,0 };

	_mainDeletionQueue.push_function([=, this]()
		{
			destroy_buffer(material_constants);
		});

	material_resources.dataBuffer = material_constants.buffer;
	material_resources.dataBufferOffset = 0;

	_defaultData = _metalRoughMaterial.write_material(_device, MaterialPass::MainColor, material_resources, _globalDescriptorAllocator);

	// meshes

	for (auto& mesh : _test_meshes)
	{
		std::shared_ptr<MeshNode> new_node = std::make_shared<MeshNode>();
		new_node->mesh = mesh;

		new_node->localTransform = glm::mat4{ 1.f };
		new_node->worldTransform = glm::mat4{ 1.f };

		for (auto& surface : new_node->mesh->surfaces)
		{
			surface.material = std::make_shared<GLTFMaterial>(_defaultData);
		}

		_loadedNodes[mesh->name] = std::move(new_node);
	}

	std::vector<Vertex> gizmo_vertices;
	std::vector<uint32_t> gizmo_indices;
	Geometry::create_gizmo(gizmo_vertices, gizmo_indices);

	GeoSurface gizmo_geosurface;
	gizmo_geosurface.startIndex = 0;
	gizmo_geosurface.count = gizmo_indices.size();
	gizmo_geosurface.bounds = Geometry::compute_bounds(gizmo_vertices);
	gizmo_geosurface.material = std::make_shared<GLTFMaterial>(_defaultData);

	std::shared_ptr<MeshAsset> gizmo_mesh_asset = std::make_shared<MeshAsset>();
	gizmo_mesh_asset->name = "gizmo";
	gizmo_mesh_asset->surfaces.push_back(gizmo_geosurface);
	gizmo_mesh_asset->meshBuffers = upload_mesh(gizmo_indices, gizmo_vertices);
	
	std::shared_ptr<MeshNode> gizmo_node = std::make_shared<MeshNode>();
	gizmo_node->mesh = gizmo_mesh_asset;
	gizmo_node->localTransform = glm::mat4{ 1.f };
	gizmo_node->worldTransform = glm::mat4{ 1.f };

	_loadedNodes["gizmo"] = std::move(gizmo_node);

	_mainDeletionQueue.push_function([=]()
		{
			destroy_buffer(gizmo_mesh_asset->meshBuffers.indexBuffer);
			destroy_buffer(gizmo_mesh_asset->meshBuffers.vertexBuffer);
		});
}

void VulkanRenderer::init_scenes()
{
	std::string structure_path = { "..\\assets\\structure.glb" };
	const BufferAllocator buffer_allocator
	{
		[this](size_t alloc_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage)
		{
			return this->create_buffer(alloc_size, usage, memory_usage);
		},
		[this](const AllocatedBuffer& buffer)
		{
			this->destroy_buffer(buffer);
		}
	};

	const ImageAllocator image_allocator =
	{
		[this](void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
		{
			return create_image(data, size, format, usage, mipmapped);
		},
		[this](const AllocatedImage& image)
		{
			destroy_image(image);
		}
	};

	auto build_material = [this](VkDevice device, MaterialPass pass, const GLTFMetallic_Roughness::MaterialResources& resources, DescriptorAllocatorGrowable& descriptor_allocator)
		{
			return _metalRoughMaterial.write_material(device, pass, resources, descriptor_allocator);
		};

	auto uploadMesh = [this](std::span<uint32_t> indices, std::span<Vertex> vertices)
		{
			return upload_mesh(indices, vertices);
		};

	LoadedGLTF::LoadGLTFParams loadGLTFParams;
	loadGLTFParams.filePath = structure_path;
	loadGLTFParams.device = _device;
	loadGLTFParams.bufferAllocator = buffer_allocator;
	loadGLTFParams.imageAllocator = image_allocator;
	loadGLTFParams.buildMaterial = build_material;
	loadGLTFParams.uploadMesh = uploadMesh;
	loadGLTFParams.whiteImage = _whiteImage;
	loadGLTFParams.errorImage = _errorCheckerboardImage;
	loadGLTFParams.defaultSampler = _defaultSamplerLinear;


	auto structureFile = LoadedGLTF::load_gltf(loadGLTFParams);

	assert(structureFile.has_value());

	_loadedScenes["structure"] = *structureFile;
}

void VulkanRenderer::init_imgui()
{
	// 1: create descriptor pool for IMGUI
	//  the size of the pool is very oversize, but it's copied from imgui demo
	//  itself.
	VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	VkDescriptorPool imguiPool;
	VK_CHECK(vkCreateDescriptorPool(_device, &pool_info, nullptr, &imguiPool));

	// 2: initialize imgui library

	// this initializes the core structures of imgui
	ImGui::CreateContext();

	// this initializes imgui for SDL
	ImGui_ImplSDL2_InitForVulkan(_window);

	// this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = _instance;
	init_info.PhysicalDevice = _chosenGPU;
	init_info.Device = _device;
	init_info.Queue = _graphicsQueue;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.UseDynamicRendering = true;

	//dynamic rendering parameters for imgui to use
	init_info.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &_swapchainImageFormat;

	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&init_info);

	ImGui_ImplVulkan_CreateFontsTexture();

	// add the destroy the imgui created structures
	_mainDeletionQueue.push_function([=]() {
		ImGui_ImplVulkan_Shutdown();
		vkDestroyDescriptorPool(_device, imguiPool, nullptr);
		});
}

void VulkanRenderer::create_swapchain(uint32_t width, uint32_t height)
{
	vkb::SwapchainBuilder swapchainBuilder{ _chosenGPU, _device, _surface };

	_swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

	vkb::Swapchain vkbSwapchain = swapchainBuilder
		//.use_default_format_selection()
		.set_desired_format(VkSurfaceFormatKHR{ .format = _swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		//use vsync present mode
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(width, height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();

	_swapchainExtent = vkbSwapchain.extent;
	//store swapchain and its related images
	_swapchain = vkbSwapchain.swapchain;
	_swapchainImages = vkbSwapchain.get_images().value();
	_swapchainImageViews = vkbSwapchain.get_image_views().value();
}

void VulkanRenderer::destroy_swapchain()
{
	vkDestroySwapchainKHR(_device, _swapchain, nullptr);

	// destroy swapchain resources
	for (int i = 0; i < _swapchainImageViews.size(); i++) {

		vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
	}
}

void VulkanRenderer::resize_swapchain()
{
	vkDeviceWaitIdle(_device);

	destroy_swapchain();

	int w, h;
	SDL_GetWindowSize(_window, &w, &h);
	_windowExtent.width = w;
	_windowExtent.height = h;

	create_swapchain(_windowExtent.width, _windowExtent.height);

	_resizeRequested = false;
}

AllocatedBuffer VulkanRenderer::create_buffer(size_t alloc_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage)
{
	VkBufferCreateInfo buffer_info = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	buffer_info.pNext = nullptr;
	buffer_info.size = alloc_size;
	buffer_info.usage = usage;

	VmaAllocationCreateInfo vma_alloc_info = {};
	vma_alloc_info.usage = memory_usage;
	vma_alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	AllocatedBuffer new_buffer;

	VK_CHECK(vmaCreateBuffer(_allocator, &buffer_info, &vma_alloc_info, &new_buffer.buffer, &new_buffer.allocation, &new_buffer.info));

	return new_buffer;
}

void VulkanRenderer::destroy_buffer(const AllocatedBuffer& buffer)
{
	vmaDestroyBuffer(_allocator, buffer.buffer, buffer.allocation);
}

AllocatedImage VulkanRenderer::create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
	AllocatedImage newImage;
	newImage.imageFormat = format;
	newImage.imageExtent = size;

	VkImageCreateInfo img_info = vkinit::image_create_info(format, usage, size);
	if (mipmapped)
	{
		img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
	}

	// always allocate images on dedicated GPU memory
	VmaAllocationCreateInfo allocinfo = {};
	allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// allocate and create the image
	VK_CHECK(vmaCreateImage(_allocator, &img_info, &allocinfo, &newImage.image, &newImage.allocation, nullptr));

	// if the format is a depth format, we will need to have it use the correct
	// aspect flag
	VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
	if (format == VK_FORMAT_D32_SFLOAT)
	{
		aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	// build a image-view for the image
	VkImageViewCreateInfo view_info = vkinit::imageview_create_info(format, newImage.image, aspectFlag);
	view_info.subresourceRange.levelCount = img_info.mipLevels;

	VK_CHECK(vkCreateImageView(_device, &view_info, nullptr, &newImage.imageView));

	return newImage;
}

AllocatedImage VulkanRenderer::create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
	const size_t dataSize = size.depth * size.width * size.height * 4;
	AllocatedBuffer uploadbuffer = create_buffer(dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	memcpy(uploadbuffer.info.pMappedData, data, dataSize);

	AllocatedImage newImage = create_image(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

	immediate_submit(
		[&](VkCommandBuffer cmd)
		{
			vkutil::transition_image(cmd, newImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			VkBufferImageCopy copyRegion = {};
			copyRegion.bufferOffset = 0;
			copyRegion.bufferRowLength = 0;
			copyRegion.bufferImageHeight = 0;

			copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.imageSubresource.mipLevel = 0;
			copyRegion.imageSubresource.baseArrayLayer = 0;
			copyRegion.imageSubresource.layerCount = 1;
			copyRegion.imageExtent = size;

			// copy the buffer into the image
			vkCmdCopyBufferToImage(cmd, uploadbuffer.buffer, newImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

			if (mipmapped)
			{
				vkutil::generate_mipmaps(cmd, newImage.image, VkExtent2D{ newImage.imageExtent.width,newImage.imageExtent.height });
			}
			else
			{
				vkutil::transition_image(cmd, newImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
		});
	destroy_buffer(uploadbuffer);
	return newImage;
}

void VulkanRenderer::destroy_image(const AllocatedImage& img)
{
	vkDestroyImageView(_device, img.imageView, nullptr);
	vmaDestroyImage(_allocator, img.image, img.allocation);
}

GPUMeshBuffers VulkanRenderer::upload_mesh(std::span<uint32_t> indices, std::span<Vertex> vertices)
{
	const size_t vertex_buffer_size = vertices.size() * sizeof(Vertex);
	const size_t index_buffer_size = indices.size() * sizeof(uint32_t);

	GPUMeshBuffers new_surface;

	//create vertex buffer
	new_surface.vertexBuffer = create_buffer(vertex_buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);

	//find the adress of the vertex buffer
	VkBufferDeviceAddressInfo device_adress_info{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = new_surface.vertexBuffer.buffer };
	new_surface.vertexBufferAddress = vkGetBufferDeviceAddress(_device, &device_adress_info);

	//create index buffer
	new_surface.indexBuffer = create_buffer(index_buffer_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

	AllocatedBuffer staging = create_buffer(vertex_buffer_size + index_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

	void* data = staging.allocation->GetMappedData();

	// copy vertex buffer
	memcpy(data, vertices.data(), vertex_buffer_size);
	// copy index buffer
	memcpy((char*)data + vertex_buffer_size, indices.data(), index_buffer_size);

	immediate_submit([&](VkCommandBuffer cmd)
		{
			VkBufferCopy vertexCopy{ 0 };
			vertexCopy.dstOffset = 0;
			vertexCopy.srcOffset = 0;
			vertexCopy.size = vertex_buffer_size;

			vkCmdCopyBuffer(cmd, staging.buffer, new_surface.vertexBuffer.buffer, 1, &vertexCopy);

			VkBufferCopy indexCopy{ 0 };
			indexCopy.dstOffset = 0;
			indexCopy.srcOffset = vertex_buffer_size;
			indexCopy.size = index_buffer_size;

			vkCmdCopyBuffer(cmd, staging.buffer, new_surface.indexBuffer.buffer, 1, &indexCopy);
		});

	destroy_buffer(staging);

	return new_surface;
}

void VulkanRenderer::update_scene()
{
	//begin clock
	auto start = std::chrono::system_clock::now();

	_mainDrawContext.OpaqueSurfaces.clear();
	_mainDrawContext.TransparentSurfaces.clear();

	// view matrix formula taken from https://johannesugb.github.io/gpu-programming/setting-up-a-proper-vulkan-projection-matrix/

	glm::mat4 view;
	view[0] = glm::vec4(_camera_axes[0], 0.0f);
	view[1] = glm::vec4(_camera_axes[1], 0.0f);
	view[2] = glm::vec4(_camera_axes[2], 0.0f);
	view[3] = glm::vec4(_camera_position, 1.0f);

	view = glm::inverse(view);

	glm::mat4 projection = Geometry::compute_perspective_projection_for_vulkan(glm::radians(70.f), (float)_windowExtent.width / (float)_windowExtent.height, 10000.f, 0.1f);

	_sceneData.view = view;
	_sceneData.proj = projection;
	_sceneData.viewproj = projection * view;

	//some default lighting parameters
	_sceneData.ambientColor = glm::vec4(.1f);
	_sceneData.sunlightColor = glm::vec4(1.f);
	_sceneData.sunlightDirection = glm::vec4(0, 1, 0.5, 1.f);

	for (auto& render_instance : _render_instances)
	{
		_loadedNodes[render_instance.mesh_name]->draw(render_instance.transform, _mainDrawContext);
	}

	//_loadedNodes["Suzanne"]->draw(glm::mat4{ 1.f }, _mainDrawContext);

	//for (int x = -3; x < 3; x++)
	//{
	//	glm::mat4 scale = glm::scale(glm::vec3{ 0.2f });
	//	glm::mat4 translation = glm::translate(glm::vec3{ x, 1, 0 });

	//	_loadedNodes["Cube"]->draw(translation * scale, _mainDrawContext);
	//}

	_loadedScenes["structure"]->draw(glm::mat4{ 1.f }, _mainDrawContext);

	auto end = std::chrono::system_clock::now();

	//convert to microseconds (integer), and then come back to miliseconds
	auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	_stats.scene_update_time = elapsed.count() / 1000.f;
}

void VulkanRenderer::update_imgui()
{
	// imgui new frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	if (ImGui::Begin("background"))
	{
		ImGui::SliderFloat("Render Scale", &_renderScale, 0.3f, 1.f);

		ComputeEffect& selected = _backgroundEffects[_currentBackgroundEffect];

		ImGui::Text("Selected effect: ", selected.name);

		ImGui::SliderInt("Effect Index", &_currentBackgroundEffect, 0, static_cast<int>(_backgroundEffects.size()) - 1);

		ImGui::InputFloat4("data1", (float*)&selected.data.data1);
		ImGui::InputFloat4("data2", (float*)&selected.data.data2);
		ImGui::InputFloat4("data3", (float*)&selected.data.data3);
		ImGui::InputFloat4("data4", (float*)&selected.data.data4);

		ImGui::BeginGroup();
		ImGui::Text("frametime %f ms", _stats.frametime);
		ImGui::Text("draw time %f ms", _stats.mesh_draw_time);
		ImGui::Text("update time %f ms", _stats.scene_update_time);
		ImGui::Text("triangles %i", _stats.triangle_count);
		ImGui::Text("draws %i", _stats.drawcall_count);
		ImGui::Text("position %f %f %f", _camera_position.x, _camera_position.y, _camera_position.z);
		ImGui::EndGroup();
	}

	ImGui::End();

	//make imgui calculate internal draw structures
	ImGui::Render();
}

void VulkanRenderer::draw_background(VkCommandBuffer cmd)
{
	// make a clear-color from frame number. This will flash with a 120 frame period.
	VkClearColorValue clearValue = { { 0.0f, 0.0f, 0.0f, 1.0f } };

	VkImageSubresourceRange clearRange = vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

	// clear image
	vkCmdClearColorImage(cmd, _drawImage.image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

	// select effect
	const ComputeEffect& effect = _backgroundEffects[_currentBackgroundEffect];

	// bind the gradient drawing compute pipeline
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);

	// bind the descriptor set containing the draw image for the compute pipeline
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.layout, 0, 1, &_drawImageDescriptors, 0, nullptr);

	// push PushConstants
	vkCmdPushConstants(cmd, _computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.data);

	// execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
	vkCmdDispatch(cmd, static_cast<uint32_t>(std::ceil(_drawExtent.width / 16.0)), static_cast<uint32_t>(std::ceil(_drawExtent.height / 16.0)), 1);
}

void VulkanRenderer::draw_geometry(VkCommandBuffer cmd)
{
	//reset counters
	_stats.drawcall_count = 0;
	_stats.triangle_count = 0;

	//begin clock
	auto start = std::chrono::system_clock::now();

	// sort opaque surfaces

	std::vector<uint32_t> opaque_draws;
	opaque_draws.reserve(_mainDrawContext.OpaqueSurfaces.size());

	for (uint32_t i = 0; i < _mainDrawContext.OpaqueSurfaces.size(); i++)
	{
		if (is_visible(_mainDrawContext.OpaqueSurfaces[i], _sceneData.viewproj))
		{
			opaque_draws.push_back(i);
		}
	}

	// sort the opaque surfaces by material and mesh
	std::sort(opaque_draws.begin(), opaque_draws.end(),
		[&](const auto& iA, const auto& iB)
		{
			const RenderObject& A = _mainDrawContext.OpaqueSurfaces[iA];
			const RenderObject& B = _mainDrawContext.OpaqueSurfaces[iB];
			if (A.material == B.material)
			{
				return A.indexBuffer < B.indexBuffer;
			}
			else
			{
				return A.material < B.material;
			}
		});

	// sort transparent surfaces

	std::vector<uint32_t> transparent_draws;
	transparent_draws.reserve(_mainDrawContext.TransparentSurfaces.size());

	for (uint32_t i = 0; i < _mainDrawContext.TransparentSurfaces.size(); i++)
	{
		if (is_visible(_mainDrawContext.TransparentSurfaces[i], _sceneData.viewproj))
		{
			transparent_draws.push_back(i);
		}
	}

	// sort the transparent surfaces by distance from camera
	std::sort(transparent_draws.begin(), transparent_draws.end(),
		[&](const auto& iA, const auto& iB)
		{
			const RenderObject& A = _mainDrawContext.TransparentSurfaces[iA];
			const RenderObject& B = _mainDrawContext.TransparentSurfaces[iB];
			float distA = (_camera_position - A.bounds.origin).length() - A.bounds.sphereRadius;
			float distB = (_camera_position - B.bounds.origin).length() - B.bounds.sphereRadius;

			return distA < distB;
		});

	// prepare GPU scene data descriptor set

   // allocate a new uniform buffer for the scene data
	AllocatedBuffer gpu_scene_data_buffer = create_buffer(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	//add it to the deletion queue of this frame so it gets deleted once it's been used
	get_current_frame()._deletionQueue.push_function(
		[=, this]()
		{
			destroy_buffer(gpu_scene_data_buffer);
		});

	// write the buffer
	GPUSceneData* scene_uniform_data = (GPUSceneData*)gpu_scene_data_buffer.allocation->GetMappedData();
	*scene_uniform_data = _sceneData;

	//create a descriptor set that binds that buffer and update it
	VkDescriptorSet global_descriptor = get_current_frame()._frameDescriptors.allocate(_device, _gpuSceneDataDescriptorLayout);

	DescriptorWriter writer;
	writer.write_buffer(0, gpu_scene_data_buffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	writer.update_set(_device, global_descriptor);

	// begin rendering

// begin a render pass connected to our draw and depth images

	VkRenderingAttachmentInfo color_attachment = vkinit::attachment_info(_drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
	VkRenderingAttachmentInfo depth_attachment = vkinit::depth_attachment_info(_depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	VkRenderingInfo render_info = vkinit::rendering_info(_drawExtent, &color_attachment, &depth_attachment);

	vkCmdBeginRendering(cmd, &render_info);

	//// draw a triangle

	//vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _trianglePipeline);

	//// set dynamic viewport and scissor
	//VkViewport viewport = {};
	//viewport.x = 0;
	//viewport.y = 0;
	//viewport.width = static_cast<float>(_drawExtent.width);
	//viewport.height = static_cast<float>(_drawExtent.height);
	//viewport.minDepth = 0.f;
	//viewport.maxDepth = 1.f;

	//vkCmdSetViewport(cmd, 0, 1, &viewport);

	//VkRect2D scissor = {};
	//scissor.offset.x = 0;
	//scissor.offset.y = 0;
	//scissor.extent.width = _drawExtent.width;
	//scissor.extent.height = _drawExtent.height;

	//vkCmdSetScissor(cmd, 0, 1, &scissor);

	//// launch a draw command to draw 3 vertices
	//vkCmdDraw(cmd, 3, 1, 0, 0);

	// draw meshes

//defined outside of the draw function, this is the state we will try to skip
	MaterialPipeline* last_pipeline = nullptr;
	MaterialInstance* last_material = nullptr;
	VkBuffer last_index_buffer = VK_NULL_HANDLE;

	auto draw = [&](const RenderObject& r)
		{
			if (r.material != last_material)
			{
				last_material = r.material;
				//rebind pipeline and descriptors if the material changed
				if (r.material->pipeline != last_pipeline)
				{
					last_pipeline = r.material->pipeline;
					vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->pipeline);
					vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout, 0, 1, &global_descriptor, 0, nullptr);

					VkViewport viewport = {};
					viewport.x = 0;
					viewport.y = 0;
					viewport.width = (float)_windowExtent.width;
					viewport.height = (float)_windowExtent.height;
					viewport.minDepth = 0.f;
					viewport.maxDepth = 1.f;

					vkCmdSetViewport(cmd, 0, 1, &viewport);

					VkRect2D scissor = {};
					scissor.offset.x = 0;
					scissor.offset.y = 0;
					scissor.extent.width = _windowExtent.width;
					scissor.extent.height = _windowExtent.height;

					vkCmdSetScissor(cmd, 0, 1, &scissor);
				}

				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout, 1, 1, &r.material->materialSet, 0, nullptr);
			}

			//rebind index buffer if needed
			if (r.indexBuffer != last_index_buffer)
			{
				last_index_buffer = r.indexBuffer;
				vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
			}

			// calculate final mesh matrix
			GPUDrawPushConstants push_constants;
			push_constants.worldMatrix = r.transform;
			push_constants.vertexBuffer = r.vertexBufferAddress;

			vkCmdPushConstants(cmd, r.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);

			vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);

			//stats
			_stats.drawcall_count++;
			_stats.triangle_count += r.indexCount / 3;
		};

	for (auto& r : opaque_draws)
	{
		draw(_mainDrawContext.OpaqueSurfaces[r]);
	}

	for (auto& r : transparent_draws)
	{
		draw(_mainDrawContext.TransparentSurfaces[r]);
	}

	vkCmdEndRendering(cmd);

	// stats

	auto end = std::chrono::system_clock::now();

	//convert to microseconds (integer), and then come back to milliseconds
	auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	_stats.mesh_draw_time = elapsed.count() / 1000.f;
}

void VulkanRenderer::draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView)
{
	VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingInfo renderInfo = vkinit::rendering_info(_swapchainExtent, &colorAttachment, nullptr);

	vkCmdBeginRendering(cmd, &renderInfo);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

	vkCmdEndRendering(cmd);
}

void VulkanRenderer::immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function)
{
	VK_CHECK(vkResetFences(_device, 1, &_immediate_fence));
	VK_CHECK(vkResetCommandBuffer(_immediate_command_buffer, 0));

	VkCommandBuffer cmd = _immediate_command_buffer;

	VkCommandBufferBeginInfo command_buffer_begin_info = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &command_buffer_begin_info));

	function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);
	VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, nullptr, nullptr);

	// submit command buffer to the queue and execute it.
	//  _renderFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, _immediate_fence));

	VK_CHECK(vkWaitForFences(_device, 1, &_immediate_fence, true, 9999999999));
}

bool VulkanRenderer::is_visible(const RenderObject& obj, const glm::mat4& viewproj)
{
	std::array<glm::vec3, 8> corners
	{
		glm::vec3 { 1, 1, 1 },
		glm::vec3 { 1, 1, -1 },
		glm::vec3 { 1, -1, 1 },
		glm::vec3 { 1, -1, -1 },
		glm::vec3 { -1, 1, 1 },
		glm::vec3 { -1, 1, -1 },
		glm::vec3 { -1, -1, 1 },
		glm::vec3 { -1, -1, -1 },
	};

	glm::mat4 matrix = viewproj * obj.transform;

	glm::vec3 min = { 1.5, 1.5, 1.5 };
	glm::vec3 max = { -1.5, -1.5, -1.5 };

	for (int c = 0; c < 8; c++)
	{
		// project each corner into clip space
		glm::vec4 v = matrix * glm::vec4(obj.bounds.origin + (corners[c] * obj.bounds.extents), 1.f);

		// perspective correction
		v.x = v.x / v.w;
		v.y = v.y / v.w;
		v.z = v.z / v.w;

		min = glm::min(glm::vec3{ v.x, v.y, v.z }, min);
		max = glm::max(glm::vec3{ v.x, v.y, v.z }, max);
	}

	// check the clip space box is within the view
	if (min.z > 1.f || max.z < 0.f || min.x > 1.f || max.x < -1.f || min.y > 1.f || max.y < -1.f)
	{
		return false;
	}
	else
	{
		return true;
	}
}

VulkanRenderer::RenderInstanceId VulkanRenderer::add_render_instance(const std::string& mesh_name, glm::mat4 transform)
{
	RenderInstance render_instance = { _id_pool.acquire_id(), mesh_name, transform };

	_render_instances.push_back({ _id_pool.acquire_id(), mesh_name, transform });

	return _render_instances.back().id;
}

void VulkanRenderer::remove_render_instance(RenderInstanceId id)
{
	auto it = std::find_if(_render_instances.begin(), _render_instances.end(),
		[id](const RenderInstance& render_instance)
		{
			return render_instance.id == id;
		});

	if (it != _render_instances.end())
	{
		_render_instances.erase(it);
	}
}