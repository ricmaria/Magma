// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include "types.h"
#include "descriptors.h"
#include "loader.h"

#include "core/id_pool.h"

union SDL_Event;

class VulkanRenderer
{
private:
	static constexpr unsigned int FRAME_OVERLAP = 2;

public:

	using RenderInstanceId = IdPool::Id;

	static const RenderInstanceId invalid_render_instance_id = IdPool::Invalid;

	static VulkanRenderer& get();

	//initializes everything in the engine
	void init(uint32_t width, uint32_t height, struct SDL_Window* window);

	//shuts down the engine
	void cleanup();

	void process_sdl_event(const SDL_Event* sdl_event);

	//draw loop
	void draw();

	inline glm::vec3 get_camera_position() { return _camera_position; }
	inline void set_camera_position(glm::vec3 position) { _camera_position = position; }
	inline void set_camera_axes(const glm::vec3& x, const glm::vec3& y, const glm::vec3& z) { _camera_axes[0] = x; _camera_axes[1] = y; _camera_axes[2] = z; }

	RenderInstanceId add_render_instance(const std::string& mesh_name, const glm::mat4& transform);
	void remove_render_instance(RenderInstanceId id);
	void update_render_instance(RenderInstanceId id, const glm::mat4& transform);

private:
	class DeletionQueue
	{
	public:
		void push_function(std::function<void()>&& function)
		{
			_deletors.push_back(function);
		}

		void flush()
		{
			// reverse iterate the deletion queue to execute all the functions
			for (auto it = _deletors.rbegin(); it != _deletors.rend(); it++)
			{
				(*it)(); //call functors
			}

			_deletors.clear();
		}
	private:
		std::deque<std::function<void()>> _deletors;
	};

	struct FrameData
	{
		VkCommandPool _commandPool;
		VkCommandBuffer _mainCommandBuffer;

		VkSemaphore _swapchainSemaphore;
		VkSemaphore _renderSemaphore;
		VkFence _renderFence;

		DeletionQueue _deletionQueue;
		DescriptorAllocatorGrowable _frameDescriptors;
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
		glm::vec4 ambientColor;
		glm::vec4 sunlightDirection; // w for sun power
		glm::vec4 sunlightColor;
	};

	struct RenderInstance
	{
		RenderInstanceId id;
		std::string mesh_name;
		glm::mat4 transform;
	};

	struct EngineStats
	{
		float frametime;
		int triangle_count;
		int drawcall_count;
		float scene_update_time;
		float mesh_draw_time;
	};

	FrameData& get_current_frame() { return _frames[_frameNumber % FRAME_OVERLAP]; };

	void init_vulkan();
	void init_swapchain();
	void init_commands();
	void init_sync_structures();
	void init_descriptors();
	void init_pipelines();
	void init_background_pipelines();
	void init_triangle_pipeline();
	void init_mesh_pipeline();
	void init_default_data();
	void init_scenes();
	void init_imgui();

	void create_swapchain(uint32_t width, uint32_t height);
	void destroy_swapchain();
	void resize_swapchain();

	AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	void destroy_buffer(const AllocatedBuffer& buffer);

	AllocatedImage create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
	AllocatedImage create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
	void destroy_image(const AllocatedImage& image);

	GPUMeshBuffers upload_mesh(std::span<uint32_t> indices, std::span<Vertex> vertices);

	void draw_scene();

	void update_scene();

	void update_imgui();

	void draw_background(VkCommandBuffer cmd);

	void draw_geometry(VkCommandBuffer cmd);

	void draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView);

	void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);

	static bool is_visible(const RenderObject& obj, const glm::mat4& viewproj);

	struct SDL_Window* _window{ nullptr };

	bool _isInitialized{ false };
	int _frameNumber{ 0 };
	bool _stopRendering{ false };
	bool _resizeRequested{ false };

	VkExtent2D _windowExtent{ 1700 , 900 };

	FrameData _frames[FRAME_OVERLAP];

	VkInstance _instance;// Vulkan library handle
	VkDebugUtilsMessengerEXT _debugMessenger;// Vulkan debug output handle
	VkPhysicalDevice _chosenGPU;// GPU chosen as the default device
	VkDevice _device; // Vulkan device for commands
	VkSurfaceKHR _surface;// Vulkan window surface

	VkSwapchainKHR _swapchain;
	VkFormat _swapchainImageFormat;

	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;
	VkExtent2D _swapchainExtent;

	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;

	DeletionQueue _mainDeletionQueue;

	VmaAllocator _allocator;

	//draw resources
	AllocatedImage _drawImage;
	AllocatedImage _depthImage;
	VkExtent2D _drawExtent;
	float _renderScale = 1.f;

	DescriptorAllocatorGrowable _globalDescriptorAllocator;

	VkDescriptorSet _drawImageDescriptors;
	VkDescriptorSetLayout _drawImageDescriptorLayout;

	GPUSceneData _sceneData;
	VkDescriptorSetLayout _gpuSceneDataDescriptorLayout;

	VkDescriptorSetLayout _singleImageDescriptorLayout;

	VkPipeline _computePipeline;
	VkPipelineLayout _computePipelineLayout;

	VkPipelineLayout _trianglePipelineLayout;
	VkPipeline _trianglePipeline;

	// immediate submit structures
	VkFence _immediate_fence;
	VkCommandBuffer _immediate_command_buffer;
	VkCommandPool _immCommandPool;

	std::vector<ComputeEffect> _backgroundEffects;
	int _currentBackgroundEffect{ 0 };

	VkPipelineLayout _meshPipelineLayout;
	VkPipeline _meshPipeline;

	GPUMeshBuffers _rectangle;

	std::vector<std::shared_ptr<MeshAsset>> _test_meshes;

	AllocatedImage _whiteImage;
	AllocatedImage _blackImage;
	AllocatedImage _greyImage;
	AllocatedImage _errorCheckerboardImage;

	VkSampler _defaultSamplerLinear;
	VkSampler _defaultSamplerNearest;

	MaterialInstance _defaultData;
	GLTFMetallic_Roughness _metalRoughMaterial;

	DrawContext _mainDrawContext;
	std::unordered_map<std::string, std::shared_ptr<Node>> _loadedNodes;

	std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>> _loadedScenes;

	EngineStats _stats;

	IdPool _id_pool;

	glm::vec3 _camera_position = { 0.f,-6.f,-10.f };

	std::array<glm::vec3,3> _camera_axes;

	std::unordered_map<RenderInstanceId, RenderInstance> m_render_instance_id_to_render_instance;
};