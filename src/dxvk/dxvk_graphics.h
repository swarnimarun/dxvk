#pragma once

#include <mutex>

#include "../util/sync/sync_list.h"

#include "dxvk_bind_mask.h"
#include "dxvk_constant_state.h"
#include "dxvk_graphics_state.h"
#include "dxvk_pipecache.h"
#include "dxvk_pipelayout.h"
#include "dxvk_renderpass.h"
#include "dxvk_resource.h"
#include "dxvk_shader.h"
#include "dxvk_stats.h"

namespace dxvk {
  
  class DxvkDevice;
  class DxvkPipelineManager;

  /**
   * \brief Vertex input info for graphics pipelines
   *
   * Can be used to compile dedicated pipeline objects for use
   * in a graphics pipeline library, or as part of the data
   * required to compile a full graphics pipeline.
   */
  struct DxvkGraphicsPipelineVertexInputState {
    DxvkGraphicsPipelineVertexInputState();

    DxvkGraphicsPipelineVertexInputState(
      const DxvkDevice*                     device,
      const DxvkGraphicsPipelineStateInfo&  state);

    VkPipelineInputAssemblyStateCreateInfo          iaInfo        = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    VkPipelineVertexInputStateCreateInfo            viInfo        = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    VkPipelineVertexInputDivisorStateCreateInfoEXT  viDivisorInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_DIVISOR_STATE_CREATE_INFO_EXT };

    std::array<VkVertexInputBindingDescription,           MaxNumVertexBindings>   viBindings    = { };
    std::array<VkVertexInputBindingDivisorDescriptionEXT, MaxNumVertexBindings>   viDivisors    = { };
    std::array<VkVertexInputAttributeDescription,         MaxNumVertexAttributes> viAttributes  = { };

    bool eq(const DxvkGraphicsPipelineVertexInputState& other) const;

    size_t hash() const;
  };


  /**
   * \brief Fragment output info for graphics pipelines
   *
   * Can be used to compile dedicated pipeline objects for use
   * in a graphics pipeline library, or as part of the data
   * required to compile a full graphics pipeline.
   */
  struct DxvkGraphicsPipelineFragmentOutputState {
    DxvkGraphicsPipelineFragmentOutputState();

    DxvkGraphicsPipelineFragmentOutputState(
      const DxvkDevice*                     device,
      const DxvkGraphicsPipelineStateInfo&  state,
      const DxvkShader*                     fs);

    VkPipelineRenderingCreateInfoKHR                rtInfo = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR };
    VkPipelineColorBlendStateCreateInfo             cbInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    VkPipelineMultisampleStateCreateInfo            msInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };

    uint32_t                                                             msSampleMask   = 0u;
    std::array<VkPipelineColorBlendAttachmentState, MaxNumRenderTargets> cbAttachments  = { };
    std::array<VkFormat,                            MaxNumRenderTargets> rtColorFormats = { };

    bool eq(const DxvkGraphicsPipelineFragmentOutputState& other) const;

    size_t hash() const;
  };


  /**
   * \brief Pre-rasterization info for graphics pipelines
   *
   * Can only be used when compiling full graphics pipelines
   * when all pipeline state is known.
   */
  struct DxvkGraphicsPipelinePreRasterizationState {
    DxvkGraphicsPipelinePreRasterizationState();

    DxvkGraphicsPipelinePreRasterizationState(
      const DxvkDevice*                     device,
      const DxvkGraphicsPipelineStateInfo&  state,
      const DxvkShader*                     gs);

    VkPipelineViewportStateCreateInfo                     vpInfo              = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    VkPipelineTessellationStateCreateInfo                 tsInfo              = { VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };
    VkPipelineRasterizationStateCreateInfo                rsInfo              = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    VkPipelineRasterizationDepthClipStateCreateInfoEXT    rsDepthClipInfo     = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT };
    VkPipelineRasterizationStateStreamCreateInfoEXT       rsXfbStreamInfo     = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_STREAM_CREATE_INFO_EXT };
    VkPipelineRasterizationConservativeStateCreateInfoEXT rsConservativeInfo  = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT };
  };

  
  /**
   * \brief Flags that describe pipeline properties
   */
  enum class DxvkGraphicsPipelineFlag {
    HasTransformFeedback,
    HasStorageDescriptors,
  };

  using DxvkGraphicsPipelineFlags = Flags<DxvkGraphicsPipelineFlag>;


  /**
   * \brief Shaders used in graphics pipelines
   */
  struct DxvkGraphicsPipelineShaders {
    Rc<DxvkShader> vs;
    Rc<DxvkShader> tcs;
    Rc<DxvkShader> tes;
    Rc<DxvkShader> gs;
    Rc<DxvkShader> fs;

    bool eq(const DxvkGraphicsPipelineShaders& other) const {
      return vs == other.vs && tcs == other.tcs
          && tes == other.tes && gs == other.gs
          && fs == other.fs;
    }

    size_t hash() const {
      DxvkHashState state;
      state.add(DxvkShader::getHash(vs));
      state.add(DxvkShader::getHash(tcs));
      state.add(DxvkShader::getHash(tes));
      state.add(DxvkShader::getHash(gs));
      state.add(DxvkShader::getHash(fs));
      return state;
    }

    bool validate() const {
      return validateShaderType(vs, VK_SHADER_STAGE_VERTEX_BIT)
          && validateShaderType(tcs, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
          && validateShaderType(tes, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
          && validateShaderType(gs, VK_SHADER_STAGE_GEOMETRY_BIT)
          && validateShaderType(fs, VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    static bool validateShaderType(const Rc<DxvkShader>& shader, VkShaderStageFlagBits stage) {
      return shader == nullptr || shader->info().stage == stage;
    }
  };


  /**
   * \brief Graphics pipeline instance
   * 
   * Stores a state vector and the
   * corresponding pipeline handle.
   */
  class DxvkGraphicsPipelineInstance {

  public:

    DxvkGraphicsPipelineInstance()
    : m_stateVector (),
      m_pipeline    (VK_NULL_HANDLE) { }

    DxvkGraphicsPipelineInstance(
      const DxvkGraphicsPipelineStateInfo&  state,
            VkPipeline                      pipe)
    : m_stateVector (state),
      m_pipeline    (pipe) { }

    /**
     * \brief Checks for matching pipeline state
     * 
     * \param [in] stateVector Graphics pipeline state
     * \returns \c true if the specialization is compatible
     */
    bool isCompatible(
      const DxvkGraphicsPipelineStateInfo&  state) {
      return m_stateVector == state;
    }

    /**
     * \brief Retrieves pipeline
     * \returns The pipeline handle
     */
    VkPipeline pipeline() const {
      return m_pipeline;
    }

  private:

    DxvkGraphicsPipelineStateInfo m_stateVector;
    VkPipeline                    m_pipeline;

  };

  
  /**
   * \brief Graphics pipeline
   * 
   * Stores the pipeline layout as well as methods to
   * recompile the graphics pipeline against a given
   * pipeline state vector.
   */
  class DxvkGraphicsPipeline {
    
  public:
    
    DxvkGraphicsPipeline(
            DxvkPipelineManager*        pipeMgr,
            DxvkGraphicsPipelineShaders shaders,
            DxvkBindingLayoutObjects*   layout);

    ~DxvkGraphicsPipeline();

    /**
     * \brief Shaders used by the pipeline
     * \returns Shaders used by the pipeline
     */
    const DxvkGraphicsPipelineShaders& shaders() const {
      return m_shaders;
    }
    
    /**
     * \brief Returns graphics pipeline flags
     * \returns Graphics pipeline property flags
     */
    DxvkGraphicsPipelineFlags flags() const {
      return m_flags;
    }
    
    /**
     * \brief Pipeline layout
     * 
     * Stores the pipeline layout and the descriptor set
     * layout, as well as information on the resource
     * slots used by the pipeline.
     * \returns Pipeline layout
     */
    DxvkBindingLayoutObjects* getBindings() const {
      return m_bindings;
    }

    /**
     * \brief Queries shader for a given stage
     * 
     * In case no shader is specified for the
     * given stage, \c nullptr will be returned.
     * \param [in] stage The shader stage
     * \returns Shader of the given stage
     */
    Rc<DxvkShader> getShader(
            VkShaderStageFlagBits             stage) const;

    /**
     * \brief Queries global resource barrier
     *
     * Returns the stages that can access resources in this
     * pipeline with the given pipeline state, as well as
     * the ways in which resources are accessed. This does
     * not include render targets. The barrier is meant to
     * be executed after the render pass.
     * \returns Global barrier
     */
    DxvkGlobalPipelineBarrier getGlobalBarrier(
      const DxvkGraphicsPipelineStateInfo&    state) const;

    /**
     * \brief Pipeline handle
     * 
     * Retrieves a pipeline handle for the given pipeline
     * state. If necessary, a new pipeline will be created.
     * \param [in] state Pipeline state vector
     * \returns Pipeline handle
     */
    VkPipeline getPipelineHandle(
      const DxvkGraphicsPipelineStateInfo&    state);
    
    /**
     * \brief Compiles a pipeline
     * 
     * Asynchronously compiles the given pipeline
     * and stores the result for future use.
     * \param [in] state Pipeline state vector
     */
    void compilePipeline(
      const DxvkGraphicsPipelineStateInfo&    state);
    
  private:
    
    Rc<vk::DeviceFn>            m_vkd;
    DxvkPipelineManager*        m_pipeMgr;

    DxvkGraphicsPipelineShaders m_shaders;
    DxvkBindingLayoutObjects*   m_bindings;
    DxvkGlobalPipelineBarrier   m_barrier;
    DxvkGraphicsPipelineFlags   m_flags;

    uint32_t m_vsIn  = 0;
    uint32_t m_fsOut = 0;
    
    // List of pipeline instances, shared between threads
    alignas(CACHE_LINE_SIZE)
    dxvk::mutex                               m_mutex;
    sync::List<DxvkGraphicsPipelineInstance>  m_pipelines;
    
    DxvkGraphicsPipelineInstance* createInstance(
      const DxvkGraphicsPipelineStateInfo& state);
    
    DxvkGraphicsPipelineInstance* findInstance(
      const DxvkGraphicsPipelineStateInfo& state);
    
    VkPipeline createPipeline(
      const DxvkGraphicsPipelineStateInfo& state) const;
    
    void destroyPipeline(
            VkPipeline                     pipeline) const;
    
    DxvkShaderModule createShaderModule(
      const Rc<DxvkShader>&                shader,
      const DxvkGraphicsPipelineStateInfo& state) const;
    
    Rc<DxvkShader> getPrevStageShader(
            VkShaderStageFlagBits          stage) const;

    bool validatePipelineState(
      const DxvkGraphicsPipelineStateInfo& state,
            bool                           trusted) const;
    
    void writePipelineStateToCache(
      const DxvkGraphicsPipelineStateInfo& state) const;
    
    void logPipelineState(
            LogLevel                       level,
      const DxvkGraphicsPipelineStateInfo& state) const;

  };
  
}