#include "PreCompiledHeader.hpp"
#include "Renderer.h"
#include "glfw3.h"
#include "Application.hpp"

const std::vector<const char*> validation_layers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> device_extensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

Renderer::Renderer():
	m_window{ nullptr },
	m_window_width{ 800 },
	m_window_height{ 600 },
	m_instance{},
	m_debug_messenger{},
	m_physical_device{ VK_NULL_HANDLE },
	m_logical_device{},
	m_graphics_queue{},
	m_swap_chain{},
	m_swap_chain_images{},
	m_swap_chain_image_format{},
	m_swap_chain_extent{},
	m_swap_chain_image_views{},
	m_render_pass{},
	m_pipeline_layout{},
	m_graphics_pipeline{},
	m_swap_chain_framebuffers{},
	m_command_pool{},
	m_command_buffers{},
	m_image_available_semaphores{ MAX_FRAMES_IN_FLIGHT },
	m_render_finished_semaphores{ MAX_FRAMES_IN_FLIGHT },
	m_in_flight_fences{ MAX_FRAMES_IN_FLIGHT },
	m_current_frame{}
{}

void Renderer::Start()
{	
	InitWindow();
	InitVulkan();
}

void Renderer::PreUpdate()
{
	glfwPollEvents();
	if (glfwWindowShouldClose(m_window)) {
		m_subject.BroadcastEvent(CloseWindowEvent{*this});
	}
}

void Renderer::Update()
{
}

void Renderer::PostUpdate()
{
	DrawFrame();
}

void Renderer::CleanUp() noexcept
{
	vkDeviceWaitIdle(m_logical_device);
	CleanUpVulkan();
	CleanUpWindow();
}

void Renderer::InitWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	m_window = glfwCreateWindow(m_window_width, m_window_height, "Vulkan", nullptr, nullptr);
}

void Renderer::InitVulkan()
{
	CreateInstance();
#ifdef _DEBUG
	SetUpDebugMessenger();
#endif
	CreateSurface();
	PickPhysicalDevice();
	CreateLogicalDevice();
	CreateSwapChain();
	CreateImageViews();
	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateFrameBuffers();
	CreateCommandPool();
	CreateCommandBuffers();
	CreateSyncObjects();
}

void Renderer::CleanUpVulkan()
{
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		vkDestroySemaphore(m_logical_device, m_render_finished_semaphores[i], nullptr);
		vkDestroySemaphore(m_logical_device, m_image_available_semaphores[i], nullptr);
		vkDestroyFence(m_logical_device, m_in_flight_fences[i], nullptr);
	}

	vkDestroyCommandPool(m_logical_device, m_command_pool, nullptr);

	for (auto framebuffer : m_swap_chain_framebuffers) {
		vkDestroyFramebuffer(m_logical_device, framebuffer, nullptr);
	}

	vkDestroyPipeline(m_logical_device, m_graphics_pipeline, nullptr);
	vkDestroyPipelineLayout(m_logical_device, m_pipeline_layout, nullptr);
	vkDestroyRenderPass(m_logical_device, m_render_pass, nullptr);

	for (auto image_view : m_swap_chain_image_views) {
		vkDestroyImageView(m_logical_device, image_view, nullptr);
	}

	vkDestroySwapchainKHR(m_logical_device, m_swap_chain, nullptr);
	vkDestroyDevice(m_logical_device, nullptr);
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
#ifdef _DEBUG
	DestroyDebugUtilsMessengerEXT(m_instance, m_debug_messenger, nullptr);
#endif
	vkDestroyInstance(m_instance, nullptr);
}

void Renderer::CleanUpWindow()
{
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void Renderer::CreateInstance()
{
#ifdef _DEBUG
		CheckValidationLayerSupport();
#endif

	auto app_info = GetVulkanAppInfoConfig();

	auto create_info = VkInstanceCreateInfo{};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;

	auto extensions = GetRequiredInstanceExtensions();

	create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	create_info.ppEnabledExtensionNames = extensions.data();	

#ifdef _DEBUG
	create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
	create_info.ppEnabledLayerNames = validation_layers.data();

	auto debug_create_info = GetDebugMessengerConfig();
	create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
#else
	create_info.enabledLayerCount = 0;
	create_info.pNext = nullptr;
#endif

	auto result = vkCreateInstance(&create_info, nullptr, &m_instance);

	if (result != VK_SUCCESS) {
		CreateInstanceErrorHandling(result);
	}
}

void Renderer::SetUpDebugMessenger()
{
	auto debug_info = GetDebugMessengerConfig();

	if (CreateDebugUtilsMessengerEXT(m_instance, &debug_info, nullptr, &m_debug_messenger) != VK_SUCCESS) {
		throw std::runtime_error("Failed to set up debug messenger.");
	}
}

void Renderer::CreateSurface()
{
	if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create window surface!");
	}
}

void Renderer::PickPhysicalDevice()
{
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr);

	if (device_count == 0) {
		throw std::runtime_error("Failed to find GPUs with Vulkan support!");
	}
	
	auto devices = std::vector<VkPhysicalDevice>{ device_count };
	vkEnumeratePhysicalDevices(m_instance, &device_count, devices.data());

	auto rated_devices = std::multimap<int, VkPhysicalDevice>{};

	for (auto const & device : devices) {
		auto rating = RatePhysicalDevice(device);
		rated_devices.emplace(std::make_pair(rating, device));
	}

	auto best_rated_device_iterator = rated_devices.rbegin();

	if (best_rated_device_iterator->first > 0) {
		m_physical_device = best_rated_device_iterator->second;
	}
	else {
		throw std::runtime_error("Failed to find a suitable GPU!");
	}
}

void Renderer::CreateLogicalDevice()
{
	auto indices = FindQueueFamilies(m_physical_device, m_surface);

	auto queue_create_infos = std::vector<VkDeviceQueueCreateInfo>{};
	auto unique_queue_families = std::set<uint32_t> { indices.graphics_family.value(), indices.presentation_family.value() };

	for (uint32_t queueFamily : unique_queue_families) {
		queue_create_infos.push_back(GetDeviceQueueConfig(queueFamily));
	}

	VkPhysicalDeviceFeatures deviceFeatures = {}; //empty for now

	auto create_info = VkDeviceCreateInfo {};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
	create_info.pQueueCreateInfos = queue_create_infos.data();
	create_info.pEnabledFeatures = &deviceFeatures;
	create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
	create_info.ppEnabledExtensionNames = device_extensions.data();
#ifdef _DEBUG
	create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
	create_info.ppEnabledLayerNames = validation_layers.data();
#else
	create_info.enabledLayerCount = 0;
#endif

	auto result = vkCreateDevice(m_physical_device, &create_info, nullptr, &m_logical_device);

	if (result != VK_SUCCESS) {
		CreateLogicalDeviceErrorHandling(result);
	}

	vkGetDeviceQueue(m_logical_device, indices.graphics_family.value(), 0, &m_graphics_queue);
	vkGetDeviceQueue(m_logical_device, indices.presentation_family.value(), 0, &m_presentation_queue);
}

void Renderer::CreateSwapChain()
{
	auto swap_chain_support = QuerySwapChainSupport(m_physical_device);

	auto surface_format = GetSwapSurfaceFormat(swap_chain_support.formats);
	auto presentation_mode = GetSwapPresentationMode(swap_chain_support.presentModes);
	auto extent = GetSwapExtent(swap_chain_support.capabilities);

	uint32_t const extra_images = 1;
	auto image_count = swap_chain_support.capabilities.minImageCount + extra_images;

	if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount) {
		image_count = swap_chain_support.capabilities.maxImageCount;
	}

	auto create_info = VkSwapchainCreateInfoKHR{};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = m_surface;

	create_info.minImageCount = image_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	auto indices = FindQueueFamilies(m_physical_device, m_surface);
	uint32_t queue_family_indices[] = { indices.graphics_family.value(), indices.presentation_family.value() };

	if (indices.graphics_family != indices.presentation_family) {
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = queue_family_indices;
	}
	else {
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0;
		create_info.pQueueFamilyIndices = nullptr;
	}

	create_info.preTransform = swap_chain_support.capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = presentation_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	auto result = vkCreateSwapchainKHR(m_logical_device, &create_info, nullptr, &m_swap_chain);

	if (result != VK_SUCCESS){
		CreateSwapChainErrorHandling(result);
	}

	vkGetSwapchainImagesKHR(m_logical_device, m_swap_chain, &image_count, nullptr);
	m_swap_chain_images.resize(image_count);
	vkGetSwapchainImagesKHR(m_logical_device, m_swap_chain, &image_count, m_swap_chain_images.data());

	m_swap_chain_image_format = surface_format.format;
	m_swap_chain_extent = extent;
}

void Renderer::CreateImageViews()
{
	m_swap_chain_image_views.resize(m_swap_chain_images.size());

	for (auto i = 0; i < m_swap_chain_images.size(); ++i) {
		auto create_info = VkImageViewCreateInfo{};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.image = m_swap_chain_images[i];
		create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format = m_swap_chain_image_format;
		create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		create_info.subresourceRange.baseMipLevel = 0;
		create_info.subresourceRange.levelCount = 1;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount = 1;

		auto result = vkCreateImageView(m_logical_device, &create_info, nullptr, &m_swap_chain_image_views[i]);
		if (result != VK_SUCCESS) {
			CreateRenderPassErrorHandling(result);
		}
	}
}

void Renderer::CreateRenderPass()
{
	auto color_attachment = GetColorAttachmentConfig(m_swap_chain_image_format);
	auto color_attachment_ref = GetColorAttachmentReferenceConfig();

	std::vector<VkAttachmentReference> color_attachment_refs{
	color_attachment_ref
	};

	auto subpass = GetSubpassConfig(color_attachment_refs);

	auto render_pass_info = VkRenderPassCreateInfo{};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = 1;
	render_pass_info.pAttachments = &color_attachment;
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;

	auto dependency = GetSubpassDependencyConfig();
	render_pass_info.dependencyCount = 1;
	render_pass_info.pDependencies = &dependency;

	auto result = vkCreateRenderPass(m_logical_device, &render_pass_info, nullptr, &m_render_pass);
	if (result != VK_SUCCESS) {
		CreateRenderPassErrorHandling(result);
	}
}

void Renderer::CreateGraphicsPipeline()
{
	auto vertex_shader_module = CreateShaderModule("Shaders/vert.spv");
	auto fragment_shader_module = CreateShaderModule("Shaders/frag.spv");
	
	auto vertex_shader_stage_info = GetVertexShaderPipelineStageConfig(vertex_shader_module);
	auto fragment_shader_stage_info = GetFragmentShaderPipelineStageConfig(fragment_shader_module);

	auto shader_stages = std::vector<VkPipelineShaderStageCreateInfo>{
		vertex_shader_stage_info,
		fragment_shader_stage_info
	};

	auto vertex_input_info = GetPipelineVertexInputConfig();
	auto input_assembly_info = GetPipelineInputAssemblyConfig();
	auto viewport = GetViewportConfig(static_cast<float>(m_window_width), static_cast<float>(m_window_height));
	auto scissor = GetScissorConfig(m_swap_chain_extent);
	auto viewport_state = GetViewportStateConfig(viewport, scissor);
	auto rasterizer = GetRasterizerConfig();
	auto multisampling = GetMultisamplingConfig();
	auto color_blend_attachment = GetColorBlendAttachmentConfig();
	auto color_blending = GetColorBlendConfig(color_blend_attachment);

	auto dynamic_states = std::vector<VkDynamicState>{
	VK_DYNAMIC_STATE_VIEWPORT,
	VK_DYNAMIC_STATE_LINE_WIDTH
	};

	auto dynamic_state = GetDynamicStateCongif(dynamic_states);
	auto pipeline_layout_info = GetPipelineLayoutConfig();

	auto create_pipeline_layout_result = vkCreatePipelineLayout(m_logical_device, &pipeline_layout_info, nullptr, &m_pipeline_layout);
	if (create_pipeline_layout_result != VK_SUCCESS) {
		CreatePipelineLayoutErrorHandling(create_pipeline_layout_result);
	}

	auto pipeline_info = VkGraphicsPipelineCreateInfo{};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount = static_cast<uint32_t>(shader_stages.size());
	pipeline_info.pStages = shader_stages.data();
	pipeline_info.pVertexInputState = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly_info;
	pipeline_info.pViewportState = &viewport_state;
	pipeline_info.pRasterizationState = &rasterizer;
	pipeline_info.pMultisampleState = &multisampling;
	pipeline_info.pDepthStencilState = nullptr;
	pipeline_info.pColorBlendState = &color_blending;
	pipeline_info.pDynamicState = nullptr;
	pipeline_info.layout = m_pipeline_layout;
	pipeline_info.renderPass = m_render_pass;
	pipeline_info.subpass = 0;
	pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
	pipeline_info.basePipelineIndex = -1;

	auto create_pipeline_result = vkCreateGraphicsPipelines(m_logical_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &m_graphics_pipeline);
	if (create_pipeline_result != VK_SUCCESS) {
		CreatePipelineErrorHandling(create_pipeline_result);
	}

	vkDestroyShaderModule(m_logical_device, fragment_shader_module, nullptr);
	vkDestroyShaderModule(m_logical_device, vertex_shader_module, nullptr);
}

void Renderer::CreateFrameBuffers()
{
	m_swap_chain_framebuffers.reserve(m_swap_chain_image_views.size());
	for (VkImageView image_view : m_swap_chain_image_views) {
		CreateFrameBuffer(image_view);
	}
}

void Renderer::CreateCommandPool()
{
	auto queue_family_indices = FindQueueFamilies(m_physical_device, m_surface);

	auto pool_info = VkCommandPoolCreateInfo{};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.queueFamilyIndex = queue_family_indices.graphics_family.value();
	pool_info.flags = 0;

	auto result = vkCreateCommandPool(m_logical_device, &pool_info, nullptr, &m_command_pool);		
	if ( result != VK_SUCCESS) {
		CreateCommandPoolErrorHandling(result);
	}
}

void Renderer::CreateCommandBuffers()
{
	m_command_buffers.resize(m_swap_chain_framebuffers.size());

	auto alloc_info = VkCommandBufferAllocateInfo{};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = m_command_pool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = static_cast<uint32_t>(m_command_buffers.size());

	auto result = vkAllocateCommandBuffers(m_logical_device, &alloc_info, m_command_buffers.data());
	if (result != VK_SUCCESS) {
		CreateCommandBuffersErrorHandling(result);
	}

	for (auto i = 0; i < m_command_buffers.size(); ++i ) {
		auto begin_info = VkCommandBufferBeginInfo{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = 0; 
		begin_info.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(m_command_buffers[i], &begin_info) != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin recording command buffer!");
		}

		auto render_pass_info = VkRenderPassBeginInfo{};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_info.renderPass = m_render_pass;
		render_pass_info.framebuffer = m_swap_chain_framebuffers[i];
		render_pass_info.renderArea.offset = { 0, 0 };
		render_pass_info.renderArea.extent = m_swap_chain_extent;
		auto clear_color = VkClearValue { 0.0f, 0.0f, 0.0f, 1.0f };
		render_pass_info.clearValueCount = 1;
		render_pass_info.pClearValues = &clear_color;

		vkCmdBeginRenderPass(m_command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(m_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics_pipeline);

		vkCmdDraw(m_command_buffers[i], 3, 1, 0, 0);

		vkCmdEndRenderPass(m_command_buffers[i]);

		if (vkEndCommandBuffer(m_command_buffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to record command buffer!");
		}
	}
}

void Renderer::CreateSyncObjects()
{
	auto sempahore_info = VkSemaphoreCreateInfo{};
	sempahore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	auto fence_info = VkFenceCreateInfo{};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i){
		auto image_available_sempahore_result = vkCreateSemaphore(m_logical_device, &sempahore_info, nullptr, &m_image_available_semaphores[i]);
		auto render_finised_semaphore_result = vkCreateSemaphore(m_logical_device, &sempahore_info, nullptr, &m_render_finished_semaphores[i]);
		auto fence_result = vkCreateFence(m_logical_device, &fence_info, nullptr, &m_in_flight_fences[i]);

		if (image_available_sempahore_result != VK_SUCCESS) {
			CreateSemaphoreErrorHandling(image_available_sempahore_result);
		}
		if (render_finised_semaphore_result != VK_SUCCESS) {
			CreateSemaphoreErrorHandling(render_finised_semaphore_result);
		}
		if (fence_result != VK_SUCCESS) {
			CreateFenceErrorHandling(fence_result);
		}
	}
}

void Renderer::DrawFrame()
{
	auto image_index = uint32_t{};
	vkAcquireNextImageKHR(m_logical_device, m_swap_chain, UINT64_MAX, m_image_available_semaphores[m_current_frame], VK_NULL_HANDLE, &image_index	);

	auto submit_info = VkSubmitInfo{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	auto wait_semaphores = std::vector<VkSemaphore>{ m_image_available_semaphores[m_current_frame] };
	auto wait_stages = std::vector<VkPipelineStageFlags>{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	submit_info.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphores.size());
	submit_info.pWaitSemaphores = wait_semaphores.data();
	submit_info.pWaitDstStageMask = wait_stages.data();

	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &m_command_buffers[image_index];

	auto signal_semaphores = std::vector<VkSemaphore>{ m_render_finished_semaphores[m_current_frame] };
	submit_info.signalSemaphoreCount = static_cast<uint32_t>(signal_semaphores.size());
	submit_info.pSignalSemaphores = signal_semaphores.data();

	if (vkQueueSubmit(m_graphics_queue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit draw command buffer!");
	}

	auto present_info = VkPresentInfoKHR{};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	present_info.waitSemaphoreCount = static_cast<uint32_t>(signal_semaphores.size());
	present_info.pWaitSemaphores = signal_semaphores.data();

	auto swap_chains = std::vector<VkSwapchainKHR>{ m_swap_chain };
	present_info.swapchainCount = static_cast<uint32_t>(swap_chains.size());
	present_info.pSwapchains = swap_chains.data();
	present_info.pImageIndices = &image_index;
	present_info.pResults = nullptr;

	vkQueuePresentKHR(m_presentation_queue, &present_info);

	vkQueueWaitIdle(m_presentation_queue);

	m_current_frame = (m_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::CreateFrameBuffer(VkImageView const & t_image_view)
{
	auto attachments = std::vector<VkImageView>{ t_image_view };

	auto framebuffer_info = VkFramebufferCreateInfo {};
	framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebuffer_info.renderPass = m_render_pass;
	framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
	framebuffer_info.pAttachments = attachments.data();
	framebuffer_info.width = m_swap_chain_extent.width;
	framebuffer_info.height = m_swap_chain_extent.height;
	framebuffer_info.layers = 1;

	m_swap_chain_framebuffers.emplace_back(VkFramebuffer{});

	auto result = vkCreateFramebuffer(m_logical_device, &framebuffer_info, nullptr, &m_swap_chain_framebuffers.back());

	if (result != VK_SUCCESS) {
		CreateFrameBufferErrorHandling(result);
	}
}

void Renderer::CreateInstanceErrorHandling(VkResult const & t_error)
{
	auto error_message = std::string{ "Vulkan - Failed to create instance - " };

	switch (t_error)
	{
	case VK_ERROR_OUT_OF_HOST_MEMORY: error_message += "Out of host memory"; break;
	case VK_ERROR_OUT_OF_DEVICE_MEMORY:error_message += "Out of device memory"; break;
	case VK_ERROR_INITIALIZATION_FAILED:error_message += "Initialization failed"; break;
	case VK_ERROR_LAYER_NOT_PRESENT:error_message += "Layer not present"; break;
	case VK_ERROR_EXTENSION_NOT_PRESENT:error_message += "Extension not present"; break;
	case VK_ERROR_INCOMPATIBLE_DRIVER:error_message += "Incompatible driver"; break;
	default: error_message += "Unidentified error"; break;
	}
	throw std::runtime_error(std::move(error_message));
}

void Renderer::CreateLogicalDeviceErrorHandling(VkResult const & t_error)
{
	auto error_message = std::string{ "Vulkan - Failed to create logical device - " };

	switch (t_error)
	{
	case VK_ERROR_OUT_OF_HOST_MEMORY: error_message += "Out of host memory"; break;
	case VK_ERROR_OUT_OF_DEVICE_MEMORY:error_message += "Out of device memory"; break;
	case VK_ERROR_INITIALIZATION_FAILED:error_message += "Initialization failed"; break;
	case VK_ERROR_EXTENSION_NOT_PRESENT:error_message += "Extension not present"; break;
	case VK_ERROR_FEATURE_NOT_PRESENT:error_message += "Feature not present"; break;
	case VK_ERROR_TOO_MANY_OBJECTS:error_message += "Too many objects"; break;
	case VK_ERROR_DEVICE_LOST:error_message += "Device lost"; break;
	default: error_message += "Unidentified error"; break;
	}
	throw std::runtime_error(std::move(error_message));
}

void Renderer::CreateSwapChainErrorHandling(VkResult const & t_error)
{
	auto error_message = std::string{ "Vulkan - Failed to create swap chain - " };

	switch (t_error)
	{
	case VK_ERROR_OUT_OF_HOST_MEMORY: error_message += "Out of host memory"; break;
	case VK_ERROR_OUT_OF_DEVICE_MEMORY:error_message += "Out of device memory"; break;
	case VK_ERROR_INITIALIZATION_FAILED:error_message += "Initialization failed"; break;
	case VK_ERROR_EXTENSION_NOT_PRESENT:error_message += "Extension not present"; break;
	case VK_ERROR_FEATURE_NOT_PRESENT:error_message += "Feature not present"; break;
	case VK_ERROR_TOO_MANY_OBJECTS:error_message += "Too many objects"; break;
	case VK_ERROR_DEVICE_LOST:error_message += "Device lost"; break;
	default: error_message += "Unidentified error"; break;
	}
	throw std::runtime_error(std::move(error_message));
}

void Renderer::CreateImageViewsErrorHandling(VkResult const & t_error)
{
	auto error_message = std::string{ "Vulkan - Failed to create image view - " };

	switch (t_error)
	{
	case VK_ERROR_OUT_OF_HOST_MEMORY: error_message += "Out of host memory"; break;
	case VK_ERROR_OUT_OF_DEVICE_MEMORY:error_message += "Out of device memory"; break;
	default: error_message += "Unidentified error"; break;
	}
	throw std::runtime_error(std::move(error_message));
}

void Renderer::CreateShaderModuleErrorHandling(VkResult const & t_error)
{
	auto error_message = std::string{ "Vulkan - Failed to create shades module - " };

	switch (t_error)
	{
	case VK_ERROR_OUT_OF_HOST_MEMORY: error_message += "Out of host memory"; break;
	case VK_ERROR_OUT_OF_DEVICE_MEMORY: error_message += "Out of device memory"; break;
	case VK_ERROR_INVALID_SHADER_NV: error_message += "Invalid shader"; break;
	default: error_message += "Unidentified error"; break;
	}
	throw std::runtime_error(std::move(error_message));
}

void Renderer::CreateRenderPassErrorHandling(VkResult const & t_error)
{
	auto error_message = std::string{ "Vulkan - Failed to create render pass - " };

	switch (t_error)
	{
	case VK_ERROR_OUT_OF_HOST_MEMORY: error_message += "Out of host memory"; break;
	case VK_ERROR_OUT_OF_DEVICE_MEMORY: error_message += "Out of device memory"; break;
	default: error_message += "Unidentified error"; break;
	}
	throw std::runtime_error(std::move(error_message));
}

void Renderer::CreatePipelineLayoutErrorHandling(VkResult const & t_error)
{
	auto error_message = std::string{ "Vulkan - Failed to create pipeline layout - " };

	switch (t_error)
	{
	case VK_ERROR_OUT_OF_HOST_MEMORY: error_message += "Out of host memory"; break;
	case VK_ERROR_OUT_OF_DEVICE_MEMORY: error_message += "Out of device memory"; break;
	default: error_message += "Unidentified error"; break;
	}
	throw std::runtime_error(std::move(error_message));
}

void Renderer::CreatePipelineErrorHandling(VkResult const & t_error)
{
	auto error_message = std::string{ "Vulkan - Failed to create graphics pipeline - " };

	switch (t_error)
	{
	case VK_ERROR_OUT_OF_HOST_MEMORY: error_message += "Out of host memory"; break;
	case VK_ERROR_OUT_OF_DEVICE_MEMORY: error_message += "Out of device memory"; break;
	case VK_ERROR_INVALID_SHADER_NV: error_message += "Invalid shader"; break;
	default: error_message += "Unidentified error"; break;
	}
	throw std::runtime_error(std::move(error_message));
}

void Renderer::CreateFrameBufferErrorHandling(VkResult const & t_error)
{
	auto error_message = std::string{ "Vulkan - Failed to create frame buffer - " };

	switch (t_error)
	{
	case VK_ERROR_OUT_OF_HOST_MEMORY: error_message += "Out of host memory"; break;
	case VK_ERROR_OUT_OF_DEVICE_MEMORY: error_message += "Out of device memory"; break;
	default: error_message += "Unidentified error"; break;
	}
	throw std::runtime_error(std::move(error_message));
}

void Renderer::CreateCommandPoolErrorHandling(VkResult const & t_error)
{
	auto error_message = std::string{ "Vulkan - Failed to create command pool - " };

	switch (t_error)
	{
	case VK_ERROR_OUT_OF_HOST_MEMORY: error_message += "Out of host memory"; break;
	case VK_ERROR_OUT_OF_DEVICE_MEMORY: error_message += "Out of device memory"; break;
	default: error_message += "Unidentified error"; break;
	}
	throw std::runtime_error(std::move(error_message));
}

void Renderer::CreateCommandBuffersErrorHandling(VkResult const & t_error)
{
	auto error_message = std::string{ "Vulkan - Failed to create command buffers - " };

	switch (t_error)
	{
	case VK_ERROR_OUT_OF_HOST_MEMORY: error_message += "Out of host memory"; break;
	case VK_ERROR_OUT_OF_DEVICE_MEMORY: error_message += "Out of device memory"; break;
	default: error_message += "Unidentified error"; break;
	}
	throw std::runtime_error(std::move(error_message));
}

void Renderer::CreateSemaphoreErrorHandling(VkResult const & t_error)
{
	auto error_message = std::string{ "Vulkan - Failed to create semaphore - " };

	switch (t_error)
	{
	case VK_ERROR_OUT_OF_HOST_MEMORY: error_message += "Out of host memory"; break;
	case VK_ERROR_OUT_OF_DEVICE_MEMORY: error_message += "Out of device memory"; break;
	default: error_message += "Unidentified error"; break;
	}
	throw std::runtime_error(std::move(error_message));
}

void Renderer::CreateFenceErrorHandling(VkResult const & t_error)
{
	auto error_message = std::string{ "Vulkan - Failed to create fence - " };

	switch (t_error)
	{
	case VK_ERROR_OUT_OF_HOST_MEMORY: error_message += "Out of host memory"; break;
	case VK_ERROR_OUT_OF_DEVICE_MEMORY: error_message += "Out of device memory"; break;
	default: error_message += "Unidentified error"; break;
	}
	throw std::runtime_error(std::move(error_message));
}

void Renderer::CheckValidationLayerSupport()
{
	uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

	std::vector<VkLayerProperties> available_layers(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

	std::vector<std::string> unsupported_layers;

	for (auto layer_name : validation_layers) {

		bool layer_supported = false;

		for (const auto& layer_properties : available_layers) {
			if (strcmp(layer_name, layer_properties.layerName) == 0) {
				layer_supported = true;
				break;
			}
		}

		if (!layer_supported) {
			unsupported_layers.emplace_back(layer_name);
		}
	}

	if (unsupported_layers.size() != 0) {
		std::string error_message("Vulkan - Validation layers requested not supported:");
		for (auto layer : unsupported_layers) {
			error_message += "\n\t" + layer;
		}
		throw std::runtime_error(error_message);
	}
}

VkApplicationInfo const Renderer::GetVulkanAppInfoConfig()
{
	auto app_info = VkApplicationInfo{};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "Hello Triangle";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "Supernova Engine";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;
	return app_info;
}

std::vector<char const*> const Renderer::GetRequiredInstanceExtensions()
{
	auto supported_extensions = std::vector<VkExtensionProperties>{};
	uint32_t extension_count = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
	supported_extensions.resize(extension_count);
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, supported_extensions.data());

	auto required_extension_count = uint32_t{ 0 };
	auto required_extensions_ptr = glfwGetRequiredInstanceExtensions(&required_extension_count);

	auto required_extencions = std::vector(required_extensions_ptr, required_extensions_ptr + required_extension_count);

	CheckRequiredInstanceExtensionsSupport(required_extencions, supported_extensions);

#ifdef _DEBUG
		required_extencions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

	return required_extencions;
}

void Renderer::CheckRequiredInstanceExtensionsSupport(
	std::vector<char const*> const & t_required_extensions,
	std::vector<VkExtensionProperties> const & t_supported_extensions)
{
	std::vector<std::string> missing_required_extensions{};

	for (auto required_extension : t_required_extensions)
	{
		if (t_supported_extensions.end() != std::find_if(t_supported_extensions.begin(), t_supported_extensions.end(),
			[&](auto const & supported_extension)
			{ return supported_extension.extensionName == required_extension; }))
		{
			missing_required_extensions.emplace_back(std::string(required_extension));
		}
	}

	if (!missing_required_extensions.empty())
	{
		std::string error_message{ "Vulkan - Failed to create instance - Missing required extensions:" };

		for (auto missing_extension : missing_required_extensions)
		{
			error_message += "\n\t" + missing_extension;
		}

		throw std::runtime_error(error_message);
	}
}

VkDebugUtilsMessengerCreateInfoEXT Renderer::GetDebugMessengerConfig()
{
	auto debug_info = VkDebugUtilsMessengerCreateInfoEXT{};
	debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debug_info.pfnUserCallback = debugCallback;
	debug_info.pUserData = nullptr;

	return debug_info;
}

int Renderer::RatePhysicalDevice(VkPhysicalDevice const & t_device) const
{
	auto score = 0;

	auto device_properties = VkPhysicalDeviceProperties{};
	auto device_features = VkPhysicalDeviceFeatures{};

	vkGetPhysicalDeviceProperties(t_device, &device_properties);
	vkGetPhysicalDeviceFeatures(t_device, &device_features);

	if (!CheckRequiredPhysicalDeviceExtensionSupport(t_device)){
		return 0;
	}

	auto supported_queue_families = FindQueueFamilies(t_device, m_surface);
	if (!supported_queue_families.SupportsAllRequiredFamilies()) {
		return 0;
	}

	auto swap_chain_support = QuerySwapChainSupport(t_device);
	if (swap_chain_support.formats.empty() || swap_chain_support.presentModes.empty()) {
		return 0;
	}
	else
	{
		// TODO should rate each color format and color space

		if (swap_chain_support.presentModes.end() !=
			std::find(swap_chain_support.presentModes.begin(),
				swap_chain_support.presentModes.end(),
				VK_PRESENT_MODE_MAILBOX_KHR)) {
			score += 1000;
		}

	}

	if (!device_features.geometryShader) {
		return 0;
	}

	if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 1000;
	}

	score += device_properties.limits.maxImageDimension2D;

	return score;
}

bool Renderer::CheckRequiredPhysicalDeviceExtensionSupport(VkPhysicalDevice const & t_device)
{
	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties(t_device, nullptr, &extension_count, nullptr);

	std::vector<VkExtensionProperties> available_extensions(extension_count);
	vkEnumerateDeviceExtensionProperties(t_device, nullptr, &extension_count, available_extensions.data());

	std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

	for (const auto& extension : available_extensions) {
		required_extensions.erase(extension.extensionName);
	}

	return required_extensions.empty();
}

Renderer::QueueFamilyIndices Renderer::FindQueueFamilies(VkPhysicalDevice const & t_physical_device, VkSurfaceKHR const & t_surface)
{
	auto indices = QueueFamilyIndices{};

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(t_physical_device, &queue_family_count, nullptr);

	auto queue_families = std::vector<VkQueueFamilyProperties> (queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(t_physical_device, &queue_family_count, queue_families.data());

	int i = 0;
	for (const auto& queue_family : queue_families) {
		if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphics_family = i;
		}

		VkBool32 presentation_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(t_physical_device, i, t_surface, &presentation_support);

		if (presentation_support) {
			indices.presentation_family = i;
		}

		if (indices.SupportsAllRequiredFamilies()) {
			break;
		}

		i++;
	}

	return indices;
}

VkDeviceQueueCreateInfo Renderer::GetDeviceQueueConfig(uint32_t t_queue_family)
{
	auto queue_create_info = VkDeviceQueueCreateInfo{};
	queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create_info.queueFamilyIndex = t_queue_family;
	queue_create_info.queueCount = 1;
	auto queue_priority = 1.0f;
	queue_create_info.pQueuePriorities = &queue_priority;
	return queue_create_info;
}

Renderer::SwapChainSupportDetails Renderer::QuerySwapChainSupport(VkPhysicalDevice const & t_device) const
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(t_device, m_surface, &details.capabilities);

	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(t_device, m_surface, &format_count, nullptr);

	if (format_count != 0) {
		details.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(t_device, m_surface, &format_count, details.formats.data());
	}

	uint32_t presentation_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(t_device, m_surface, &presentation_mode_count, nullptr);

	if (presentation_mode_count != 0) {
		details.presentModes.resize(presentation_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(t_device, m_surface, &presentation_mode_count, details.presentModes.data());
	}

	return details;
}

VkSurfaceFormatKHR Renderer::GetSwapSurfaceFormat(std::vector<VkSurfaceFormatKHR> const & t_available_formats)
{
	// TODO should change to pick next best format if VK_FORMAT_B8G8R8A8_SRGB or VK_COLOR_SPACE_SRGB_NONLINEAR_KHR are not available
	for (const auto& available_format : t_available_formats) {
		if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return available_format;
		}
	}

	return t_available_formats[0];
}

VkPresentModeKHR Renderer::GetSwapPresentationMode(const std::vector<VkPresentModeKHR>& t_available_present_modes)
{
	for (const auto& available_present_mode : t_available_present_modes) {
		if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return available_present_mode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Renderer::GetSwapExtent(VkSurfaceCapabilitiesKHR const & t_capabilities) const
{
	if (t_capabilities.currentExtent.width != UINT32_MAX) {
		return t_capabilities.currentExtent;
	}
	else {
		VkExtent2D actual_extent = { m_window_width, m_window_height };

		actual_extent.width = std::clamp(actual_extent.width, t_capabilities.minImageExtent.width, t_capabilities.maxImageExtent.width);
		actual_extent.height = std::clamp(actual_extent.height, t_capabilities.minImageExtent.height, t_capabilities.maxImageExtent.height);

		return actual_extent;
	}
}

VkAttachmentDescription Renderer::GetColorAttachmentConfig(VkFormat const & t_format)
{
	auto color_attachment = VkAttachmentDescription{};
	color_attachment.format = t_format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	return color_attachment;
}

VkAttachmentReference Renderer::GetColorAttachmentReferenceConfig()
{
	auto color_attachment_ref = VkAttachmentReference{};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	return color_attachment_ref;
}

VkSubpassDescription Renderer::GetSubpassConfig(std::vector<VkAttachmentReference> const & t_color_attachment_refs)
{
	auto subpass = VkSubpassDescription{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = static_cast<uint32_t>(t_color_attachment_refs.size());
	subpass.pColorAttachments = t_color_attachment_refs.data();
	return subpass;
}

VkSubpassDependency Renderer::GetSubpassDependencyConfig()
{
	auto dependency = VkSubpassDependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	return dependency;
}

VkShaderModule Renderer::CreateShaderModule(std::string const & file) const
{
	std::vector<char> code = ReadShaderFile(file);

	auto create_info = VkShaderModuleCreateInfo{};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = code.size();
	create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shader_module;
	auto result = vkCreateShaderModule(m_logical_device, &create_info, nullptr, &shader_module);

	if (result != VK_SUCCESS) {
		CreateShaderModuleErrorHandling(result);
	}

	return shader_module;
}

VkPipelineShaderStageCreateInfo Renderer::GetVertexShaderPipelineStageConfig(VkShaderModule const & t_vertex_shader_module)
{
	auto vertex_shader_stage_info = VkPipelineShaderStageCreateInfo{};
	vertex_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertex_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertex_shader_stage_info.module = t_vertex_shader_module;
	vertex_shader_stage_info.pName = "main";
	return vertex_shader_stage_info;
}

VkPipelineShaderStageCreateInfo Renderer::GetFragmentShaderPipelineStageConfig(VkShaderModule const & t_fragment_shader_module)
{
	auto fragment_shader_stage_info = VkPipelineShaderStageCreateInfo{};
	fragment_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragment_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragment_shader_stage_info.module = t_fragment_shader_module;
	fragment_shader_stage_info.pName = "main";
	return fragment_shader_stage_info;
}

VkPipelineVertexInputStateCreateInfo Renderer::GetPipelineVertexInputConfig()
{
	auto vertex_input_info = VkPipelineVertexInputStateCreateInfo{};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 0;
	vertex_input_info.pVertexBindingDescriptions = nullptr;
	vertex_input_info.vertexAttributeDescriptionCount = 0;
	vertex_input_info.pVertexAttributeDescriptions = nullptr;
	return vertex_input_info;
}

VkPipelineInputAssemblyStateCreateInfo Renderer::GetPipelineInputAssemblyConfig()
{
	auto input_assembly_info = VkPipelineInputAssemblyStateCreateInfo{};
	input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_info.primitiveRestartEnable = VK_FALSE;
	return input_assembly_info;
}

VkViewport Renderer::GetViewportConfig(float t_width, float t_height)
{
	auto viewport = VkViewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = t_width;
	viewport.height = t_height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	return viewport;
}

VkRect2D Renderer::GetScissorConfig(VkExtent2D const & t_extent)
{
	auto scissor = VkRect2D{};
	scissor.offset = { 0, 0 };
	scissor.extent = t_extent;
	return scissor;
}

VkPipelineViewportStateCreateInfo Renderer::GetViewportStateConfig(VkViewport const & t_viewport, VkRect2D const & t_scissor)
{
	auto viewport_state = VkPipelineViewportStateCreateInfo{};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = &t_viewport;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = &t_scissor;
	return viewport_state;
}

VkPipelineRasterizationStateCreateInfo Renderer::GetRasterizerConfig()
{
	auto rasterizer = VkPipelineRasterizationStateCreateInfo{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;
	return rasterizer;
}

VkPipelineMultisampleStateCreateInfo Renderer::GetMultisamplingConfig()
{
	auto multisampling = VkPipelineMultisampleStateCreateInfo{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;
	return multisampling;
}

VkPipelineColorBlendAttachmentState Renderer::GetColorBlendAttachmentConfig()
{
	auto color_blend_attachment = VkPipelineColorBlendAttachmentState{};
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable = VK_TRUE;
	color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
	return color_blend_attachment;
}

VkPipelineColorBlendStateCreateInfo Renderer::GetColorBlendConfig(VkPipelineColorBlendAttachmentState const & t_color_blend_attachment)
{
	auto color_blending = VkPipelineColorBlendStateCreateInfo{};
	color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.logicOpEnable = VK_FALSE;
	color_blending.logicOp = VK_LOGIC_OP_COPY;
	color_blending.attachmentCount = 1;
	color_blending.pAttachments = &t_color_blend_attachment;
	color_blending.blendConstants[0] = 0.0f;
	color_blending.blendConstants[1] = 0.0f;
	color_blending.blendConstants[2] = 0.0f;
	color_blending.blendConstants[3] = 0.0f;
	return color_blending;
}

VkPipelineDynamicStateCreateInfo Renderer::GetDynamicStateCongif(std::vector<VkDynamicState> const & t_dynamic_states)
{
	auto dynamic_state = VkPipelineDynamicStateCreateInfo{};
	dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.dynamicStateCount = static_cast<uint32_t>(t_dynamic_states.size());
	dynamic_state.pDynamicStates = t_dynamic_states.data();
	return dynamic_state;
}

VkPipelineLayoutCreateInfo Renderer::GetPipelineLayoutConfig()
{
	auto pipeline_layout_info = VkPipelineLayoutCreateInfo{};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 0;
	pipeline_layout_info.pSetLayouts = nullptr;
	pipeline_layout_info.pushConstantRangeCount = 0;
	pipeline_layout_info.pPushConstantRanges = nullptr;
	return pipeline_layout_info;
}

VKAPI_ATTR VkBool32 VKAPI_CALL Renderer::debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT t_message_severity,
	VkDebugUtilsMessageTypeFlagsEXT t_message_type,
	const VkDebugUtilsMessengerCallbackDataEXT* t_callback_data,
	void* ) {

	auto error_message = std::string{ "Vulkan Debuger:\n\tSeverity: " };
	switch (t_message_severity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: error_message += "Verbose"; break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: error_message += "Info"; break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: error_message += "Warning"; break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: error_message += "Error"; break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT: error_message += "Max"; break;
	default: error_message += "Unknown"; break;
	}

	error_message += "\n\tType: ";
	switch (t_message_type)
	{
	case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: error_message += "General - Unrelated to specification or performance"; break;
	case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: error_message += "Validation - Specification violation or possible mistake"; break;
	case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: error_message += "Performance - Use of Vulkan in non optimal way"; break;
	default: break;
	}

	error_message += "\n\tMessage:\n";
	error_message += t_callback_data->pMessage;
	error_message += "\n";
	Application::LogError(error_message);

	return VK_FALSE;
}

VkResult Renderer::CreateDebugUtilsMessengerEXT(
	VkInstance t_instance,
	const VkDebugUtilsMessengerCreateInfoEXT* t_debug_info,
	const VkAllocationCallbacks* t_alocator,
	VkDebugUtilsMessengerEXT* t_debug_messenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(t_instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(t_instance, t_debug_info, t_alocator, t_debug_messenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void Renderer::DestroyDebugUtilsMessengerEXT(
	VkInstance t_instance,
	VkDebugUtilsMessengerEXT t_debug_messenger,
	const VkAllocationCallbacks* allocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(t_instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(t_instance, t_debug_messenger, allocator);
	}
}

std::vector<char> Renderer::ReadShaderFile(std::string const & filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file!");
	}

	size_t file_size = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(file_size);

	file.seekg(0);
	file.read(buffer.data(), file_size);

	file.close();

	return buffer;
}
