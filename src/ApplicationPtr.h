#if !defined(MXC_APPLICATIONPTR_H)
#define MXC_APPLICATIONPTR_H

#include <cstdint>
#include <type_traits>

// Heavily inspired from pbrt-v4's repository

namespace mxc
{
	class Application;
	class VulkanApplication;

	template <typename T>
	concept ApplicationType = std::is_same_v<T, Application> || std::is_same_v<T, VulkanApplication>;

	class ApplicationPtr
	{
	public:
		template <ApplicationType T>
		explicit ApplicationPtr(T* app) : m_app(app) {}

		template <ApplicationType T>
		auto get() const -> T& { return *reinterpret_cast<T*>(m_app); }
		
	private:

		void* m_app;
	};
}

#endif // MXC_APPLICATIONPTR_H
