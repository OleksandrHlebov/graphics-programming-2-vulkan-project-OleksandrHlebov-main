#pragma once

class Application
{
public:
	virtual ~Application() = 0 {}

	virtual void Run() = 0;

protected:
	Application() = default;
};