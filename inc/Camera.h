#pragma once
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <GLFW/glfw3.h>
#include <algorithm>

#include "WorldTime.h"

class Camera final
{
public:
	Camera() = delete;
	Camera(glm::vec3 position, float fov, float aspectRatio, float near, float far) 
		: m_Position{ position }
		, m_Fov{ fov }
		, m_AspectRatio{ aspectRatio }
		, m_Near{ near }
		, m_Far{ far }
	{
		m_Projection[1][1] *= -1; // glm was designed for opengl, invert y to comply with vulkan
	}
	~Camera() = default;
	
	Camera(const Camera&) 				= delete;
	Camera(Camera&&) noexcept 			= delete;
	Camera& operator=(const Camera&) 	 	= delete;
	Camera& operator=(Camera&&) noexcept 	= delete;

	// GLFW_REPEAT is not reliable for consistent movement using keyboard
	//void KeyboardStateChanged(int key, int scancode, int action, int mods)
	//{
	//	//if (key == GLFW_KEY_W && (action == GLFW_REPEAT || action == GLFW_PRESS))
	//	//	m_Position += m_Forward * m_Speed;
	//	//if (key == GLFW_KEY_S && (action == GLFW_REPEAT || action == GLFW_PRESS))
	//	//	m_Position -= m_Forward * m_Speed;
	//}

	//void MouseMoved(GLFWwindow* window, double xPos, double yPos)
	//{
	//	//static double cache_xPos{ xPos };
	//	//static double cache_yPos{ yPos };
	//	//double deltaX{ cache_xPos - xPos };
	//	//double deltaY{ cache_yPos - yPos };
	//	//cache_xPos = xPos;
	//	//cache_yPos = yPos;
	//	//if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS)
	//	//{
	//	//	static float totalYaw{};
	//	//	static float totalPitch{};
	//	//	totalYaw += deltaX * m_Sensitivity;
	//	//	totalPitch += deltaY * m_Sensitivity;
	//	//	totalPitch = std::clamp(totalPitch, -89.f, 89.f);

	//	//	m_Forward.x = cos(glm::radians(totalYaw)) * cos(glm::radians(totalPitch));
	//	//	m_Forward.y = sin(glm::radians(totalYaw)) * cos(glm::radians(totalPitch));
	//	//	m_Forward.z = sin(glm::radians(totalPitch));
	//	//}
	//}

	void SetAspectRatio(float aspectRatio)
	{
		m_AspectRatio = aspectRatio;
		m_Projection = glm::perspective(glm::radians(m_Fov), m_AspectRatio, m_Near, m_Far);
		m_Projection[1][1] *= -1;
	}

	void Update(GLFWwindow* window)
	{
		double xPos{};
		double yPos{};
		static double cache_xPos{ 0 };
		static double cache_yPos{ 0 };
		glfwGetCursorPos(window, &xPos, &yPos);

		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS)
		{
			const float boost{ 1 + (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) * m_Boost) };
			const float travelThisUpdate{ m_Speed * boost * WorldTime::GetElapsedSec() };
			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
				m_Position += m_Forward * travelThisUpdate;
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
				m_Position -= m_Forward * travelThisUpdate;
			if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
				m_Position += m_Up * travelThisUpdate;
			if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
				m_Position -= m_Up * travelThisUpdate;
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			{
				glm::vec3 right{ glm::cross(m_Forward, m_Up) };
				m_Position += right * travelThisUpdate;
			}
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			{
				glm::vec3 right{ glm::cross(m_Forward, m_Up) };
				m_Position -= right * travelThisUpdate; 
			}

			double deltaX{ cache_xPos - xPos };
			double deltaY{ cache_yPos - yPos }; 

			if (abs(deltaX) > FLT_EPSILON || abs(deltaY) > FLT_EPSILON)
			{
				static float totalYaw{};
				static float totalPitch{}; 
				totalYaw += deltaX * m_Sensitivity;
				totalPitch += deltaY * m_Sensitivity;
				totalPitch = std::clamp(totalPitch, -89.f, 89.f);
				 
				m_Forward.x = cos(glm::radians(totalYaw)) * cos(glm::radians(totalPitch));
				m_Forward.y = sin(glm::radians(totalYaw)) * cos(glm::radians(totalPitch));
				m_Forward.z = sin(glm::radians(totalPitch));
			}
		}  
		 
		cache_xPos = xPos; 
		cache_yPos = yPos;
	} 
	   
	glm::mat4x4& CalculateView()
	{
		static glm::mat4x4 view{};
		view = glm::lookAt(m_Position, m_Position + m_Forward, m_Up);
		return view;
	}
	 
	glm::mat4x4& GetProjection()
	{
		return m_Projection; 
	}

private:
	glm::vec3 m_Position;
	const glm::vec3 m_Up			{ .0f, .0f, 1.f };
	const glm::vec3 m_WorldForward	{ 1.f, .0f, .0f };
	glm::vec3 m_Forward				{ m_WorldForward };
	float m_Speed		{ 2.f };
	float m_Boost		{ 1.f };
	float m_Sensitivity	{ 0.3f };
	float m_Fov;
	float m_AspectRatio;
	float m_Near;
	float m_Far;
	glm::mat4x4 m_Projection		{ glm::perspective(glm::radians(m_Fov), m_AspectRatio, m_Near, m_Far) };
};