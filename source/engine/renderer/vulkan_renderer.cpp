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

#include "geometry.h"
#include "gltf_mesh_loader.h"

constexpr bool bUseValidationLayers = true;

void VulkanRenderer::init(uint32_t width, uint32_t height, SDL_Window* window)
{
	m_window_extent.width = width;
	m_window_extent.height = height;

	m_window = window;

	init_vulkan();

	init_swapchain();

	init_commands();

	init_sync_structures();

	init_descriptors();

	init_pipelines();

	init_default_data();

	init_imgui();

	m_is_initialized = true;
}

void VulkanRenderer::cleanup()
{
	if (m_is_initialized)
	{
		//make sure the gpu has stopped doing its things
		vkDeviceWaitIdle(m_device);

		m_render_object_id_to_render_object.clear();

		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
			vkDestroyCommandPool(m_device, m_frames[i].command_pool, nullptr);

			//destroy sync objects
			vkDestroyFence(m_device, m_frames[i].render_fence, nullptr);
			vkDestroySemaphore(m_device, m_frames[i].render_semaphore, nullptr);
			vkDestroySemaphore(m_device, m_frames[i].swapchain_semaphore, nullptr);

			m_frames[i].deletion_queue.flush();
		}

		//flush the global deletion queue
		m_main_deletion_queue.flush();

		destroy_swapchain();

		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
		vkDestroyDevice(m_device, nullptr);

		vkb::destroy_debug_utils_messenger(m_instance, m_debug_messenger);
		vkDestroyInstance(m_instance, nullptr);
		SDL_DestroyWindow(m_window);
	}
}

void VulkanRenderer::process_sdl_event(const SDL_Event* sdl_event)
{
	ImGui_ImplSDL2_ProcessEvent(sdl_event);
}

void VulkanRenderer::draw()
{
	auto start = std::chrono::system_clock::now();

	if (m_resize_requested)
	{
		resize_swapchain();
	}

	update_imgui();

	add_render_objects_to_context();

	draw_scene();

	//get clock again, compare with start clock
	auto end = std::chrono::system_clock::now();

	//convert to microseconds (integer), and then come back to miliseconds
	auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	m_stats.frametime = elapsed.count() / 1000.f;
}

void VulkanRenderer::draw_scene()
{
	// wait on the fence

// wait until the gpu has finished rendering the last frame. Timeout of 1
// second
	VK_CHECK(vkWaitForFences(m_device, 1, &get_current_frame().render_fence, true, 1000000000));

	get_current_frame().deletion_queue.flush();
	get_current_frame().frame_descriptors.clear_pools(m_device);

	VK_CHECK(vkResetFences(m_device, 1, &get_current_frame().render_fence));

	// acquire the swapchain

// request image from the swapchain
	uint32_t swapchain_image_index;
	VkResult acquire_result = vkAcquireNextImageKHR(m_device, m_swapchain, 1000000000, get_current_frame().swapchain_semaphore, nullptr, &swapchain_image_index);
	if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		m_resize_requested = true;
		return;
	}

	// fill the command buffer

// naming it cmd for shorter writing
	VkCommandBuffer cmd = get_current_frame().main_command_buffer;

	// now that we are sure that the commands finished executing, we can safely
	// reset the command buffer to begin recording again.
	VK_CHECK(vkResetCommandBuffer(cmd, 0));

	// begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
	VkCommandBufferBeginInfo cmd_begin_info = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	m_draw_extent.height = static_cast<uint32_t>(std::min(m_swapchain_extent.height, m_draw_image.image_extent.height) * m_render_scale);
	m_draw_extent.width = static_cast<uint32_t>(std::min(m_swapchain_extent.width, m_draw_image.image_extent.width) * m_render_scale);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));

	// transition our main draw image into general layout so we can write into it with a compute shader
	// we will overwrite it all so we dont care about what was the older layout
	vkutil::transition_image(cmd, m_draw_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	draw_background(cmd);

	// transition our main draw and depth images into color attachment optimal layout so we can render into it with vertex and fragment shaders
	vkutil::transition_image(cmd, m_draw_image.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	vkutil::transition_image(cmd, m_depth_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	draw_main_render_context(cmd);

	//transition the draw image and the swapchain image into their correct transfer layouts
	vkutil::transition_image(cmd, m_draw_image.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	vkutil::transition_image(cmd, m_swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// execute a copy from the draw image into the swapchain
	vkutil::copy_image_to_image(cmd, m_draw_image.image, m_swapchain_images[swapchain_image_index], m_draw_extent, m_swapchain_extent);

	// set swapchain image layout to Attachment Optimal so we can draw it with ImGui
	vkutil::transition_image(cmd, m_swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	// draw imgui into the swapchain image
	draw_imgui(cmd, m_swapchain_image_views[swapchain_image_index]);

	// set swapchain image layout to Present so we can draw it
	vkutil::transition_image(cmd, m_swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	// finalize the command buffer (we can no longer add commands, but it can now be executed)
	VK_CHECK(vkEndCommandBuffer(cmd));


	// submit to the queue

//prepare the submission to the queue. 
//we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
//we will signal the _renderSemaphore, to signal that rendering has finished

	VkCommandBufferSubmitInfo cmd_info = vkinit::command_buffer_submit_info(cmd);

	VkSemaphoreSubmitInfo wait_info = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, get_current_frame().swapchain_semaphore);
	VkSemaphoreSubmitInfo signal_info = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, get_current_frame().render_semaphore);

	VkSubmitInfo2 submit = vkinit::submit_info(&cmd_info, &signal_info, &wait_info);

	//submit command buffer to the queue and execute it.
	// _renderFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit2(m_graphics_queue, 1, &submit, get_current_frame().render_fence));


	// present to the swapchain

//prepare present
// this will put the image we just rendered to into the visible window.
// we want to wait on the _renderSemaphore for that, 
// as it's necessary that drawing commands have finished before the image is displayed to the user
	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pNext = nullptr;
	present_info.pSwapchains = &m_swapchain;
	present_info.swapchainCount = 1;

	present_info.pWaitSemaphores = &get_current_frame().render_semaphore;
	present_info.waitSemaphoreCount = 1;

	present_info.pImageIndices = &swapchain_image_index;

	VkResult presentResult = vkQueuePresentKHR(m_graphics_queue, &present_info);
	if (presentResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		m_resize_requested = true;
	}

	//increase the number of frames drawn
	m_frame_number++;
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
	m_instance = vkb_inst.instance;
	m_debug_messenger = vkb_inst.debug_messenger;

	SDL_Vulkan_CreateSurface(m_window, m_instance, &m_surface);

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
	vkb::PhysicalDevice physical_device = selector
		.set_minimum_version(1, 3)
		.set_required_features_13(features13)
		.set_required_features_12(features12)
		.set_surface(m_surface)
		.select()
		.value();

	//create the final vulkan device
	vkb::DeviceBuilder device_builder{ physical_device };

	vkb::Device vkbDevice = device_builder.build().value();

	// Get the VkDevice handle used in the rest of a vulkan application
	m_device = vkbDevice.device;
	m_chosen_gpu = physical_device.physical_device;

	// use vkbootstrap to get a Graphics queue
	m_graphics_queue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	m_graphics_queue_family = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	// initialize the memory allocator
	VmaAllocatorCreateInfo allocator_info = {};
	allocator_info.physicalDevice = m_chosen_gpu;
	allocator_info.device = m_device;
	allocator_info.instance = m_instance;
	allocator_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	vmaCreateAllocator(&allocator_info, &m_allocator);

	m_main_deletion_queue.push_function([&]()
		{
			vmaDestroyAllocator(m_allocator);
		});
}

void VulkanRenderer::init_swapchain()
{
	create_swapchain(m_window_extent.width, m_window_extent.height);

	//draw image size will match the window
	VkExtent3D draw_image_extent = {
		m_window_extent.width,
		m_window_extent.height,
		1
	};

	//hardcoding the draw format to 32 bit float
	m_draw_image.image_format = VK_FORMAT_R16G16B16A16_SFLOAT;
	m_draw_image.image_extent = draw_image_extent;

	VkImageUsageFlags draw_image_usages{};
	draw_image_usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	draw_image_usages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	draw_image_usages |= VK_IMAGE_USAGE_STORAGE_BIT;
	draw_image_usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	VkImageCreateInfo draw_img_info = vkinit::image_create_info(m_draw_image.image_format, draw_image_usages, draw_image_extent);

	//for the draw image, we want to allocate it from gpu local memory
	VmaAllocationCreateInfo rimg_allocinfo = {};
	rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//allocate and create the image
	vmaCreateImage(m_allocator, &draw_img_info, &rimg_allocinfo, &m_draw_image.image, &m_draw_image.allocation, nullptr);

	//build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo draw_image_view_info = vkinit::image_view_create_info(m_draw_image.image_format, m_draw_image.image, VK_IMAGE_ASPECT_COLOR_BIT);

	VK_CHECK(vkCreateImageView(m_device, &draw_image_view_info, nullptr, &m_draw_image.image_view));

	m_depth_image.image_format = VK_FORMAT_D32_SFLOAT;
	m_depth_image.image_extent = draw_image_extent;
	VkImageUsageFlags depth_image_usages{};
	depth_image_usages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	VkImageCreateInfo dimg_info = vkinit::image_create_info(m_depth_image.image_format, depth_image_usages, draw_image_extent);

	//allocate and create the image
	vmaCreateImage(m_allocator, &dimg_info, &rimg_allocinfo, &m_depth_image.image, &m_depth_image.allocation, nullptr);

	//build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo depth_image_view_info = vkinit::image_view_create_info(m_depth_image.image_format, m_depth_image.image, VK_IMAGE_ASPECT_DEPTH_BIT);

	VK_CHECK(vkCreateImageView(m_device, &depth_image_view_info, nullptr, &m_depth_image.image_view));

	//add to deletion queues
	m_main_deletion_queue.push_function([=]()
		{
			vkDestroyImageView(m_device, m_draw_image.image_view, nullptr);
			vmaDestroyImage(m_allocator, m_draw_image.image, m_draw_image.allocation);

			vkDestroyImageView(m_device, m_depth_image.image_view, nullptr);
			vmaDestroyImage(m_allocator, m_depth_image.image, m_depth_image.allocation);
		});
}

void VulkanRenderer::init_commands()
{
	//create a command pool for commands submitted to the graphics queue.
	//we also want the pool to allow for resetting of individual command buffers
	VkCommandPoolCreateInfo command_pool_info = vkinit::command_pool_create_info(m_graphics_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		VK_CHECK(vkCreateCommandPool(m_device, &command_pool_info, nullptr, &m_frames[i].command_pool));

		// allocate the default command buffer that we will use for rendering
		VkCommandBufferAllocateInfo cmd_alloc_info = vkinit::command_buffer_allocate_info(m_frames[i].command_pool, 1);

		VK_CHECK(vkAllocateCommandBuffers(m_device, &cmd_alloc_info, &m_frames[i].main_command_buffer));
	}

	// immediate commands

	VK_CHECK(vkCreateCommandPool(m_device, &command_pool_info, nullptr, &m_immediate_command_pool));

	// allocate the command buffer for immediate submits
	VkCommandBufferAllocateInfo cmd_alloc_info = vkinit::command_buffer_allocate_info(m_immediate_command_pool, 1);

	VK_CHECK(vkAllocateCommandBuffers(m_device, &cmd_alloc_info, &m_immediate_command_buffer));

	m_main_deletion_queue.push_function([=]()
		{
			vkDestroyCommandPool(m_device, m_immediate_command_pool, nullptr);
		});

}

void VulkanRenderer::init_sync_structures()
{
	//create syncronization structures
	//one fence to control when the gpu has finished rendering the frame,
	//and 2 semaphores to syncronize rendering with swapchain
	//we want the fence to start signalled so we can wait on it on the first frame

	VkFenceCreateInfo fence_create_info = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphore_create_info = vkinit::semaphore_create_info();

	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		VK_CHECK(vkCreateFence(m_device, &fence_create_info, nullptr, &m_frames[i].render_fence));

		VK_CHECK(vkCreateSemaphore(m_device, &semaphore_create_info, nullptr, &m_frames[i].swapchain_semaphore));
		VK_CHECK(vkCreateSemaphore(m_device, &semaphore_create_info, nullptr, &m_frames[i].render_semaphore));
	}

	// immediate

	VK_CHECK(vkCreateFence(m_device, &fence_create_info, nullptr, &m_immediate_fence));
	m_main_deletion_queue.push_function([=]()
		{
			vkDestroyFence(m_device, m_immediate_fence, nullptr);
		});
}

void VulkanRenderer::init_descriptors()
{
	//create a descriptor pool that will hold 10 sets with 1 image each
	std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes =
	{
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
	};

	m_global_descriptor_allocator.init(m_device, 10, sizes);

	//make the descriptor set layout for our compute draw
	{
		DescriptorLayoutBuilder builder;
		builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		m_draw_image_descriptor_layout = builder.build(m_device, VK_SHADER_STAGE_COMPUTE_BIT);
	}

	//allocate a descriptor set for our draw image
	m_draw_image_descriptors = m_global_descriptor_allocator.allocate(m_device, m_draw_image_descriptor_layout);

	VkDescriptorImageInfo img_info{};
	img_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	img_info.imageView = m_draw_image.image_view;

	VkWriteDescriptorSet draw_image_write = {};
	draw_image_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	draw_image_write.pNext = nullptr;

	draw_image_write.dstBinding = 0;
	draw_image_write.dstSet = m_draw_image_descriptors;
	draw_image_write.descriptorCount = 1;
	draw_image_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	draw_image_write.pImageInfo = &img_info;

	vkUpdateDescriptorSets(m_device, 1, &draw_image_write, 0, nullptr);

	//make sure both the descriptor allocator and the new layout get cleaned up properly
	m_main_deletion_queue.push_function([&]()
		{
			m_global_descriptor_allocator.destroy_pools(m_device);

			vkDestroyDescriptorSetLayout(m_device, m_draw_image_descriptor_layout, nullptr);
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

		m_frames[i].frame_descriptors = DescriptorAllocatorGrowable{};
		m_frames[i].frame_descriptors.init(m_device, 1000, frame_sizes);

		m_main_deletion_queue.push_function([&, i]() {
			m_frames[i].frame_descriptors.destroy_pools(m_device);
			});
	}

	//make the descriptor set layout for our scene data
	{
		DescriptorLayoutBuilder builder;
		builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		m_gpu_scene_data_descriptor_layout = builder.build(m_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	}

	//make the descriptor set layout for our textured mesh draw
	{
		DescriptorLayoutBuilder builder;
		builder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		m_single_image_descriptor_layout = builder.build(m_device, VK_SHADER_STAGE_FRAGMENT_BIT);
	}
}

void VulkanRenderer::init_pipelines()
{
	// compute pipelines
	init_background_pipelines();

	m_metal_rough_material.build_pipelines(m_device, m_gpu_scene_data_descriptor_layout, m_draw_image.image_format, m_depth_image.image_format);
}

void VulkanRenderer::init_background_pipelines()
{
	// create the pipeline layout

	VkPipelineLayoutCreateInfo compute_layout{};
	compute_layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	compute_layout.pNext = nullptr;
	compute_layout.pSetLayouts = &m_draw_image_descriptor_layout;
	compute_layout.setLayoutCount = 1;

	VkPushConstantRange push_constant{};
	push_constant.offset = 0;
	push_constant.size = sizeof(ComputePushConstants);
	push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	compute_layout.pPushConstantRanges = &push_constant;
	compute_layout.pushConstantRangeCount = 1;

	VK_CHECK(vkCreatePipelineLayout(m_device, &compute_layout, nullptr, &m_compute_pipeline_layout));

	//create the pipeline

	VkShaderModule gradient_shader;
	if (!vkutil::load_shader_module("../shaders/gradient_color.comp.spv", m_device, &gradient_shader))
	{
		fmt::println("Error when building the compute shader (gradient_color.comp)");
	}
	else
	{
		fmt::println("Gradient compute shader succesfully loaded (gradient_color.comp)");
	}

	VkShaderModule sky_shader;
	if (!vkutil::load_shader_module("../shaders/sky.comp.spv", m_device, &sky_shader))
	{
		fmt::println("Error when building the sky compute shader (sky.comp)");
	}
	else
	{
		fmt::println("The sky compute shader succesfully loaded (sky.comp)");
	}

	VkPipelineShaderStageCreateInfo stage_info{};
	stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage_info.pNext = nullptr;
	stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stage_info.module = gradient_shader;
	stage_info.pName = "main";

	VkComputePipelineCreateInfo compute_pipeline_create_info{};
	compute_pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	compute_pipeline_create_info.pNext = nullptr;
	compute_pipeline_create_info.layout = m_compute_pipeline_layout;
	compute_pipeline_create_info.stage = stage_info;

	ComputeEffect gradient;
	gradient.layout = m_compute_pipeline_layout;
	gradient.name = "gradient";
	gradient.data = {};

	//default colors
	gradient.data.data1 = glm::vec4(1, 0, 0, 1);
	gradient.data.data2 = glm::vec4(0, 0, 1, 1);

	VK_CHECK(vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &compute_pipeline_create_info, nullptr, &gradient.pipeline));

	//change the shader module only to create the sky shader
	compute_pipeline_create_info.stage.module = sky_shader;

	ComputeEffect sky;
	sky.layout = m_compute_pipeline_layout;
	sky.name = "sky";
	sky.data = {};
	//default sky parameters
	sky.data.data1 = glm::vec4(0.1, 0.2, 0.4, 0.97);

	VK_CHECK(vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &compute_pipeline_create_info, nullptr, &sky.pipeline));

	//add the 2 background effects into the array
	m_background_effects.push_back(gradient);
	m_background_effects.push_back(sky);

	//destroy structures properly
	vkDestroyShaderModule(m_device, gradient_shader, nullptr);
	vkDestroyShaderModule(m_device, sky_shader, nullptr);
	m_main_deletion_queue.push_function([=]()
		{
			vkDestroyPipelineLayout(m_device, m_compute_pipeline_layout, nullptr);
			vkDestroyPipeline(m_device, sky.pipeline, nullptr);
			vkDestroyPipeline(m_device, gradient.pipeline, nullptr);
		});
}

void VulkanRenderer::init_default_data()
{
		// textures

	//3 default textures, white, grey, black. 1 pixel each
	uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
	m_white_image = create_image((void*)&white, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

	uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
	m_grey_image = create_image((void*)&grey, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

	uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
	m_black_image = create_image((void*)&black, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

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
	m_error_checkerboard_image = create_image(pixels.data(), VkExtent3D{ 16, 16, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT);

	VkSamplerCreateInfo sampler = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

	sampler.magFilter = VK_FILTER_NEAREST;
	sampler.minFilter = VK_FILTER_NEAREST;

	vkCreateSampler(m_device, &sampler, nullptr, &m_default_sampler_nearest);

	sampler.magFilter = VK_FILTER_LINEAR;
	sampler.minFilter = VK_FILTER_LINEAR;
	vkCreateSampler(m_device, &sampler, nullptr, &m_default_sampler_linear);

	m_main_deletion_queue.push_function([&]() {
		vkDestroySampler(m_device, m_default_sampler_nearest, nullptr);
		vkDestroySampler(m_device, m_default_sampler_linear, nullptr);

		destroy_image(m_white_image);
		destroy_image(m_grey_image);
		destroy_image(m_black_image);
		destroy_image(m_error_checkerboard_image);
		});

		// material

	GltfMetallicRoughness::MaterialResources material_resources;
	//default the material textures
	material_resources.color_image = m_white_image;
	material_resources.color_sampler = m_default_sampler_linear;
	material_resources.metal_rough_image = m_white_image;
	material_resources.metal_rough_sampler = m_default_sampler_linear;

	//set the uniform buffer for the material data
	AllocatedBuffer material_constants = create_buffer(sizeof(GltfMetallicRoughness::MaterialConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	//write the buffer
	GltfMetallicRoughness::MaterialConstants* scene_uniform_data = (GltfMetallicRoughness::MaterialConstants*)material_constants.allocation->GetMappedData();
	scene_uniform_data->color_factors = glm::vec4{ 1,1,1,1 };
	scene_uniform_data->metal_rough_factors = glm::vec4{ 1,0.5,0,0 };

	m_main_deletion_queue.push_function([=, this]()
		{
			destroy_buffer(material_constants);
		});

	material_resources.data_buffer = material_constants.buffer;
	material_resources.data_buffer_offset = 0;

	m_default_data = m_metal_rough_material.write_material(m_device, MaterialPassType::MainColor, material_resources, m_global_descriptor_allocator);

		// gizmo

	std::vector<Vertex> gizmo_vertices;
	std::vector<uint32_t> gizmo_indices;
	Geometry::create_gizmo(gizmo_vertices, gizmo_indices);

	GeoSurface gizmo_geosurface;
	gizmo_geosurface.start_index = 0;
	gizmo_geosurface.count = gizmo_indices.size();
	gizmo_geosurface.bounds = Geometry::compute_bounds(gizmo_vertices);
	gizmo_geosurface.material = std::make_shared<Material>(m_default_data);

	std::shared_ptr<MeshAsset> gizmo_mesh_asset = std::make_shared<MeshAsset>();
	gizmo_mesh_asset->name = "gizmo";
	gizmo_mesh_asset->surfaces.push_back(gizmo_geosurface);
	gizmo_mesh_asset->mesh_buffers = upload_mesh(gizmo_indices, gizmo_vertices);
	
	std::shared_ptr<MeshNode> gizmo_node = std::make_shared<MeshNode>();
	gizmo_node->mesh = gizmo_mesh_asset;
	gizmo_node->local_transform = glm::mat4{ 1.f };
	gizmo_node->world_transform = glm::mat4{ 1.f };

	m_predefined_meshes["gizmo"] = std::move(gizmo_node);

		// deletion queue

	m_main_deletion_queue.push_function([=]()
		{
			destroy_buffer(gizmo_mesh_asset->mesh_buffers.index_buffer);
			destroy_buffer(gizmo_mesh_asset->mesh_buffers.vertex_buffer);
		});
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

	VkDescriptorPool imgui_pool;
	VK_CHECK(vkCreateDescriptorPool(m_device, &pool_info, nullptr, &imgui_pool));

	// 2: initialize imgui library

	// this initializes the core structures of imgui
	ImGui::CreateContext();

	// this initializes imgui for SDL
	ImGui_ImplSDL2_InitForVulkan(m_window);

	// this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = m_instance;
	init_info.PhysicalDevice = m_chosen_gpu;
	init_info.Device = m_device;
	init_info.Queue = m_graphics_queue;
	init_info.DescriptorPool = imgui_pool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.UseDynamicRendering = true;

	//dynamic rendering parameters for imgui to use
	init_info.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_swapchain_image_format;

	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&init_info);

	ImGui_ImplVulkan_CreateFontsTexture();

	// add the destroy the imgui created structures
	m_main_deletion_queue.push_function([=]()
		{
			ImGui_ImplVulkan_Shutdown();
			vkDestroyDescriptorPool(m_device, imgui_pool, nullptr);
		});
}

void VulkanRenderer::create_swapchain(uint32_t width, uint32_t height)
{
	vkb::SwapchainBuilder swapchain_builder{ m_chosen_gpu, m_device, m_surface };

	m_swapchain_image_format = VK_FORMAT_B8G8R8A8_UNORM;

	vkb::Swapchain vkb_swapchain = swapchain_builder
		//.use_default_format_selection()
		.set_desired_format(VkSurfaceFormatKHR{ .format = m_swapchain_image_format, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		//use vsync present mode
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(width, height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();

	m_swapchain_extent = vkb_swapchain.extent;
	//store swapchain and its related images
	m_swapchain = vkb_swapchain.swapchain;
	m_swapchain_images = vkb_swapchain.get_images().value();
	m_swapchain_image_views = vkb_swapchain.get_image_views().value();
}

void VulkanRenderer::destroy_swapchain()
{
	vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

	// destroy swapchain resources
	for (int i = 0; i < m_swapchain_image_views.size(); i++) {

		vkDestroyImageView(m_device, m_swapchain_image_views[i], nullptr);
	}
}

void VulkanRenderer::resize_swapchain()
{
	vkDeviceWaitIdle(m_device);

	destroy_swapchain();

	int w, h;
	SDL_GetWindowSize(m_window, &w, &h);
	m_window_extent.width = w;
	m_window_extent.height = h;

	create_swapchain(m_window_extent.width, m_window_extent.height);

	m_resize_requested = false;
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

	VK_CHECK(vmaCreateBuffer(m_allocator, &buffer_info, &vma_alloc_info, &new_buffer.buffer, &new_buffer.allocation, &new_buffer.info));

	return new_buffer;
}

void VulkanRenderer::destroy_buffer(const AllocatedBuffer& buffer)
{
	vmaDestroyBuffer(m_allocator, buffer.buffer, buffer.allocation);
}

AllocatedImage VulkanRenderer::create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
	AllocatedImage new_image;
	new_image.image_format = format;
	new_image.image_extent = size;

	VkImageCreateInfo img_info = vkinit::image_create_info(format, usage, size);
	if (mipmapped)
	{
		img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
	}

	// always allocate images on dedicated GPU memory
	VmaAllocationCreateInfo alloc_info = {};
	alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	alloc_info.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// allocate and create the image
	VK_CHECK(vmaCreateImage(m_allocator, &img_info, &alloc_info, &new_image.image, &new_image.allocation, nullptr));

	// if the format is a depth format, we will need to have it use the correct
	// aspect flag
	VkImageAspectFlags aspect_flag = VK_IMAGE_ASPECT_COLOR_BIT;
	if (format == VK_FORMAT_D32_SFLOAT)
	{
		aspect_flag = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	// build a image-view for the image
	VkImageViewCreateInfo view_info = vkinit::image_view_create_info(format, new_image.image, aspect_flag);
	view_info.subresourceRange.levelCount = img_info.mipLevels;

	VK_CHECK(vkCreateImageView(m_device, &view_info, nullptr, &new_image.image_view));

	return new_image;
}

AllocatedImage VulkanRenderer::create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
	const size_t data_size = size.depth * size.width * size.height * 4;
	AllocatedBuffer upload_buffer = create_buffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	memcpy(upload_buffer.info.pMappedData, data, data_size);

	AllocatedImage new_image = create_image(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

	immediate_submit(
		[&](VkCommandBuffer cmd)
		{
			vkutil::transition_image(cmd, new_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			VkBufferImageCopy copy_region = {};
			copy_region.bufferOffset = 0;
			copy_region.bufferRowLength = 0;
			copy_region.bufferImageHeight = 0;

			copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copy_region.imageSubresource.mipLevel = 0;
			copy_region.imageSubresource.baseArrayLayer = 0;
			copy_region.imageSubresource.layerCount = 1;
			copy_region.imageExtent = size;

			// copy the buffer into the image
			vkCmdCopyBufferToImage(cmd, upload_buffer.buffer, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

			if (mipmapped)
			{
				vkutil::generate_mipmaps(cmd, new_image.image, VkExtent2D{ new_image.image_extent.width,new_image.image_extent.height });
			}
			else
			{
				vkutil::transition_image(cmd, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
		});
	destroy_buffer(upload_buffer);
	return new_image;
}

void VulkanRenderer::destroy_image(const AllocatedImage& img)
{
	vkDestroyImageView(m_device, img.image_view, nullptr);
	vmaDestroyImage(m_allocator, img.image, img.allocation);
}

GpuMeshBuffers VulkanRenderer::upload_mesh(std::span<uint32_t> indices, std::span<Vertex> vertices)
{
	const size_t vertex_buffer_size = vertices.size() * sizeof(Vertex);
	const size_t index_buffer_size = indices.size() * sizeof(uint32_t);

	GpuMeshBuffers new_surface;

	//create vertex buffer
	new_surface.vertex_buffer = create_buffer(vertex_buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);

	//find the adress of the vertex buffer
	VkBufferDeviceAddressInfo device_adress_info{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = new_surface.vertex_buffer.buffer };
	new_surface.vertex_buffer_address = vkGetBufferDeviceAddress(m_device, &device_adress_info);

	//create index buffer
	new_surface.index_buffer = create_buffer(index_buffer_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

	AllocatedBuffer staging = create_buffer(vertex_buffer_size + index_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

	void* data = staging.allocation->GetMappedData();

	// copy vertex buffer
	memcpy(data, vertices.data(), vertex_buffer_size);
	// copy index buffer
	memcpy((char*)data + vertex_buffer_size, indices.data(), index_buffer_size);

	immediate_submit([&](VkCommandBuffer cmd)
		{
			VkBufferCopy vertex_copy{ 0 };
			vertex_copy.dstOffset = 0;
			vertex_copy.srcOffset = 0;
			vertex_copy.size = vertex_buffer_size;

			vkCmdCopyBuffer(cmd, staging.buffer, new_surface.vertex_buffer.buffer, 1, &vertex_copy);

			VkBufferCopy index_copy{ 0 };
			index_copy.dstOffset = 0;
			index_copy.srcOffset = vertex_buffer_size;
			index_copy.size = index_buffer_size;

			vkCmdCopyBuffer(cmd, staging.buffer, new_surface.index_buffer.buffer, 1, &index_copy);
		});

	destroy_buffer(staging);

	return new_surface;
}

std::optional<std::shared_ptr<GltfMesh>> VulkanRenderer::load_gltf_mesh(std::string_view gltf_file_path)
{
	const BufferAllocator buffer_allocator_func
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

	const ImageAllocator image_allocator_func =
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

	auto build_material_func = [this](VkDevice device, MaterialPassType pass, const GltfMetallicRoughness::MaterialResources& resources, DescriptorAllocatorGrowable& descriptor_allocator)
		{
			return m_metal_rough_material.write_material(device, pass, resources, descriptor_allocator);
		};

	auto upload_mesh_func = [this](std::span<uint32_t> indices, std::span<Vertex> vertices)
		{
			return upload_mesh(indices, vertices);
		};

	GltfMeshLoader gltf_loader;
	gltf_loader.file_path = gltf_file_path;
	gltf_loader.device = m_device;
	gltf_loader.buffer_allocator = buffer_allocator_func;
	gltf_loader.image_allocator = image_allocator_func;
	gltf_loader.build_material = build_material_func;
	gltf_loader.upload_mesh = upload_mesh_func;
	gltf_loader.white_image = m_white_image;
	gltf_loader.error_image = m_error_checkerboard_image;
	gltf_loader.default_sampler = m_default_sampler_linear;

	auto gltf_mesh = gltf_loader.load_gltf_mesh();

	return gltf_mesh;
}

void VulkanRenderer::add_render_objects_to_context()
{
	//begin clock
	auto start = std::chrono::system_clock::now();

	m_main_render_context.opaque_surfaces.clear();
	m_main_render_context.transparent_surfaces.clear();

	// view matrix formula taken from https://johannesugb.github.io/gpu-programming/setting-up-a-proper-vulkan-projection-matrix/

	glm::mat4 view = glm::inverse(m_camera_transform.get_matrix());

	glm::mat4 projection = Geometry::compute_perspective_projection_for_vulkan(glm::radians(70.f), (float)m_window_extent.width / (float)m_window_extent.height, 0.1f, 10000.f);

	m_scene_data.view = view;
	m_scene_data.proj = projection;
	m_scene_data.viewproj = projection * view;

	//some default lighting parameters
	m_scene_data.ambient_color = glm::vec4(.1f);
	m_scene_data.sunlight_color = glm::vec4(1.f);
	m_scene_data.sunlight_direction = glm::vec4(0, 1, 0.5, 1.f);

	for (auto& render_object : m_render_object_id_to_render_object)
	{
		render_object.second->renderable->add_to_render_context(render_object.second->transform, m_main_render_context);
	}

	//_loadedNodes["Suzanne"]->draw(glm::mat4{ 1.f }, _mainDrawContext);

	//for (int x = -3; x < 3; x++)
	//{
	//	glm::mat4 scale = glm::scale(glm::vec3{ 0.2f });
	//	glm::mat4 translation = glm::translate(glm::vec3{ x, 1, 0 });

	//	_loadedNodes["Cube"]->draw(translation * scale, _mainDrawContext);
	//}

	//m_loaded_scenes["structure"]->add_to_render_context(glm::mat4{ 1.f }, m_main_render_context);

	auto end = std::chrono::system_clock::now();

	//convert to microseconds (integer), and then come back to miliseconds
	auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	m_stats.scene_update_time = elapsed.count() / 1000.f;
}

void VulkanRenderer::update_imgui()
{
	// imgui new frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	if (ImGui::Begin("background"))
	{
		ImGui::SliderFloat("Render Scale", &m_render_scale, 0.3f, 1.f);

		ComputeEffect& selected = m_background_effects[m_current_background_effect];

		ImGui::Text("Selected effect: ", selected.name);

		ImGui::SliderInt("Effect Index", &m_current_background_effect, 0, static_cast<int>(m_background_effects.size()) - 1);

		ImGui::InputFloat4("data1", (float*)&selected.data.data1);
		ImGui::InputFloat4("data2", (float*)&selected.data.data2);
		ImGui::InputFloat4("data3", (float*)&selected.data.data3);
		ImGui::InputFloat4("data4", (float*)&selected.data.data4);

		ImGui::BeginGroup();
		ImGui::Text("frametime %f ms", m_stats.frametime);
		ImGui::Text("draw time %f ms", m_stats.mesh_draw_time);
		ImGui::Text("update time %f ms", m_stats.scene_update_time);
		ImGui::Text("triangles %i", m_stats.triangle_count);
		ImGui::Text("draws %i", m_stats.drawcall_count);
		ImGui::Text("position %f %f %f", m_camera_transform.position.x, m_camera_transform.position.y, m_camera_transform.position.z);
		ImGui::EndGroup();
	}

	ImGui::End();

	//make imgui calculate internal draw structures
	ImGui::Render();
}

void VulkanRenderer::draw_background(VkCommandBuffer cmd)
{
	// make a clear-color from frame number. This will flash with a 120 frame period.
	VkClearColorValue clear_value = { { 0.0f, 0.0f, 0.0f, 1.0f } };

	VkImageSubresourceRange clear_range = vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

	// clear image
	vkCmdClearColorImage(cmd, m_draw_image.image, VK_IMAGE_LAYOUT_GENERAL, &clear_value, 1, &clear_range);

	// select effect
	const ComputeEffect& effect = m_background_effects[m_current_background_effect];

	// bind the gradient drawing compute pipeline
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);

	// bind the descriptor set containing the draw image for the compute pipeline
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.layout, 0, 1, &m_draw_image_descriptors, 0, nullptr);

	// push PushConstants
	vkCmdPushConstants(cmd, m_compute_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.data);

	// execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
	vkCmdDispatch(cmd, static_cast<uint32_t>(std::ceil(m_draw_extent.width / 16.0)), static_cast<uint32_t>(std::ceil(m_draw_extent.height / 16.0)), 1);
}

void VulkanRenderer::draw_main_render_context(VkCommandBuffer cmd)
{
	//reset counters
	m_stats.drawcall_count = 0;
	m_stats.triangle_count = 0;

	//begin clock
	auto start = std::chrono::system_clock::now();

	// sort opaque surfaces

	std::vector<uint32_t> opaque_draws;
	opaque_draws.reserve(m_main_render_context.opaque_surfaces.size());

	for (uint32_t i = 0; i < m_main_render_context.opaque_surfaces.size(); i++)
	{
		if (is_visible(m_main_render_context.opaque_surfaces[i], m_scene_data.viewproj))
		{
			opaque_draws.push_back(i);
		}
	}

	// sort the opaque surfaces by material and mesh
	std::sort(opaque_draws.begin(), opaque_draws.end(),
		[&](const auto& iA, const auto& iB)
		{
			const GpuRenderObject& A = m_main_render_context.opaque_surfaces[iA];
			const GpuRenderObject& B = m_main_render_context.opaque_surfaces[iB];
			if (A.material == B.material)
			{
				return A.index_buffer < B.index_buffer;
			}
			else
			{
				return A.material < B.material;
			}
		});

	// sort transparent surfaces

	std::vector<uint32_t> transparent_draws;
	transparent_draws.reserve(m_main_render_context.transparent_surfaces.size());

	for (uint32_t i = 0; i < m_main_render_context.transparent_surfaces.size(); i++)
	{
		if (is_visible(m_main_render_context.transparent_surfaces[i], m_scene_data.viewproj))
		{
			transparent_draws.push_back(i);
		}
	}

	const glm::vec3 camera_position = m_camera_transform.position;

	// sort the transparent surfaces by distance from camera
	std::sort(transparent_draws.begin(), transparent_draws.end(),
		[&](const auto& iA, const auto& iB)
		{
			const GpuRenderObject& A = m_main_render_context.transparent_surfaces[iA];
			const GpuRenderObject& B = m_main_render_context.transparent_surfaces[iB];
			float distA = (camera_position - A.bounds.origin).length() - A.bounds.sphere_radius;
			float distB = (camera_position - B.bounds.origin).length() - B.bounds.sphere_radius;

			return distA < distB;
		});

	// prepare GPU scene data descriptor set

   // allocate a new uniform buffer for the scene data
	AllocatedBuffer gpu_scene_data_buffer = create_buffer(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	//add it to the deletion queue of this frame so it gets deleted once it's been used
	get_current_frame().deletion_queue.push_function(
		[=, this]()
		{
			destroy_buffer(gpu_scene_data_buffer);
		});

	// write the buffer
	GPUSceneData* scene_uniform_data = (GPUSceneData*)gpu_scene_data_buffer.allocation->GetMappedData();
	*scene_uniform_data = m_scene_data;

	//create a descriptor set that binds that buffer and update it
	VkDescriptorSet global_descriptor = get_current_frame().frame_descriptors.allocate(m_device, m_gpu_scene_data_descriptor_layout);

	DescriptorWriter writer;
	writer.write_buffer(0, gpu_scene_data_buffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	writer.update_set(m_device, global_descriptor);

	// begin rendering

// begin a render pass connected to our draw and depth images

	VkRenderingAttachmentInfo color_attachment = vkinit::attachment_info(m_draw_image.image_view, nullptr, VK_IMAGE_LAYOUT_GENERAL);
	VkRenderingAttachmentInfo depth_attachment = vkinit::depth_attachment_info(m_depth_image.image_view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	VkRenderingInfo render_info = vkinit::rendering_info(m_draw_extent, &color_attachment, &depth_attachment);

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

	auto draw = [&](const GpuRenderObject& r)
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
					viewport.width = (float)m_window_extent.width;
					viewport.height = (float)m_window_extent.height;
					viewport.minDepth = 0.f;
					viewport.maxDepth = 1.f;

					vkCmdSetViewport(cmd, 0, 1, &viewport);

					VkRect2D scissor = {};
					scissor.offset.x = 0;
					scissor.offset.y = 0;
					scissor.extent.width = m_window_extent.width;
					scissor.extent.height = m_window_extent.height;

					vkCmdSetScissor(cmd, 0, 1, &scissor);
				}

				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout, 1, 1, &r.material->material_set, 0, nullptr);
			}

			//rebind index buffer if needed
			if (r.index_buffer != last_index_buffer)
			{
				last_index_buffer = r.index_buffer;
				vkCmdBindIndexBuffer(cmd, r.index_buffer, 0, VK_INDEX_TYPE_UINT32);
			}

			// calculate final mesh matrix
			GpuDrawPushConstants push_constants;
			push_constants.world_matrix = r.transform;
			push_constants.vertex_buffer = r.vertex_buffer_address;

			vkCmdPushConstants(cmd, r.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GpuDrawPushConstants), &push_constants);

			vkCmdDrawIndexed(cmd, r.index_count, 1, r.first_index, 0, 0);

			//stats
			m_stats.drawcall_count++;
			m_stats.triangle_count += r.index_count / 3;
		};

	for (auto& r : opaque_draws)
	{
		draw(m_main_render_context.opaque_surfaces[r]);
	}

	for (auto& r : transparent_draws)
	{
		draw(m_main_render_context.transparent_surfaces[r]);
	}

	vkCmdEndRendering(cmd);

	// stats

	auto end = std::chrono::system_clock::now();

	//convert to microseconds (integer), and then come back to milliseconds
	auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	m_stats.mesh_draw_time = elapsed.count() / 1000.f;
}

void VulkanRenderer::draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView)
{
	VkRenderingAttachmentInfo color_attachment = vkinit::attachment_info(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingInfo render_info = vkinit::rendering_info(m_swapchain_extent, &color_attachment, nullptr);

	vkCmdBeginRendering(cmd, &render_info);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

	vkCmdEndRendering(cmd);
}

void VulkanRenderer::immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function)
{
	VK_CHECK(vkResetFences(m_device, 1, &m_immediate_fence));
	VK_CHECK(vkResetCommandBuffer(m_immediate_command_buffer, 0));

	VkCommandBuffer cmd = m_immediate_command_buffer;

	VkCommandBufferBeginInfo command_buffer_begin_info = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &command_buffer_begin_info));

	function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkCommandBufferSubmitInfo cmd_info = vkinit::command_buffer_submit_info(cmd);
	VkSubmitInfo2 submit = vkinit::submit_info(&cmd_info, nullptr, nullptr);

	// submit command buffer to the queue and execute it.
	//  _renderFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit2(m_graphics_queue, 1, &submit, m_immediate_fence));

	VK_CHECK(vkWaitForFences(m_device, 1, &m_immediate_fence, true, 9999999999));
}

bool VulkanRenderer::is_visible(const GpuRenderObject& obj, const glm::mat4& viewproj)
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

VulkanRenderer::RenderObjectId VulkanRenderer::add_render_object_predefined_mesh(const std::string& mesh_name, const glm::mat4& transform)
{
	assert(m_predefined_meshes.find(mesh_name) != m_predefined_meshes.end());

	auto id = m_id_pool.acquire_id();

	auto render_object = std::make_shared<RenderObject>();
	render_object->transform = transform;
	render_object->renderable = m_predefined_meshes[mesh_name];

	m_render_object_id_to_render_object[id] = render_object;

	return id;
}

VulkanRenderer::RenderObjectId VulkanRenderer::add_render_object_gltf_mesh(const std::string& gltf_file_path, const glm::mat4& transform)
{
	auto id = m_id_pool.acquire_id();

	auto gltf_mesh = load_gltf_mesh(gltf_file_path);

	assert(gltf_mesh.has_value());

	auto render_object = std::make_shared<RenderObject>();
	render_object->transform = transform;
	render_object->renderable = gltf_mesh.value();

	m_render_object_id_to_render_object[id] = render_object;

	return id;
}

void VulkanRenderer::remove_render_object(RenderObjectId id)
{
	auto it = m_render_object_id_to_render_object.find(id);

	if (it != m_render_object_id_to_render_object.end())
	{
		m_render_object_id_to_render_object.erase(it);
	}
}

void VulkanRenderer::update_render_object(RenderObjectId id, const glm::mat4& transform)
{
	auto it = m_render_object_id_to_render_object.find(id);

	if (it != m_render_object_id_to_render_object.end())
	{
		it->second->transform = transform;
	}
}
