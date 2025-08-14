#pragma once

#ifdef NDEBUG
	const bool ENABLE_VALIDATION_LAYERS{ false };
#else
	const bool ENABLE_VALIDATION_LAYERS{ true };
#endif

	const int MAX_FRAMES_IN_FLIGHT{ 1 };  