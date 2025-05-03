#pragma once

#include <functional>

union SDL_Event;

class SDLManager
{
public:
	void init(uint32_t width, uint32_t height);
	void run(std::function<void(const SDL_Event *)> process_sdl_event, std::function<bool()> update);
	void cleanup();

	uint32_t get_width() const { return m_width; }
	uint32_t get_height() const { return m_height; }

	struct SDL_Window* get_sdl_window() const { return m_window; }

	bool is_window_visible() const;

	void log(const char* text);

private:
	struct SDL_Window* m_window{ nullptr };
	uint32_t m_width = 0;
	uint32_t m_height = 0;

	bool m_windows_visible = true;
};