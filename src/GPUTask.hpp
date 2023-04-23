#ifndef MXC_GPUTASK_HPP
#define MXC_GPUTASK_HPP

#include "initializers.hpp"

#include <vulkan/vulkan.h>
#include <refl.hpp>

#include <cstddef>
#include <tuple>
#include <concepts>

// The lambda_traits class is defined recursively, where the base case is a lambda with no captures, and the recursive case is a 
// lambda with captures. In the recursive case, the lambda_traits class derives from itself with the lambda's capture-less operator(). 
// which extracts its type signature, which can be used as template type parameter
// This allows the class to extract the parameter types and return type of the lambda.
template <typename Func>
struct lambda_traits : public lambda_traits<decltype(&Func::operator())> {};

template <typename LambdaGeneratedClassType, typename ReturnType, typename... Args>
struct lambda_traits<ReturnType(LambdaGeneratedClassType::*)(Args...) const> // const allows to match const qualified lambda objects
{
    static size_t constexpr arity = sizeof...(Args);

    template <size_t I>
    struct arg
    {
        using type = typename std::tuple_element<I, std::tuple<Args...>>::type;
    };
};

struct TaskTag {};

struct TaskDataTag {};

template <typename TaskDataT>
concept TaskData = 
    std::is_standard_layout_v<TaskDataT> && std::is_trivial_v<TaskDataT> && 
    std::derived_from<TaskDataT, TaskDataTag>;

template<typename F, TaskData TaskDataT> requires std::is_nothrow_invocable_r<VkResult, F, TaskDataT&>::value
class LambdaTask 
{
public:
    explicit constexpr LambdaTask(F&& f) : m_f{std::forward<F>(f)} {}

    auto execute_impl(TaskDataT& data) const -> VkResult
    {
        return m_f(data);
    }

private:
    F m_f;
};

template<typename F, TaskData TaskDataT> requires std::is_invocable_r<VkResult, F, TaskDataT&>::value
LambdaTask(F&& f) -> LambdaTask<F, typename lambda_traits<decltype(f)>::arg<0>::type>;

template <typename LHS, typename RHS>
class CombinedTask;

template <typename Derived>
class GPUTaskBase : public TaskTag
{
public:
    // Execute the task by recording commands into the command buffer(s), create resources ...
    template <TaskData TaskDataT>
    auto execute(TaskDataT& data) const -> VkResult
    {
        return static_cast<const Derived*>(this)->execute_impl(data);
    }

    // Combine this task with another task using the pipe operator. Note that GPUTasks in the same chain
    // have to use the same TaskData struct
    template <typename Other>
    constexpr auto operator|(const Other& other) const -> CombinedTask<Derived, Other>
    {
        return CombinedTask<Derived, Other>(*static_cast<Derived const*>(this), other);
    }

private:
    GPUTaskBase() = default; // this will prevent CRTP typos
    friend Derived;
};

template <typename TaskType>
class GPUTask : public GPUTaskBase<GPUTask<TaskType>> 
{
public:
    explicit constexpr GPUTask(const TaskType& task) : m_task{task} {}

    // Execute the task by recording commands into the command buffer
    template <TaskData TaskDataT>
    auto execute_impl(TaskDataT& data) const -> VkResult
    {
        return m_task.execute_impl(data);
    }

    // Create a GPUTask from a lambda function
    template <TaskData TaskDataT, typename F> requires std::is_nothrow_invocable_r<VkResult, F, TaskDataT&>::value 
    static constexpr auto fromLambda(F f) -> GPUTask<LambdaTask<F, TaskDataT>>
    {
        return GPUTask<LambdaTask<F, TaskDataT>>(LambdaTask<F, TaskDataT>{f});
    }

private:
    TaskType m_task;
};

template <typename LHS, typename RHS>
class CombinedTask : public GPUTaskBase<CombinedTask<LHS, RHS>> 
{
public:
    constexpr CombinedTask(const LHS& lhs, const RHS& rhs) : m_lhs{lhs}, m_rhs{rhs} {}

    template <TaskData TaskDataT>
    auto execute_impl(TaskDataT& data) const -> VkResult
    {
            VkResult result = m_lhs->execute(data);
            if (result != VK_SUCCESS) {
                return result;
            }
            return m_rhs.execute(data);
    }

private:
    const LHS& m_lhs;
    const RHS& m_rhs;
};

class EmptyTask : public GPUTaskBase<EmptyTask>
{
public:
    template <TaskData TaskDataT>
    constexpr auto execute_impl(TaskDataT& data) const -> VkResult { return VK_SUCCESS; }
};

// TODO This is 1) Ugly 2) supports only renderPasses with 1 subpass
template <typename TaskType, TaskData TaskDataT>
class RenderTask : public GPUTaskBase<RenderTask<TaskType, TaskDataT>>
{
public:
    using RenderFunction = VkResult(*)(TaskDataT&);

    static inline uint32_t constexpr maxVariableNameSize = 64;
    using MemberName = refl::util::const_string<maxVariableNameSize>;

    constexpr RenderTask(RenderFunction func, MemberName const& recordingCommandBufferMemberName,
                         MemberName const& renderPassBeginInfoMemberName, 
                         MemberName const& subpassBeginInfoMemberName, MemberName const& subpassEndInfoMemberName) 
        : m_renderFunction{func}, m_renderPassBeginInfoMemberName{renderPassBeginInfoMemberName}
        , m_subpassBeginInfoMemberName{subpassBeginInfoMemberName}, m_subpassEndInfoMemberName{subpassEndInfoMemberName}
        , m_recordingCommandBufferMemberName{recordingCommandBufferMemberName}
        {}

    auto execute_impl(TaskDataT& data) const -> VkResult
    {
        auto recordingCmdBuffer = refl::runtime::invoke<VkCommandBuffer>(data, m_recordingCommandBufferMemberName);
        auto* pRenderPassBeginInfo = &refl::runtime::invoke<VkRenderPassBeginInfo>(data, m_renderPassBeginInfoMemberName);
        auto* pSubpassBeginInfo = &refl::runtime::invoke<VkSubpassBeginInfo>(data, m_subpassBeginInfoMemberName);
        auto* pSubpassEndInfo = &refl::runtime::invoke<VkSubpassEndInfo>(data, m_subpassEndInfoMemberName);

        vkCmdBeginRenderPass2(recordingCmdBuffer, pRenderPassBeginInfo, pSubpassBeginInfo);

        VkResult res = m_renderFunction(data);

        vkCmdEndRenderPass2(recordingCmdBuffer, pSubpassEndInfo);
        return res;
    }

private:
    RenderFunction m_renderFunction;
    MemberName const& m_renderPassBeginInfoMemberName;
    MemberName const& m_subpassBeginInfoMemberName;
    MemberName const& m_subpassEndInfoMemberName;
    MemberName const& m_recordingCommandBufferMemberName;
};

#endif // MXC_GPUTASK_HPP
