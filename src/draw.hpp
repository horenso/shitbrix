/**
 * draw.hpp
 * Routines for drawing objects on the screen.
 */
#pragma once

// TODO: use pImpl to remove SDL dependencies from draw interface.
#include "stage.hpp"
#include "event.hpp"
#include "asset.hpp"
#include "context.hpp"
#include <SDL.h>
#include <SDL_image.h>

/**
 * Interface for classes that can draw stuff.
 * One IDraw will usually draw a whole screen with all the objects in it.
 */
class IDraw
{

public:

	/**
	 * Draw something on the screen with the given fraction elapsed since the last tick.
	 * Template method interface.
	 */
	void draw(float dt) const;

	/**
	 * Draw everything using the configured renderer, but do not SDL_RenderPresent.
	 * Template method implementation.
	 * Called by the draw function.
	 * To be overridden by subclasses.
	 */
	virtual void draw_offscreen(float dt) const =0;

};

/**
 * Not-drawaing implementation.
 * This implementation does nothing when asked to draw.
 * It can be used when SDL's video subsystem was not initialized,
 * i.e. on the server.
 */
class NoDraw : public IDraw
{

public:

	virtual void draw_offscreen(float dt) const override {}

};

/**
 * Debugging draw implementation.
 * This is never used in actual releases.
 */
class DrawPink : public IDraw
{

public:

	DrawPink(Uint8 r, Uint8 g, Uint8 b) : m_r(r), m_g(g), m_b(b) {}
	virtual void draw_offscreen(float dt) const override
	{
		SDL_Renderer* renderer = &the_context.sdl->renderer();
		SDL_SetRenderDrawColor(renderer, m_r, m_g, m_b, SDL_ALPHA_OPAQUE);
		SDL_Rect canvas_rect{0, 0, CANVAS_W, CANVAS_H};
		SDL_RenderFillRect(renderer, &canvas_rect);
	}

private:

	Uint8 m_r, m_g, m_b;

};

/**
 * Draw the main menu to the screen.
 */
class DrawMenu : public IDraw
{

public:

	virtual void draw_offscreen(float) const override;

};

/**
 * DrawGame draws gameplay-related objects to the screen.
 * It knows how to interpret various objects’ state and which textures to use.
 */
class DrawGame : public IDraw
{

public:

	/**
	 * Construct a new DrawGame object from the given dependencies.
	 */
	DrawGame(const Stage& stage);

	void fade(float fraction);

	/**
	 * Render all that we know to the screen.
	 * This includes background, pits and cursors.
	 */
	virtual void draw_offscreen(float dt) const override;

	/**
	 * Set whether or not the cursors should be displayed.
	 */
	void show_cursor(bool show);

	/**
	 * Set whether or not the banners should be displayed.
	 */
	void show_banner(bool show);

	/**
	 * Show or hide the debug info on the pits.
	 */
	void toggle_pit_debug_overlay();

	/**
	 * Show or hide the debug highlight on the pits.
	 */
	void toggle_pit_debug_highlight();


	// Animation contants
	static constexpr float BLOCK_BOUNCE_H = 10.f; // height of a block’s bouncing animation when it lands
	static constexpr int CURSOR_FRAME_TIME = 4; // how many sceen frames to display one cursor frame
	static constexpr int CURSOR_FRAMES = 4; // number of available cursor frames

private:

	const Stage& m_stage; // drawable objects container
	bool m_show_cursor;
	bool m_show_banner;
	bool m_show_pit_debug_overlay = false;
	bool m_show_pit_debug_highlight = false;

	float m_fade = 1.f;
	mutable Point m_pitloc{0,0}; //!< point location of the current pit, translate sprites
	mutable uint8_t m_alpha = 255;
	TexturePtr m_fadetex; // solid pixel used for fading

	Point translate(Point p) const noexcept;

	void draw_background() const;
	void draw_pit(const Pit& pit, float dt) const;
	void draw_pit_debug_overlay(const Pit& pit) const;
	void draw_block(const Block& block, float dt) const;
	void draw_garbage(const Garbage& garbage, float dt) const;
	void draw_cursor(const Cursor& cursor, float dt) const;
	void draw_banner(const Banner& banner, float dt) const;
	void draw_bonus(const BonusIndicator& bonus, float dt) const;
	void draw_highlight(Point top_left, int width, int height,
	                    uint8_t r, uint8_t g, uint8_t b, uint8_t a) const;
	
	void putsprite(Point loc, Gfx gfx, size_t frame = 0) const;

	/**
	 * Apply the configured m_fade value to the screen.
	 */
	void tint() const;

};

class DrawTransition : public IDraw
{

public:

	DrawTransition(const IDraw& pred_draw, const IDraw& succ_draw);
	DrawTransition(const DrawTransition& ) = delete;
	DrawTransition(DrawTransition&& rhs) = default;
	DrawTransition& operator=(const DrawTransition& ) = delete;
	DrawTransition& operator=(DrawTransition&& rhs) = default;

	void set_time(int transition_time) { m_time = transition_time;  }

	/**
	 * Draw both screens with a transition animation.
	 */
	virtual void draw_offscreen(float dt) const override;

private:

	const IDraw& m_pred_draw; //!< for drawing the predecessor screen
	const IDraw& m_succ_draw; //!< for drawing the successor screen
	std::unique_ptr<SDL_Texture, SdlDeleter> m_pred_texture; //!< for compositing the predecessor screen
	std::unique_ptr<SDL_Texture, SdlDeleter> m_succ_texture; //!< for compositing the successor screen
	int m_time;

};
