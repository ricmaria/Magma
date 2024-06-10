// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vector>
#include <functional>
#include <deque>
#include <string>
#include <unordered_map>

#define NOMINMAX // GLM includes windows.h, which defines min and max. This stops it.

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include "core/id_pool.h"
#include "types.h"
#include "mesh.h"

class PipelineBuilder
{
public:
	std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
	VkPipelineVertexInputStateCreateInfo _vertexInputInfo;
	VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
	VkViewport _viewport;
	VkRect2D _scissor;
	VkPipelineRasterizationStateCreateInfo _rasterizer;
	VkPipelineColorBlendAttachmentState _colorBlendAttachment;
	VkPipelineMultisampleStateCreateInfo _multisampling;
	VkPipelineLayout _pipelineLayout;
	VkPipelineDepthStencilStateCreateInfo _depthStencil;

	VkPipeline build_pipeline(VkDevice device, VkRenderPass pass);
};

struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void push_function(std::function<void()>&& function)
	{
		deletors.push_back(function);
	}

	void flush()
	{
        // reverse iterate the deletion queue to execute all the functions
        for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
            (*it)(); //call functors
        }

        deletors.clear();
    }
};

struct MeshPushConstants
{
	glm::vec4 data;
	glm::mat4 render_matrix;
};


struct Material
{
	VkDescriptorSet textureSet{VK_NULL_HANDLE};
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
};

struct Texture
{
	AllocatedImage image;
	VkImageView imageView;
};

using RenderObjectId = IdPool::Id;

struct RenderObject
{
	RenderObjectId id;
	Mesh* mesh;
	Material* material;
	glm::mat4 transformMatrix;	
};


struct FrameData
{
	VkSemaphore _presentSemaphore, _renderSemaphore;
	VkFence _renderFence;

	DeletionQueue _frameDeletionQueue;

	VkCommandPool _commandPool;
	VkCommandBuffer _mainCommandBuffer;

	AllocatedBuffer cameraBuffer;
	VkDescriptorSet globalDescriptor;

	AllocatedBuffer objectBuffer;
	VkDescriptorSet objectDescriptor;
};

struct UploadContext
{
	VkFence _uploadFence;
	VkCommandPool _commandPool;	
	VkCommandBuffer _commandBuffer;
};
struct GPUCameraData
{
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 viewproj;
};


struct GPUSceneData
{
	glm::vec4 fogColor; // w is for exponent
	glm::vec4 fogDistances; //x for min, y for max, zw unused.
	glm::vec4 ambientColor;
	glm::vec4 sunlightDirection; //w for sun power
	glm::vec4 sunlightColor;
};

struct GPUObjectData
{
	glm::mat4 modelMatrix;
};

constexpr unsigned int FRAME_OVERLAP = 2;

class VulkanRenderer
{
public:

	bool _isInitialized{ false };
	int _frameNumber {0};
	int _selectedShader{ 0 };

	VkExtent2D _windowExtent{ 800, 600 };

	VkInstance _instance;
	VkDebugUtilsMessengerEXT _debug_messenger;
	VkPhysicalDevice _chosenGPU;
	VkDevice _device;

	VkPhysicalDeviceProperties _gpuProperties;

	FrameData _frames[FRAME_OVERLAP];
	
	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;
	
	VkRenderPass _renderPass;

	VkSurfaceKHR _surface;
	VkSwapchainKHR _swapchain;
	VkFormat _swachainImageFormat;

	std::vector<VkFramebuffer> _framebuffers;
	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;	

    DeletionQueue _mainDeletionQueue;
	
	VmaAllocator _allocator; //vma lib allocator

	//depth resources
	VkImageView _depthImageView;
	AllocatedImage _depthImage;

	//the format for the depth image
	VkFormat _depthFormat;

	VkDescriptorPool _descriptorPool;

	VkDescriptorSetLayout _globalSetLayout;
	VkDescriptorSetLayout _objectSetLayout;
	VkDescriptorSetLayout _singleTextureSetLayout;

	GPUSceneData _sceneParameters;
	AllocatedBuffer _sceneParameterBuffer;

	UploadContext _uploadContext;
	//initializes everything in the engine
	void init(uint32_t width, uint32_t height, struct SDL_Window* window);

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();
	
	FrameData& get_current_frame();
	FrameData& get_last_frame();

	inline glm::vec3 get_camera_position() { return _camera_position; }
	inline void set_camera_position(glm::vec3 position) { _camera_position = position; }
	void set_camera_view(const glm::vec3& forward, const glm::vec3& left, const glm::vec3& up);

	RenderObjectId add_render_object(const std::string& mesh_name, const std::string& material_name, glm::mat4 transform);
	void remove_render_object(RenderObjectId id);

	//default array of renderable objects
	std::vector<RenderObject> _renderables;

	std::unordered_map<std::string, Material> _materials;
	std::unordered_map<std::string, Mesh> _meshes;
	std::unordered_map<std::string, Texture> _loadedTextures;
	//functions

	//create material and add it to the map
	Material* create_material(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name);

	//returns nullptr if it cant be found
	Material* get_material(const std::string& name);

	//returns nullptr if it cant be found
	Mesh* get_mesh(const std::string& name);

	//our draw function
	void draw_objects(VkCommandBuffer cmd, RenderObject* first, int count);

	AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

	size_t pad_uniform_buffer_size(size_t originalSize);

	void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);
private:

	void init_vulkan(SDL_Window* window);

	void init_swapchain();

	void init_default_renderpass();

	void init_framebuffers();

	void init_commands();

	void init_sync_structures();

	void init_pipelines();

	void init_scene();

	void init_descriptors();

	//loads a shader module from a spir-v file. Returns false if it errors
	bool load_shader_module(const char* filePath, VkShaderModule* outShaderModule);

	void load_meshes();

	void load_images();

	void upload_mesh(Mesh& mesh);

	glm::vec3 _camera_position = { 0.f,-6.f,-10.f };

	glm::mat4 _camera_view{ glm::vec4{1,0,0,0}, glm::vec4{0,1,0,0}, glm::vec4{0,0,-1,0}, glm::vec4{0,0,0,1} };

	IdPool id_pool;
};
