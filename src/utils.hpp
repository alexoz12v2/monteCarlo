#ifndef MXC_UTILS_HPP
#define MXC_UTILS_HPP

#ifndef NDEBUG
#define MXC_IF_DEBUG_IMPL(...) __VA_ARGS__()
#define MXC_IF_DEBUG(...) MXC_IF_DEBUG_IMPL([&]()->void{__VA_ARGS__})
#else
#define MXC_IF_DEBUG(...) 
#endif

#ifdef _MSVC_LANG
#define MXC_restrict __restrict
#elif defined(__GNUC__)
#define MXC_restrict __restrict
#elif defined(__clang__)
#define MXC_restrict __restrict
#else
#define MXC_restrict __restrict
#endif

#include "initializers.hpp"
#include "StaticVector.hpp"

#include <vulkan/vulkan.h>

#include <concepts>
#include <iterator>
#include <array>
#include <optional>
#include <variant>
#include <utility>
#include <cassert>

namespace mxc
{



enum class Status
{
    SUCCESS, VULKAN_ERROR, MEMORY_ERROR, INPUT_ERROR
};


namespace vkdefs 
{
    inline VkPhysicalDeviceFeatures2 constexpr noFeatures {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = nullptr,
        .features = {
    		.robustBufferAccess = VK_FALSE,
    		.fullDrawIndexUint32 = VK_FALSE,
    		.imageCubeArray = VK_FALSE,
    		.independentBlend = VK_FALSE,
    		.geometryShader = VK_FALSE,
    		.tessellationShader = VK_FALSE,
    		.sampleRateShading = VK_FALSE,
    		.dualSrcBlend = VK_FALSE,
    		.logicOp = VK_FALSE,
    		.multiDrawIndirect = VK_FALSE,
    		.drawIndirectFirstInstance = VK_FALSE,
    		.depthClamp = VK_FALSE,
    		.depthBiasClamp = VK_FALSE,
    		.fillModeNonSolid = VK_FALSE,
    		.depthBounds = VK_FALSE,
    		.wideLines = VK_FALSE,
    		.largePoints = VK_FALSE,
    		.alphaToOne = VK_FALSE,
    		.multiViewport = VK_FALSE,
    		.samplerAnisotropy = VK_FALSE,
    		.textureCompressionETC2 = VK_FALSE,
    		.textureCompressionASTC_LDR = VK_FALSE,
    		.textureCompressionBC = VK_FALSE,
    		.occlusionQueryPrecise = VK_FALSE,
    		.pipelineStatisticsQuery = VK_FALSE,
    		.vertexPipelineStoresAndAtomics = VK_FALSE,
    		.fragmentStoresAndAtomics = VK_FALSE,
    		.shaderTessellationAndGeometryPointSize = VK_FALSE,
    		.shaderImageGatherExtended = VK_FALSE,
    		.shaderStorageImageExtendedFormats = VK_FALSE,
    		.shaderStorageImageMultisample = VK_FALSE,
    		.shaderStorageImageReadWithoutFormat = VK_FALSE,
    		.shaderStorageImageWriteWithoutFormat = VK_FALSE,
    		.shaderUniformBufferArrayDynamicIndexing = VK_FALSE,
    		.shaderSampledImageArrayDynamicIndexing = VK_FALSE,
    		.shaderStorageBufferArrayDynamicIndexing = VK_FALSE,
    		.shaderStorageImageArrayDynamicIndexing = VK_FALSE,
    		.shaderClipDistance = VK_FALSE,
    		.shaderCullDistance = VK_FALSE,
    		.shaderFloat64 = VK_FALSE,
    		.shaderInt64 = VK_FALSE,
    		.shaderInt16 = VK_FALSE,
    		.shaderResourceResidency = VK_FALSE,
    		.shaderResourceMinLod = VK_FALSE,
    		.sparseBinding = VK_FALSE,
    		.sparseResidencyBuffer = VK_FALSE,
    		.sparseResidencyImage2D = VK_FALSE,
    		.sparseResidencyImage3D = VK_FALSE,
    		.sparseResidency2Samples = VK_FALSE,
    		.sparseResidency4Samples = VK_FALSE,
    		.sparseResidency8Samples = VK_FALSE,
    		.sparseResidency16Samples = VK_FALSE,
    		.sparseResidencyAliased = VK_FALSE,
    		.variableMultisampleRate = VK_FALSE,
    		.inheritedQueries = VK_FALSE
        }
    };

    inline struct {VkQueueFamilyProperties2 properties; bool presentationSupport;} constexpr graphicsTransfer {
        .properties = {
            .sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2,
            .pNext = nullptr,
            .queueFamilyProperties = {
    			.queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT,
    			.queueCount = 1,
    			.timestampValidBits = 0, // means not used. TODO = add support for queries
                // image transfer granularity refers to the minimum size in texels that can be copied or accessed in a single image transfer 
                // or access operation. It is specified by the VkPhysicalDeviceLimits structure, which is queried from the physical 
                // device during device creation. The image transfer granularity is dependent on the hardware and can vary between implementations. 
                // It is important to be aware of the image transfer granularity when performing image copies or accessing image data 
                // to avoid performance penalties due to unnecessary data transfers. For example, if the transfer granularity is 
                // 64x64 texels and an application tries to copy only a single 32x32 texel subregion, 
                // the driver may have to transfer an entire 64x64 texel block, causing unnecessary overhead.
                // THIS MEMBER IS NOW DEPRECATED AND SHOULD BE IGNORED. we use tiling and usage in image create info, and the driver implementation
                // will derive the granularity from there
    			.minImageTransferGranularity = {
                    .width=0, .height=0, .depth=0
                }
            }
        },
        .presentationSupport = false
    };

    // Assuming Vulkan 1.3
    enum class InstanceExtension
    {
        KHR_android_surface,              //DEP: VK_KHR_surface
        KHR_display,                      //DEP: VK_KHR_surface
        KHR_get_display_properties2,      //DEP: VK_KHR_display
        KHR_get_surface_capabilities2,    //DEP: VK_KHR_surface
        KHR_portability_enumeration,
        KHR_surface,                     
        KHR_surface_protected_capabilities,  //DEP: Version 1.1 && VK_KHR_get_surface_capabilities2
        KHR_wayland_surface,              //DEP: VK_KHR_surface
        KHR_win32_surface,                //DEP: VK_KHR_surface
        KHR_xcb_surface,                  //DEP: VK_KHR_surface
        KHR_xlib_surface,                 //DEP: VK_KHR_surface
        EXT_acquire_drm_display,          //DEP: VK_EXT_direct_mode_display
        EXT_acquire_xlib_display,         //DEP: VK_EXT_direct_mode_display
        EXT_debug_utils,
        EXT_direct_mode_display,          //DEP: VK_KHR_display
        EXT_directfb_surface,             //DEP: VK_KHR_surface
        EXT_display_surface_counter,      //DEP: VK_KHR_display
        EXT_headless_surface,             //DEP: VK_KHR_surface
        EXT_metal_surface,                //DEP: VK_KHR_surface
        EXT_surface_maintenance1,         //DEP: VK_KHR_surface && KH_KHR_get_surface_capabilities2
        EXT_swapchain_colorspace,         //DEP: VK_KHR_surface
        EXT_validation_features,
        FUCHSIA_imagepipe_surface,        //DEP: VK_KHR_surface
        GGP_stream_descriptor_surface,    //DEP: VK_KHR_surface
        GOOGLE_surfaceless_query,         //DEP: VK_KHR_surface
        LUNARG_direct_driver_loading,
        NN_vi_surface,                    //DEP: VK_KHR_surface
        QNX_screen_surface                //DEP: VK_KHR_surface
    };

    // Assuming Vulkan 1.3
    enum class DeviceExtension
    {
        KHR_16bit_storage,               //DEP: (VK_KHR_get_physical_device_properties2 && VK_KHR_storage_buffer_storage_class) || Version 1.1
        KHR_8bit_storage,                //DEP: (VK_KHR_get_physical_device_properties2 && VK_KHR_storage_buffer_storage_class) || Version 1.1
        KHR_acceleration_structure,      //DEP: Version 1.1 && VK_EXT_descriptor_indexing && VK_KHR_buffer_device_address && VK_KHR_deferred_host_operations
        KHR_deferred_host_operations,
        KHR_display_swapchain,           //DEP: VK_KHR_swapchain && VK_KHR_display
        KHR_external_fence_fd,           //DEP: VK_KHR_external_fence && Version 1.1
        KHR_external_fence_win32,        //DEP: VK_KHR_external_fence
        KHR_external_memory_fd,          //DEP: VK_KHR_external_memory || Version 1.1
        KHR_external_memory_win32,       //DEP: VK_KHR_external_memory
        KHR_external_semaphore_fd,       //DEP: VK_KHR_external_semaphore || Version 1.1
        KHR_external_semaphore_win32,    //DEP: VK_KHR_external_semaphore
        KHR_fragment_shader_barycentric, //DEP: VK_KHR_get_physical_device_properties2
        KHR_fragment_shading_rate,       //DEP: (VK_KHR_create_renderpass2 || Version 1.2) && (VK_KHR_get_physical_device_properties2 || Version 1.1)
        KHR_global_priority,             //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        KHR_incremental_present,         //DEP: VK_KHR_swapchain
        KHR_map_memory2,
        KHR_performance_query,           //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        KHR_pipeline_executable_properties,  //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        KHR_pipeline_library,
        KHR_portability_subset,          //DEP: VK_KHR_get_physical_device_properties2
        KHR_present_id,                  //DEP: VK_KHR_swapchain && VK_KHR_get_physical_device_properties2
        KHR_present_wait,                //DEP: VK_KHR_swapchain && VK_KHR_present_id
        KHR_push_descriptor,             //DEP: VK_KHR_get_physical_device_properties2
        KHR_ray_query,                   //DEP: VK_KHR_spirv_1_4 && VK_KHR_acceleration_structure
        KHR_ray_tracing_maintenance1,    //DEP: VK_KHR_acceleration_structure
        KHR_ray_tracing_pipeline,        //DEP: VK_KHR_spirv_1_4 && VK_KHR_acceleration_structure
        KHR_shader_clock,                //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        KHR_shader_subgroup_uniform_control_flow, //DEP: Version 1.1
        KHR_shared_presentable_image,    //DEP: VK_KHR_swapchain && VK_KHR_get_surface_capabilities && (VK_KHR_get_physical_device_properties2 || Version 1.1)
        KHR_swapchain,                   //DEP: VK_KHR_surface
        KHR_swapchain_mutable_format,    //DEP: VK_KHR_swapchain && (VK_KHR_maintenance2 || Version 1.1) && (VK_KHR_image_format_list || Version 1.2)
        KHR_video_decode_h264,           //DEP: VK_KHR_video_decode_queue
        KHR_video_decode_h265,           //DEP: VK_KHR_video_decode_queue
        KHR_video_decode_queue,          //DEP: VK_KHR_video_queue && VK_KHR_synchronization2
        KHR_video_encode_queue,          //DEP: VK_KHR_video_queue && VK_KHR_synchronization2
        KHR_video_queue,                 //DEP: Version 1.1 && VK_KHR_synchronization2
        KHR_win32_keyed_mutex,           //DEP: VK_KHR_external_memory_win32
        KHR_workgroup_memory_explicit_layout,    //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_astc_decode_mode,            //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_attachment_feedback_loop_layout, //DEP: VK_KHR_get_physical_device_properties || Version 1.1
        EXT_blend_operation_advanced,    //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_border_color_swizzle,        //DEP: VK_EXT_custom_border_color
        EXT_calibrated_timestamps,       //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_color_write_enable,          //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_conditional_rendering,       //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_conservative_rasterization,  //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_custom_border_color,         //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_depth_clamp_zero_one,        //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_depth_clip_control,          //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_depth_clip_enable,           //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_depth_range_unrestricted,
        EXT_descriptor_buffer,           //DEP: (VK_KHR_get_physical_device_properties || Version 1.1) && (VK_KHR_buffer_device_address || Version 1.2) && (VK_KHR_synchronization2 || Version 1.3) && (VK_EXT_descriptor_indexing || Version 1.2)
        EXT_device_address_binding_report,   //DEP: (VK_KHR_get_physical_device_properties || Version 1.1) && VK_EXT_debug_utils
        EXT_device_fault,                //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_device_memory_report,        //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_discard_rectangles,          //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_display_control,             //DEP: VK_EXT_display_surface_counter && VK_KHR_swapchain
        EXT_extended_dynamic_state3,     //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_external_memory_dma_buf,     //DEP: VK_KHR_external_memory_fd
        EXT_external_memory_host,        //DEP: VK_KHR_external_memory || Version 1.1
        EXT_filter_cubic,
        EXT_fragment_density_map,        //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_fragment_density_map2,       //DEP: VK_EXT_fragment_density_map
        EXT_fragment_shader_interlock,   //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_full_screen_exclusive,       //DEP: (VK_KHR_get_physical_device_properties2 || Version 1.1) && VK_KHR_get_surface_capabilities2 && VK_KHR_surface && VK_KHR_swapchain
        EXT_graphics_pipeline_library,   //DEP: (VK_KHR_get_physical_device_properties2 || Version 1.1) && VK_KHR_pipeline_library
        EXT_hdr_metadata,                //DEP: VK_KHR_swapchain
        EXT_image_2d_view_of_3d,         //DEP: (VK_KHR_maintenance1 && VK_KHR_get_physical_device_properties2) || Version 1.1
        EXT_image_compression_control,   //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_image_compression_control_swapchain, //DEP: VK_EXT_image_compression_control
        EXT_image_drm_format_modifier,   //DEP: ((VK_KHR_bind_memory2 && VK_KHR_get_physical_device_properties2 && VK_KHR_sampler_ycbcr_conversion) || Version 1.1) && ((VK_KHR_image_format_list) || Version 1.2)
        EXT_image_sliced_view_of_3d,     //DEP: (VK_KHR_maintenance1 && VK_KHR_get_physical_device_properties2) || Version 1.1
        EXT_image_view_min_lod,          //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_index_type_uint8,            //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_legacy_dithering,            //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_line_rasterization,          //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_load_store_op_none,          
        EXT_memory_budget,               //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_memory_priority,             //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_mesh_shader,                 //DEP: VK_KHR_spirv_1_4
        EXT_metal_objects,
        EXT_multi_draw,                  //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_multisampled_render_to_single_sampled,       //DEP: (VK_KHR_create_renderpass2 && VK_KHR_depth_stencil_resolve) || Version 1.2
        EXT_mutable_descriptor_type,     //DEP: VK_KHR_maintenance3 || Version 1.1
        EXT_non_seamless_cube_map,       //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_opacity_micromap,            //DEP: VK_KHR_acceleration_structure || (VK_KHR_synchronization2 || Version 1.3)
        EXT_pageable_device_local_memory,//DEP: VK_EXT_memory_priority
        EXT_pci_bus_info,                //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_physical_device_drm,         //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_pipeline_library_group_handles,  //DEP: VK_KHR_ray_tracing_pipeline && VK_KHR_pipeline_library
        EXT_pipeline_properties,         //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_pipeline_protected_access,   //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_pipeline_robustness,         //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_post_depth_coverage,
        EXT_primitive_topology_list_restart, //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_primitives_generated_query,  //DEP: VK_EXT_transform_feedback
        EXT_provoking_vertex,            //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_queue_family_foreign,        //DEP: VK_KHR_KHR_external_memory || Version 1.1
        EXT_rasterization_order_attachment_access,   //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_rgba10x6_formats,            //DEP: VK_KHR_sampler_ycbcr_conversion || Version 1.1
        EXT_robustness2,                 //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_sample_locations,            //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_shader_atomic_float,         //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_shader_atomic_float2,        //DEP: VK_EXT_shader_atomic_float
        EXT_shader_image_atomic_int64,   //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_shader_module_identifier,    //DEP: (VK_KHR_get_physical_device_properties2 || Version 1.1) && ( VK_EXT_pipeline_creation_cache_control || Version 1.3)
        EXT_shader_object,               //DEP: (VK_KHR_get_physical_device_properties2 || Version 1.1) && (VK_KHR_dynamic_rendering || Version 1.3)
        EXT_shader_stencil_export,
        EXT_shader_tile_image,           //DEP: Version 1.3
        EXT_subpass_merge_feedback,      //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_swapchain_maintenance1,      //DEP: VK_KHR_swapchain && VK_EXT_surface_maintenance1 && (VK_KHR_get_physical_device_properties2 || Version 1.1)
        EXT_transform_feedback,          //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_validation_cache,            
        EXT_vertex_attribute_divisor,    //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_vertex_input_dynamic_state,  //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        EXT_video_encode_h264,           //DEP: VK_KHR_video_encode_queue
        EXT_video_encode_h265,           //DEP: VK_KHR_video_encode_queue
        EXT_ycbcr_image_arrays,          //DEP: VK_KHR_sampler_ycbcr_conversion || Version 1.1
        AMD_buffer_marker,
        AMD_device_coherent_memory,      //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        AMD_display_native_hdr,          //DEP: (VK_KHR_get_physical_device_properties2 || Version 1.1) && VK_KHR_get_surface_capabilities2 && VK_KHR_swapchain
        AMD_gcn_shader,
        AMD_memory_overallocation_behavior,   
        AMD_mixed_attachment_samples,
        AMD_pipeline_compiler_control,
        AMD_rasterization_order,
        AMD_shader_ballot,
        AMD_shader_core_properties,      //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        AMD_shader_core_properties2,     //DEP: VK_AMD_shader_core_properties
        AMD_shader_early_and_late_fragment_tests,    //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        AMD_shader_explicit_vertex_parameter,
        AMD_shader_fragment_mask,
        AMD_shader_image_load_store_lod,
        AMD_shader_info,
        AMD_shader_trinary_minmax,
        AMD_texture_gather_bias_lod,     //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        ANDROID_external_memory_android_hardware_buffer, //DEP: (Version 1.1 || (VK_KHR_sampler_ycbcr_conversion && VK_KHR_external_memory && VK_KHR_dedicated_allocation)) && VK_EXT_queue_family_foreign 
        ARM_shader_core_builtins,        //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        ARM_shader_core_properties,      //DEP: Version 1.1
        FUCHSIA_buffer_collection,       //DEP: VK_FUCHSIA_external_memory && (VK_KHR_sampler_ycbcr_conversion || Version 1.1)
        FUCHSIA_external_memory,         //DEP: (VK_KHR_external_memory_capabilities && VK_KHR_external_memory) || Version 1.1
        FUCHSIA_external_semaphore,      //DEP: (VK_KHR_external_semaphore_capabilities && VK_KHR_external_semaphore) || Version 1.1
        GGP_frame_token,                 //DEP: VK_KHR_swapchain && VK_GGP_stream_descriptor_surface
        GOOGLE_decorate_string,
        GOOGLE_display_timing,           //DEP: VK_KHR_swapchain
        GOOGLE_hlsl_functionality1,
        GOOGLE_user_type,
        HUAWEI_cluster_culling_shader,   //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        HUAWEI_invocation_mask,          //DEP: VK_KHR_ray_tracing_pipeline && (VK_KHR_synchronization2 || Version 1.3)
        HUAWEI_subpass_shading,          //DEP: (VK_KHR_create_renderpass2 || Version 1.1) && (VK_KHR_synchronization2 || Version 1.3)
        IMG_filter_cubic,
        INTEL_performance_query,
        INTEL_shader_integer_functions2, //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        NV_acquire_winrt_display,        //DEP: VK_EXT_direct_mode_display
        NV_clip_space_w_scaling,
        NV_compute_shader_derivatives,   //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        NV_cooperative_matrix,           //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        NV_copy_memory_indirect,         //DEP: (VK_KHR_get_physical_device_properties2 || Version 1.1) && (VK_KHR_buffer_device_address || Version 1.2)
        NV_corner_sampled_image,         //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        NV_coverage_reduction_mode,      //DEP: VK_NV_framebuffer_mixed_samples && (VK_KHR_get_physical_device_properties2 || Version 1.1)
        NV_dedicated_allocation_image_aliasing,  //DEP: (VK_KHR_get_physical_device_properties2 && VK_KHR_dedicated_allocation) || Version 1.1
        NV_device_diagnostic_checkpoints,//DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        NV_device_diagnostics_config,    //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        NV_device_generated_commands,    //DEP: Version 1.1 && (VK_KHR_buffer_device_address || Version 1.2)
        NV_displacement_micromap,        //DEP: VK_EXT_opacity_micropmap
        NV_external_memory_rdma,         //DEP: VK_KHR_external_memory || Version 1.1
        NV_fill_rectangle,
        NV_fragment_coverage_to_color,
        NV_fragment_shading_rate_enums,  //DEP: VK_KHR_fragment_shading_rate
        NV_framebuffer_mixed_samples,
        NV_geometry_shader_passthrough,
        NV_inherited_viewport_scissor,   //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        NV_linear_color_attachment,      //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        NV_low_latency,
        NV_memory_decompression,         //DEP: (VK_KHR_get_physical_device_properties2 || Version 1.1) && (VK_KHR_buffer_device_address || Version 1.2)
        NV_mesh_shader,                  //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        NV_optical_flow,                 //DEP: (VK_KHR_get_physical_device_properties2 || Version 1.1) && ((VK_KHR_format_feature_flags2 && VK_KHR_synchronization2) || Version 1.3)
        NV_present_barrier,              //DEP: (VK_KHR_get_physical_device_properties2 || Version 1.1) && VK_KHR_get_surface_capabilities2 && VK_KHR_surface && VK_KHR_swapchain
        NV_ray_tracing,                  //DEP: (VK_KHR_get_physical_device_properties2 && VK_KHR_get_memory_requirements2) || Version 1.1
        NV_ray_tracing_invocation_reorder,   //DEP: VK_KHR_ray_tracing_pipeline
        NV_ray_tracing_motion_blur,      //DEP: VK_KHR_ray_tracing_pipeline
        NV_representative_fragment_test, //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        NV_sample_mask_override_coverage,
        NV_scissor_exclusive,            //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        NV_shader_image_footprint,       //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        NV_shader_sm_builtins,           //DEP: Version 1.1
        NV_shader_subgroup_partitioned,  //DEP: Version 1.1
        NV_shading_rate_image,           //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        NV_viewport_array2,
        NV_viewport_swizzle,
        NVX_binary_import,
        NVX_image_view_handle,
        NVX_multiview_per_view_attributes,   //DEP: VK_KHR_multiview || Version 1.1
        QCOM_fragment_density_map_offset,//DEP: (VK_KHR_get_physical_device_properties2 || Version 1.1) && VK_EXT_fragment_density_map
        QCOM_image_processing,           //DEP: VK_KHR_format_feature_flags2 || Version 1.3
        QCOM_multiview_per_view_render_areas,
        QCOM_multiview_per_view_viewports,   //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        QCOM_render_pass_shader_resolve,
        QCOM_render_pass_store_ops,
        QCOM_render_pass_transform,      //DEP: VK_KHR_swapchain && VK_KHR_surface
        QCOM_rotated_copy_commands,      //DEP: VK_KHR_swapchain && (VK_KHR_copy_commands2 || Version 1.3)
        QCOM_tile_properties,            //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        SEC_amigo_profiling,             //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
        VALVE_descriptor_set_host_mapping,   //DEP: VK_KHR_get_physical_device_properties2 || Version 1.1
    };

    consteval auto instanceExtensionToString(InstanceExtension const ext) -> std::string_view
    {
        std::string_view str;
        switch (ext)
        {
            case InstanceExtension::KHR_android_surface: str = "VK_KHR_android_surface"; break;
            case InstanceExtension::KHR_display: str = "VK_KHR_display"; break;
            case InstanceExtension::KHR_get_display_properties2: str = "VK_KHR_get_display_properties2"; break;
            case InstanceExtension::KHR_get_surface_capabilities2: str = "VK_KHR_get_surface_capabilities2"; break;
            case InstanceExtension::KHR_portability_enumeration: str = "VK_KHR_portability_enumeration"; break;
            case InstanceExtension::KHR_surface: str = "VK_KHR_surface"; break;
            case InstanceExtension::KHR_surface_protected_capabilities: str = "VK_KHR_surface_protected_capabilities"; break;
            case InstanceExtension::KHR_wayland_surface: str = "VK_KHR_wayland_surface"; break;
            case InstanceExtension::KHR_win32_surface: str = "VK_KHR_win32_surface"; break;
            case InstanceExtension::KHR_xcb_surface: str = "VK_KHR_xcb_surface"; break;
            case InstanceExtension::KHR_xlib_surface: str = "VK_KHR_xlib_surface"; break;
            case InstanceExtension::EXT_acquire_drm_display: str = "VK_EXT_acquire_drm_display"; break;
            case InstanceExtension::EXT_acquire_xlib_display: str = "VK_EXT_acquire_xlib_display"; break;
            case InstanceExtension::EXT_debug_utils: str = "VK_EXT_debug_utils"; break;
            case InstanceExtension::EXT_direct_mode_display: str = "VK_EXT_direct_mode_display"; break;
            case InstanceExtension::EXT_directfb_surface: str = "VK_EXT_directfb_surface"; break;
            case InstanceExtension::EXT_display_surface_counter: str = "VK_EXT_display_surface_counter"; break;
            case InstanceExtension::EXT_headless_surface: str = "VK_EXT_headless_surface"; break;
            case InstanceExtension::EXT_metal_surface: str = "VK_EXT_metal_surface"; break;
            case InstanceExtension::EXT_surface_maintenance1: str = "VK_EXT_surface_maintenance1"; break;
            case InstanceExtension::EXT_swapchain_colorspace: str = "VK_EXT_swapchain_colorspace"; break;
            case InstanceExtension::EXT_validation_features: str = "VK_EXT_validation_features"; break;
            case InstanceExtension::FUCHSIA_imagepipe_surface: str = "VK_FUCHSIA_imagepipe_surface"; break;
            case InstanceExtension::GGP_stream_descriptor_surface: str = "VK_GGP_stream_descriptor_surface"; break;
            case InstanceExtension::GOOGLE_surfaceless_query: str = "VK_GOOGLE_surfaceless_query"; break;
            case InstanceExtension::LUNARG_direct_driver_loading: str = "VK_LUNARG_direct_driver_loading"; break;
            case InstanceExtension::NN_vi_surface: str = "VK_NN_vi_surface"; break;
            case InstanceExtension::QNX_screen_surface: str = "VK_QNX_screen_surface"; break;
        } 
        return str;
    }

    consteval auto deviceExtensionToString(DeviceExtension const ext) -> std::string_view
    {
        std::string_view str;
        switch (ext)
        {
            case DeviceExtension::KHR_16bit_storage: str = "VK_KHR_16bit_storage"; break;
            case DeviceExtension::KHR_8bit_storage: str = "VK_KHR_8bit_storage"; break;
            case DeviceExtension::KHR_acceleration_structure: str = "VK_KHR_acceleration_structure"; break;
            case DeviceExtension::KHR_deferred_host_operations: str = "VK_KHR_deferred_host_operations"; break;
            case DeviceExtension::KHR_display_swapchain: str = "VK_KHR_display_swapchain"; break;
            case DeviceExtension::KHR_external_fence_fd: str = "VK_KHR_external_fence_fd"; break;
            case DeviceExtension::KHR_external_fence_win32: str = "VK_KHR_external_fence_win32"; break;
            case DeviceExtension::KHR_external_memory_fd: str = "VK_KHR_external_memory_fd"; break;
            case DeviceExtension::KHR_external_memory_win32: str = "VK_KHR_external_memory_win32"; break;
            case DeviceExtension::KHR_external_semaphore_fd: str = "VK_KHR_external_semaphore_fd"; break;
            case DeviceExtension::KHR_external_semaphore_win32: str = "VK_KHR_external_semaphore_win32"; break;
            case DeviceExtension::KHR_fragment_shader_barycentric: str = "VK_KHR_fragment_shader_barycentric"; break;
            case DeviceExtension::KHR_fragment_shading_rate: str = "VK_KHR_fragment_shading_rate"; break;
            case DeviceExtension::KHR_global_priority: str = "VK_KHR_global_priority"; break;
            case DeviceExtension::KHR_incremental_present: str = "VK_KHR_incremental_present"; break;
            case DeviceExtension::KHR_map_memory2: str = "VK_KHR_map_memory2"; break;
            case DeviceExtension::KHR_performance_query: str = "VK_KHR_performance_query"; break;
            case DeviceExtension::KHR_pipeline_executable_properties: str = "VK_KHR_pipeline_executable_properties"; break;
            case DeviceExtension::KHR_pipeline_library: str = "VK_KHR_pipeline_library"; break;
            case DeviceExtension::KHR_portability_subset: str = "VK_KHR_portability_subset"; break;
            case DeviceExtension::KHR_present_id: str = "VK_KHR_present_id"; break;
            case DeviceExtension::KHR_present_wait: str = "VK_KHR_present_wait"; break;
            case DeviceExtension::KHR_push_descriptor: str = "VK_KHR_push_descriptor"; break;
            case DeviceExtension::KHR_ray_query: str = "VK_KHR_ray_query"; break;
            case DeviceExtension::KHR_ray_tracing_maintenance1: str = "VK_KHR_ray_tracing_maintenance1"; break;
            case DeviceExtension::KHR_ray_tracing_pipeline: str = "VK_KHR_ray_tracing_pipeline"; break;
            case DeviceExtension::KHR_shader_clock: str = "VK_KHR_shader_clock"; break;
            case DeviceExtension::KHR_shader_subgroup_uniform_control_flow: str = "VK_KHR_shader_subgroup_uniform_control_flow"; break;
            case DeviceExtension::KHR_shared_presentable_image: str = "VK_KHR_shared_presentable_image"; break;
            case DeviceExtension::KHR_swapchain: str = "VK_KHR_swapchain"; break;
            case DeviceExtension::KHR_swapchain_mutable_format: str = "VK_KHR_swapchain_mutable_format"; break;
            case DeviceExtension::KHR_video_decode_h264: str = "VK_KHR_video_decode_h264"; break;
            case DeviceExtension::KHR_video_decode_h265: str = "VK_KHR_video_decode_h265"; break;
            case DeviceExtension::KHR_video_decode_queue: str = "VK_KHR_video_decode_queue"; break;
            case DeviceExtension::KHR_video_encode_queue: str = "VK_KHR_video_encode_queue"; break;
            case DeviceExtension::KHR_video_queue: str = "VK_KHR_video_queue"; break;
            case DeviceExtension::KHR_win32_keyed_mutex: str = "VK_KHR_win32_keyed_mutex"; break;
            case DeviceExtension::KHR_workgroup_memory_explicit_layout: str = "VK_KHR_workgroup_memory_explicit_layout"; break;
            case DeviceExtension::EXT_astc_decode_mode: str = "VK_EXT_astc_decode_mode"; break;
            case DeviceExtension::EXT_attachment_feedback_loop_layout: str = "VK_EXT_attachment_feedback_loop_layout"; break;
            case DeviceExtension::EXT_blend_operation_advanced: str = "VK_EXT_blend_operation_advanced"; break;
            case DeviceExtension::EXT_border_color_swizzle: str = "VK_EXT_border_color_swizzle"; break;
            case DeviceExtension::EXT_calibrated_timestamps: str = "VK_EXT_calibrated_timestamps"; break;
            case DeviceExtension::EXT_color_write_enable: str = "VK_EXT_color_write_enable"; break;
            case DeviceExtension::EXT_conditional_rendering: str = "VK_EXT_conditional_rendering"; break;
            case DeviceExtension::EXT_conservative_rasterization: str = "VK_EXT_conservative_rasterization"; break;
            case DeviceExtension::EXT_custom_border_color: str = "VK_EXT_custom_border_color"; break;
            case DeviceExtension::EXT_depth_clamp_zero_one: str = "VK_EXT_depth_clamp_zero_one"; break;
            case DeviceExtension::EXT_depth_clip_control: str = "VK_EXT_depth_clip_control"; break;
            case DeviceExtension::EXT_depth_clip_enable: str = "VK_EXT_depth_clip_enable"; break;
            case DeviceExtension::EXT_depth_range_unrestricted: str = "VK_EXT_depth_range_unrestricted"; break;
            case DeviceExtension::EXT_descriptor_buffer: str = "VK_EXT_descriptor_buffer"; break;
            case DeviceExtension::EXT_device_address_binding_report: str = "VK_EXT_device_address_binding_report"; break;
            case DeviceExtension::EXT_device_fault: str = "VK_EXT_device_fault"; break;
            case DeviceExtension::EXT_device_memory_report: str = "VK_EXT_device_memory_report"; break;
            case DeviceExtension::EXT_discard_rectangles: str = "VK_EXT_discard_rectangles"; break;
            case DeviceExtension::EXT_display_control: str = "VK_EXT_display_control"; break;
            case DeviceExtension::EXT_extended_dynamic_state3: str = "VK_EXT_extended_dynamic_state3"; break;
            case DeviceExtension::EXT_external_memory_dma_buf: str = "VK_EXT_external_memory_dma_buf"; break;
            case DeviceExtension::EXT_external_memory_host: str = "VK_EXT_external_memory_host"; break;
            case DeviceExtension::EXT_filter_cubic: str = "VK_EXT_filter_cubic"; break;
            case DeviceExtension::EXT_fragment_density_map: str = "VK_EXT_fragment_density_map"; break;
            case DeviceExtension::EXT_fragment_density_map2: str = "VK_EXT_fragment_density_map2"; break;
            case DeviceExtension::EXT_fragment_shader_interlock: str = "VK_EXT_fragment_shader_interlock"; break;
            case DeviceExtension::EXT_full_screen_exclusive: str = "VK_EXT_full_screen_exclusive"; break;
            case DeviceExtension::EXT_graphics_pipeline_library: str = "VK_EXT_graphics_pipeline_library"; break;
            case DeviceExtension::EXT_hdr_metadata: str = "VK_EXT_hdr_metadata"; break;
            case DeviceExtension::EXT_image_2d_view_of_3d: str = "VK_EXT_image_2d_view_of_3d"; break;
            case DeviceExtension::EXT_image_compression_control: str = "VK_EXT_image_compression_control"; break;
            case DeviceExtension::EXT_image_compression_control_swapchain: str = "VK_EXT_image_compression_control_swapchain"; break;
            case DeviceExtension::EXT_image_drm_format_modifier: str = "VK_EXT_image_drm_format_modifier"; break;
            case DeviceExtension::EXT_image_sliced_view_of_3d: str = "VK_EXT_image_sliced_view_of_3d"; break;
            case DeviceExtension::EXT_image_view_min_lod: str = "VK_EXT_image_view_min_lod"; break;
            case DeviceExtension::EXT_index_type_uint8: str = "VK_EXT_index_type_uint8"; break;
            case DeviceExtension::EXT_legacy_dithering: str = "VK_EXT_legacy_dithering"; break;
            case DeviceExtension::EXT_line_rasterization: str = "VK_EXT_line_rasterization"; break;
            case DeviceExtension::EXT_load_store_op_none: str = "VK_EXT_load_store_op_none"; break;
            case DeviceExtension::EXT_memory_budget: str = "VK_EXT_memory_budget"; break;
            case DeviceExtension::EXT_memory_priority: str = "VK_EXT_memory_priority"; break;
            case DeviceExtension::EXT_mesh_shader: str = "VK_EXT_mesh_shader"; break;
            case DeviceExtension::EXT_metal_objects: str = "VK_EXT_metal_objects"; break;
            case DeviceExtension::EXT_multi_draw: str = "VK_EXT_multi_draw"; break;
            case DeviceExtension::EXT_multisampled_render_to_single_sampled: str = "VK_EXT_multisampled_render_to_single_sampled"; break;
            case DeviceExtension::EXT_mutable_descriptor_type: str = "VK_EXT_mutable_descriptor_type"; break;
            case DeviceExtension::EXT_non_seamless_cube_map: str = "VK_EXT_non_seamless_cube_map"; break;
            case DeviceExtension::EXT_opacity_micromap: str = "VK_EXT_opacity_micromap"; break;
            case DeviceExtension::EXT_pageable_device_local_memory: str = "VK_EXT_pageable_device_local_memory"; break;
            case DeviceExtension::EXT_pci_bus_info: str = "VK_EXT_pci_bus_info"; break;
            case DeviceExtension::EXT_physical_device_drm: str = "VK_EXT_physical_device_drm"; break;
            case DeviceExtension::EXT_pipeline_library_group_handles: str = "VK_EXT_pipeline_library_group_handles"; break;
            case DeviceExtension::EXT_pipeline_properties: str = "VK_EXT_pipeline_properties"; break;
            case DeviceExtension::EXT_pipeline_protected_access: str = "VK_EXT_pipeline_protected_access"; break;
            case DeviceExtension::EXT_pipeline_robustness: str = "VK_EXT_pipeline_robustness"; break;
            case DeviceExtension::EXT_post_depth_coverage: str = "VK_EXT_post_depth_coverage"; break;
            case DeviceExtension::EXT_primitive_topology_list_restart: str = "VK_EXT_primitive_topology_list_restart"; break;
            case DeviceExtension::EXT_primitives_generated_query: str = "VK_EXT_primitives_generated_query"; break;
            case DeviceExtension::EXT_provoking_vertex: str = "VK_EXT_provoking_vertex"; break;
            case DeviceExtension::EXT_queue_family_foreign: str = "VK_EXT_queue_family_foreign"; break;
            case DeviceExtension::EXT_rasterization_order_attachment_access: str = "VK_EXT_rasterization_order_attachment_access"; break;
            case DeviceExtension::EXT_rgba10x6_formats: str = "VK_EXT_rgba10x6_formats"; break;
            case DeviceExtension::EXT_robustness2: str = "VK_EXT_robustness2"; break;
            case DeviceExtension::EXT_sample_locations: str = "VK_EXT_sample_locations"; break;
            case DeviceExtension::EXT_shader_atomic_float: str = "VK_EXT_shader_atomic_float"; break;
            case DeviceExtension::EXT_shader_atomic_float2: str = "VK_EXT_shader_atomic_float2"; break;
            case DeviceExtension::EXT_shader_image_atomic_int64: str = "VK_EXT_shader_image_atomic_int64"; break;
            case DeviceExtension::EXT_shader_module_identifier: str = "VK_EXT_shader_module_identifier"; break;
            case DeviceExtension::EXT_shader_object: str = "VK_EXT_shader_object"; break;
            case DeviceExtension::EXT_shader_stencil_export: str = "VK_EXT_shader_stencil_export"; break;
            case DeviceExtension::EXT_shader_tile_image: str = "VK_EXT_shader_tile_image"; break;
            case DeviceExtension::EXT_subpass_merge_feedback: str = "VK_EXT_subpass_merge_feedback"; break;
            case DeviceExtension::EXT_swapchain_maintenance1: str = "VK_EXT_swapchain_maintenance1"; break;
            case DeviceExtension::EXT_transform_feedback: str = "VK_EXT_transform_feedback"; break;
            case DeviceExtension::EXT_validation_cache: str = "VK_EXT_validation_cache"; break;
            case DeviceExtension::EXT_vertex_attribute_divisor: str = "VK_EXT_vertex_attribute_divisor"; break;
            case DeviceExtension::EXT_vertex_input_dynamic_state: str = "VK_EXT_vertex_input_dynamic_state"; break;
            case DeviceExtension::EXT_video_encode_h264: str = "VK_EXT_video_encode_h264"; break;
            case DeviceExtension::EXT_video_encode_h265: str = "VK_EXT_video_encode_h265"; break;
            case DeviceExtension::EXT_ycbcr_image_arrays: str = "VK_EXT_ycbcr_image_arrays"; break;
            case DeviceExtension::AMD_buffer_marker: str = "VK_AMD_buffer_marker"; break;
            case DeviceExtension::AMD_device_coherent_memory: str = "VK_AMD_device_coherent_memory"; break;
            case DeviceExtension::AMD_display_native_hdr: str = "VK_AMD_display_native_hdr"; break;
            case DeviceExtension::AMD_gcn_shader: str = "VK_AMD_gcn_shader"; break;
            case DeviceExtension::AMD_memory_overallocation_behavior: str = "VK_AMD_memory_overallocation_behavior"; break;
            case DeviceExtension::AMD_mixed_attachment_samples: str = "VK_AMD_mixed_attachment_samples"; break;
            case DeviceExtension::AMD_pipeline_compiler_control: str = "VK_AMD_pipeline_compiler_control"; break;
            case DeviceExtension::AMD_rasterization_order: str = "VK_AMD_rasterization_order"; break;
            case DeviceExtension::AMD_shader_ballot: str = "VK_AMD_shader_ballot"; break;
            case DeviceExtension::AMD_shader_core_properties: str = "VK_AMD_shader_core_properties"; break;
            case DeviceExtension::AMD_shader_core_properties2: str = "VK_AMD_shader_core_properties2"; break;
            case DeviceExtension::AMD_shader_early_and_late_fragment_tests: str = "VK_AMD_shader_early_and_late_fragment_tests"; break;
            case DeviceExtension::AMD_shader_explicit_vertex_parameter: str = "VK_AMD_shader_explicit_vertex_parameter"; break;
            case DeviceExtension::AMD_shader_fragment_mask: str = "VK_AMD_shader_fragment_mask"; break;
            case DeviceExtension::AMD_shader_image_load_store_lod: str = "VK_AMD_shader_image_load_store_lod"; break;
            case DeviceExtension::AMD_shader_info: str = "VK_AMD_shader_info"; break;
            case DeviceExtension::AMD_shader_trinary_minmax: str = "VK_AMD_shader_trinary_minmax"; break;
            case DeviceExtension::AMD_texture_gather_bias_lod: str = "VK_AMD_texture_gather_bias_lod"; break;
            case DeviceExtension::ANDROID_external_memory_android_hardware_buffer: str = "VK_ANDROID_external_memory_android_hardware_buffer"; break;
            case DeviceExtension::ARM_shader_core_builtins: str = "VK_ARM_shader_core_builtins"; break;
            case DeviceExtension::ARM_shader_core_properties: str = "VK_ARM_shader_core_properties"; break;
            case DeviceExtension::FUCHSIA_buffer_collection: str = "VK_FUCHSIA_buffer_collection"; break;
            case DeviceExtension::FUCHSIA_external_memory: str = "VK_FUCHSIA_external_memory"; break;
            case DeviceExtension::FUCHSIA_external_semaphore: str = "VK_FUCHSIA_external_semaphore"; break;
            case DeviceExtension::GGP_frame_token: str = "VK_GGP_frame_token"; break;
            case DeviceExtension::GOOGLE_decorate_string: str = "VK_GOOGLE_decorate_string"; break;
            case DeviceExtension::GOOGLE_display_timing: str = "VK_GOOGLE_display_timing"; break;
            case DeviceExtension::GOOGLE_hlsl_functionality1: str = "VK_GOOGLE_hlsl_functionality1"; break;
            case DeviceExtension::GOOGLE_user_type: str = "VK_GOOGLE_user_type"; break;
            case DeviceExtension::HUAWEI_cluster_culling_shader: str = "VK_HUAWEI_cluster_culling_shader"; break;
            case DeviceExtension::HUAWEI_invocation_mask: str = "VK_HUAWEI_invocation_mask"; break;
            case DeviceExtension::HUAWEI_subpass_shading: str = "VK_HUAWEI_subpass_shading"; break;
            case DeviceExtension::IMG_filter_cubic: str = "VK_IMG_filter_cubic"; break;
            case DeviceExtension::INTEL_performance_query: str = "VK_INTEL_performance_query"; break;
            case DeviceExtension::INTEL_shader_integer_functions2: str = "VK_INTEL_shader_integer_functions2"; break;
            case DeviceExtension::NV_acquire_winrt_display: str = "VK_NV_acquire_winrt_display"; break;
            case DeviceExtension::NV_clip_space_w_scaling: str = "VK_NV_clip_space_w_scaling"; break;
            case DeviceExtension::NV_compute_shader_derivatives: str = "VK_NV_compute_shader_derivatives"; break;
            case DeviceExtension::NV_cooperative_matrix: str = "VK_NV_cooperative_matrix"; break;
            case DeviceExtension::NV_copy_memory_indirect: str = "VK_NV_copy_memory_indirect"; break;
            case DeviceExtension::NV_corner_sampled_image: str = "VK_NV_corner_sampled_image"; break;
            case DeviceExtension::NV_coverage_reduction_mode: str = "VK_NV_coverage_reduction_mode"; break;
            case DeviceExtension::NV_dedicated_allocation_image_aliasing: str = "VK_NV_dedicated_allocation_image_aliasing"; break;
            case DeviceExtension::NV_device_diagnostic_checkpoints: str = "VK_NV_device_diagnostic_checkpoints"; break;
            case DeviceExtension::NV_device_diagnostics_config: str = "VK_NV_device_diagnostics_config"; break;
            case DeviceExtension::NV_device_generated_commands: str = "VK_NV_device_generated_commands"; break;
            case DeviceExtension::NV_displacement_micromap: str = "VK_NV_displacement_micromap"; break;
            case DeviceExtension::NV_external_memory_rdma: str = "VK_NV_external_memory_rdma"; break;
            case DeviceExtension::NV_fill_rectangle: str = "VK_NV_fill_rectangle"; break;
            case DeviceExtension::NV_fragment_coverage_to_color: str = "VK_NV_fragment_coverage_to_color"; break;
            case DeviceExtension::NV_fragment_shading_rate_enums: str = "VK_NV_fragment_shading_rate_enums"; break;
            case DeviceExtension::NV_framebuffer_mixed_samples: str = "VK_NV_framebuffer_mixed_samples"; break;
            case DeviceExtension::NV_geometry_shader_passthrough: str = "VK_NV_geometry_shader_passthrough"; break;
            case DeviceExtension::NV_inherited_viewport_scissor: str = "VK_NV_inherited_viewport_scissor"; break;
            case DeviceExtension::NV_linear_color_attachment: str = "VK_NV_linear_color_attachment"; break;
            case DeviceExtension::NV_low_latency: str = "VK_NV_low_latency"; break;
            case DeviceExtension::NV_memory_decompression: str = "VK_NV_memory_decompression"; break;
            case DeviceExtension::NV_mesh_shader: str = "VK_NV_mesh_shader"; break;
            case DeviceExtension::NV_optical_flow: str = "VK_NV_optical_flow"; break;
            case DeviceExtension::NV_present_barrier: str = "VK_NV_present_barrier"; break;
            case DeviceExtension::NV_ray_tracing: str = "VK_NV_ray_tracing"; break;
            case DeviceExtension::NV_ray_tracing_invocation_reorder: str = "VK_NV_ray_tracing_invocation_reorder"; break;
            case DeviceExtension::NV_ray_tracing_motion_blur: str = "VK_NV_ray_tracing_motion_blur"; break;
            case DeviceExtension::NV_representative_fragment_test: str = "VK_NV_representative_fragment_test"; break;
            case DeviceExtension::NV_sample_mask_override_coverage: str = "VK_NV_sample_mask_override_coverage"; break;
            case DeviceExtension::NV_scissor_exclusive: str = "VK_NV_scissor_exclusive"; break;
            case DeviceExtension::NV_shader_image_footprint: str = "VK_NV_shader_image_footprint"; break;
            case DeviceExtension::NV_shader_sm_builtins: str = "VK_NV_shader_sm_builtins"; break;
            case DeviceExtension::NV_shader_subgroup_partitioned: str = "VK_NV_shader_subgroup_partitioned"; break;
            case DeviceExtension::NV_shading_rate_image: str = "VK_NV_shading_rate_image"; break;
            case DeviceExtension::NV_viewport_array2: str = "VK_NV_viewport_array2"; break;
            case DeviceExtension::NV_viewport_swizzle: str = "VK_NV_viewport_swizzle"; break;
            case DeviceExtension::NVX_binary_import: str = "VK_NVX_binary_import"; break;
            case DeviceExtension::NVX_image_view_handle: str = "VK_NVX_image_view_handle"; break;
            case DeviceExtension::NVX_multiview_per_view_attributes: str = "VK_NVX_multiview_per_view_attributes"; break;
            case DeviceExtension::QCOM_fragment_density_map_offset: str = "VK_QCOM_fragment_density_map_offset"; break;
            case DeviceExtension::QCOM_image_processing: str = "VK_QCOM_image_processing"; break;
            case DeviceExtension::QCOM_multiview_per_view_render_areas: str = "VK_QCOM_multiview_per_view_render_areas"; break;
            case DeviceExtension::QCOM_multiview_per_view_viewports: str = "VK_QCOM_multiview_per_view_viewports"; break;
            case DeviceExtension::QCOM_render_pass_shader_resolve: str = "VK_QCOM_render_pass_shader_resolve"; break;
            case DeviceExtension::QCOM_render_pass_store_ops: str = "VK_QCOM_render_pass_store_ops"; break;
            case DeviceExtension::QCOM_render_pass_transform: str = "VK_QCOM_render_pass_transform"; break;
            case DeviceExtension::QCOM_rotated_copy_commands: str = "VK_QCOM_rotated_copy_commands"; break;
            case DeviceExtension::QCOM_tile_properties: str = "VK_QCOM_tile_properties"; break;
            case DeviceExtension::SEC_amigo_profiling: str = "VK_SEC_amigo_profiling"; break;
            case DeviceExtension::VALVE_descriptor_set_host_mapping: str = "VK_VALVE_descriptor_set_host_mapping"; break;
        }
        return str;
    }


    template <typename Key, typename Value, std::size_t size>
    struct Map_dbg// just an array of pairs, heavily inspired from Jason Turner video about it 
    #ifndef NDEBUG 
    { 
        using value_type = std::pair<Key, Value>;
        using mapped_type = Value;
        using key_type = Key;
        std::array<std::pair<Key, Value>, size> data;

        [[nodiscard]] constexpr auto at(const Key &key) const -> std::optional<Value> 
        {
            // I can't include the whole algorithm header just for this. TODO: see if any other usage of the
            // header comes up and then include it and uncomment this
            // const auto itr =     
                // std::find_if(std::begin(data), std::end(data),
                //              [&key](const auto &v) { return v.first == key; });
            typename decltype(data)::const_iterator itr = std::begin(data);
            for (; itr != std::end(data); ++itr)
            {
                if (itr->first == key)
                    break;
            }
            if (itr != std::end(data)) 
                return itr->second;
            else
                return std::nullopt;
        }
    };
    #else
    {};
    #endif

    uint32_t constexpr maxDependencies = 4;
    uint32_t constexpr dependenciesCount = 227;
    using Extension = std::variant<InstanceExtension, DeviceExtension>; // not constant expression in gcc 12.2?
    using DependencyVec = StaticVector<Extension, maxDependencies>;
    using ThePair = std::pair<Extension const, DependencyVec>;
 
    // relying on Named Return Value Optimization
    consteval auto makeExtensionDependencyMap_dbg() -> Map_dbg<Extension, StaticVector<Extension, maxDependencies>, dependenciesCount>
    #ifndef NDEBUG
    {
        return {
            std::make_pair<Extension, DependencyVec>(InstanceExtension::KHR_android_surface, {InstanceExtension::KHR_surface}),
            {InstanceExtension::KHR_display, {InstanceExtension::KHR_surface}},
            {InstanceExtension::KHR_get_surface_capabilities2, {InstanceExtension::KHR_surface}},
            {InstanceExtension::KHR_get_display_properties2, {InstanceExtension::KHR_display}},
            {InstanceExtension::KHR_surface_protected_capabilities, {InstanceExtension::KHR_get_surface_capabilities2}},
            {InstanceExtension::KHR_wayland_surface, {InstanceExtension::KHR_surface}},
            {InstanceExtension::KHR_win32_surface, {InstanceExtension::KHR_surface}},
            {InstanceExtension::KHR_xcb_surface, {InstanceExtension::KHR_surface}},
            {InstanceExtension::KHR_xlib_surface, {InstanceExtension::KHR_surface}},
            {InstanceExtension::EXT_acquire_drm_display, {InstanceExtension::EXT_direct_mode_display}},
            {InstanceExtension::EXT_acquire_xlib_display, {InstanceExtension::EXT_direct_mode_display}},
            {InstanceExtension::EXT_direct_mode_display, {InstanceExtension::KHR_display}},
            {InstanceExtension::EXT_directfb_surface, {InstanceExtension::KHR_surface}},
            {InstanceExtension::EXT_display_surface_counter, {InstanceExtension::KHR_display}},
            {InstanceExtension::EXT_headless_surface, {InstanceExtension::KHR_surface}},
            {InstanceExtension::EXT_metal_surface, {InstanceExtension::KHR_surface}},
            {InstanceExtension::EXT_surface_maintenance1, {InstanceExtension::KHR_surface, InstanceExtension::KHR_get_surface_capabilities2}},
            {InstanceExtension::EXT_swapchain_colorspace, {InstanceExtension::KHR_surface}},
            {InstanceExtension::FUCHSIA_imagepipe_surface, {InstanceExtension::KHR_surface}},
            {InstanceExtension::GGP_stream_descriptor_surface, {InstanceExtension::KHR_surface}},
            {InstanceExtension::GOOGLE_surfaceless_query, {InstanceExtension::KHR_surface}},
            {InstanceExtension::NN_vi_surface, {InstanceExtension::KHR_surface}},
            {InstanceExtension::QNX_screen_surface, {InstanceExtension::KHR_surface}},
            {DeviceExtension::KHR_16bit_storage, {}},
            {DeviceExtension::KHR_8bit_storage, {}},
            {DeviceExtension::KHR_acceleration_structure, {DeviceExtension::KHR_deferred_host_operations}},
            {DeviceExtension::KHR_deferred_host_operations, {}},
            {DeviceExtension::KHR_display_swapchain, {DeviceExtension::KHR_swapchain, InstanceExtension::KHR_display}},
            {DeviceExtension::KHR_external_fence_fd, {}},
            {DeviceExtension::KHR_external_fence_win32, {}},
            {DeviceExtension::KHR_external_memory_fd, {}},
            {DeviceExtension::KHR_external_memory_win32, {}},
            {DeviceExtension::KHR_external_semaphore_fd, {}},
            {DeviceExtension::KHR_external_semaphore_win32, {}},
            {DeviceExtension::KHR_fragment_shader_barycentric, {}},
            {DeviceExtension::KHR_fragment_shading_rate, {}},
            {DeviceExtension::KHR_global_priority, {}},
            {DeviceExtension::KHR_incremental_present, {DeviceExtension::KHR_swapchain}},
            {DeviceExtension::KHR_map_memory2, {}},
            {DeviceExtension::KHR_performance_query, {}},
            {DeviceExtension::KHR_pipeline_executable_properties, {}},
            {DeviceExtension::KHR_pipeline_library, {}},
            {DeviceExtension::KHR_portability_subset, {}},
            {DeviceExtension::KHR_present_id, {DeviceExtension::KHR_swapchain}},
            {DeviceExtension::KHR_present_wait, {DeviceExtension::KHR_swapchain, DeviceExtension::KHR_present_id}},
            {DeviceExtension::KHR_push_descriptor, {}},
            {DeviceExtension::KHR_ray_query, {DeviceExtension::KHR_acceleration_structure}},
            {DeviceExtension::KHR_ray_tracing_maintenance1, {DeviceExtension::KHR_acceleration_structure}},
            {DeviceExtension::KHR_ray_tracing_pipeline, {DeviceExtension::KHR_acceleration_structure}},
            {DeviceExtension::KHR_shader_clock, {}},
            {DeviceExtension::KHR_shader_subgroup_uniform_control_flow, {}},
            {DeviceExtension::KHR_shared_presentable_image, {DeviceExtension::KHR_swapchain, InstanceExtension::KHR_get_surface_capabilities2}},
            {DeviceExtension::KHR_swapchain, {InstanceExtension::KHR_surface}},
            {DeviceExtension::KHR_swapchain_mutable_format, {DeviceExtension::KHR_swapchain}},
            {DeviceExtension::KHR_video_decode_h264, {DeviceExtension::KHR_video_decode_queue}},
            {DeviceExtension::KHR_video_decode_h265, {DeviceExtension::KHR_video_decode_queue}},
            {DeviceExtension::KHR_video_decode_queue, {DeviceExtension::KHR_video_queue}},
            {DeviceExtension::KHR_video_encode_queue, {DeviceExtension::KHR_video_queue}},
            {DeviceExtension::KHR_video_queue, {}},
            {DeviceExtension::KHR_win32_keyed_mutex, {DeviceExtension::KHR_external_memory_win32}},
            {DeviceExtension::KHR_workgroup_memory_explicit_layout, {}},
            {DeviceExtension::EXT_astc_decode_mode, {}},
            {DeviceExtension::EXT_attachment_feedback_loop_layout, {}},
            {DeviceExtension::EXT_blend_operation_advanced, {}},
            {DeviceExtension::EXT_border_color_swizzle, {DeviceExtension::EXT_custom_border_color}},
            {DeviceExtension::EXT_calibrated_timestamps, {}},
            {DeviceExtension::EXT_color_write_enable, {}},
            {DeviceExtension::EXT_conditional_rendering, {}},
            {DeviceExtension::EXT_conservative_rasterization, {}},
            {DeviceExtension::EXT_custom_border_color, {}},
            {DeviceExtension::EXT_depth_clamp_zero_one, {}},
            {DeviceExtension::EXT_depth_clip_control, {}},
            {DeviceExtension::EXT_depth_clip_enable, {}},
            {DeviceExtension::EXT_depth_range_unrestricted, {}},
            {DeviceExtension::EXT_descriptor_buffer, {}},
            {DeviceExtension::EXT_device_address_binding_report, {InstanceExtension::EXT_debug_utils}},
            {DeviceExtension::EXT_device_fault, {}},
            {DeviceExtension::EXT_device_memory_report, {}},
            {DeviceExtension::EXT_discard_rectangles, {}},
            {DeviceExtension::EXT_display_control, {InstanceExtension::EXT_display_surface_counter, DeviceExtension::KHR_swapchain}},
            {DeviceExtension::EXT_extended_dynamic_state3, {}},
            {DeviceExtension::EXT_external_memory_dma_buf, {DeviceExtension::KHR_external_memory_fd}},
            {DeviceExtension::EXT_external_memory_host, {}},
            {DeviceExtension::EXT_filter_cubic, {}},
            {DeviceExtension::EXT_fragment_density_map, {}},
            {DeviceExtension::EXT_fragment_density_map2, {DeviceExtension::EXT_fragment_density_map}},
            {DeviceExtension::EXT_fragment_shader_interlock, {}},
            {DeviceExtension::EXT_full_screen_exclusive, {InstanceExtension::KHR_get_surface_capabilities2, InstanceExtension::KHR_surface, DeviceExtension::KHR_swapchain}},
            {DeviceExtension::EXT_graphics_pipeline_library, {DeviceExtension::KHR_pipeline_library}},
            {DeviceExtension::EXT_hdr_metadata, {DeviceExtension::KHR_swapchain}},
            {DeviceExtension::EXT_image_2d_view_of_3d, {}},
            {DeviceExtension::EXT_image_compression_control, {}},
            {DeviceExtension::EXT_image_compression_control_swapchain, {DeviceExtension::EXT_image_compression_control}},
            {DeviceExtension::EXT_image_drm_format_modifier, {}},
            {DeviceExtension::EXT_image_sliced_view_of_3d, {}},
            {DeviceExtension::EXT_image_view_min_lod, {}},
            {DeviceExtension::EXT_index_type_uint8, {}},
            {DeviceExtension::EXT_legacy_dithering, {}},
            {DeviceExtension::EXT_line_rasterization, {}},
            {DeviceExtension::EXT_load_store_op_none, {}},
            {DeviceExtension::EXT_memory_budget, {}},
            {DeviceExtension::EXT_memory_priority, {}},
            {DeviceExtension::EXT_mesh_shader, {}},
            {DeviceExtension::EXT_metal_objects, {}},
            {DeviceExtension::EXT_multi_draw, {}},
            {DeviceExtension::EXT_multisampled_render_to_single_sampled, {}},
            {DeviceExtension::EXT_mutable_descriptor_type, {}},
            {DeviceExtension::EXT_non_seamless_cube_map, {}},
            {DeviceExtension::EXT_opacity_micromap, {DeviceExtension::KHR_acceleration_structure}},
            {DeviceExtension::EXT_pageable_device_local_memory, {DeviceExtension::EXT_memory_priority}},
            {DeviceExtension::EXT_pci_bus_info, {}},
            {DeviceExtension::EXT_physical_device_drm, {}},
            {DeviceExtension::EXT_pipeline_library_group_handles, {DeviceExtension::KHR_ray_tracing_pipeline, DeviceExtension::KHR_pipeline_library}},
            {DeviceExtension::EXT_pipeline_properties, {}},
            {DeviceExtension::EXT_pipeline_protected_access, {}},
            {DeviceExtension::EXT_pipeline_robustness, {}},
            {DeviceExtension::EXT_post_depth_coverage, {}},
            {DeviceExtension::EXT_primitive_topology_list_restart, {}},
            {DeviceExtension::EXT_primitives_generated_query, {DeviceExtension::EXT_transform_feedback}},
            {DeviceExtension::EXT_provoking_vertex, {}},
            {DeviceExtension::EXT_queue_family_foreign, {}},
            {DeviceExtension::EXT_rasterization_order_attachment_access, {}},
            {DeviceExtension::EXT_rgba10x6_formats, {}},
            {DeviceExtension::EXT_robustness2, {}},
            {DeviceExtension::EXT_sample_locations, {}},
            {DeviceExtension::EXT_shader_atomic_float, {}},
            {DeviceExtension::EXT_shader_atomic_float2, {DeviceExtension::EXT_shader_atomic_float}},
            {DeviceExtension::EXT_shader_image_atomic_int64, {}},
            {DeviceExtension::EXT_shader_module_identifier, {}},
            {DeviceExtension::EXT_shader_object, {}},
            {DeviceExtension::EXT_shader_stencil_export, {}},
            {DeviceExtension::EXT_shader_tile_image, {}},
            {DeviceExtension::EXT_subpass_merge_feedback, {}},
            {DeviceExtension::EXT_swapchain_maintenance1, {DeviceExtension::KHR_swapchain, InstanceExtension::EXT_surface_maintenance1}},
            {DeviceExtension::EXT_transform_feedback, {}},
            {DeviceExtension::EXT_validation_cache, {}},
            {DeviceExtension::EXT_vertex_attribute_divisor, {}},
            {DeviceExtension::EXT_vertex_input_dynamic_state, {}},
            {DeviceExtension::EXT_video_encode_h264, {DeviceExtension::KHR_video_encode_queue}},
            {DeviceExtension::EXT_video_encode_h265, {DeviceExtension::KHR_video_encode_queue}},
            {DeviceExtension::EXT_ycbcr_image_arrays, {}},
            {DeviceExtension::AMD_buffer_marker, {}},
            {DeviceExtension::AMD_device_coherent_memory, {}},
            {DeviceExtension::AMD_display_native_hdr, {InstanceExtension::KHR_get_surface_capabilities2, DeviceExtension::KHR_swapchain}},
            {DeviceExtension::AMD_gcn_shader, {}},
            {DeviceExtension::AMD_memory_overallocation_behavior, {}},
            {DeviceExtension::AMD_mixed_attachment_samples, {}},
            {DeviceExtension::AMD_pipeline_compiler_control, {}},
            {DeviceExtension::AMD_rasterization_order, {}},
            {DeviceExtension::AMD_shader_ballot, {}},
            {DeviceExtension::AMD_shader_core_properties, {}},
            {DeviceExtension::AMD_shader_core_properties2, {DeviceExtension::AMD_shader_core_properties}},
            {DeviceExtension::AMD_shader_early_and_late_fragment_tests, {}},
            {DeviceExtension::AMD_shader_explicit_vertex_parameter, {}},
            {DeviceExtension::AMD_shader_fragment_mask, {}},
            {DeviceExtension::AMD_shader_image_load_store_lod, {}},
            {DeviceExtension::AMD_shader_info, {}},
            {DeviceExtension::AMD_shader_trinary_minmax, {}},
            {DeviceExtension::AMD_texture_gather_bias_lod, {}},
            {DeviceExtension::ANDROID_external_memory_android_hardware_buffer, {DeviceExtension::EXT_queue_family_foreign}},
            {DeviceExtension::ARM_shader_core_builtins, {}},
            {DeviceExtension::ARM_shader_core_properties, {}},
            {DeviceExtension::FUCHSIA_buffer_collection, {DeviceExtension::FUCHSIA_external_memory}},
            {DeviceExtension::FUCHSIA_external_memory, {}},
            {DeviceExtension::FUCHSIA_external_semaphore, {}},
            {DeviceExtension::GGP_frame_token, {DeviceExtension::KHR_swapchain, InstanceExtension::GGP_stream_descriptor_surface}},
            {DeviceExtension::GOOGLE_decorate_string, {}},
            {DeviceExtension::GOOGLE_display_timing, {DeviceExtension::KHR_swapchain}},
            {DeviceExtension::GOOGLE_hlsl_functionality1, {}},
            {DeviceExtension::GOOGLE_user_type, {}},
            {DeviceExtension::HUAWEI_cluster_culling_shader, {}},
            {DeviceExtension::HUAWEI_invocation_mask, {DeviceExtension::KHR_ray_tracing_pipeline}},
            {DeviceExtension::HUAWEI_subpass_shading, {}},
            {DeviceExtension::IMG_filter_cubic, {}},
            {DeviceExtension::INTEL_performance_query, {}},
            {DeviceExtension::INTEL_shader_integer_functions2, {}},
            {DeviceExtension::NV_acquire_winrt_display, {InstanceExtension::EXT_direct_mode_display}},
            {DeviceExtension::NV_clip_space_w_scaling, {}},
            {DeviceExtension::NV_compute_shader_derivatives, {}},
            {DeviceExtension::NV_cooperative_matrix, {}},
            {DeviceExtension::NV_copy_memory_indirect, {}},
            {DeviceExtension::NV_corner_sampled_image, {}},
            {DeviceExtension::NV_coverage_reduction_mode, {DeviceExtension::NV_framebuffer_mixed_samples}},
            {DeviceExtension::NV_dedicated_allocation_image_aliasing, {}},
            {DeviceExtension::NV_device_diagnostic_checkpoints, {}},
            {DeviceExtension::NV_device_diagnostics_config, {}},
            {DeviceExtension::NV_device_generated_commands, {}},
            {DeviceExtension::NV_displacement_micromap, {DeviceExtension::EXT_opacity_micromap}},
            {DeviceExtension::NV_external_memory_rdma, {}},
            {DeviceExtension::NV_fill_rectangle, {}},
            {DeviceExtension::NV_fragment_coverage_to_color, {}},
            {DeviceExtension::NV_fragment_shading_rate_enums, {DeviceExtension::KHR_fragment_shading_rate}},
            {DeviceExtension::NV_framebuffer_mixed_samples, {}},
            {DeviceExtension::NV_geometry_shader_passthrough, {}},
            {DeviceExtension::NV_inherited_viewport_scissor, {}},
            {DeviceExtension::NV_linear_color_attachment, {}},
            {DeviceExtension::NV_low_latency, {}},
            {DeviceExtension::NV_memory_decompression, {}},
            {DeviceExtension::NV_mesh_shader, {}},
            {DeviceExtension::NV_optical_flow, {}},
            {DeviceExtension::NV_present_barrier, {InstanceExtension::KHR_get_surface_capabilities2, InstanceExtension::KHR_surface, DeviceExtension::KHR_swapchain}},
            {DeviceExtension::NV_ray_tracing, {}},
            {DeviceExtension::NV_ray_tracing_invocation_reorder, {DeviceExtension::KHR_ray_tracing_pipeline}},
            {DeviceExtension::NV_ray_tracing_motion_blur, {DeviceExtension::KHR_ray_tracing_pipeline}},
            {DeviceExtension::NV_representative_fragment_test, {}},
            {DeviceExtension::NV_sample_mask_override_coverage, {}},
            {DeviceExtension::NV_scissor_exclusive, {}},
            {DeviceExtension::NV_shader_image_footprint, {}},
            {DeviceExtension::NV_shader_sm_builtins, {}},
            {DeviceExtension::NV_shader_subgroup_partitioned, {}},
            {DeviceExtension::NV_shading_rate_image, {}},
            {DeviceExtension::NV_viewport_array2, {}},
            {DeviceExtension::NV_viewport_swizzle, {}},
            {DeviceExtension::NVX_binary_import, {}},
            {DeviceExtension::NVX_image_view_handle, {}},
            {DeviceExtension::NVX_multiview_per_view_attributes, {}},
            {DeviceExtension::QCOM_fragment_density_map_offset, {DeviceExtension::EXT_fragment_density_map}},
            {DeviceExtension::QCOM_image_processing, {}},
            {DeviceExtension::QCOM_multiview_per_view_render_areas, {}},
            {DeviceExtension::QCOM_multiview_per_view_viewports, {}},
            {DeviceExtension::QCOM_render_pass_shader_resolve, {}},
            {DeviceExtension::QCOM_render_pass_store_ops, {}},
            {DeviceExtension::QCOM_render_pass_transform, {DeviceExtension::KHR_swapchain, InstanceExtension::KHR_surface}},
            {DeviceExtension::QCOM_rotated_copy_commands, {DeviceExtension::KHR_swapchain}},
            {DeviceExtension::QCOM_tile_properties, {}},
            {DeviceExtension::SEC_amigo_profiling, {}},
            {DeviceExtension::VALVE_descriptor_set_host_mapping, {}},
        };
    }
    #else
    {
        return {};
    }
    #endif
    inline Map_dbg<Extension, StaticVector<Extension, maxDependencies>, dependenciesCount> constexpr 
        extensionDependencyMap_dbg = makeExtensionDependencyMap_dbg();
    
    struct SlicedVkStruct { VkStructureType sType; void* pNext;}; // should work because vulkan structs are standard layout

    // TODO you didn't get all the features. see  VUID-VkDeviceCreateInfo-pNext-pNext from docs to correct that later.
    // really space inefficient class.
    template <typename T>
    concept FeatureType = 
        std::is_same_v<T, VkPhysicalDeviceVulkan11Features> ||
        std::is_same_v<T, VkPhysicalDeviceVulkan12Features> ||
        std::is_same_v<T, VkPhysicalDeviceVulkan13Features> ||
        std::is_same_v<T, VkPhysicalDeviceVariablePointersFeatures> ||
        std::is_same_v<T, VkPhysicalDeviceMultiviewFeatures> ||
        std::is_same_v<T, VkPhysicalDeviceShaderAtomicInt64Features> ||
        std::is_same_v<T, VkPhysicalDevice8BitStorageFeatures> ||
        std::is_same_v<T, VkPhysicalDevice16BitStorageFeatures> ||
        std::is_same_v<T, VkPhysicalDeviceShaderFloat16Int8Features> ||
        std::is_same_v<T, VkPhysicalDeviceSamplerYcbcrConversionFeatures> ||
        std::is_same_v<T, VkPhysicalDeviceProtectedMemoryFeatures> ||
        std::is_same_v<T, VkPhysicalDeviceShaderDrawParametersFeatures> ||
        std::is_same_v<T, VkPhysicalDeviceDescriptorIndexingFeatures> ||
        std::is_same_v<T, VkPhysicalDeviceVulkanMemoryModelFeatures> ||
        std::is_same_v<T, VkPhysicalDeviceInlineUniformBlockFeatures> ||
        std::is_same_v<T, VkPhysicalDeviceScalarBlockLayoutFeatures> ||
        std::is_same_v<T, VkPhysicalDeviceUniformBufferStandardLayoutFeatures> ||
        std::is_same_v<T, VkPhysicalDeviceBufferDeviceAddressFeatures> ||
        std::is_same_v<T, VkPhysicalDeviceImagelessFramebufferFeatures> ||
        std::is_same_v<T, VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures> ||
        std::is_same_v<T, VkPhysicalDeviceHostQueryResetFeatures> ||
        std::is_same_v<T, VkPhysicalDeviceTimelineSemaphoreFeatures> ||
        std::is_same_v<T, VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures> ||
        std::is_same_v<T, VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures> ||
        std::is_same_v<T, VkPhysicalDeviceTextureCompressionASTCHDRFeatures> ||
        std::is_same_v<T, VkPhysicalDeviceSubgroupSizeControlFeatures> ||
        std::is_same_v<T, VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures> ||
        std::is_same_v<T, VkPhysicalDevicePrivateDataFeatures> ||
        std::is_same_v<T, VkPhysicalDeviceImageRobustnessFeatures> ||
        std::is_same_v<T, VkPhysicalDeviceShaderTerminateInvocationFeatures> ||
        std::is_same_v<T, VkPhysicalDeviceSynchronization2Features> ||
        std::is_same_v<T, VkPhysicalDeviceShaderIntegerDotProductFeatures> ||
        std::is_same_v<T, VkPhysicalDeviceMaintenance4Features> ||
        std::is_same_v<T, VkPhysicalDeviceDynamicRenderingFeatures> ||
#ifdef VK_EXT_rgba10x6_formats
        std::is_same_v<T, VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT> ||
#endif
#ifdef VK_EXT_pipeline_robustness
        std::is_same_v<T, VkPhysicalDevicePipelineRobustnessFeaturesEXT> ||
#endif
#ifdef VK_EXT_image_view_min_lod
        std::is_same_v<T, VkPhysicalDeviceImageViewMinLodFeaturesEXT> ||
#endif
#ifdef VK_EXT_rasterization_order_attachment_access
        std::is_same_v<T, VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT> ||
#endif
#ifdef VK_EXT_subpass_merge_feedback
        std::is_same_v<T, VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT> ||
#endif
#ifdef VK_NV_linear_color_attachment
        std::is_same_v<T, VkPhysicalDeviceLinearColorAttachmentFeaturesNV> ||
#endif
#ifdef VK_EXT_attachment_feedback_loop_layout
        std::is_same_v<T, VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT> ||
#endif
#ifdef VK_EXT_graphics_pipeline_library
        std::is_same_v<T, VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT> ||
#endif
#ifdef VK_EXT_multisampled_render_to_single_sampled
        std::is_same_v<T, VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT> ||
#endif
#ifdef VK_EXT_image_2d_view_of_3d
        std::is_same_v<T, VkPhysicalDeviceImage2DViewOf3DFeaturesEXT> ||
#endif
#ifdef VK_EXT_image_sliced_view_of_3d
        std::is_same_v<T, VkPhysicalDeviceImageSlicedViewOf3DFeaturesEXT> ||
#endif
#ifdef VK_EXT_image_compression_control_swapchain
        std::is_same_v<T, VkPhysicalDeviceImageCompressionControlFeaturesEXT> ||
        std::is_same_v<T, VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT> ||
#endif
#ifdef VK_AMD_shader_early_and_late_fragment_tests
        std::is_same_v<T, VkPhysicalDeviceShaderEarlyAndLateFragmentTestsFeaturesAMD> ||
#endif
#ifdef VK_EXT_non_seamless_cube_map
        std::is_same_v<T, VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT> ||
#endif
#ifdef VK_EXT_shader_module_identifier
        std::is_same_v<T, VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT> ||
#endif
#ifdef VK_QCOM_tile_properties
        std::is_same_v<T, VkPhysicalDeviceTilePropertiesFeaturesQCOM> ||
#endif
#ifdef VK_QCOM_image_processing
        std::is_same_v<T, VkPhysicalDeviceImageProcessingFeaturesQCOM> ||
#endif
#ifdef VK_EXT_depth_clamp_zero_one
        std::is_same_v<T, VkPhysicalDeviceDepthClampZeroOneFeaturesEXT> ||
#endif
#ifdef VK_EXT_shader_tile_image
        std::is_same_v<T, VkPhysicalDeviceShaderTileImageFeaturesEXT> ||
#endif
#ifdef VK_EXT_device_address_binding_report
        std::is_same_v<T, VkPhysicalDeviceAddressBindingReportFeaturesEXT> ||
#endif
#ifdef VK_NV_optical_flow
        std::is_same_v<T, VkPhysicalDeviceOpticalFlowFeaturesNV> ||
#endif
#ifdef VK_EXT_device_fault
        std::is_same_v<T, VkPhysicalDeviceFaultFeaturesEXT> ||
#endif
#ifdef VK_EXT_pipeline_library_group_handles
        std::is_same_v<T, VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT> ||
#endif
#ifdef VK_EXT_shader_object
        std::is_same_v<T, VkPhysicalDeviceShaderObjectFeaturesEXT> ||
#endif
#ifdef VK_ARM_shader_core_builtins
        std::is_same_v<T, VkPhysicalDeviceShaderCoreBuiltinsFeaturesARM> ||
#endif
#ifdef VK_EXT_swapchain_maintenance1
        std::is_same_v<T, VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT> ||
#endif
#ifdef VK_NV_ray_tracing_invocation_reorder
        std::is_same_v<T, VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV> ||
#endif
#ifdef VK_QCOM_multiview_per_view_viewports
        std::is_same_v<T, VkPhysicalDeviceMultiviewPerViewViewportsFeaturesQCOM> ||
#endif
#ifdef VK_QCOM_multiview_per_view_render_areas
        std::is_same_v<T, VkPhysicalDeviceMultiviewPerViewRenderAreasFeaturesQCOM> ||
#endif
#ifdef VK_HUAWEI_cluster_culling_shader
        std::is_same_v<T, VkPhysicalDeviceClusterCullingShaderFeaturesHUAWEI>;
#endif
        
    class DeviceFeatureChainNode 
    {
        template <typename F>
        friend constexpr auto visitVulkanFeature(F&& f, DeviceFeatureChainNode const& deviceFeatureChainElement) -> decltype(f(deviceFeatureChainElement));

        // if this macro doesn't work then use template specialization
        #define CONCAT_IMPL(a, b) a##b
        #define CONCAT(a, b) CONCAT_IMPL(a, b)

    public:
        constexpr DeviceFeatureChainNode() : featureStruct(), discriminant(Discriminant_t::Nothing) 
        {
            std::construct_at(&featureStruct.m_empty, Empty());
        }
        
        template <FeatureType T>
        constexpr explicit DeviceFeatureChainNode(T const& v) : DeviceFeatureChainNode()
        {        
			if constexpr(std::is_same_v<T, VkPhysicalDeviceVulkan11Features>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceVulkan11Features), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceVulkan11Features;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceVulkan12Features>)
			{------------------
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceVulkan12Features), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceVulkan12Features;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceVulkan13Features>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceVulkan13Features), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceVulkan13Features;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceVariablePointersFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceVariablePointersFeatures), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceVariablePointersFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceMultiviewFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceMultiviewFeatures), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceMultiviewFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceShaderAtomicInt64Features>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceShaderAtomicInt64Features), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceShaderAtomicInt64Features;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDevice8BitStorageFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDevice8BitStorageFeatures), v);
            	discriminant = Discriminant_t::VkPhysicalDevice8BitStorageFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDevice16BitStorageFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDevice16BitStorageFeatures), v);
            	discriminant = Discriminant_t::VkPhysicalDevice16BitStorageFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceShaderFloat16Int8Features>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceShaderFloat16Int8Features), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceShaderFloat16Int8Features;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceSamplerYcbcrConversionFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceSamplerYcbcrConversionFeatures), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceSamplerYcbcrConversionFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceProtectedMemoryFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceProtectedMemoryFeatures), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceProtectedMemoryFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceShaderDrawParametersFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceShaderDrawParametersFeatures), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceShaderDrawParametersFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceDescriptorIndexingFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceDescriptorIndexingFeatures), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceDescriptorIndexingFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceVulkanMemoryModelFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceVulkanMemoryModelFeatures), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceVulkanMemoryModelFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceInlineUniformBlockFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceInlineUniformBlockFeatures), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceInlineUniformBlockFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceScalarBlockLayoutFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceScalarBlockLayoutFeatures), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceScalarBlockLayoutFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceUniformBufferStandardLayoutFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceUniformBufferStandardLayoutFeatures), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceUniformBufferStandardLayoutFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceBufferDeviceAddressFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceBufferDeviceAddressFeatures), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceBufferDeviceAddressFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceImagelessFramebufferFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceImagelessFramebufferFeatures), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceImagelessFramebufferFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceHostQueryResetFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceHostQueryResetFeatures), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceHostQueryResetFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceTimelineSemaphoreFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceTimelineSemaphoreFeatures), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceTimelineSemaphoreFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceTextureCompressionASTCHDRFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceTextureCompressionASTCHDRFeatures), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceTextureCompressionASTCHDRFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceSubgroupSizeControlFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceSubgroupSizeControlFeatures), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceSubgroupSizeControlFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDevicePrivateDataFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDevicePrivateDataFeatures), v);
            	discriminant = Discriminant_t::VkPhysicalDevicePrivateDataFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceImageRobustnessFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceImageRobustnessFeatures), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceImageRobustnessFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceShaderTerminateInvocationFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceShaderTerminateInvocationFeatures), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceShaderTerminateInvocationFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceSynchronization2Features>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceSynchronization2Features), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceSynchronization2Features;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceShaderIntegerDotProductFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceShaderIntegerDotProductFeatures), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceShaderIntegerDotProductFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceMaintenance4Features>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceMaintenance4Features), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceMaintenance4Features;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceDynamicRenderingFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceDynamicRenderingFeatures), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceDynamicRenderingFeatures;
			}
#ifdef VK_EXT_rgba10x6_formats
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT;
			}
#endif
#ifdef VK_EXT_pipeline_robustness
        	if constexpr(std::is_same_v<T, VkPhysicalDevicePipelineRobustnessFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDevicePipelineRobustnessFeaturesEXT), v);
            	discriminant = Discriminant_t::VkPhysicalDevicePipelineRobustnessFeaturesEXT;
			}
#endif
#ifdef VK_EXT_image_view_min_lod
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceImageViewMinLodFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceImageViewMinLodFeaturesEXT), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceImageViewMinLodFeaturesEXT;
			}
#endif
#ifdef VK_EXT_rasterization_order_attachment_access
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT;
			}
#endif
#ifdef VK_EXT_subpass_merge_feedback
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT;
			}
#endif
#ifdef VK_NV_linear_color_attachment
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceLinearColorAttachmentFeaturesNV>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceLinearColorAttachmentFeaturesNV), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceLinearColorAttachmentFeaturesNV;
			}
#endif
#ifdef VK_EXT_attachment_feedback_loop_layout
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT;
			}
#endif
#ifdef VK_EXT_graphics_pipeline_library
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT;
			}
#endif
#ifdef VK_EXT_multisampled_render_to_single_sampled
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT;
			}
#endif
#ifdef VK_EXT_image_2d_view_of_3d
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceImage2DViewOf3DFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceImage2DViewOf3DFeaturesEXT), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceImage2DViewOf3DFeaturesEXT;
			}
#endif
#ifdef VK_EXT_image_sliced_view_of_3d
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceImageSlicedViewOf3DFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceImageSlicedViewOf3DFeaturesEXT), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceImageSlicedViewOf3DFeaturesEXT;
			}
#endif
#ifdef VK_EXT_image_compression_control_swapchain
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceImageCompressionControlFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceImageCompressionControlFeaturesEXT), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceImageCompressionControlFeaturesEXT;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT;
			}
#endif
#ifdef VK_AMD_shader_early_and_late_fragment_tests
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceShaderEarlyAndLateFragmentTestsFeaturesAMD>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceShaderEarlyAndLateFragmentTestsFeaturesAMD), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceShaderEarlyAndLateFragmentTestsFeaturesAMD;
			}
#endif
#ifdef VK_EXT_non_seamless_cube_map
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT;
			}
#endif
#ifdef VK_EXT_shader_module_identifier
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT;
			}
#endif
#ifdef VK_QCOM_tile_properties
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceTilePropertiesFeaturesQCOM>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceTilePropertiesFeaturesQCOM), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceTilePropertiesFeaturesQCOM;
			}
#endif
#ifdef VK_QCOM_image_processing
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceImageProcessingFeaturesQCOM>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceImageProcessingFeaturesQCOM), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceImageProcessingFeaturesQCOM;
			}
#endif
#ifdef VK_EXT_depth_clamp_zero_one
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceDepthClampZeroOneFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceDepthClampZeroOneFeaturesEXT), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceDepthClampZeroOneFeaturesEXT;
			}
#endif
#ifdef VK_EXT_shader_tile_image
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceShaderTileImageFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceShaderTileImageFeaturesEXT), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceShaderTileImageFeaturesEXT;
			}
#endif
#ifdef VK_EXT_device_address_binding_report
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceAddressBindingReportFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceAddressBindingReportFeaturesEXT), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceAddressBindingReportFeaturesEXT;
			}
#endif
#ifdef VK_NV_optical_flow
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceOpticalFlowFeaturesNV>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceOpticalFlowFeaturesNV), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceOpticalFlowFeaturesNV;
			}
#endif
#ifdef VK_EXT_device_fault
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceFaultFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceFaultFeaturesEXT), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceFaultFeaturesEXT;
			}
#endif
#ifdef VK_EXT_pipeline_library_group_handles
        	if constexpr(std::is_same_v<T, VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT), v);
            	discriminant = Discriminant_t::VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT;
			}
#endif
#ifdef VK_EXT_shader_object
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceShaderObjectFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceShaderObjectFeaturesEXT), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceShaderObjectFeaturesEXT;
			}
#endif
#ifdef VK_ARM_shader_core_builtins
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceShaderCoreBuiltinsFeaturesARM>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceShaderCoreBuiltinsFeaturesARM), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceShaderCoreBuiltinsFeaturesARM;
			}
#endif
#ifdef VK_EXT_swapchain_maintenance1
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT;
			}
#endif
#ifdef VK_NV_ray_tracing_invocation_reorder
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV;
			}
#endif
#ifdef VK_QCOM_multiview_per_view_viewports
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceMultiviewPerViewViewportsFeaturesQCOM>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceMultiviewPerViewViewportsFeaturesQCOM), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceMultiviewPerViewViewportsFeaturesQCOM;
			}
#endif
#ifdef VK_QCOM_multiview_per_view_render_areas
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceMultiviewPerViewRenderAreasFeaturesQCOM>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceMultiviewPerViewRenderAreasFeaturesQCOM), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceMultiviewPerViewRenderAreasFeaturesQCOM;
			}
#endif
#ifdef VK_HUAWEI_cluster_culling_shader
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceClusterCullingShaderFeaturesHUAWEI>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceClusterCullingShaderFeaturesHUAWEI), v);
            	discriminant = Discriminant_t::VkPhysicalDeviceClusterCullingShaderFeaturesHUAWEI;
			}
#endif
        }

		// I am aware that this gains me nothing, done just to refresh on std::exchange
        template <FeatureType T>
        constexpr explicit DeviceFeatureChainNode(T&& v) : DeviceFeatureChainNode()
        {
			if constexpr(std::is_same_v<T, VkPhysicalDeviceVulkan11Features>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceVulkan11Features), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceVulkan11Features;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceVulkan12Features>)
			{------------------
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceVulkan12Features), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceVulkan12Features;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceVulkan13Features>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceVulkan13Features), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceVulkan13Features;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceVariablePointersFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceVariablePointersFeatures), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceVariablePointersFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceMultiviewFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceMultiviewFeatures), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceMultiviewFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceShaderAtomicInt64Features>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceShaderAtomicInt64Features), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceShaderAtomicInt64Features;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDevice8BitStorageFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDevice8BitStorageFeatures), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDevice8BitStorageFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDevice16BitStorageFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDevice16BitStorageFeatures), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDevice16BitStorageFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceShaderFloat16Int8Features>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceShaderFloat16Int8Features), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceShaderFloat16Int8Features;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceSamplerYcbcrConversionFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceSamplerYcbcrConversionFeatures), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceSamplerYcbcrConversionFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceProtectedMemoryFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceProtectedMemoryFeatures), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceProtectedMemoryFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceShaderDrawParametersFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceShaderDrawParametersFeatures), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceShaderDrawParametersFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceDescriptorIndexingFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceDescriptorIndexingFeatures), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceDescriptorIndexingFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceVulkanMemoryModelFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceVulkanMemoryModelFeatures), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceVulkanMemoryModelFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceInlineUniformBlockFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceInlineUniformBlockFeatures), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceInlineUniformBlockFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceScalarBlockLayoutFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceScalarBlockLayoutFeatures), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceScalarBlockLayoutFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceUniformBufferStandardLayoutFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceUniformBufferStandardLayoutFeatures), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceUniformBufferStandardLayoutFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceBufferDeviceAddressFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceBufferDeviceAddressFeatures), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceBufferDeviceAddressFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceImagelessFramebufferFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceImagelessFramebufferFeatures), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceImagelessFramebufferFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceHostQueryResetFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceHostQueryResetFeatures), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceHostQueryResetFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceTimelineSemaphoreFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceTimelineSemaphoreFeatures), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceTimelineSemaphoreFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceTextureCompressionASTCHDRFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceTextureCompressionASTCHDRFeatures), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceTextureCompressionASTCHDRFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceSubgroupSizeControlFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceSubgroupSizeControlFeatures), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceSubgroupSizeControlFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDevicePrivateDataFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDevicePrivateDataFeatures), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDevicePrivateDataFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceImageRobustnessFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceImageRobustnessFeatures), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceImageRobustnessFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceShaderTerminateInvocationFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceShaderTerminateInvocationFeatures), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceShaderTerminateInvocationFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceSynchronization2Features>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceSynchronization2Features), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceSynchronization2Features;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceShaderIntegerDotProductFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceShaderIntegerDotProductFeatures), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceShaderIntegerDotProductFeatures;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceMaintenance4Features>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceMaintenance4Features), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceMaintenance4Features;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceDynamicRenderingFeatures>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceDynamicRenderingFeatures), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceDynamicRenderingFeatures;
			}
#ifdef VK_EXT_rgba10x6_formats
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT;
			}
#endif
#ifdef VK_EXT_pipeline_robustness
        	if constexpr(std::is_same_v<T, VkPhysicalDevicePipelineRobustnessFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDevicePipelineRobustnessFeaturesEXT), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDevicePipelineRobustnessFeaturesEXT;
			}
#endif
#ifdef VK_EXT_image_view_min_lod
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceImageViewMinLodFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceImageViewMinLodFeaturesEXT), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceImageViewMinLodFeaturesEXT;
			}
#endif
#ifdef VK_EXT_rasterization_order_attachment_access
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT;
			}
#endif
#ifdef VK_EXT_subpass_merge_feedback
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT;
			}
#endif
#ifdef VK_NV_linear_color_attachment
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceLinearColorAttachmentFeaturesNV>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceLinearColorAttachmentFeaturesNV), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceLinearColorAttachmentFeaturesNV;
			}
#endif
#ifdef VK_EXT_attachment_feedback_loop_layout
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT;
			}
#endif
#ifdef VK_EXT_graphics_pipeline_library
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT;
			}
#endif
#ifdef VK_EXT_multisampled_render_to_single_sampled
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT;
			}
#endif
#ifdef VK_EXT_image_2d_view_of_3d
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceImage2DViewOf3DFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceImage2DViewOf3DFeaturesEXT), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceImage2DViewOf3DFeaturesEXT;
			}
#endif
#ifdef VK_EXT_image_sliced_view_of_3d
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceImageSlicedViewOf3DFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceImageSlicedViewOf3DFeaturesEXT), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceImageSlicedViewOf3DFeaturesEXT;
			}
#endif
#ifdef VK_EXT_image_compression_control_swapchain
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceImageCompressionControlFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceImageCompressionControlFeaturesEXT), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceImageCompressionControlFeaturesEXT;
			}
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT;
			}
#endif
#ifdef VK_AMD_shader_early_and_late_fragment_tests
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceShaderEarlyAndLateFragmentTestsFeaturesAMD>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceShaderEarlyAndLateFragmentTestsFeaturesAMD), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceShaderEarlyAndLateFragmentTestsFeaturesAMD;
			}
#endif
#ifdef VK_EXT_non_seamless_cube_map
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT;
			}
#endif
#ifdef VK_EXT_shader_module_identifier
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT;
			}
#endif
#ifdef VK_QCOM_tile_properties
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceTilePropertiesFeaturesQCOM>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceTilePropertiesFeaturesQCOM), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceTilePropertiesFeaturesQCOM;
			}
#endif
#ifdef VK_QCOM_image_processing
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceImageProcessingFeaturesQCOM>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceImageProcessingFeaturesQCOM), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceImageProcessingFeaturesQCOM;
			}
#endif
#ifdef VK_EXT_depth_clamp_zero_one
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceDepthClampZeroOneFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceDepthClampZeroOneFeaturesEXT), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceDepthClampZeroOneFeaturesEXT;
			}
#endif
#ifdef VK_EXT_shader_tile_image
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceShaderTileImageFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceShaderTileImageFeaturesEXT), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceShaderTileImageFeaturesEXT;
			}
#endif
#ifdef VK_EXT_device_address_binding_report
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceAddressBindingReportFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceAddressBindingReportFeaturesEXT), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceAddressBindingReportFeaturesEXT;
			}
#endif
#ifdef VK_NV_optical_flow
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceOpticalFlowFeaturesNV>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceOpticalFlowFeaturesNV), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceOpticalFlowFeaturesNV;
			}
#endif
#ifdef VK_EXT_device_fault
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceFaultFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceFaultFeaturesEXT), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceFaultFeaturesEXT;
			}
#endif
#ifdef VK_EXT_pipeline_library_group_handles
        	if constexpr(std::is_same_v<T, VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT;
			}
#endif
#ifdef VK_EXT_shader_object
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceShaderObjectFeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceShaderObjectFeaturesEXT), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceShaderObjectFeaturesEXT;
			}
#endif
#ifdef VK_ARM_shader_core_builtins
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceShaderCoreBuiltinsFeaturesARM>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceShaderCoreBuiltinsFeaturesARM), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceShaderCoreBuiltinsFeaturesARM;
			}
#endif
#ifdef VK_EXT_swapchain_maintenance1
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT;
			}
#endif
#ifdef VK_NV_ray_tracing_invocation_reorder
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV;
			}
#endif
#ifdef VK_QCOM_multiview_per_view_viewports
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceMultiviewPerViewViewportsFeaturesQCOM>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceMultiviewPerViewViewportsFeaturesQCOM), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceMultiviewPerViewViewportsFeaturesQCOM;
			}
#endif
#ifdef VK_QCOM_multiview_per_view_render_areas
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceMultiviewPerViewRenderAreasFeaturesQCOM>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceMultiviewPerViewRenderAreasFeaturesQCOM), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceMultiviewPerViewRenderAreasFeaturesQCOM;
			}
#endif
#ifdef VK_HUAWEI_cluster_culling_shader
        	if constexpr(std::is_same_v<T, VkPhysicalDeviceClusterCullingShaderFeaturesHUAWEI>)
			{
            	std::construct_at(&featureStruct.CONCAT(m_, VkPhysicalDeviceClusterCullingShaderFeaturesHUAWEI), std::exchange(v, {}));
            	discriminant = Discriminant_t::VkPhysicalDeviceClusterCullingShaderFeaturesHUAWEI;
			}
#endif
        }

        explicit DeviceFeatureChainNode(SlicedVkStruct const* vkStruct) : DeviceFeatureChainNode()
        {
            switch (vkStruct->sType)
            {
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceVulkan11Features, *reinterpret_cast<VkPhysicalDeviceVulkan11Features const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceVulkan11Features; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceVulkan12Features, *reinterpret_cast<VkPhysicalDeviceVulkan12Features const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceVulkan12Features; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceVulkan13Features, *reinterpret_cast<VkPhysicalDeviceVulkan13Features const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceVulkan13Features; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTERS_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceVariablePointersFeatures, *reinterpret_cast<VkPhysicalDeviceVariablePointersFeatures const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceVariablePointersFeatures; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceMultiviewFeatures, *reinterpret_cast<VkPhysicalDeviceMultiviewFeatures const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceMultiviewFeatures; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceShaderAtomicInt64Features, *reinterpret_cast<VkPhysicalDeviceShaderAtomicInt64Features const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceShaderAtomicInt64Features; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDevice8BitStorageFeatures, *reinterpret_cast<VkPhysicalDevice8BitStorageFeatures const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDevice8BitStorageFeatures; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDevice16BitStorageFeatures, *reinterpret_cast<VkPhysicalDevice16BitStorageFeatures const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDevice16BitStorageFeatures; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceShaderFloat16Int8Features, *reinterpret_cast<VkPhysicalDeviceShaderFloat16Int8Features const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceShaderFloat16Int8Features; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceSamplerYcbcrConversionFeatures, *reinterpret_cast<VkPhysicalDeviceSamplerYcbcrConversionFeatures const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceSamplerYcbcrConversionFeatures; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceProtectedMemoryFeatures, *reinterpret_cast<VkPhysicalDeviceProtectedMemoryFeatures const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceProtectedMemoryFeatures; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceShaderDrawParametersFeatures, *reinterpret_cast<VkPhysicalDeviceShaderDrawParametersFeatures const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceShaderDrawParametersFeatures; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceDescriptorIndexingFeatures, *reinterpret_cast<VkPhysicalDeviceDescriptorIndexingFeatures const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceDescriptorIndexingFeatures; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceVulkanMemoryModelFeatures, *reinterpret_cast<VkPhysicalDeviceVulkanMemoryModelFeatures const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceVulkanMemoryModelFeatures; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceInlineUniformBlockFeatures, *reinterpret_cast<VkPhysicalDeviceInlineUniformBlockFeatures const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceInlineUniformBlockFeatures; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceScalarBlockLayoutFeatures, *reinterpret_cast<VkPhysicalDeviceScalarBlockLayoutFeatures const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceScalarBlockLayoutFeatures; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFORM_BUFFER_STANDARD_LAYOUT_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceUniformBufferStandardLayoutFeatures, *reinterpret_cast<VkPhysicalDeviceUniformBufferStandardLayoutFeatures const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceUniformBufferStandardLayoutFeatures; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceBufferDeviceAddressFeatures, *reinterpret_cast<VkPhysicalDeviceBufferDeviceAddressFeatures const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceBufferDeviceAddressFeatures; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceImagelessFramebufferFeatures, *reinterpret_cast<VkPhysicalDeviceImagelessFramebufferFeatures const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceImagelessFramebufferFeatures; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures, *reinterpret_cast<VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceHostQueryResetFeatures, *reinterpret_cast<VkPhysicalDeviceHostQueryResetFeatures const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceHostQueryResetFeatures; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceTimelineSemaphoreFeatures, *reinterpret_cast<VkPhysicalDeviceTimelineSemaphoreFeatures const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceTimelineSemaphoreFeatures; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures, *reinterpret_cast<VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures, *reinterpret_cast<VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXTURE_COMPRESSION_ASTC_HDR_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceTextureCompressionASTCHDRFeatures, *reinterpret_cast<VkPhysicalDeviceTextureCompressionASTCHDRFeatures const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceTextureCompressionASTCHDRFeatures; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceSubgroupSizeControlFeatures, *reinterpret_cast<VkPhysicalDeviceSubgroupSizeControlFeatures const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceSubgroupSizeControlFeatures; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ZERO_INITIALIZE_WORKGROUP_MEMORY_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures, *reinterpret_cast<VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIVATE_DATA_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDevicePrivateDataFeatures, *reinterpret_cast<VkPhysicalDevicePrivateDataFeatures const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDevicePrivateDataFeatures; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_ROBUSTNESS_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceImageRobustnessFeatures, *reinterpret_cast<VkPhysicalDeviceImageRobustnessFeatures const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceImageRobustnessFeatures; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_TERMINATE_INVOCATION_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceShaderTerminateInvocationFeatures, *reinterpret_cast<VkPhysicalDeviceShaderTerminateInvocationFeatures const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceShaderTerminateInvocationFeatures; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceSynchronization2Features, *reinterpret_cast<VkPhysicalDeviceSynchronization2Features const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceSynchronization2Features; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_DOT_PRODUCT_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceShaderIntegerDotProductFeatures, *reinterpret_cast<VkPhysicalDeviceShaderIntegerDotProductFeatures const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceShaderIntegerDotProductFeatures; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceMaintenance4Features, *reinterpret_cast<VkPhysicalDeviceMaintenance4Features const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceMaintenance4Features; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES: std::construct_at(&featureStruct.m_VkPhysicalDeviceDynamicRenderingFeatures, *reinterpret_cast<VkPhysicalDeviceDynamicRenderingFeatures const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceDynamicRenderingFeatures; break;
#ifdef VK_EXT_rgba10x6_formats
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RGBA10X6_FORMATS_FEATURES_EXT: std::construct_at(&featureStruct.m_VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT, *reinterpret_cast<VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT; break;
#endif
#ifdef VK_EXT_pipeline_robustness
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_ROBUSTNESS_FEATURES_EXT: std::construct_at(&featureStruct.m_VkPhysicalDevicePipelineRobustnessFeaturesEXT, *reinterpret_cast<VkPhysicalDevicePipelineRobustnessFeaturesEXT const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDevicePipelineRobustnessFeaturesEXT; break;
#endif
#ifdef VK_EXT_image_view_min_lod
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_VIEW_MIN_LOD_FEATURES_EXT: std::construct_at(&featureStruct.m_VkPhysicalDeviceImageViewMinLodFeaturesEXT, *reinterpret_cast<VkPhysicalDeviceImageViewMinLodFeaturesEXT const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceImageViewMinLodFeaturesEXT; break;
#endif
#ifdef VK_EXT_rasterization_order_attachment_access
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RASTERIZATION_ORDER_ATTACHMENT_ACCESS_FEATURES_EXT: std::construct_at(&featureStruct.m_VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT, *reinterpret_cast<VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT; break;
#endif
#ifdef VK_EXT_subpass_merge_feedback
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBPASS_MERGE_FEEDBACK_FEATURES_EXT: std::construct_at(&featureStruct.m_VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT, *reinterpret_cast<VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT; break;
#endif
#ifdef VK_NV_linear_color_attachment
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINEAR_COLOR_ATTACHMENT_FEATURES_NV: std::construct_at(&featureStruct.m_VkPhysicalDeviceLinearColorAttachmentFeaturesNV, *reinterpret_cast<VkPhysicalDeviceLinearColorAttachmentFeaturesNV const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceLinearColorAttachmentFeaturesNV; break;
#endif
#ifdef VK_EXT_attachment_feedback_loop_layout
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ATTACHMENT_FEEDBACK_LOOP_LAYOUT_FEATURES_EXT: std::construct_at(&featureStruct.m_VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT, *reinterpret_cast<VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT; break;
#endif
#ifdef VK_EXT_graphics_pipeline_library
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_FEATURES_EXT: std::construct_at(&featureStruct.m_VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT, *reinterpret_cast<VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT; break;
#endif
#ifdef VK_EXT_multisampled_render_to_single_sampled
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTISAMPLED_RENDER_TO_SINGLE_SAMPLED_FEATURES_EXT: std::construct_at(&featureStruct.m_VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT, *reinterpret_cast<VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT; break;
#endif
#ifdef VK_EXT_image_2d_view_of_3d
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_2D_VIEW_OF_3D_FEATURES_EXT: std::construct_at(&featureStruct.m_VkPhysicalDeviceImage2DViewOf3DFeaturesEXT, *reinterpret_cast<VkPhysicalDeviceImage2DViewOf3DFeaturesEXT const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceImage2DViewOf3DFeaturesEXT; break;
#endif
#ifdef VK_EXT_image_sliced_view_of_3d
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_SLICED_VIEW_OF_3D_FEATURES_EXT: std::construct_at(&featureStruct.m_VkPhysicalDeviceImageSlicedViewOf3DFeaturesEXT, *reinterpret_cast<VkPhysicalDeviceImageSlicedViewOf3DFeaturesEXT const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceImageSlicedViewOf3DFeaturesEXT; break;
#endif
#ifdef VK_EXT_image_compression_control_swapchain
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_COMPRESSION_CONTROL_FEATURES_EXT: std::construct_at(&featureStruct.m_VkPhysicalDeviceImageCompressionControlFeaturesEXT, *reinterpret_cast<VkPhysicalDeviceImageCompressionControlFeaturesEXT const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceImageCompressionControlFeaturesEXT; break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_COMPRESSION_CONTROL_SWAPCHAIN_FEATURES_EXT: std::construct_at(&featureStruct.m_VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT, *reinterpret_cast<VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT; break;
#endif
#ifdef VK_AMD_shader_early_and_late_fragment_tests
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_EARLY_AND_LATE_FRAGMENT_TESTS_FEATURES_AMD: std::construct_at(&featureStruct.m_VkPhysicalDeviceShaderEarlyAndLateFragmentTestsFeaturesAMD, *reinterpret_cast<VkPhysicalDeviceShaderEarlyAndLateFragmentTestsFeaturesAMD const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceShaderEarlyAndLateFragmentTestsFeaturesAMD; break;
#endif
#ifdef VK_EXT_non_seamless_cube_map
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_NON_SEAMLESS_CUBE_MAP_FEATURES_EXT: std::construct_at(&featureStruct.m_VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT, *reinterpret_cast<VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT; break;
#endif
#ifdef VK_EXT_shader_module_identifier
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_MODULE_IDENTIFIER_FEATURES_EXT: std::construct_at(&featureStruct.m_VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT, *reinterpret_cast<VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT; break;
#endif
#ifdef VK_QCOM_tile_properties
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TILE_PROPERTIES_FEATURES_QCOM: std::construct_at(&featureStruct.m_VkPhysicalDeviceTilePropertiesFeaturesQCOM, *reinterpret_cast<VkPhysicalDeviceTilePropertiesFeaturesQCOM const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceTilePropertiesFeaturesQCOM; break;
#endif
#ifdef VK_QCOM_image_processing
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_PROCESSING_FEATURES_QCOM: std::construct_at(&featureStruct.m_VkPhysicalDeviceImageProcessingFeaturesQCOM, *reinterpret_cast<VkPhysicalDeviceImageProcessingFeaturesQCOM const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceImageProcessingFeaturesQCOM; break;
#endif
#ifdef VK_EXT_depth_clamp_zero_one
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLAMP_ZERO_ONE_FEATURES_EXT: std::construct_at(&featureStruct.m_VkPhysicalDeviceDepthClampZeroOneFeaturesEXT, *reinterpret_cast<VkPhysicalDeviceDepthClampZeroOneFeaturesEXT const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceDepthClampZeroOneFeaturesEXT; break;
#endif
#ifdef VK_EXT_shader_tile_image
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_TILE_IMAGE_FEATURES_EXT: std::construct_at(&featureStruct.m_VkPhysicalDeviceShaderTileImageFeaturesEXT, *reinterpret_cast<VkPhysicalDeviceShaderTileImageFeaturesEXT const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceShaderTileImageFeaturesEXT; break;
#endif
#ifdef VK_EXT_device_address_binding_report
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ADDRESS_BINDING_REPORT_FEATURES_EXT: std::construct_at(&featureStruct.m_VkPhysicalDeviceAddressBindingReportFeaturesEXT, *reinterpret_cast<VkPhysicalDeviceAddressBindingReportFeaturesEXT const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceAddressBindingReportFeaturesEXT; break;
#endif
#ifdef VK_NV_optical_flow
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_OPTICAL_FLOW_FEATURES_NV: std::construct_at(&featureStruct.m_VkPhysicalDeviceOpticalFlowFeaturesNV, *reinterpret_cast<VkPhysicalDeviceOpticalFlowFeaturesNV const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceOpticalFlowFeaturesNV; break;
#endif
#ifdef VK_EXT_device_fault
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT: std::construct_at(&featureStruct.m_VkPhysicalDeviceFaultFeaturesEXT, *reinterpret_cast<VkPhysicalDeviceFaultFeaturesEXT const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceFaultFeaturesEXT; break;
#endif
#ifdef VK_EXT_pipeline_library_group_handles
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_LIBRARY_GROUP_HANDLES_FEATURES_EXT: std::construct_at(&featureStruct.m_VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT, *reinterpret_cast<VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT; break;
#endif
#ifdef VK_EXT_shader_object
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT: std::construct_at(&featureStruct.m_VkPhysicalDeviceShaderObjectFeaturesEXT, *reinterpret_cast<VkPhysicalDeviceShaderObjectFeaturesEXT const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceShaderObjectFeaturesEXT; break;
#endif
#ifdef VK_ARM_shader_core_builtins
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_BUILTINS_FEATURES_ARM: std::construct_at(&featureStruct.m_VkPhysicalDeviceShaderCoreBuiltinsFeaturesARM, *reinterpret_cast<VkPhysicalDeviceShaderCoreBuiltinsFeaturesARM const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceShaderCoreBuiltinsFeaturesARM; break;
#endif
#ifdef VK_EXT_swapchain_maintenance1
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT: std::construct_at(&featureStruct.m_VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT, *reinterpret_cast<VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT; break;
#endif
#ifdef VK_NV_ray_tracing_invocation_reorder
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_INVOCATION_REORDER_FEATURES_NV: std::construct_at(&featureStruct.m_VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV, *reinterpret_cast<VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV; break;
#endif
#ifdef VK_QCOM_multiview_per_view_viewports
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PER_VIEW_VIEWPORTS_FEATURES_QCOM: std::construct_at(&featureStruct.m_VkPhysicalDeviceMultiviewPerViewViewportsFeaturesQCOM, *reinterpret_cast<VkPhysicalDeviceMultiviewPerViewViewportsFeaturesQCOM const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceMultiviewPerViewViewportsFeaturesQCOM; break;
#endif
#ifdef VK_QCOM_multiview_per_view_render_areas
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PER_VIEW_RENDER_AREAS_FEATURES_QCOM: std::construct_at(&featureStruct.m_VkPhysicalDeviceMultiviewPerViewRenderAreasFeaturesQCOM, *reinterpret_cast<VkPhysicalDeviceMultiviewPerViewRenderAreasFeaturesQCOM const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceMultiviewPerViewRenderAreasFeaturesQCOM; break;
#endif
#ifdef VK_HUAWEI_cluster_culling_shader
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CLUSTER_CULLING_SHADER_FEATURES_HUAWEI: std::construct_at(&featureStruct.m_VkPhysicalDeviceClusterCullingShaderFeaturesHUAWEI, *reinterpret_cast<VkPhysicalDeviceClusterCullingShaderFeaturesHUAWEI const*>(vkStruct)); discriminant = Discriminant_t::VkPhysicalDeviceClusterCullingShaderFeaturesHUAWEI; break;
#endif
                default: 
                    // TODO insert log here
                    std::abort();
                    break;
            }
        }
        
    private:
        struct Empty {};
        union U 
        {
            constexpr U() {}
            constexpr ~U() {}
            
            Empty m_empty;
            VkPhysicalDeviceVulkan11Features m_VkPhysicalDeviceVulkan11Features; // from VK_VERSION_1_1
            VkPhysicalDeviceVulkan12Features m_VkPhysicalDeviceVulkan12Features; // from VK_VERSION_1_2
            VkPhysicalDeviceVulkan13Features m_VkPhysicalDeviceVulkan13Features; // from VK_VERSION_1_3
            VkPhysicalDeviceVariablePointersFeatures m_VkPhysicalDeviceVariablePointersFeatures; // from VK_VERSION_1_1
            VkPhysicalDeviceMultiviewFeatures m_VkPhysicalDeviceMultiviewFeatures; // from VK_VERSION_1_1
            VkPhysicalDeviceShaderAtomicInt64Features m_VkPhysicalDeviceShaderAtomicInt64Features; // from VK_VERSION_1_2
            VkPhysicalDevice8BitStorageFeatures m_VkPhysicalDevice8BitStorageFeatures; // from VK_VERSION_1_2
            VkPhysicalDevice16BitStorageFeatures m_VkPhysicalDevice16BitStorageFeatures; // from VK_VERSION_1_1
            VkPhysicalDeviceShaderFloat16Int8Features m_VkPhysicalDeviceShaderFloat16Int8Features; // from VK_VERSION_1_2
            VkPhysicalDeviceSamplerYcbcrConversionFeatures m_VkPhysicalDeviceSamplerYcbcrConversionFeatures; // from VK_VERSION_1_1
            VkPhysicalDeviceProtectedMemoryFeatures m_VkPhysicalDeviceProtectedMemoryFeatures; // from VK_VERSION_1_1
            VkPhysicalDeviceShaderDrawParametersFeatures m_VkPhysicalDeviceShaderDrawParametersFeatures; // from VK_VERSION_1_1
            VkPhysicalDeviceDescriptorIndexingFeatures m_VkPhysicalDeviceDescriptorIndexingFeatures; // from VK_VERSION_1_2
            VkPhysicalDeviceVulkanMemoryModelFeatures m_VkPhysicalDeviceVulkanMemoryModelFeatures; // from VK_VERSION_1_2
            VkPhysicalDeviceInlineUniformBlockFeatures m_VkPhysicalDeviceInlineUniformBlockFeatures; // from VK_VERSION_1_3
            VkPhysicalDeviceScalarBlockLayoutFeatures m_VkPhysicalDeviceScalarBlockLayoutFeatures; // from VK_VERSION_1_2
            VkPhysicalDeviceUniformBufferStandardLayoutFeatures m_VkPhysicalDeviceUniformBufferStandardLayoutFeatures; // from VK_VERSION_1_2
            VkPhysicalDeviceBufferDeviceAddressFeatures m_VkPhysicalDeviceBufferDeviceAddressFeatures; // from VK_VERSION_1_2
            VkPhysicalDeviceImagelessFramebufferFeatures m_VkPhysicalDeviceImagelessFramebufferFeatures; // from VK_VERSION_1_2
            VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures m_VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures; // from VK_VERSION_1_2
            VkPhysicalDeviceHostQueryResetFeatures m_VkPhysicalDeviceHostQueryResetFeatures; // from VK_VERSION_1_2
            VkPhysicalDeviceTimelineSemaphoreFeatures m_VkPhysicalDeviceTimelineSemaphoreFeatures; // from VK_VERSION_1_2
            VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures m_VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures; // from VK_VERSION_1_2
            VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures m_VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures; // from VK_VERSION_1_3
            VkPhysicalDeviceTextureCompressionASTCHDRFeatures m_VkPhysicalDeviceTextureCompressionASTCHDRFeatures; // from VK_VERSION_1_3
            VkPhysicalDeviceSubgroupSizeControlFeatures m_VkPhysicalDeviceSubgroupSizeControlFeatures; // from VK_VERSION_1_3
            VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures m_VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures; // from VK_VERSION_1_3
            VkPhysicalDevicePrivateDataFeatures m_VkPhysicalDevicePrivateDataFeatures; // from VK_VERSION_1_3
            VkPhysicalDeviceImageRobustnessFeatures m_VkPhysicalDeviceImageRobustnessFeatures; // from VK_VERSION_1_3
            VkPhysicalDeviceShaderTerminateInvocationFeatures m_VkPhysicalDeviceShaderTerminateInvocationFeatures; // from VK_VERSION_1_3
            VkPhysicalDeviceSynchronization2Features m_VkPhysicalDeviceSynchronization2Features; // from VK_VERSION_1_3
            VkPhysicalDeviceShaderIntegerDotProductFeatures m_VkPhysicalDeviceShaderIntegerDotProductFeatures; // from VK_VERSION_1_3
            VkPhysicalDeviceMaintenance4Features m_VkPhysicalDeviceMaintenance4Features; // from VK_VERSION_1_3
            VkPhysicalDeviceDynamicRenderingFeatures m_VkPhysicalDeviceDynamicRenderingFeatures; // from VK_VERSION_1_3 
#ifdef VK_EXT_rgba10x6_formats
            VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT m_VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT; // from VK_EXT_rgba10x6_formats
#endif
#ifdef VK_EXT_pipeline_robustness
            VkPhysicalDevicePipelineRobustnessFeaturesEXT m_VkPhysicalDevicePipelineRobustnessFeaturesEXT; // from VK_EXT_pipeline_robustness 
#endif
#ifdef VK_EXT_image_view_min_lod
            VkPhysicalDeviceImageViewMinLodFeaturesEXT m_VkPhysicalDeviceImageViewMinLodFeaturesEXT; // from VK_EXT_image_view_min_lod
#endif
#ifdef VK_EXT_rasterization_order_attachment_access
            VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT m_VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT; // from VK_EXT_rasterization_order_attachment_access
#endif
#ifdef VK_EXT_subpass_merge_feedback
            VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT m_VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT; // from VK_EXT_subpass_merge_feedback
#endif
#ifdef VK_NV_linear_color_attachment
            VkPhysicalDeviceLinearColorAttachmentFeaturesNV m_VkPhysicalDeviceLinearColorAttachmentFeaturesNV; // from VK_NV_linear_color_attachment 
#endif
#ifdef VK_EXT_attachment_feedback_loop_layout
            VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT m_VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT; // from VK_EXT_attachment_feedback_loop_layout 
#endif
#ifdef VK_EXT_graphics_pipeline_library
            VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT m_VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT; // from VK_EXT_graphics_pipeline_library 
#endif
#ifdef VK_EXT_multisampled_render_to_single_sampled
            VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT m_VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT; // from VK_EXT_multisampled_render_to_single_sampled
#endif
#ifdef VK_EXT_image_2d_view_of_3d
            VkPhysicalDeviceImage2DViewOf3DFeaturesEXT m_VkPhysicalDeviceImage2DViewOf3DFeaturesEXT; // from VK_EXT_image_2d_view_of_3d 
#endif
#ifdef VK_EXT_image_sliced_view_of_3d
            VkPhysicalDeviceImageSlicedViewOf3DFeaturesEXT m_VkPhysicalDeviceImageSlicedViewOf3DFeaturesEXT; // from VK_EXT_image_sliced_view_of_3d 
#endif
#ifdef VK_EXT_image_compression_control_swapchain
            VkPhysicalDeviceImageCompressionControlFeaturesEXT m_VkPhysicalDeviceImageCompressionControlFeaturesEXT; // from VK_EXT_image_compression_control 
            VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT m_VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT; // from VK_EXT_image_compression_control_swapchain 
#endif
#ifdef VK_AMD_shader_early_and_late_fragment_tests
            VkPhysicalDeviceShaderEarlyAndLateFragmentTestsFeaturesAMD m_VkPhysicalDeviceShaderEarlyAndLateFragmentTestsFeaturesAMD; // from VK_AMD_shader_early_and_late_fragment_tests 
#endif
#ifdef VK_EXT_non_seamless_cube_map
            VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT m_VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT; // from VK_EXT_non_seamless_cube_map 
#endif
#ifdef VK_EXT_shader_module_identifier
            VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT m_VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT; // from VK_EXT_shader_module_identifier 
#endif
#ifdef VK_QCOM_tile_properties
            VkPhysicalDeviceTilePropertiesFeaturesQCOM m_VkPhysicalDeviceTilePropertiesFeaturesQCOM; // from VK_QCOM_tile_properties 
#endif
#ifdef VK_QCOM_image_processing
            VkPhysicalDeviceImageProcessingFeaturesQCOM m_VkPhysicalDeviceImageProcessingFeaturesQCOM; // from VK_QCOM_image_processing
#endif
#ifdef VK_EXT_depth_clamp_zero_one
            VkPhysicalDeviceDepthClampZeroOneFeaturesEXT m_VkPhysicalDeviceDepthClampZeroOneFeaturesEXT; // VK_EXT_depth_clamp_zero_one 
#endif
#ifdef VK_EXT_shader_tile_image
            VkPhysicalDeviceShaderTileImageFeaturesEXT m_VkPhysicalDeviceShaderTileImageFeaturesEXT; // VK_EXT_shader_tile_image 
#endif
#ifdef VK_EXT_device_address_binding_report
            VkPhysicalDeviceAddressBindingReportFeaturesEXT m_VkPhysicalDeviceAddressBindingReportFeaturesEXT; // VK_EXT_device_address_binding_report 
#endif
#ifdef VK_NV_optical_flow
            VkPhysicalDeviceOpticalFlowFeaturesNV m_VkPhysicalDeviceOpticalFlowFeaturesNV; // VK_NV_optical_flow
#endif
#ifdef VK_EXT_device_fault
            VkPhysicalDeviceFaultFeaturesEXT m_VkPhysicalDeviceFaultFeaturesEXT; // VK_EXT_device_fault
#endif
#ifdef VK_EXT_pipeline_library_group_handles
            VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT m_VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT; // VK_EXT_pipeline_library_group_handles 
#endif
#ifdef VK_EXT_shader_object
            VkPhysicalDeviceShaderObjectFeaturesEXT m_VkPhysicalDeviceShaderObjectFeaturesEXT; // from VK_EXT_shader_object 
#endif
#ifdef VK_ARM_shader_core_builtins
            VkPhysicalDeviceShaderCoreBuiltinsFeaturesARM m_VkPhysicalDeviceShaderCoreBuiltinsFeaturesARM; // from VK_ARM_shader_core_builtins 
#endif
#ifdef VK_EXT_swapchain_maintenance1
            VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT m_VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT; // from VK_EXT_swapchain_maintenance1
#endif
#ifdef VK_NV_ray_tracing_invocation_reorder
            VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV m_VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV; // from VK_NV_ray_tracing_invocation_reorder 
#endif
#ifdef VK_QCOM_multiview_per_view_viewports
            VkPhysicalDeviceMultiviewPerViewViewportsFeaturesQCOM m_VkPhysicalDeviceMultiviewPerViewViewportsFeaturesQCOM; // from VK_QCOM_multiview_per_view_viewports 
#endif
#ifdef VK_QCOM_multiview_per_view_render_areas
            VkPhysicalDeviceMultiviewPerViewRenderAreasFeaturesQCOM m_VkPhysicalDeviceMultiviewPerViewRenderAreasFeaturesQCOM; // from VK_QCOM_multiview_per_view_render_areas 
#endif
#ifdef VK_HUAWEI_cluster_culling_shader
            VkPhysicalDeviceClusterCullingShaderFeaturesHUAWEI m_VkPhysicalDeviceClusterCullingShaderFeaturesHUAWEI; // from VK_HUAWEI_cluster_culling_shader
#endif
        };
        enum class Discriminant_t : uint16_t
        {
            Nothing = 0,
            VkPhysicalDeviceVulkan11Features,
            VkPhysicalDeviceVulkan12Features,
            VkPhysicalDeviceVulkan13Features,
            VkPhysicalDeviceVariablePointersFeatures,
            VkPhysicalDeviceMultiviewFeatures,
            VkPhysicalDeviceShaderAtomicInt64Features,
            VkPhysicalDevice8BitStorageFeatures,
            VkPhysicalDevice16BitStorageFeatures,
            VkPhysicalDeviceShaderFloat16Int8Features,
            VkPhysicalDeviceSamplerYcbcrConversionFeatures,
            VkPhysicalDeviceProtectedMemoryFeatures,
            VkPhysicalDeviceShaderDrawParametersFeatures,
            VkPhysicalDeviceDescriptorIndexingFeatures,
            VkPhysicalDeviceVulkanMemoryModelFeatures,
            VkPhysicalDeviceInlineUniformBlockFeatures,
            VkPhysicalDeviceScalarBlockLayoutFeatures,
            VkPhysicalDeviceUniformBufferStandardLayoutFeatures,
            VkPhysicalDeviceBufferDeviceAddressFeatures,
            VkPhysicalDeviceImagelessFramebufferFeatures,
            VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures,
            VkPhysicalDeviceHostQueryResetFeatures,
            VkPhysicalDeviceTimelineSemaphoreFeatures,
            VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures,
            VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures,
            VkPhysicalDeviceTextureCompressionASTCHDRFeatures,
            VkPhysicalDeviceSubgroupSizeControlFeatures,
            VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures,
            VkPhysicalDevicePrivateDataFeatures,
            VkPhysicalDeviceImageRobustnessFeatures,
            VkPhysicalDeviceShaderTerminateInvocationFeatures,
            VkPhysicalDeviceSynchronization2Features,
            VkPhysicalDeviceShaderIntegerDotProductFeatures,
            VkPhysicalDeviceMaintenance4Features,
            VkPhysicalDeviceDynamicRenderingFeatures,
#ifdef VK_EXT_rgba10x6_formats
            VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT,
#endif
#ifdef VK_EXT_pipeline_robustness
            VkPhysicalDevicePipelineRobustnessFeaturesEXT,
#endif
#ifdef VK_EXT_image_view_min_lod
            VkPhysicalDeviceImageViewMinLodFeaturesEXT,
#endif
#ifdef VK_EXT_rasterization_order_attachment_access
            VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT,
#endif
#ifdef VK_EXT_subpass_merge_feedback
            VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT,
#endif
#ifdef VK_NV_linear_color_attachment
            VkPhysicalDeviceLinearColorAttachmentFeaturesNV,
#endif
#ifdef VK_EXT_attachment_feedback_loop_layout
            VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT,
#endif
#ifdef VK_EXT_graphics_pipeline_library
            VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT,
#endif
#ifdef VK_EXT_multisampled_render_to_single_sampled
            VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT,
#endif
#ifdef VK_EXT_image_2d_view_of_3d
            VkPhysicalDeviceImage2DViewOf3DFeaturesEXT,
#endif
#ifdef VK_EXT_image_sliced_view_of_3d
            VkPhysicalDeviceImageSlicedViewOf3DFeaturesEXT,
#endif
#ifdef VK_EXT_image_compression_control_swapchain
            VkPhysicalDeviceImageCompressionControlFeaturesEXT,
            VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT,
#endif
#ifdef VK_AMD_shader_early_and_late_fragment_tests
            VkPhysicalDeviceShaderEarlyAndLateFragmentTestsFeaturesAMD,
#endif
#ifdef VK_EXT_non_seamless_cube_map
            VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT,
#endif
#ifdef VK_EXT_shader_module_identifier
            VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT,
#endif
#ifdef VK_QCOM_tile_properties
            VkPhysicalDeviceTilePropertiesFeaturesQCOM,
#endif
#ifdef VK_QCOM_image_processing
            VkPhysicalDeviceImageProcessingFeaturesQCOM,
#endif
#ifdef VK_EXT_depth_clamp_zero_one
            VkPhysicalDeviceDepthClampZeroOneFeaturesEXT,
#endif
#ifdef VK_EXT_shader_tile_image
            VkPhysicalDeviceShaderTileImageFeaturesEXT,
#endif
#ifdef VK_EXT_device_address_binding_report
            VkPhysicalDeviceAddressBindingReportFeaturesEXT,
#endif
#ifdef VK_NV_optical_flow
            VkPhysicalDeviceOpticalFlowFeaturesNV,
#endif
#ifdef VK_EXT_device_fault
            VkPhysicalDeviceFaultFeaturesEXT,
#endif
#ifdef VK_EXT_pipeline_library_group_handles
            VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT,
#endif
#ifdef VK_EXT_shader_object
            VkPhysicalDeviceShaderObjectFeaturesEXT,
#endif
#ifdef VK_ARM_shader_core_builtins
            VkPhysicalDeviceShaderCoreBuiltinsFeaturesARM,
#endif
#ifdef VK_EXT_swapchain_maintenance1
            VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT,
#endif
#ifdef VK_NV_ray_tracing_invocation_reorder
            VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV,
#endif
#ifdef VK_QCOM_multiview_per_view_viewports
            VkPhysicalDeviceMultiviewPerViewViewportsFeaturesQCOM,
#endif
#ifdef VK_QCOM_multiview_per_view_render_areas
            VkPhysicalDeviceMultiviewPerViewRenderAreasFeaturesQCOM,
#endif
#ifdef VK_HUAWEI_cluster_culling_shader
            VkPhysicalDeviceClusterCullingShaderFeaturesHUAWEI
#endif
        };

        U featureStruct;
        Discriminant_t discriminant;
    };

    inline DeviceFeatureChainNode constexpr emptyDeviceFeatureChainNode = DeviceFeatureChainNode();

    template <typename F>
    constexpr auto visitVulkanFeature(F&& f, DeviceFeatureChainNode const& deviceFeatureChainElement) -> decltype(f(deviceFeatureChainElement))
    {
        switch (deviceFeatureChainElement.discriminant)
        {
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceVulkan11Features:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceVulkan11Features);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceVulkan12Features:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceVulkan12Features);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceVulkan13Features:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceVulkan13Features);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceVariablePointersFeatures:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceVariablePointersFeatures);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceMultiviewFeatures:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceMultiviewFeatures);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceShaderAtomicInt64Features:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceShaderAtomicInt64Features);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDevice8BitStorageFeatures:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDevice8BitStorageFeatures);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDevice16BitStorageFeatures:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDevice16BitStorageFeatures);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceShaderFloat16Int8Features:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceShaderFloat16Int8Features);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceSamplerYcbcrConversionFeatures:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceSamplerYcbcrConversionFeatures);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceProtectedMemoryFeatures:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceProtectedMemoryFeatures);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceShaderDrawParametersFeatures:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceShaderDrawParametersFeatures);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceDescriptorIndexingFeatures:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceDescriptorIndexingFeatures);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceVulkanMemoryModelFeatures:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceVulkanMemoryModelFeatures);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceInlineUniformBlockFeatures:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceInlineUniformBlockFeatures);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceScalarBlockLayoutFeatures:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceScalarBlockLayoutFeatures);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceUniformBufferStandardLayoutFeatures:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceUniformBufferStandardLayoutFeatures);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceBufferDeviceAddressFeatures:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceBufferDeviceAddressFeatures);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceImagelessFramebufferFeatures:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceImagelessFramebufferFeatures);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceHostQueryResetFeatures:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceHostQueryResetFeatures);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceTimelineSemaphoreFeatures:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceTimelineSemaphoreFeatures);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceTextureCompressionASTCHDRFeatures:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceTextureCompressionASTCHDRFeatures);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceSubgroupSizeControlFeatures:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceSubgroupSizeControlFeatures);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDevicePrivateDataFeatures:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDevicePrivateDataFeatures);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceImageRobustnessFeatures:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceImageRobustnessFeatures);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceShaderTerminateInvocationFeatures:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceShaderTerminateInvocationFeatures);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceSynchronization2Features:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceSynchronization2Features);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceShaderIntegerDotProductFeatures:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceShaderIntegerDotProductFeatures);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceMaintenance4Features:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceMaintenance4Features);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceDynamicRenderingFeatures:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceDynamicRenderingFeatures);
#ifdef VK_EXT_rgba10x6_formats
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT);
#endif
#ifdef VK_EXT_pipeline_robustness
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDevicePipelineRobustnessFeaturesEXT:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDevicePipelineRobustnessFeaturesEXT);
#endif
#ifdef VK_EXT_image_view_min_lod
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceImageViewMinLodFeaturesEXT:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceImageViewMinLodFeaturesEXT);
#endif
#ifdef VK_EXT_rasterization_order_attachment_access
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT);
#endif
#ifdef VK_EXT_subpass_merge_feedback
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT);
#endif
#ifdef VK_NV_linear_color_attachment
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceLinearColorAttachmentFeaturesNV:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceLinearColorAttachmentFeaturesNV);
#endif
#ifdef VK_EXT_attachment_feedback_loop_layout
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT);
#endif
#ifdef VK_EXT_graphics_pipeline_library
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT);
#endif
#ifdef VK_EXT_multisampled_render_to_single_sampled
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT);
#endif
#ifdef VK_EXT_image_2d_view_of_3d
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceImage2DViewOf3DFeaturesEXT:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceImage2DViewOf3DFeaturesEXT);
#endif
#ifdef VK_EXT_image_sliced_view_of_3d
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceImageSlicedViewOf3DFeaturesEXT:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceImageSlicedViewOf3DFeaturesEXT);
#endif
#ifdef VK_EXT_image_compression_control_swapchain
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceImageCompressionControlFeaturesEXT:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceImageCompressionControlFeaturesEXT);
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT);
#endif
#ifdef VK_AMD_shader_early_and_late_fragment_tests
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceShaderEarlyAndLateFragmentTestsFeaturesAMD:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceShaderEarlyAndLateFragmentTestsFeaturesAMD);
#endif
#ifdef VK_EXT_non_seamless_cube_map
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT);
#endif
#ifdef VK_EXT_shader_module_identifier
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT);
#endif
#ifdef VK_QCOM_tile_properties
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceTilePropertiesFeaturesQCOM:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceTilePropertiesFeaturesQCOM);
#endif
#ifdef VK_QCOM_image_processing
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceImageProcessingFeaturesQCOM:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceImageProcessingFeaturesQCOM);
#endif
#ifdef VK_EXT_depth_clamp_zero_one
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceDepthClampZeroOneFeaturesEXT:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceDepthClampZeroOneFeaturesEXT);
#endif
#ifdef VK_EXT_shader_tile_image
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceShaderTileImageFeaturesEXT:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceShaderTileImageFeaturesEXT);
#endif
#ifdef VK_EXT_device_address_binding_report
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceAddressBindingReportFeaturesEXT:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceAddressBindingReportFeaturesEXT);
#endif
#ifdef VK_NV_optical_flow
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceOpticalFlowFeaturesNV:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceOpticalFlowFeaturesNV);
#endif
#ifdef VK_EXT_device_fault
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceFaultFeaturesEXT:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceFaultFeaturesEXT);
#endif
#ifdef VK_EXT_pipeline_library_group_handles
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT);
#endif
#ifdef VK_EXT_shader_object
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceShaderObjectFeaturesEXT:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceShaderObjectFeaturesEXT);
#endif
#ifdef VK_ARM_shader_core_builtins
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceShaderCoreBuiltinsFeaturesARM:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceShaderCoreBuiltinsFeaturesARM);
#endif
#ifdef VK_EXT_swapchain_maintenance1
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT);
#endif
#ifdef VK_NV_ray_tracing_invocation_reorder
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV);
#endif
#ifdef VK_QCOM_multiview_per_view_viewports
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceMultiviewPerViewViewportsFeaturesQCOM:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceMultiviewPerViewViewportsFeaturesQCOM);
#endif
#ifdef VK_QCOM_multiview_per_view_render_areas
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceMultiviewPerViewRenderAreasFeaturesQCOM:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceMultiviewPerViewRenderAreasFeaturesQCOM);
#endif
#ifdef VK_HUAWEI_cluster_culling_shader
            case DeviceFeatureChainNode::Discriminant_t::VkPhysicalDeviceClusterCullingShaderFeaturesHUAWEI:
                return f(deviceFeatureChainElement.featureStruct.m_VkPhysicalDeviceClusterCullingShaderFeaturesHUAWEI);
#endif
			case DeviceFeatureChainNode::Discriminant_t::Nothing: 
				std::abort();
				break;
        }
    }

    template <typename T>
    concept QueueFamilyPropertiesType = 
            std::is_same_v<T, VkQueueFamilyProperties2> ||
            std::is_same_v<T, VkQueueFamilyCheckpointProperties2NV> ||
            std::is_same_v<T, VkQueueFamilyCheckpointPropertiesNV> ||
            std::is_same_v<T, VkQueueFamilyGlobalPriorityPropertiesKHR> ||
            std::is_same_v<T, VkQueueFamilyQueryResultStatusPropertiesKHR> ||
            std::is_same_v<T, VkQueueFamilyVideoPropertiesKHR>;

    class QueueFamilyPropertiesChainNode
    {
    public:
        constexpr QueueFamilyPropertiesChainNode() : queuePropertiesStruct(), discriminant(Discriminant_t::Nothing) {}

        template <QueueFamilyPropertiesType T>
        constexpr explicit QueueFamilyPropertiesChainNode(T const& v) : QueueFamilyPropertiesChainNode()
        {
            if constexpr (std::is_same_v<VkQueueFamilyProperties2, T>) { std::construct_at(&queuePropertiesStruct.m_VkQueueFamilyProperties2, v); discriminant = Discriminant_t::VkQueueFamilyProperties2; }
            if constexpr (std::is_same_v<VkQueueFamilyCheckpointProperties2NV, T>) { std::construct_at(&queuePropertiesStruct.m_VkQueueFamilyCheckpointProperties2NV, v); discriminant = Discriminant_t::VkQueueFamilyCheckpointProperties2NV; }
            if constexpr (std::is_same_v<VkQueueFamilyCheckpointPropertiesNV, T>) { std::construct_at(&queuePropertiesStruct.m_VkQueueFamilyCheckpointPropertiesNV, v); discriminant = Discriminant_t::VkQueueFamilyCheckpointPropertiesNV; }
            if constexpr (std::is_same_v<VkQueueFamilyGlobalPriorityPropertiesKHR, T>) { std::construct_at(&queuePropertiesStruct.m_VkQueueFamilyGlobalPriorityPropertiesKHR, v); discriminant = Discriminant_t::VkQueueFamilyGlobalPriorityPropertiesKHR; }
            if constexpr (std::is_same_v<VkQueueFamilyQueryResultStatusPropertiesKHR, T>) { std::construct_at(&queuePropertiesStruct.m_VkQueueFamilyQueryResultStatusPropertiesKHR, v); discriminant = Discriminant_t::VkQueueFamilyQueryResultStatusPropertiesKHR; }
            if constexpr (std::is_same_v<VkQueueFamilyVideoPropertiesKHR, T>) { std::construct_at(&queuePropertiesStruct.m_VkQueueFamilyVideoPropertiesKHR, v); discriminant = Discriminant_t::VkQueueFamilyVideoPropertiesKHR; }
        }

        explicit QueueFamilyPropertiesChainNode(SlicedVkStruct const* vkStruct) : QueueFamilyPropertiesChainNode()
        {
            assert(vkStruct);
            switch (vkStruct->sType)
            {
                case VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2:
                    std::construct_at(&queuePropertiesStruct.m_VkQueueFamilyProperties2, *reinterpret_cast<VkQueueFamilyProperties2 const*>(vkStruct));
                    discriminant = Discriminant_t::VkQueueFamilyProperties2;
                    break;
                case VK_STRUCTURE_TYPE_QUEUE_FAMILY_CHECKPOINT_PROPERTIES_2_NV:
                    std::construct_at(&queuePropertiesStruct.m_VkQueueFamilyCheckpointProperties2NV, *reinterpret_cast<VkQueueFamilyCheckpointProperties2NV const*>(vkStruct));
                    discriminant = Discriminant_t::VkQueueFamilyCheckpointProperties2NV;
                    break;
                case VK_STRUCTURE_TYPE_QUEUE_FAMILY_CHECKPOINT_PROPERTIES_NV:
                    std::construct_at(&queuePropertiesStruct.m_VkQueueFamilyCheckpointPropertiesNV, *reinterpret_cast<VkQueueFamilyCheckpointPropertiesNV const*>(vkStruct));
                    discriminant = Discriminant_t::VkQueueFamilyCheckpointPropertiesNV;
                    break;
                case VK_STRUCTURE_TYPE_QUEUE_FAMILY_GLOBAL_PRIORITY_PROPERTIES_KHR:
                    std::construct_at(&queuePropertiesStruct.m_VkQueueFamilyGlobalPriorityPropertiesKHR, *reinterpret_cast<VkQueueFamilyGlobalPriorityPropertiesKHR const*>(vkStruct));
                    discriminant = Discriminant_t::VkQueueFamilyGlobalPriorityPropertiesKHR;
                    break;
                case VK_STRUCTURE_TYPE_QUEUE_FAMILY_QUERY_RESULT_STATUS_PROPERTIES_KHR:
                    std::construct_at(&queuePropertiesStruct.m_VkQueueFamilyQueryResultStatusPropertiesKHR, *reinterpret_cast<VkQueueFamilyQueryResultStatusPropertiesKHR const*>(vkStruct));
                    discriminant = Discriminant_t::VkQueueFamilyQueryResultStatusPropertiesKHR;
                    break;
                case VK_STRUCTURE_TYPE_QUEUE_FAMILY_VIDEO_PROPERTIES_KHR:
                    std::construct_at(&queuePropertiesStruct.m_VkQueueFamilyVideoPropertiesKHR, *reinterpret_cast<VkQueueFamilyVideoPropertiesKHR const*>(vkStruct));
                    discriminant = Discriminant_t::VkQueueFamilyVideoPropertiesKHR;
                    break;
                default:
                    std::abort();
                    break;
            }
        }

    private:
        union U
        {
            constexpr U() {}
            constexpr ~U() {}

            VkQueueFamilyProperties2 m_VkQueueFamilyProperties2;
            VkQueueFamilyCheckpointProperties2NV m_VkQueueFamilyCheckpointProperties2NV;
            VkQueueFamilyCheckpointPropertiesNV m_VkQueueFamilyCheckpointPropertiesNV;
            VkQueueFamilyGlobalPriorityPropertiesKHR m_VkQueueFamilyGlobalPriorityPropertiesKHR;
            VkQueueFamilyQueryResultStatusPropertiesKHR m_VkQueueFamilyQueryResultStatusPropertiesKHR;
            VkQueueFamilyVideoPropertiesKHR m_VkQueueFamilyVideoPropertiesKHR;
        };
        enum class Discriminant_t : uint8_t
        {
            Nothing = 0,
            VkQueueFamilyProperties2,
            VkQueueFamilyCheckpointProperties2NV,
            VkQueueFamilyCheckpointPropertiesNV,
            VkQueueFamilyGlobalPriorityPropertiesKHR,
            VkQueueFamilyQueryResultStatusPropertiesKHR,
            VkQueueFamilyVideoPropertiesKHR
        };
        U queuePropertiesStruct;
        Discriminant_t discriminant;
    };

}

namespace vkutils
{
    // could be more optimized if I wrote some overloads, eg for const lvalue references, or take value by copy if the type is lightweight
    // and trivially copy constructible,...
    template <typename Callable, std::ranges::input_range Range> requires std::is_nothrow_invocable_r<void, Callable>::value
    constexpr auto findOr(Range&& range, typename Range::const_iterator<typename Range::value_type>&& value, Callable callable) 
        -> typename Range::const_iterator<typename Range::value_type>
    {
        auto const it = std::find(range.cbegin(), range.cend(), std::forward<typename Range::value_type>(value));
        if (it == range.cend())
        {
            callable();
        }
        return it;
    }
    
    // convert VkResult to std_string_view. Credit to https://github.com/SaschaWillems/Vulkan
    std::string_view errorString(VkResult errorCode)
	{
	    switch (errorCode)
	    {
#define STR(r) case VK_ ##r: return #r
			STR(NOT_READY);
			STR(TIMEOUT);
			STR(EVENT_SET);
			STR(EVENT_RESET);
			STR(INCOMPLETE);
			STR(ERROR_OUT_OF_HOST_MEMORY);
			STR(ERROR_OUT_OF_DEVICE_MEMORY);
			STR(ERROR_INITIALIZATION_FAILED);
			STR(ERROR_DEVICE_LOST);
			STR(ERROR_MEMORY_MAP_FAILED);
			STR(ERROR_LAYER_NOT_PRESENT);
			STR(ERROR_EXTENSION_NOT_PRESENT);
			STR(ERROR_FEATURE_NOT_PRESENT);
			STR(ERROR_INCOMPATIBLE_DRIVER);
			STR(ERROR_TOO_MANY_OBJECTS);
			STR(ERROR_FORMAT_NOT_SUPPORTED);
			STR(ERROR_SURFACE_LOST_KHR);
			STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
			STR(SUBOPTIMAL_KHR);
			STR(ERROR_OUT_OF_DATE_KHR);
			STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
			STR(ERROR_VALIDATION_FAILED_EXT);
			STR(ERROR_INVALID_SHADER_NV);
#undef STR
			default:
				return "UNKNOWN_ERROR";
		}
	}   
} // namespace vkutils


}

#endif // MXC_UTILS_HPP
