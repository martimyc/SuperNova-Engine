#ifndef RENDERER
#define RENDERER

#include "Module.h"
#include "vulkan/vulkan.hpp"

struct GLFWwindow;

class Renderer : public Module
{
public:
	explicit Renderer();
	Renderer(Renderer const &) = delete;
	Renderer(Renderer &&) noexcept = default;
	Renderer & operator = (Renderer const &) = delete;
	Renderer & operator = (Renderer &&) noexcept = default;
	~Renderer() noexcept = default;

	void Start() final;
	void PreUpdate() final;
	void Update() final;
	void PostUpdate() final;
	void CleanUp() noexcept final;

private:
	struct QueueFamilyIndices;
	struct SwapChainSupportDetails;

	void InitWindow();
	void InitVulkan();
	void CleanUpVulkan();
	void CleanUpWindow();

	void CreateInstance();
	void SetUpDebugMessenger();
	void CreateSurface();
	void PickPhysicalDevice();
	void CreateLogicalDevice();
	void CreateSwapChain();
	void CreateImageViews();
	void CreateRenderPass();
	void CreateGraphicsPipeline();
	void CreateFrameBuffers();
	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateSyncObjects();

	void DrawFrame();

	[[noreturn]] static void CreateInstanceErrorHandling(VkResult const &);
	[[noreturn]] static void CreateLogicalDeviceErrorHandling(VkResult const &);
	[[noreturn]] static void CreateSwapChainErrorHandling(VkResult const &);
	[[noreturn]] static void CreateImageViewsErrorHandling(VkResult const &);
	[[noreturn]] static void CreateShaderModuleErrorHandling(VkResult const &);
	[[noreturn]] static void CreateRenderPassErrorHandling(VkResult const &);
	[[noreturn]] static void CreatePipelineLayoutErrorHandling(VkResult const &);
	[[noreturn]] static void CreatePipelineErrorHandling(VkResult const &);
	[[noreturn]] static void CreateFrameBufferErrorHandling(VkResult const &);
	[[noreturn]] static void CreateCommandPoolErrorHandling(VkResult const &);
	[[noreturn]] static void CreateCommandBuffersErrorHandling(VkResult const &);
	[[noreturn]] static void CreateSemaphoreErrorHandling(VkResult const &);
	[[noreturn]] static void CreateFenceErrorHandling(VkResult const &);

	// Instance
	static void CheckValidationLayerSupport();
	[[nodiscard]] static VkApplicationInfo const GetVulkanAppInfoConfig();
	[[nodiscard]] static std::vector<char const*> const GetRequiredInstanceExtensions();
	static void CheckRequiredInstanceExtensionsSupport(std::vector<char const*> const &, std::vector<VkExtensionProperties> const &);

	// Debug Messenger
	[[nodiscard]] static VkDebugUtilsMessengerCreateInfoEXT GetDebugMessengerConfig();

	// Physical Device
	[[nodiscard]] int RatePhysicalDevice(VkPhysicalDevice const &) const;
	[[nodiscard]] static bool CheckRequiredPhysicalDeviceExtensionSupport(VkPhysicalDevice const &);

	// Logical Device
	[[nodiscard]] static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice const &, VkSurfaceKHR const &);
	[[nodiscard]] static VkDeviceQueueCreateInfo GetDeviceQueueConfig(uint32_t);

	// Swap Chain
	[[nodiscard]] SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice const &) const;
	[[nodiscard]] static VkSurfaceFormatKHR GetSwapSurfaceFormat(std::vector<VkSurfaceFormatKHR> const &);
	[[nodiscard]] static VkPresentModeKHR GetSwapPresentationMode(std::vector<VkPresentModeKHR> const &);
	[[nodiscard]] VkExtent2D GetSwapExtent(VkSurfaceCapabilitiesKHR const &) const;

	// Render Pass
	[[nodiscard]] static VkAttachmentDescription GetColorAttachmentConfig(VkFormat const &);
	[[nodiscard]] static VkAttachmentReference GetColorAttachmentReferenceConfig();
	[[nodiscard]] static VkSubpassDescription GetSubpassConfig(std::vector<VkAttachmentReference> const &);
	[[nodiscard]] static VkSubpassDependency GetSubpassDependencyConfig();

	// Graphics Pipeline
	[[nodiscard]] VkShaderModule CreateShaderModule(std::string const &) const;
	[[nodiscard]] static VkPipelineShaderStageCreateInfo GetVertexShaderPipelineStageConfig(VkShaderModule const &);
	[[nodiscard]] static VkPipelineShaderStageCreateInfo GetFragmentShaderPipelineStageConfig(VkShaderModule const &);
	[[nodiscard]] static VkPipelineVertexInputStateCreateInfo GetPipelineVertexInputConfig();
	[[nodiscard]] static VkPipelineInputAssemblyStateCreateInfo GetPipelineInputAssemblyConfig();
	[[nodiscard]] static VkViewport GetViewportConfig(float, float);
	[[nodiscard]] static VkRect2D GetScissorConfig(VkExtent2D const &);
	[[nodiscard]] static VkPipelineViewportStateCreateInfo GetViewportStateConfig(VkViewport const &, VkRect2D const &);
	[[nodiscard]] static VkPipelineRasterizationStateCreateInfo GetRasterizerConfig();
	[[nodiscard]] static VkPipelineMultisampleStateCreateInfo GetMultisamplingConfig();
	[[nodiscard]] static VkPipelineColorBlendAttachmentState GetColorBlendAttachmentConfig();
	[[nodiscard]] static VkPipelineColorBlendStateCreateInfo GetColorBlendConfig(VkPipelineColorBlendAttachmentState const &);
	[[nodiscard]] static VkPipelineDynamicStateCreateInfo GetDynamicStateCongif(std::vector<VkDynamicState> const &);
	[[nodiscard]] static VkPipelineLayoutCreateInfo GetPipelineLayoutConfig();

	// Frame buffers
	void CreateFrameBuffer(VkImageView const &);

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT,
		VkDebugUtilsMessageTypeFlagsEXT,
		const VkDebugUtilsMessengerCallbackDataEXT*,
		void*);

	static VkResult CreateDebugUtilsMessengerEXT(
		VkInstance,
		const VkDebugUtilsMessengerCreateInfoEXT*,
		const VkAllocationCallbacks*,
		VkDebugUtilsMessengerEXT*);

	static void DestroyDebugUtilsMessengerEXT(
		VkInstance,
		VkDebugUtilsMessengerEXT,
		const VkAllocationCallbacks*);

	static std::vector<char> ReadShaderFile(std::string const & filename);

private:
	GLFWwindow* m_window{ nullptr };
	uint32_t const m_window_width{};
	uint32_t const m_window_height{};
	VkInstance m_instance{};
	VkSurfaceKHR m_surface{};
	VkDebugUtilsMessengerEXT m_debug_messenger{};
	VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
	VkDevice m_logical_device{};
	VkQueue m_graphics_queue{};
	VkQueue m_presentation_queue{};
	VkSwapchainKHR m_swap_chain{};
	std::vector<VkImage> m_swap_chain_images{};
	VkFormat m_swap_chain_image_format{};
	VkExtent2D m_swap_chain_extent{};
	std::vector<VkImageView> m_swap_chain_image_views{};
	VkRenderPass m_render_pass{};
	VkPipelineLayout m_pipeline_layout{};
	VkPipeline m_graphics_pipeline{};
	std::vector<VkFramebuffer> m_swap_chain_framebuffers{};
	VkCommandPool m_command_pool{};
	std::vector<VkCommandBuffer> m_command_buffers{};
	std::vector<VkSemaphore> m_image_available_semaphores{};
	std::vector<VkSemaphore> m_render_finished_semaphores{};
	std::vector<VkFence> m_in_flight_fences{};
	size_t m_current_frame{0};

private:
	struct QueueFamilyIndices {
		std::optional<uint32_t> graphics_family{};
		std::optional<uint32_t> presentation_family;

		[[nodiscard]] bool SupportsGraphicsFamily() {
			return graphics_family.has_value();
		}
		[[nodiscard]] bool SupportsPresentationFamily() {
			return presentation_family.has_value();
		}
		[[nodiscard]] bool SupportsAllRequiredFamilies() {
			return SupportsGraphicsFamily() &&
				SupportsPresentationFamily();
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};
};

#endif // !RENDERER
