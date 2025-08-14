#include "WorldTime.h"
#include <chrono>

namespace
{
	static auto lastUpdateTime{ std::chrono::steady_clock::now() };
	static float elapsedSec{ .0f };
}

namespace WorldTime
{
	float GetElapsedSec()
	{
		return elapsedSec;
	}

	void Tick()
	{
		const auto currentTime{ std::chrono::steady_clock::now() };
		elapsedSec = std::chrono::duration<float>(currentTime - lastUpdateTime).count();
		lastUpdateTime = currentTime;
	}

}