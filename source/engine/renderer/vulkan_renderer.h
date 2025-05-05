// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include "types.h"
#include "descriptors.h"
#include "gltf_mesh.h"

#include "core/id_pool.h"
#include "core/math.h"

union SDL_Event;

class VulkanRenderer
{
private:
	static constexpr unsigned int FRAME_OVERLAP = 2;

public:

	using RenderObjectId = IdPool::Id;

	static const RenderObjectId invalid_render_object_id = IdPool::Invalid;

	//initializes everything in the engine
	void init(uint32_t width, uint32_t height, struct SDL_Window* window);

	//shuts down the engine
	void cleanup();

	void process_sdl_event(const SDL_Event* sdl_event);

	//draw loop
	void draw();

	inline void set_camera_transform(const Transform& transform) { m_camera_transform = transform; }

	inline void set_directional_light_direction(const glm::vec3& direction) { m_directional_light_direction = direction; }
	inline void set_directional_light_color(const glm::vec4& color) { m_directional_light_color = color; }

	RenderObjectId add_render_object_predefined_mesh(const std::string& mesh_name, const Transform& transform);
	RenderObjectId add_render_object_gltf_mesh(const std::string& gltf_file_path, const Transform& transform);

	void remove_render_object(RenderObjectId id);
	
	void update_render_object(RenderObjectId id, const Transform& transform);

private:
	class DeletionQueue
	{
	public:
		void push_function(std::function<void()>&& function)
		{
			m_deletors.push_back(function);
		}

		void flush()
		{
			// reverse iterate the deletion queue to execute all the functions
			for (auto it = m_deletors.rbegin(); it != m_deletors.rend(); it++)
			{
				(*it)(); //call functors
			}

			m_deletors.clear();
		}
	private:
		std::deque<std::function<void()>> m_deletors;
	};

	struct FrameData
	{
		VkCommandPool command_pool;
		VkCommandBuffer main_command_buffer;

		VkSemaphore swapchain_semaphore;
		VkSemaphore render_semaphore;
		VkFence render_fence;

		DeletionQueue deletion_queue;
		DescriptorAllocatorGrowable frame_descriptors;
	};

	struct ComputePushConstants
	{
		glm::vec4 data1;
		glm::vec4 data2;
		glm::vec4 data3;
		glm::vec4 data4;
	};

	struct ComputeEffect
	{
		const char* name;

		VkPipeline pipeline;
		VkPipelineLayout layout;

		ComputePushConstants data;
	};

	struct GPUSceneData
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 viewproj;
		glm::vec4 ambient_color;
		glm::vec4 sunlight_direction; // w for sun power
		glm::vec4 sunlight_color;
	};

	struct RenderObject
	{
		RenderObjectId id;
		glm::mat4 transform;
		std::shared_ptr<IRenderable> renderable;
	};

	struct EngineStats
	{
		float frametime;
		int triangle_count;
		int drawcall_count;
		float scene_update_time;
		float mesh_draw_time;
	};

	FrameData& get_current_frame() { return m_frames[m_frame_number % FRAME_OVERLAP]; };

	void init_vulkan();
	void init_swapchain();
	void init_commands();
	void init_sync_structures();
	void init_descriptors();
	void init_pipelines();
	void init_background_pipelines();
	void init_default_data();
	void init_imgui();

	void create_swapchain(uint32_t width, uint32_t height);
	void destroy_swapchain();
	void resize_swapchain();

	AllocatedBuffer create_buffer(size_t alloc_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage);
	void destroy_buffer(const AllocatedBuffer& buffer);

	AllocatedImage create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
	AllocatedImage create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
	void destroy_image(const AllocatedImage& image);

	GpuMeshBuffers upload_mesh(std::span<uint32_t> indices, std::span<Vertex> vertices);

	std::optional<std::shared_ptr<GltfMesh>> load_gltf_mesh(std::string_view gltf_file_path);

	void add_render_objects_to_context();

	void update_imgui();

	void draw_scene();

	void draw_background(VkCommandBuffer cmd);

	void draw_main_render_context(VkCommandBuffer cmd);

	void draw_imgui(VkCommandBuffer cmd, VkImageView target_image_view);

	void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);

	static bool is_visible(const GpuRenderObject& obj, const glm::mat4& viewproj);

	struct SDL_Window* m_window{ nullptr };

	bool m_initialized{ false };
	int m_frame_number{ 0 };
	bool m_stop_rendering{ false };
	bool m_resize_requested{ false };

	VkExtent2D m_window_extent{ 1700 , 900 };

	FrameData m_frames[FRAME_OVERLAP];

	VkInstance m_instance;// Vulkan library handle
	VkDebugUtilsMessengerEXT m_debug_messenger;// Vulkan debug output handle
	VkPhysicalDevice m_chosen_gpu;// GPU chosen as the default device
	VkDevice m_device; // Vulkan device for commands
	VkSurfaceKHR m_surface;// Vulkan window surface

	VkSwapchainKHR m_swapchain;
	VkFormat m_swapchain_image_format;

	std::vector<VkImage> m_swapchain_images;
	std::vector<VkImageView> m_swapchain_image_views;
	VkExtent2D m_swapchain_extent;

	VkQueue m_graphics_queue;
	uint32_t m_graphics_queue_family;

	DeletionQueue m_main_deletion_queue;

	VmaAllocator m_allocator;

	//draw resources
	AllocatedImage m_draw_image;
	AllocatedImage m_depth_image;
	VkExtent2D m_draw_extent;
	float m_render_scale = 1.f;

	DescriptorAllocatorGrowable m_global_descriptor_allocator;

	VkDescriptorSet m_draw_image_descriptors;
	VkDescriptorSetLayout m_draw_image_descriptor_layout;

	GPUSceneData m_scene_data;
	VkDescriptorSetLayout m_gpu_scene_data_descriptor_layout;

	VkDescriptorSetLayout m_single_image_descriptor_layout;

	VkPipeline m_compute_pipeline;
	VkPipelineLayout m_compute_pipeline_layout ;

	// immediate submit structures
	VkFence m_immediate_fence;
	VkCommandBuffer m_immediate_command_buffer;
	VkCommandPool m_immediate_command_pool;

	std::vector<ComputeEffect> m_background_effects;
	int m_current_background_effect{ 0 };

	AllocatedImage m_white_image;
	AllocatedImage m_black_image;
	AllocatedImage m_grey_image;
	AllocatedImage m_error_checkerboard_image;

	VkSampler m_default_sampler_linear;
	VkSampler m_default_sampler_nearest;

	MaterialInstance m_default_data;
	MetallicRoughnessMaterial m_metal_rough_material;

	RenderContext m_main_render_context;
	std::unordered_map<std::string, std::shared_ptr<Node>> m_predefined_meshes;

	EngineStats m_stats;

	IdPool m_id_pool;

	Transform m_camera_transform;

	std::unordered_map<RenderObjectId, std::shared_ptr<RenderObject>> m_render_object_id_to_render_object;

	glm::vec3 m_directional_light_direction = { 0.0f, 1.0f, 0.0f };
	glm::vec4 m_directional_light_color = glm::vec4{ 1.0f };
};